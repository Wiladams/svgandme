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


#include "charset.h"
#include "xmltypes.h"
#include "xmltoken.h"
#include "xmlschema.h"

namespace waavs {
    static constexpr charset xmlwsp(" \t\r\n");
    static constexpr charset xmlalpha("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    static constexpr charset xmldigit("0123456789");
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
    
    //============================================================
    // readTag()
    // The dataChunk
    // src is positioned just past the opening '<'
    // with any whitespace already consumed.
    //============================================================
    static bool readTag(ByteSpan& src, ByteSpan& tagName, ByteSpan& rest) noexcept
    {
        if (src.empty())
            return false;

        // Read the name of the tag
        // Might include namespace
        if (!readXsdName(src, tagName))
            return false;

        // Use memchr to quickly locate the '>' character
        // trim whitespace
		//src.prefix_trim(chrWspChars);
        src.skipWhile(chrWspChars);

        const unsigned char* srcPtr = src.begin();
        const unsigned char* endPtr = src.end();

        const unsigned char* closingBracket = static_cast<const unsigned char*>(std::memchr(srcPtr, '>', endPtr - srcPtr));

        // If no '>' is found, return false
        if (!closingBracket)
            return false;

        // Capture everything between the '<' and '>' characters
        rest = { srcPtr, closingBracket };

        // Trim trailing whitespace
        rest = chunk_rtrim(rest, xmlwsp);

        // Move the source past '>', so scanning can continue afterward
        src.fStart = closingBracket + 1;

        return true;
    }

    static bool readEndTag(ByteSpan& src, ByteSpan& tagName, ByteSpan& rest) noexcept
    {
        if (src.empty() || src[0] != '/')
            return false;

        // move past '/'
        src.remove_prefix(1);

        // Read the name of the tag
        // Might include namespace
        if (!readXsdName(src, tagName))
            return false;

        // Use memchr to quickly locate the '>' character
        // trim whitespace
        const unsigned char* srcPtr = src.begin();
        const unsigned char* endPtr = src.end();

        const unsigned char* closingBracket = static_cast<const unsigned char*>(std::memchr(srcPtr, '>', endPtr - srcPtr));

        // If no '>' is found, return false
        if (!closingBracket)
            return false;

        // we don't really do anything with the 'rest' and there
        // should not actually be any, but we'll capture it anyway
        // Capture everything between the '<' and '>' characters
        rest = { srcPtr, closingBracket };

        // Trim trailing whitespace
        //rest = chunk_rtrim(rest, xmlwsp);

        // Move the source past '>', so scanning can continue afterward
        src.fStart = closingBracket + 1;

        return true;
    }
}


// 
// The XmlElementIterator is a 'pull model' scanner of in-memory xml data.
// It will return the next xml info in the chunk.
//
namespace waavs {
    // XML_ITERATOR_STATE
    // An enumeration that represents the control
    // state of the iterator
    enum XML_ITERATOR_STATE {
        XML_ITERATOR_STATE_CONTENT = 0
        , XML_ITERATOR_STATE_START_TAG

    };
    
    // XmlIteratorParams
    // The set of parameters that configure how the iterator
    // will operate
    struct XmlIteratorParams {
        bool fSkipComments = true;
        bool fSkipProcessingInstructions = false;
        bool fSkipWhitespace = true;
        bool fSkipCData = false;
        bool fAutoScanAttributes = false;
    };

	// XmlIteratorState
    // The information needed for the iterator to continue
    // after it has returned a value.
    struct XmlIteratorState {
        ByteSpan fSource{};
    };
    

    // nextXmlElement
    // 
    // A function to get the next element in an iteration
    // By putting the necessary parsing in here, we can use both std
    // c++ iteration idioms, as well as a more straight forward C++ style
    // which does not require all the C++ iteration boilerplate
    static bool nextXmlElement(const XmlIteratorParams& params, XmlIteratorState& st, XmlElement& elem)
    {
        elem.reset(XML_ELEMENT_TYPE_INVALID, {}, {});

        if (st.fSource.empty())
            return false;

        // 1. Search for next '<'
        const unsigned char* lt = (const unsigned char*)memchr(st.fSource.fStart, '<', st.fSource.size());

        if (lt) {
            // Return content if present before the '<'
            if (lt != st.fSource.fStart) {
                elem.reset(XML_ELEMENT_TYPE_CONTENT, {}, { st.fSource.fStart, lt });
                st.fSource.fStart = lt; // next call will process the tag
                return true;
            }

            // Now at a tag, consume '<'
            st.fSource.fStart = lt + 1;

            // Begin tag parsing
            st.fSource.skipWhile(chrWspChars);
            ByteSpan tagName{};
            ByteSpan elementChunk = st.fSource;
            elementChunk.fEnd = st.fSource.fStart;
            int kind = XML_ELEMENT_TYPE_START_TAG;

            switch (*st.fSource)
            {
            default:
                readTag(st.fSource, tagName, elementChunk);
                if (chunk_ends_with_char(elementChunk, '/'))
                    kind = XML_ELEMENT_TYPE_SELF_CLOSING;
                break;

            case '?':
                kind = XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION;
                if (!readPI(st.fSource, tagName, elementChunk))
                    return false;
                if (tagName == "xml")
                    kind = XML_ELEMENT_TYPE_XMLDECL;
                break;

            case '!':
                if (st.fSource.startsWith("!DOCTYPE")) {
                    kind = XML_ELEMENT_TYPE_DOCTYPE;
                    readDoctype(st.fSource, elementChunk);
                }
                else if (st.fSource.startsWith("!--")) {
                    kind = XML_ELEMENT_TYPE_COMMENT;
                    readComment(st.fSource, elementChunk);
                }
                else if (st.fSource.startsWith("![CDATA[")) {
                    kind = XML_ELEMENT_TYPE_CDATA;
                    readCData(st.fSource, elementChunk);
                }
                else if (st.fSource.startsWith("!ENTITY")) {
                    kind = XML_ELEMENT_TYPE_ENTITY;
                    readEntityDeclaration(st.fSource, elementChunk);
                }
                break;

            case '/':
                kind = XML_ELEMENT_TYPE_END_TAG;
                readEndTag(st.fSource, tagName, elementChunk);
                break;
            }

            elem.reset(kind, tagName, elementChunk);
            return true;
        }

        // 2. No '<' found: optional trailing content
        if (!st.fSource.empty()) {
            elem.reset(XML_ELEMENT_TYPE_CONTENT, {}, st.fSource);
            st.fSource.fStart = st.fSource.fEnd;  // EOF
            return true;
        }

        return false;
    }


}

namespace waavs
{
    // Objectified XML token generator
    struct XmlElementGenerator : IProduce<XmlElement>
    {
		XmlIteratorParams fParams{};
        XmlIteratorState fState{};

        XmlElementGenerator(const ByteSpan& src)
            :fState{ src}
        {
        }

        bool next(XmlElement & elem) override
        {
            return nextXmlElement(fParams, fState, elem);
        }
    };
}