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
#include "charset.h"

namespace waavs {
    static charset xmlwsp(" \t\r\n\f\v");
    static charset xmlalpha("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    static charset xmldigit("0123456789");
}

namespace waavs {

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
        src = chunk_ltrim(src, xmlwsp);

        // Get the name of the root element
        auto element = chunk_token(src, xmlwsp);
        
        // Trim whitespace as usual
        src = chunk_ltrim(src, xmlwsp);
        
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
			src = chunk_ltrim(src, xmlwsp);

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
			src = chunk_ltrim(src, xmlwsp);

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
    //============================================================
    static bool readTag(ByteSpan& src, ByteSpan& dataChunk) noexcept
    {
        if (!src)
            return false;

        const unsigned char* srcPtr = src.fStart;
        const unsigned char* endPtr = src.fEnd;

        // Use memchr to quickly locate the '>' character
        const unsigned char* closingBracket = static_cast<const unsigned char*>(std::memchr(srcPtr, '>', endPtr - srcPtr));

        // If no '>' is found, return false
        if (!closingBracket)
            return false;

        // Capture everything between the '<' and '>' characters
        dataChunk = { srcPtr, closingBracket };

        // Trim trailing whitespace
        dataChunk = chunk_rtrim(dataChunk, xmlwsp);

        // Move the source past '>', so scanning can continue afterward
        src.fStart = closingBracket + 1;

        return true;
    }

    static bool scanInfoNameSpan(const ByteSpan &src, ByteSpan &name, ByteSpan &rest)
    {
        ByteSpan s = src;
        bool start = false, end = false;

        if (!s)
            return false;

        // Look to see if it's a closing tag name
        if (*s == '/')
        {
            s++;
            end = true;
        }
        else {
            start = true;
        }

        name = s;
        // skip leading whitespace
        while (s && !chrWspChars[*s])
            s++;

        name.fEnd = s.fStart;
        rest = s;

        return true;
    }

    static bool readInfoTag(ByteSpan& src, ByteSpan& dataChunk, ByteSpan &nameSpan) noexcept
    {
        if (!src)
            return false;

        const unsigned char* srcPtr = src.fStart;
        const unsigned char* endPtr = src.fEnd;

        // Use memchr to quickly locate the '>' character
        const unsigned char* closingBracket = static_cast<const unsigned char*>(std::memchr(srcPtr, '>', endPtr - srcPtr));

        // If no '>' is found, return false
        if (!closingBracket)
            return false;

        // Capture everything between the '<' and '>' characters
        dataChunk = { srcPtr, closingBracket };

		// scan the name span
        if (!scanInfoNameSpan(dataChunk, nameSpan, dataChunk))
            return false;

        // Trim trailing whitespace
        dataChunk = chunk_rtrim(dataChunk, xmlwsp);

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
        int fState{ XML_ITERATOR_STATE_CONTENT };
        ByteSpan fSource{};
        ByteSpan fMark{};
    };
    
    // XmlElementGenerator
    // A function to get the next element in an iteration
    // By putting the necessary parsing in here, we can use both std
    // c++ iteration idioms, as well as a more straight forward C++ style
    // which does not require all the C++ iteration boilerplate
    // Returns
	// true - if there is another element to return
	// false - if there are no more elements to return, or error
    static bool XmlNextElementInfo(const XmlIteratorParams& params, XmlIteratorState& st, XmlElementInfo & elem)
    {
        elem.reset();

        while (st.fSource)
        {
            if (st.fState == XML_ITERATOR_STATE_CONTENT)
            {
                if (*st.fSource == '<')
                {
                    // Change state to beginning of start tag
                    // for next turn through iteration
                    st.fState = XML_ITERATOR_STATE_START_TAG;

                    if (st.fSource != st.fMark)
                    {
                        // Encapsulate the content in a chunk
                        ByteSpan content = { st.fMark.fStart, st.fSource.fStart };

                        // collapse whitespace
                        // if the content is all whitespace
                        // don't return anything
                        // BUGBUG - deal with XML preserve whitespace
                        if (params.fSkipWhitespace)
                            content = chunk_trim(content, xmlwsp);

                        if (content)
                        {
                            // Set the state for next iteration
                            st.fSource++;
                            st.fMark = st.fSource;
                            elem.reset(XML_ELEMENT_TYPE_CONTENT, content);

                            return !elem.empty();
                        }
                    }

                    st.fSource++;
                    st.fMark = st.fSource;
                }
                else {
                    st.fSource++;
                }
            }
            else if (st.fState == XML_ITERATOR_STATE_START_TAG)
            {
                // Create a chunk that encapsulates the element tag 
                // up to, but not including, the '>' character
                ByteSpan elementChunk = st.fSource;
                ByteSpan nameChunk{};
                elementChunk.fEnd = st.fSource.fStart;
                int kind = XML_ELEMENT_TYPE_START_TAG;

                if (st.fSource.startsWith("?xml"))
                {
                    kind = XML_ELEMENT_TYPE_XMLDECL;
                    readInfoTag(st.fSource, elementChunk, nameChunk);
                }
                else if (st.fSource.startsWith("?"))
                {
                    kind = XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION;
                    readInfoTag(st.fSource, elementChunk, nameChunk);
                }
                else if (st.fSource.startsWith("!DOCTYPE"))
                {
                    kind = XML_ELEMENT_TYPE_DOCTYPE;
                    readDoctype(st.fSource, elementChunk);
                }
                else if (st.fSource.startsWith("!--"))
                {
                    kind = XML_ELEMENT_TYPE_COMMENT;
                    readComment(st.fSource, elementChunk);
                }
                else if (st.fSource.startsWith("![CDATA["))
                {
                    kind = XML_ELEMENT_TYPE_CDATA;
                    readCData(st.fSource, elementChunk);
                }
                else if (st.fSource.startsWith("!ENTITY"))
                {
                    kind = XML_ELEMENT_TYPE_ENTITY;
                    readEntityDeclaration(st.fSource, elementChunk);
                }
                else if (st.fSource.startsWith("/"))
                {
                    kind = XML_ELEMENT_TYPE_END_TAG;
                    readInfoTag(st.fSource, elementChunk, nameChunk);
                }
                else {
                    readInfoTag(st.fSource, elementChunk, nameChunk);
                    if (chunk_ends_with_char(elementChunk, '/'))
                        kind = XML_ELEMENT_TYPE_SELF_CLOSING;
                }

                st.fState = XML_ITERATOR_STATE_CONTENT;
                st.fMark = st.fSource;

                elem.reset(kind, elementChunk, nameChunk);

                return !elem.empty();

            }
            else {
                // Just advance to next character
                st.fSource++;
            }
        }


        return !elem.empty();
    }
        

    // XmlElementGenerator
    // A function to get the next element in an iteration
    // By putting the necessary parsing in here, we can use both std
    // c++ iteration idioms, as well as a more straight forward C++ style
    // which does not require all the C++ iteration boilerplate
    static bool XmlElementGenerator(const XmlIteratorParams& params, XmlIteratorState& st, XmlElement &elem)
    {
        elem.reset(XML_ELEMENT_TYPE_INVALID, {});

        
        while (st.fSource)
        {
            if (st.fState == XML_ITERATOR_STATE_CONTENT)
            {

                if (*st.fSource == '<')
                {
                    // Change state to beginning of start tag
                    // for next turn through iteration
                    st.fState = XML_ITERATOR_STATE_START_TAG;

                    if (st.fSource != st.fMark)
                    {
                        // Encapsulate the content in a chunk
                        ByteSpan content = { st.fMark.fStart, st.fSource.fStart };

                        // collapse whitespace
                        // if the content is all whitespace
                        // don't return anything
                        // BUGBUG - deal with XML preserve whitespace
                        if (params.fSkipWhitespace)
                            content = chunk_trim(content, xmlwsp);
                        
                        if (content)
                        {
                            // Set the state for next iteration
                            st.fSource++;
                            st.fMark = st.fSource;
                            elem.reset(XML_ELEMENT_TYPE_CONTENT, content);

                            return !elem.isEmpty();
                        }
                    }

                    st.fSource++;
                    st.fMark = st.fSource;
                }
                else {
                    st.fSource++;
                }
            } else if (st.fState == XML_ITERATOR_STATE_START_TAG)
            {
                // Create a chunk that encapsulates the element tag 
                // up to, but not including, the '>' character
                ByteSpan elementChunk = st.fSource;
                elementChunk.fEnd = st.fSource.fStart;
                int kind = XML_ELEMENT_TYPE_START_TAG;

                if (st.fSource.startsWith("?xml"))
                {
                    kind = XML_ELEMENT_TYPE_XMLDECL;
                    readTag(st.fSource, elementChunk);
                }
                else if (st.fSource.startsWith("?"))
                {
                    kind = XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION;
                    readTag(st.fSource, elementChunk);
                }
                else if (st.fSource.startsWith("!DOCTYPE"))
                {
                    kind = XML_ELEMENT_TYPE_DOCTYPE;
                    readDoctype(st.fSource, elementChunk);
                }
                else if (st.fSource.startsWith("!--"))
                {
                    kind = XML_ELEMENT_TYPE_COMMENT;
                    readComment(st.fSource, elementChunk);
                }
                else if (st.fSource.startsWith("![CDATA["))
                {
                    kind = XML_ELEMENT_TYPE_CDATA;
                    readCData(st.fSource, elementChunk);
                }
                else if (st.fSource.startsWith("!ENTITY"))
                {
                    kind = XML_ELEMENT_TYPE_ENTITY;
                    readEntityDeclaration(st.fSource, elementChunk);
                }
                else if (st.fSource.startsWith("/"))
                {
                    kind = XML_ELEMENT_TYPE_END_TAG;
                    readTag(st.fSource, elementChunk);
                }
                else {
                    readTag(st.fSource, elementChunk);
                    if (chunk_ends_with_char(elementChunk, '/'))
                        kind = XML_ELEMENT_TYPE_SELF_CLOSING;
                }

                st.fState = XML_ITERATOR_STATE_CONTENT;
                st.fMark = st.fSource;

                elem.reset(kind, elementChunk);

                return !elem.isEmpty();

            } else {
                // Just advance to next character
                st.fSource++;
            }
        }

        
        return !elem.isEmpty();
    }
    



}

