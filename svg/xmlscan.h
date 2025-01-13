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
// no deal with files, or streams, or anything high level like that, just a chunk.
// It does not alter the chunk, just reads bytes from it, and returns chunks in 
// responses.
//
// The fundamental unit is the XmlElement, which encapsulates a single XML element
// and its attributes.
//
// The element contains individual members for
//  kind - content, self-closing, start-tag, end-tag, comment, processing-instruction
//  name - the name of the element, if opening or closing tag
//  attributes - a map of attribute names to attribute values.  Values are still in raw form
//  data - the raw data of the element.  The starting name has been removed, to be turned into the name
//
// References:
// https://dvcs.w3.org/hg/microxml/raw-file/tip/spec/microxml.html
// https://www.w3.org/TR/REC-xml/
// https://www.w3.org/TR/xml/
//


#include <string>

#include "bspan.h"



namespace waavs {
    static charset xmlwsp(" \t\r\n\f\v");
    static charset xmlalpha("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    static charset xmldigit("0123456789");
}

namespace waavs {
    
    enum XML_ELEMENT_TYPE {
        XML_ELEMENT_TYPE_INVALID = 0
		, XML_ELEMENT_TYPE_XMLDECL                      // <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
        , XML_ELEMENT_TYPE_START_TAG                    // <tag>
		, XML_ELEMENT_TYPE_END_TAG                      // </tag>
		, XML_ELEMENT_TYPE_SELF_CLOSING                 // <tag/>
		, XML_ELEMENT_TYPE_EMPTY_TAG                    // <br>
		, XML_ELEMENT_TYPE_CONTENT                      // <tag>content</tag>
		, XML_ELEMENT_TYPE_COMMENT                      // <!-- comment -->
		, XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION       // <?target data?>
		, XML_ELEMENT_TYPE_CDATA                        // <![CDATA[<greeting>Hello, world!</greeting>]]>
		, XML_ELEMENT_TYPE_DOCTYPE                      // <!DOCTYPE greeting SYSTEM "hello.dtd">
		, XML_ELEMENT_TYPE_ENTITY                       // <!ENTITY hello "Hello">
    };
    
 
    /*
    //
    // readQuoted()
    // 
	// Read a quoted string from the input stream
	// Read a first quote, then use that as the delimiter
    // to read to the end of the string
    //
    static bool readQuoted(ByteSpan &src, ByteSpan &dataChunk) noexcept
    {
        uint8_t* beginattrValue = nullptr;
        uint8_t* endattrValue = nullptr;
        uint8_t quote{};

        // Skip white space before the quoted bytes
        src = chunk_ltrim(src, xmlwsp);

        if (!src)
            return false;

        // capture the quote character
        quote = *src;

        // advance past the quote, then look for the matching close quote
        src++;
        beginattrValue = (uint8_t*)src.fStart;

        // Skip until end of the value.
        while (src && *src != quote)
            src++;

        if (src)
        {
            endattrValue = (uint8_t*)src.fStart;
            src++;
        }

        // Store only well formed attributes
        dataChunk = { beginattrValue, endattrValue };

        return true;
    }
    */
    
    //============================================================
    // readCData()
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
    // A processing instruction.  We are at the beginning of
    // !ENTITY  name 'quoted value' >
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
    // fSource is currently sitting at the beginning of !DOCTYPE
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
    //============================================================

    static bool readTag(ByteSpan& src, ByteSpan& dataChunk) noexcept
    {
        const unsigned char* srcPtr = src.fStart;
		const unsigned char* endPtr = src.fEnd;
        
        dataChunk = src;
        dataChunk.fEnd = src.fStart;

		while ((srcPtr < endPtr) && (*srcPtr != '>'))
			srcPtr++;
        
		// if we get to the end of the input, before seeing the closing '>'
        // the we return false, indicating we did not read
        if (srcPtr == endPtr)
			return false;
        
        // we did see the closing, so capture the name into 
        // the data chunk, and trim whitespace off the end.
        dataChunk.fEnd = srcPtr;
        dataChunk = chunk_rtrim(dataChunk, xmlwsp);

        // move past the '>'
        srcPtr++;
        src.fStart = srcPtr;


        return true;
    }


    

}


namespace waavs {
//============================================================
// XmlName
//============================================================

struct XmlName {
    ByteSpan fNamespace{};
    ByteSpan fName{};

    constexpr XmlName() noexcept = default;

    XmlName(const ByteSpan& inChunk)
    {
        reset(inChunk);
    }

    XmlName(const XmlName& other) noexcept
        :fNamespace(other.fNamespace), 
        fName(other.fName) {}

    XmlName& operator =(const XmlName& rhs)
    {
        fNamespace = rhs.fNamespace;
        fName = rhs.fName;
        return *this;
    }

    XmlName& reset(const ByteSpan& inChunk)
    {
        fName = inChunk;
        fNamespace = chunk_token(fName, charset(':'));
        if (fName.size() < 1)
        {
            fName = fNamespace;
            fNamespace = {};
        }
        return *this;
    }

    // fqname
    // Fully Qualified Name
    // This is the namespace, plus the basic name
    // as a std::string
    std::string fqname() const {
		if (fNamespace.size() > 0)
			return std::string((const char*)fNamespace.fStart, fNamespace.size()) + ":" + std::string((const char*)fName.fStart, fName.size());
		else
			return std::string((const char*)fName.fStart, fName.size());
    }
    
    ByteSpan name() const noexcept { return fName; }         // The name
    ByteSpan ns() const noexcept { return fNamespace; }      // The namespace
};
}




namespace waavs {

    // Representation of an xml element
    // The xml scanner will generate these
    struct XmlElement
    {
    private:
        int fElementKind{ XML_ELEMENT_TYPE_INVALID };

        
        ByteSpan fNameSpan{};
        ByteSpan fData{};
        XmlName fXmlName{};

        ByteSpan scanNameSpan()
        {
            ByteSpan s = fData;
            bool start = false;
            bool end = false;

            // If the chunk is empty, just return
            if (!s)
                return ByteSpan{};

            // Check if the tag is end tag
            if (*s == '/')
            {
                s++;
                end = true;
            }
            else {
                start = true;
            }

            // Get tag name
            fNameSpan = s;
            fNameSpan.fEnd = s.fStart;

            while (s && !xmlwsp[*s])
                s++;

            fNameSpan.fEnd = s.fStart;
            fXmlName.reset(fNameSpan);

            // Modify the data chunk to point to the next attribute
            // part of the element
            fData = s;

            return fNameSpan;
        }

    public:
        XmlElement() {}
        XmlElement(const XmlElement& other)
            :fElementKind(other.fElementKind),
            fNameSpan(other.fNameSpan),
            fData(other.fData),
            fXmlName(other.fXmlName)
        {
        }

        XmlElement(int kind, const ByteSpan& data)
            :fElementKind(kind)
            , fData(data)
        {
            reset(kind, data);
        }

        void reset(int kind, const ByteSpan& data)
        {
            clear();

            fElementKind = kind;
            fData = data;

            if ((fElementKind == XML_ELEMENT_TYPE_START_TAG) ||
                (fElementKind == XML_ELEMENT_TYPE_SELF_CLOSING) ||
                (fElementKind == XML_ELEMENT_TYPE_END_TAG))
            {
                scanNameSpan();
            }
        }

        XmlElement& operator=(const XmlElement& other)
        {
            fElementKind = other.fElementKind;

            fNameSpan = other.fNameSpan;
            fData = other.fData;
            fXmlName.reset(fNameSpan);

            return *this;
        }

        // Clear this element to a default state
        virtual void clear()
        {
            fXmlName.reset({});
            fElementKind = XML_ELEMENT_TYPE_INVALID;
            fData.reset();
        }

        // determines whether the element is currently empty
        constexpr bool isEmpty() const { return fElementKind == XML_ELEMENT_TYPE_INVALID; }

        explicit operator bool() const { return !isEmpty(); }

        // Returning information about the element
        ByteSpan nameSpan() const { return fNameSpan; }
        XmlName xmlName() const { return fXmlName; }
        ByteSpan tagName() const { return fXmlName.name(); }
        ByteSpan tagNamespace() const { return fXmlName.ns(); }
        ByteSpan name() const { return fXmlName.name(); }

        int kind() const { return fElementKind; }
        void setKind(const int kind) { fElementKind = kind; }

        ByteSpan data() const { return fData; }


        // Convenience for what kind of tag it is
		bool isXmlDecl() const { return fElementKind == XML_ELEMENT_TYPE_XMLDECL; }
        bool isStart() const { return (fElementKind == XML_ELEMENT_TYPE_START_TAG); }
        bool isSelfClosing() const { return fElementKind == XML_ELEMENT_TYPE_SELF_CLOSING; }
        bool isEnd() const { return fElementKind == XML_ELEMENT_TYPE_END_TAG; }
        bool isComment() const { return fElementKind == XML_ELEMENT_TYPE_COMMENT; }
        bool isProcessingInstruction() const { return fElementKind == XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION; }
        bool isContent() const { return fElementKind == XML_ELEMENT_TYPE_CONTENT; }
        bool isCData() const { return fElementKind == XML_ELEMENT_TYPE_CDATA; }
        bool isDoctype() const { return fElementKind == XML_ELEMENT_TYPE_DOCTYPE; }
		bool isEntityDeclaration() const { return fElementKind == XML_ELEMENT_TYPE_ENTITY; }
    };
}

// 
// The XmlElementIterator is used to iterate over the elements in a chunk of memory.
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
    static bool XmlElementGenerator(const XmlIteratorParams& params, XmlIteratorState& st, XmlElement &elem)
    {
        elem.clear();

        
        while (st.fSource)
        {
            switch (st.fState)
            {
            case XML_ITERATOR_STATE_CONTENT: {

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
            }
            break;

            case XML_ITERATOR_STATE_START_TAG: {
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

            }
            break;

            default:
                // Just advance to next character
                st.fSource++;
                break;

            }
        }

        
        return !elem.isEmpty();
    }
    
    // XmlElementIterator
    // scans XML generating a sequence of XmlElements
    // This is a forward only non-writeable iterator
    // 
    // Usage:
    //   XmlElementIterator iter(xmlChunk, false);
    // 
    //   while(iter.next())
    //   {
    //     printXmlElement(*iter);
    //   }
    //
    //.  or, you can do it this way
    //.  
    //.  iter++;
    //.  while (iter) {
    //.    printXmlElement(*iter);
    //.    iter++;
    //.  }
    //
    struct XmlElementIterator {
    private:
        XmlIteratorParams fParams{};
        XmlIteratorState fState{};
        XmlElement fCurrentElement{};

    public:
		XmlElementIterator(const ByteSpan& inChunk, bool autoScanAttributes = false)
            : fState{ XML_ITERATOR_STATE_CONTENT, inChunk, inChunk }
        {
			fParams.fAutoScanAttributes = autoScanAttributes;
            // We do not advance in the constructor, leaving that
            // for the user to call 'next()' to get the first element
            //next();
        }

		// return 'true' if the node we're currently sitting on is valid
        // return 'false' if otherwise
        explicit operator bool() { return !fCurrentElement.isEmpty(); }

        // These operators make it operate like an iterator
        const XmlElement& operator*() const { return fCurrentElement; }
        const XmlElement* operator->() const { return &fCurrentElement; }

        // Increment the iterator either pre, or post notation
        XmlElementIterator& operator++() { next(); return *this; }
        XmlElementIterator& operator++(int) { next(); return *this; }

        // Return the current value of the iteration
        //const XmlElement & next(XmlElement& elem)
        const bool next()
        {
            bool validElement = XmlElementGenerator(fParams, fState, fCurrentElement);
            return validElement;
        }
    };
}


