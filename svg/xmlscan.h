#pragma once


//
// This file represents a very small, fast, simple XML scanner
// The purpose is to break a chunk of XML down into component parts, that higher
// level code can then use to do whatever it wants.
// 
// You can construct an iterator, and use that to scan through the XML
// using a 'pull model'.
// 
// One key aspect of the design is that it operates on a span of memory.  It does
// not deal with files, or streams, or anything high level like that, just a chunk.
// It does not alter the chunk, just reads bytes from it, and returns chunks in 
// responses.
//
// The fundamental unit is the XmlElement, which encapsulates a single unit of XML 
// element whether it be a tag name, or text content.
//
// The element contains individual members for
//  kind - content, self-closing, start-tag, end-tag, comment, processing-instruction
//  name - the name of the element, if opening or closing tag
//  attributes - a map of attribute names to attribute values.  Values are still in raw form
//  data - the raw data of the element.  
//  The starting name has been removed, to be turned into the name
//
// References:
// https://dvcs.w3.org/hg/microxml/raw-file/tip/spec/microxml.html
// https://www.w3.org/TR/REC-xml/
// https://www.w3.org/TR/xml/
//



#include "xmltypes.h"
#include "xmltoken.h"
#include "xmltokengen.h"
#include "xmlschema.h"


namespace waavs {
    static constexpr charset xmlwsp(" \t\r\n");
    static constexpr charset xmlalpha("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    static constexpr charset xmldigit("0123456789");
}



namespace waavs {
    // XML_ITERATOR_STATE
    // An enumeration that represents the control
    // state of the iterator
    //enum XML_ITERATOR_STATE {
    //    XML_ITERATOR_STATE_CONTENT = 0
    //    , XML_ITERATOR_STATE_START_TAG
    //};

    // XmlIteratorParams
    // 
    // The set of parameters that configure how the iterator
    // will operate
    struct XmlIteratorParams
    {
        bool fSkipComments = true;
        bool fSkipProcessingInstructions = false;
        bool fSkipWhitespace = true;
        bool fSkipCData = false;
        bool fAutoScanAttributes = false;
    };


    struct XmlIterator
    {
        XmlIteratorParams fParams{};
        XmlTokenState fState{};

        XmlIterator() = default;
        XmlIterator(const ByteSpan& inChunk)
            : fState{  inChunk, false  }
        {
        }
    };
}


// Forward declaration of some functions
namespace waavs {
    static bool readPI(ByteSpan& src, ByteSpan& target, ByteSpan& rest);
    static bool readComment(ByteSpan& src, ByteSpan& dataChunk) noexcept;
    static bool readCData(ByteSpan& src, ByteSpan& dataChunk) noexcept;
    static bool readEntityDeclaration(ByteSpan& src, ByteSpan& dataChunk) noexcept;
    static bool readDoctype(ByteSpan& src, ByteSpan& dataChunk) noexcept;


    inline bool isAllXmlWhitespace(const ByteSpan& span) noexcept;
    static bool skipAttributes(XmlIterator& iter, ByteSpan& attrSpan);

    static bool parsePIFromTokens(XmlIterator& st, XmlElement& elem) noexcept;
    static bool parseEndTagFromTokens(XmlIterator& st, XmlElement& elem) noexcept;
    static bool parseStartOrSelfClosingFromTokens(XmlIterator& iter, XmlElement& elem, const XmlToken& firstNameToken);
    static bool parseBangConstructFromTokens(XmlIterator& st, XmlElement& elem) noexcept;

    static bool scanAttributes(XmlAttributeCollection& attrs, const ByteSpan& inChunk) noexcept;
    static bool nextXmlElement(XmlIterator& iter, XmlElement& elem);

}


namespace waavs
{
    // XML whitespace per your xmlwsp charset: " \t\r\n"
    inline bool isAllXmlWhitespace(const ByteSpan& span) noexcept
    {
        bool allWhiteSpace = isAll(span, xmlwsp);
        return allWhiteSpace;
    }
}


namespace waavs {

    // readPI()
    // 
    // Read Processing Instruction
    // '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
    static bool readPI(ByteSpan& src, ByteSpan& target, ByteSpan &rest)
    {
        if (src.empty() || src[0]!='?')
            return false;

        // move past '?'
        src.remove_prefix(1);

        // read PITarget
        ByteSpan tempTarget;
        if (!readXsdName(src, tempTarget))
            return false;

        target = tempTarget;

        // trim whitespace
        src.skipWhile(chrWspChars);

        // Mark content start
        rest = src;

        // Find the closing '?>' sequence
        const unsigned char* srcPtr = src.fStart;
        const unsigned char* endPtr = src.fEnd;

        //while (!src.empty())
        //{
            // Use memchr to quickly locate the '?' character
            const unsigned char* closingBracket = static_cast<const unsigned char*>(std::memchr(srcPtr, '?', endPtr - srcPtr-1));

            // If no '?' is found, return false
            if (!closingBracket)
                return false;

            // If the '?' was found, then check if next character
            // is '>'.  If it is, we've found our closing.
            if (closingBracket[1] != '>')
            {
                return false;
            }
        //}

            rest.fEnd = closingBracket;

        // move past closing '?>'
        closingBracket += 2;
        
        // increment the source stream
        src.fStart = closingBracket;

        return true;
    }

    //============================================================
    // readCData()
    // starting: ![CDATA[
    //============================================================

    static bool readCData(ByteSpan& src, ByteSpan& dataChunk) noexcept
    {
        // Skip past the ![CDATA[
        src += 8;

        dataChunk = src;
        dataChunk.fEnd = src.fStart;

        // Extend the data chunk until we find the closing ]]>
        ByteSpan endCData = chunk_find_cstr(src, "]]>");
        if (!endCData)
            return false;

        dataChunk.fEnd = endCData.fStart;

        src = endCData;

        // skip past the closing ]]>.
        src += 3;

        return true;
    }

    //============================================================
    // readComment()
    // starting: !--
    //============================================================
    static bool readComment(ByteSpan &src, ByteSpan &dataChunk) noexcept
	{
		// Skip past the !--
		src += 3;

		dataChunk = src;
		dataChunk.fEnd = src.fStart;

		// Extend the data chunk until we find the closing -->
		ByteSpan endComment = chunk_find_cstr(src, "-->");
        if (!endComment)
            return false;
        
		dataChunk.fEnd = endComment.fStart;

		src = endComment;

		// skip past the closing -->
		src += 3;

		return true;
	}
    
    //============================================================
	// readEntityDeclaration()
    // A processing instruction.  
    // Starting: !ENTITY
    // 
    // Return a name and value
	//============================================================
    static bool readEntityDeclaration(ByteSpan& src, ByteSpan& dataChunk) noexcept
    {
        // Skip past the !ENTITY
		src += 7;

        dataChunk = src;

		// skip until we see the closing '>' character
		src = chunk_find_char(src, '>');
        if (!src)
            return false;

        dataChunk.fEnd = src.fStart;

        // skip past that character and return
		src++;

		return true;
    }
    
    //============================================================
    // readDoctype
    // Reads the doctype chunk, and returns it as a ByteSpan
    // fSource is currently sitting at the beginning of 
    // Starting: !DOCTYPE
    // Note: https://www.tutorialspoint.com/xml/xml_dtds.htm
    //============================================================
    static bool readDoctype(ByteSpan& src, ByteSpan& dataChunk) noexcept
    {
        // skip past the !DOCTYPE to the first whitespace character
        src += 8;


        // Skip past the whitespace
        // to get to the beginning of things
		src.skipWhile(chrWspChars);

        // Get the name of the root element
        auto element = chunk_token(src, chrWspChars);
        
        // Trim whitespace as usual
        src.skipWhile(chrWspChars);
        
        // If the next thing we see is a '[', then we have 
        // an 'internal' DTD.  Read to the closing ']>' and be done
		if (*src == '[')
		{
			// Skip past the opening '['
			src++;

			// Find the closing ']>'
			ByteSpan endDTD = chunk_find_cstr(src, "]>");
            if (!endDTD)
                return false;

			dataChunk = src;
			dataChunk.fEnd = endDTD.fStart;

			// Skip past the closing ']>'
			src.fStart = endDTD.fStart + 2;
            return true;
		}
        
        // If we've gotten here, we have an 'external' DTD
		// It can either be a SYSTEM or PUBLIC DTD
        // First check for a PUBLIC DTD
		if (src.startsWith("PUBLIC"))
		{
			// Skip past the PUBLIC
			src += 6;

            ByteSpan publicId{};
			ByteSpan systemId{};
            
            chunk_read_quoted(src, publicId);
            chunk_read_quoted(src, systemId);

			// Skip past the whitespace
            src.skipWhile(chrWspChars);

			// If we have a closing '>', then we're done
			if (*src == '>')
			{
				src++;
                dataChunk.fStart = src.fStart;
				dataChunk.fEnd = src.fStart;
                
				return true;
			}

			// If we have an opening '[', then we have more to parse
			if (*src == '[')
			{
				// Skip past the opening '['
				src++;

				// Find the closing ']>'
				ByteSpan endDTD = chunk_find_cstr(src, "]>");
				dataChunk = src;
				dataChunk.fEnd = endDTD.fStart;

				// Skip past the closing ']>'
				src.fStart = endDTD.fStart + 2;
				return true;
			}
		}
		else if (src.startsWith("SYSTEM"))
		{
			// Skip past the SYSTEM
			src += 6;

            ByteSpan systemId{};
            chunk_read_quoted(src, systemId);

			// Skip past the whitespace
            src.skipWhile(chrWspChars);

			// If we have a closing '>', then we're done
			if (*src == '>')
			{
				src++;
				return true;
			}

			// If we have an opening '[', then we have more to parse
			if (*src == '[')
			{
				// Skip past the opening '['
				src++;

				// Find the closing ']>'
				ByteSpan endDTD = chunk_find_cstr(src, "]>");
                if (!endDTD)
                    return false;

				dataChunk = src;
				dataChunk.fEnd = endDTD.fStart;

				// Skip past the closing ']>'
				src.fStart = endDTD.fStart + 2;
				return true;
			}
		}

		// We have an invalid DTD
		return false;
    }
    
}


// 
// The XmlElementIterator is a 'pull model' scanner of in-memory xml data.
// It will return the next xml info in the chunk.
//
namespace waavs {

    // parsePIFromTokens()
    //
    // Preconditions:
    //   - We've just consumed a '<' token.
    //   - The last token we saw was XML_TOKEN_QMARK (i.e. we just consumed '?').
    //   - st.fState.input.fStart now points to the first byte *after* '?'.
    //
    // Behavior:
    //   - Use readPI() to parse:
    //        <?target    content   ?>
    // 
    //   - Advances the token state's input past the closing '?>'.
    //   - Sets st.fState.inTag = false (we are now outside any tag).
    //   - Fills 'elem' with either XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION
    //     or XML_ELEMENT_TYPE_XMLDECL (if the target is "xml").
    //
    static bool parsePIFromTokens(XmlIterator& st, XmlElement& elem) noexcept
    {
        // Rebuild a ByteSpan that *includes* the '?' we just tokenized.
        ByteSpan src;
        src.fStart = st.fState.input.fStart - 1;   // points to '?'
        src.fEnd = st.fState.input.fEnd;

        ByteSpan target{};
        ByteSpan content{};

        if (!readPI(src, target, content))
            return false;

        // Sync the token state's input with where readPI() left off
        st.fState.input.fStart = src.fStart;
        // We've consumed the entire '<?...?>', so we're now outside the tag.
        st.fState.inTag = false;

        int kind = XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION;
        if (target == "xml")
            kind = XML_ELEMENT_TYPE_XMLDECL;

        elem.reset(kind, target, content);

        return true;
    }


    // parseEndTagFromTokens()
    //
    // Preconditions:
    //   - We are inside a tag.
    //   - The '<' and following '/' have already been consumed as tokens.
    //   - tokenState.input points just after '</'.
    //
    // Behavior:
    //   - Reads the tag name as a NAME token.
    //   - Skips any whitespace (handled inside readTagToken / nextXmlToken).
    //   - Expects a '>' token to terminate the end tag.
    //   - Emits an XmlElement of type XML_ELEMENT_TYPE_END_TAG.
    //
    static bool parseEndTagFromTokens(XmlIterator& st, XmlElement& elem) noexcept
    {
        XmlToken tok{};

        // 1. Read the tag name token
        if (!nextXmlToken(st.fState, tok))
            return false; // EOF after '</'

        if (tok.type != XML_TOKEN_NAME) {
            // Malformed end tag: no name after '</'
            // You can choose to create an INVALID element instead of hard-failing.
            return false;
        }

        ByteSpan tagName = tok.value;

        // 2. After the name, XML only permits whitespace and then '>'.
        //    nextXmlToken / readTagToken already skip whitespace, so we just
        //    advance until we see XML_TOKEN_GT.
        for (;;) {
            if (!nextXmlToken(st.fState, tok))
                return false; // EOF before closing '>'

            if (tok.type == XML_TOKEN_GT) {
                // We don't preserve a "rest" span here, mirroring your current
                // readEndTag(), which also notes it doesn't really use 'rest'.
                elem.reset(XML_ELEMENT_TYPE_END_TAG, tagName, {});
                return true;
            }

            // Any other token after an end-tag name is technically malformed XML.
            // Options:
            //   - return false to signal a hard error
            //   - or, if you want to be more forgiving, you could just continue
            //     until you eventually see XML_TOKEN_GT.
            return false;
        }
    }

    // use the token stream to skip over the attributes, while retaining
    // a byteSpan that represents those attributes.
    // The attrSpan returns the beginning and ending of the attributes
    // return 'false' if any error
    // Note: assume we're already inside a tag, and have just consumed
    // the tag name.
    static bool skipAttributes(XmlIterator &iter, ByteSpan &attrSpan, XmlToken &endtok)
    {
        const unsigned char* attrStart = iter.fState.input.fStart;
        XmlToken tok{};
        // Scan until '>' (or '/>')
        for (;;) {
            if (!nextXmlToken(iter.fState, tok))
                return false;   // EOF

            if (tok.type == XML_TOKEN_GT) {
                // normal end of tag
                attrSpan.fStart = attrStart;
                attrSpan.fEnd = tok.value.fStart - 1; // exclude '>'
                endtok = tok;

                return true;
            }
            else if (tok.type == XML_TOKEN_SLASH) {
                // possible self-closing tag
                if (!nextXmlToken(iter.fState, tok))
                    return false;   // EOF
                if (tok.type == XML_TOKEN_GT) {
                    // self-closing tag
                    attrSpan.fStart = attrStart;
                    attrSpan.fEnd = tok.value.fStart - 2; // exclude '/>'
                    return true;
                }
                else {
                    // malformed
                    return false;
                }
            }
            // else, continue scanning attributes
        }

        return false;

    }

    // parseStartOrSelfClosingFromTokens()
    // 
    // We've gotten the '<' and the NAME token for the tag name (firstNameToken).
    // Here, we read the attributes to the end of the tag, and return that as the 'data'
    // of the element.  We also determine if it's a self-closing tag or a start tag.
    // We will not auto parse attributes here; that can be done later if desired.
    //
    static bool parseStartOrSelfClosingFromTokens(XmlIterator& iter, XmlElement& elem, const XmlToken& firstNameToken)
    {
        ByteSpan tagName = firstNameToken.value;
        const unsigned char* attrStart = iter.fState.input.fStart; // start of attributes-ish area
        bool selfClosing = false;


        // Scan until '>' (or '/>')
        // we don't want to use the tokenizer here, because it will parse attributes
        // which we don't want right now.  We just want to find the closing '>' or '/>'

        // Use memchr to quickly locate the '>' character
        //iter.fState.input.skipWhile(chrWspChars);

        const unsigned char* srcPtr = iter.fState.input.begin();
        const unsigned char* endPtr = iter.fState.input.end();

        const unsigned char* closingBracket = static_cast<const unsigned char*>(std::memchr(srcPtr, '>', endPtr - srcPtr));

        // If no '>' is found, malformed, so return false
        if (!closingBracket)
            return false;

        // if the character before '>' is '/', then it's self-closing
        if (closingBracket > srcPtr && closingBracket[-1] == '/') {
            selfClosing = true;
            // Move back one to exclude the '/' from the attributes span
            closingBracket--;
        }
        ByteSpan rest(srcPtr, closingBracket);

        // Move the source past '>', so scanning can continue afterward
        if (selfClosing)
            iter.fState.input.fStart = closingBracket + 2; // skip '/>'
        else
            iter.fState.input.fStart = closingBracket + 1;
        // either way, we're now outside the tag
        iter.fState.inTag = false;

        elem.reset(selfClosing ? XML_ELEMENT_TYPE_SELF_CLOSING : XML_ELEMENT_TYPE_START_TAG, tagName, rest);

        return true;
    }


    // parseBangConstructFromTokens()
    //
    // Preconditions:
    //   - We've just consumed a '<' token.
    //   - The last token we saw was XML_TOKEN_BANG (for '!').
    //   - st.fState.input.fStart currently points to the first byte
    //     *after* '!'.
    //
    // Behavior:
    //   - Rebuilds a ByteSpan that includes the leading '!' (one byte back).
    //   - Dispatches to readComment / readCData / readDoctype / readEntityDeclaration
    //     based on the prefix.
    //   - Advances st.fState.input to the position where the helper leaves off
    //     (i.e., past '-->', ']]>', or the final '>').
    //   - Sets st.fState.inTag = false, because we've consumed the entire markup.
    //   - Fills 'elem' with the appropriate XmlElementType and data span.
    //
    static bool parseBangConstructFromTokens(XmlIterator& st, XmlElement& elem) noexcept
    {
        // Reconstruct a ByteSpan that starts at '!'
        ByteSpan src;
        src.fStart = st.fState.input.fStart - 1;   // back up to '!'
        src.fEnd = st.fState.input.fEnd;

        ByteSpan data{};

        // COMMENT: <!-- ... -->
        if (src.startsWith("!--")) {
            if (!readComment(src, data))
                return false;

            // Sync token state to where readComment() left off
            st.fState.input.fStart = src.fStart;
            st.fState.inTag = false;

            elem.reset(XML_ELEMENT_TYPE_COMMENT, {}, data);
            return true;
        }

        // CDATA: <![CDATA[ ... ]]>
        if (src.startsWith("![CDATA[")) {
            if (!readCData(src, data))
                return false;

            st.fState.input.fStart = src.fStart;
            st.fState.inTag = false;

            elem.reset(XML_ELEMENT_TYPE_CDATA, {}, data);
            return true;
        }

        // DOCTYPE: <!DOCTYPE ... >
        if (src.startsWith("!DOCTYPE")) {
            if (!readDoctype(src, data))
                return false;

            st.fState.input.fStart = src.fStart;
            st.fState.inTag = false;

            elem.reset(XML_ELEMENT_TYPE_DOCTYPE, {}, data);
            return true;
        }

        // ENTITY declaration: <!ENTITY ... >
        if (src.startsWith("!ENTITY")) {
            if (!readEntityDeclaration(src, data))
                return false;

            st.fState.input.fStart = src.fStart;
            st.fState.inTag = false;

            elem.reset(XML_ELEMENT_TYPE_ENTITY, {}, data);
            return true;
        }

        // Unknown or malformed <! ... > construct
        return false;
    }


    // scanAttributes()
    // Given a chunk that contains attribute key value pairs
    // separated by whitespace, parse them, and store the key/value pairs 
    // in the fAttributes map
    static bool scanAttributes(XmlAttributeCollection& attrs, const ByteSpan& inChunk) noexcept
    {
        ByteSpan src = inChunk;
        ByteSpan key;
        ByteSpan value;

        while (readNextKeyAttribute(src, key, value))
        {
            attrs.addAttribute(key, value);
        }

        return true;
    }
    
    // nextXmlElement
    // 
    // A function to get the next element in an iteration
    // By putting the necessary parsing in here, we can use both std
    // c++ iteration idioms, as well as a more straight forward C++ style
    // which does not require all the C++ iteration boilerplate
    //    static bool nextXmlElement(const XmlIteratorParams& params, XmlIteratorState& st, XmlElement& elem)
    static bool nextXmlElement(XmlIterator& iter, XmlElement& elem)
    {
        //elem.reset(XML_ELEMENT_TYPE_INVALID, {}, {});
        elem.reset();

        if (iter.fState.input.empty())
            return false;

        XmlToken tok{};

        for (;;) {
            if (!nextXmlToken(iter.fState, tok))
                return false;   // EOF

            if (!tok.inTag) {
                // Outside tag: TEXT tokens become CONTENT elements
                if (tok.type == XML_TOKEN_TEXT) {
                    if (iter.fParams.fSkipWhitespace && isAllXmlWhitespace(tok.value))
                        continue;

                    elem.reset(XML_ELEMENT_TYPE_CONTENT, {}, tok.value);
                    return true;
                }

                // We should never see NAME/STRING/etc with inTag == false
                continue;
            }


            // At this point, tok.inTag == true

            // Start of a tag is signaled as XML_TOKEN_LT with inTag = true
            if (tok.type == XML_TOKEN_LT) {
                if (!nextXmlToken(iter.fState, tok))
                    return false;

                switch (tok.type)
                {
                case XML_TOKEN_SLASH:  // End tag
                    return parseEndTagFromTokens(iter, elem);
                case XML_TOKEN_QMARK:  // Processing Instruction
                    return parsePIFromTokens(iter, elem);
                case XML_TOKEN_BANG:   // Comment, CDATA, Doctype, Entity
                    return parseBangConstructFromTokens(iter, elem);
                case XML_TOKEN_NAME:  // Start tag
                    return parseStartOrSelfClosingFromTokens(iter, elem, tok);

                default:
                    // malformed tag; maybe signal an error element
                    return false;
                    break;
                }
            }

            // If inTag but not a '<', that's also malformed
        }
    }
}

