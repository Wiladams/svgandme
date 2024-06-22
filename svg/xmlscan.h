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


#include <unordered_map>
#include <string>
#include <sstream>
#include <optional>

#include "bspan.h"



namespace waavs {
    static charset xmlwsp(" \t\r\n\f\v");
    static charset xmlalpha("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    static charset xmldigit("0123456789");

    
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
    

	static std::string expandStandardEntities(const ByteSpan &inSpan)
	{
        std::ostringstream oss;
        
		for (auto it = inSpan.begin(); it != inSpan.end(); ++it)
		{
			if (*it == '&')
			{
				auto next = it + 1;
				if (next == inSpan.end())
					break;

				if (*next == '#')
				{
					auto next2 = next + 1;
					if (next2 == inSpan.end())
						break;

					if (*next2 == 'x')
					{
						auto next3 = next2 + 1;
						if (next3 == inSpan.end())
							break;

						auto end = next3;
						while (end != inSpan.end() && isxdigit(*end))
							++end;

						if (end == inSpan.end())
							break;

						if (*end != ';')
							break;

						std::string hexStr(next3, end);
						int hexVal = std::stoi(hexStr, nullptr, 16);
						oss << (char)hexVal;

						it = end;
					}
					else
					{
						auto end = next2;
						while (end != inSpan.end() && isdigit(*end))
							++end;

						if (end == inSpan.end())
							break;

						if (*end != ';')
							break;

						std::string decStr(next2, end);
						int decVal = std::stoi(decStr, nullptr, 10);
						oss << (char)decVal;

						it = end;
					}
				}
				else
				{
					auto end = next;
					while (end != inSpan.end() && isalnum(*end))
						++end;

					if (end == inSpan.end())
						break;

					if (*end != ';')
						break;

					std::string entity(next, end);
					if (entity == "lt")
						oss << '<';
					else if (entity == "gt")
						oss << '>';
					else if (entity == "amp")
						oss << '&';
					else if (entity == "apos")
						oss << '\'';
					else if (entity == "quot")
						oss << '"';
					else
						break;

					it = end;
				}
			}
			else
			{
                if (*it != '\r' && *it != '\n') {
                    oss << *it;
                }
			}
		}

		return oss.str();
	}
    
    static bool readQuoted(ByteSpan &src, ByteSpan &dataChunk)
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
    
    //============================================================
    // readCData()
    //============================================================

    static bool readCData(ByteSpan& src, ByteSpan& dataChunk)
    {
        // Skip past the <![CDATA[
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
    static bool readComment(ByteSpan &src, ByteSpan &dataChunk)
	{
		// Skip past the <!--
		src += 4;

		dataChunk = src;
		dataChunk.fEnd = src.fStart;

		// Extend the data chunk until we find the closing -->
		ByteSpan endComment = chunk_find_cstr(src, "-->");
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
    static bool readEntityDeclaration(ByteSpan& src, ByteSpan& name, ByteSpan &value)
    {
        // Skip past the !ENTITY
		src += 7;

		// Get the name of the entity
		name = chunk_token(src, xmlwsp);
        
		// Skip past the whitespace
		src = chunk_ltrim(src, xmlwsp);

		// Get the value of the entity from a quoted string
		readQuoted(src, value);
        
		// skip until we see the closing '>' character
		src = chunk_find_char(src, '>');
        
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
    static bool readDoctype(ByteSpan& src, ByteSpan& dataChunk)
    {
        // skip past the !DOCTYPE to the first whitespace character
        while (src && !xmlwsp[*src])
            src++;

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
		if (chunk_starts_with(src, "PUBLIC"))
		{
			// Skip past the PUBLIC
			src += 6;

            ByteSpan publicId{};
			ByteSpan systemId{};
            
            readQuoted(src, publicId);
            readQuoted(src, systemId);

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
		else if (chunk_starts_with(src, "SYSTEM"))
		{
			// Skip past the SYSTEM
			src += 6;

            ByteSpan systemId{};
			readQuoted(src, systemId);

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

    static bool readTag(ByteSpan& src, ByteSpan& dataChunk)
    {
        dataChunk = src;
        dataChunk.fEnd = src.fStart;

        while (src && *src != '>')
            src++;

        dataChunk.fEnd = src.fStart;
        dataChunk = chunk_rtrim(dataChunk, xmlwsp);

        // Get past the '>' if it was there
        src++;

        return true;
    }

    // Attributes are separated by '=' and values are quoted with 
    // either "'" or '"'
    // alters the src so it points past the end of the retrieved value
    // Typical of xml attributes
    static bool nextAttributeKeyValue(ByteSpan& src, ByteSpan& key, ByteSpan& value)
    {
        // Zero these out in case there is failure
        key = {};
        value = {};

        static charset equalChars("=");
        static charset quoteChars("\"'");

        bool start = false;
        bool end = false;
        uint8_t quote{};

        uint8_t* beginattrValue = nullptr;
        uint8_t* endattrValue = nullptr;


        // Skip leading white space before the key name
        src = chunk_ltrim(src, xmlwsp);

        if (!src)
            return false;

        // Special case of running into an end tag
        if (*src == '/') {
            end = true;
            return false;
        }

        // Find end of the attrib name.
        auto attrNameChunk = chunk_token(src, equalChars);
        key = chunk_trim(attrNameChunk, xmlwsp);

        // Skip stuff past '=' until we see one of our quoteChars
        while (src && !quoteChars[*src])
            src++;

        // If we've run out of input, return false
        if (!src)
            return false;

        // capture the quote character
        quote = *src;

        // move past the beginning of the quote
        // and mark the beginning of the value portion
        src++;
        beginattrValue = (uint8_t*)src.fStart;

        // Skip anything that is not the quote character
        // to mark the end of the value
        // don't use quoteChars here, because it's valid to 
        // embed the other quote within the value
        while (src && *src != quote)
            src++;

        // If we still have input, it means we found
        // the quote character, so mark the end of the
        // value
        if (src)
        {
            endattrValue = (uint8_t*)src.fStart;
            src++;
        }
        else {
            // We did not find the closing quote
            // so we don't have a valid value
            return false;
        }


        // Store only well formed attributes
        value = { beginattrValue, endattrValue };

        return true;
    }
    
	static bool gatherXmlAttributes(const ByteSpan& inChunk, std::unordered_map<std::string, ByteSpan>& outAttributes)
	{
		ByteSpan src = inChunk;
		ByteSpan key;
		ByteSpan value;

		while (nextAttributeKeyValue(src, key, value))
		{
			std::string keyStr((const char*)key.fStart, key.size());
			outAttributes[keyStr] = value;
		}

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

    XmlName() = default;

    XmlName(const ByteSpan& inChunk)
    {
        reset(inChunk);
    }

    XmlName(const XmlName& other) :fNamespace(other.fNamespace), fName(other.fName) {}

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
        if (chunk_size(fName) < 1)
        {
            fName = fNamespace;
            fNamespace = {};
        }
        return *this;
    }

    std::string fqname() const {
		if (fNamespace.size() > 0)
			return std::string((const char*)fNamespace.fStart, fNamespace.size()) + ":" + std::string((const char*)fName.fStart, fName.size());
		else
			return std::string((const char*)fName.fStart, fName.size());
    }
    
    ByteSpan name() const { return fName; }
    ByteSpan ns() const { return fNamespace; }
};
}

namespace waavs {
	//============================================================
    // XmlAttributeCollection
    // A collection of the attibutes found on an XmlElement
    //============================================================
    
    struct XmlAttributeCollection
    {
        std::unordered_map<std::string, ByteSpan> fAttributes{};

        XmlAttributeCollection() = default;
        XmlAttributeCollection(const XmlAttributeCollection& other)
            :fAttributes(other.fAttributes)
        {}
        
        XmlAttributeCollection(const ByteSpan& inChunk)
        {
            scanAttributes(inChunk);
        }
        
        // Return a const attribute collection
	    const std::unordered_map<std::string, ByteSpan>& attributes() const { return fAttributes; }
        
		size_t size() const { return fAttributes.size(); }
        
		virtual void clear() { fAttributes.clear(); }
        
        bool scanAttributes(const ByteSpan& inChunk)
        {
            return gatherXmlAttributes(inChunk, fAttributes);
        }
        
        bool hasAttribute(const std::string& inName) const
		{
			return fAttributes.find(inName) != fAttributes.end();
		}
        
		// Add a single attribute to our collection of attributes
        // if the attribute already exists, replace its value
        // with the new value
        void addAttribute(const std::string& name, const ByteSpan& valueChunk)
        {
            fAttributes[name] = valueChunk;
        }

        //std::optional<ByteSpan> 
        ByteSpan getAttribute(const std::string& name) const
        {
            auto it = fAttributes.find(name);
            if (it != fAttributes.end())
                return it->second;
            else
                return ByteSpan{};
            
                //return std::nullopt;
        }

        XmlAttributeCollection & mergeProperties(const XmlAttributeCollection & other)
        {
            for (auto& attr : other.fAttributes)
            {
            	fAttributes[attr.first] = attr.second;
            }
            return *this;
        }
        
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
            fData = {};
        }

        // determines whether the element is currently empty
        bool isEmpty() const { return fElementKind == XML_ELEMENT_TYPE_INVALID; }

        explicit operator bool() const { return !isEmpty(); }

        // Returning information about the element
		ByteSpan nameSpan() const { return fNameSpan; }
		XmlName xmlName() const { return fXmlName; }
		ByteSpan tagName() const { return fXmlName.name(); }
		ByteSpan tagNamespace() const { return fXmlName.ns(); }
		std::string name() const { return std::string(fXmlName.name().fStart, fXmlName.name().fEnd); }
        
        int kind() const { return fElementKind; }
        void setKind(int kind) { fElementKind = kind; }

        ByteSpan data() const { return fData; }


        // Convenience for what kind of tag it is
        bool isStart() const { return (fElementKind == XML_ELEMENT_TYPE_START_TAG); }
        bool isSelfClosing() const { return fElementKind == XML_ELEMENT_TYPE_SELF_CLOSING; }
        bool isEnd() const { return fElementKind == XML_ELEMENT_TYPE_END_TAG; }
        bool isComment() const { return fElementKind == XML_ELEMENT_TYPE_COMMENT; }
        bool isProcessingInstruction() const { return fElementKind == XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION; }
        bool isContent() const { return fElementKind == XML_ELEMENT_TYPE_CONTENT; }
        bool isCData() const { return fElementKind == XML_ELEMENT_TYPE_CDATA; }
        bool isDoctype() const { return fElementKind == XML_ELEMENT_TYPE_DOCTYPE; }

    };

   
    struct XmlNode : public XmlElement, public XmlAttributeCollection
    {
        virtual bool loadProperties(const XmlAttributeCollection& attrs)
        {
            return false;
        }
        
        virtual void loadFromXmlElement(const XmlElement& elem)
        {
			// First assign
            XmlElement::operator=(elem);
            
            scanAttributes(elem.data());
            loadProperties(*this);
        }
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
        bool fSkipProcessingInstructions = true;
        bool fSkipWhitespace = true;
        bool fSkipCData = true;
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
    static XmlElement XmlElementGenerator(const XmlIteratorParams& params, XmlIteratorState& st)
    {
        XmlElement elem{};

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
                        content = chunk_trim(content, xmlwsp);
                        if (content)
                        {
                            // Set the state for next iteration
                            st.fSource++;
                            st.fMark = st.fSource;
                            elem.reset(XML_ELEMENT_TYPE_CONTENT, content);

                            return elem;
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

                if (chunk_starts_with_cstr(st.fSource, "?xml"))
                {
                    kind = XML_ELEMENT_TYPE_XMLDECL;
                    readTag(st.fSource, elementChunk);
                }
                else if (chunk_starts_with_cstr(st.fSource, "?"))
                {
                    kind = XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION;
                    readTag(st.fSource, elementChunk);
                }
                else if (chunk_starts_with_cstr(st.fSource, "!DOCTYPE"))
                {
                    kind = XML_ELEMENT_TYPE_DOCTYPE;
                    readDoctype(st.fSource, elementChunk);
                }
                else if (chunk_starts_with_cstr(st.fSource, "!--"))
                {
                    kind = XML_ELEMENT_TYPE_COMMENT;
                    readComment(st.fSource, elementChunk);
                }
                else if (chunk_starts_with_cstr(st.fSource, "![CDATA["))
                {
                    kind = XML_ELEMENT_TYPE_CDATA;
                    readCData(st.fSource, elementChunk);
                }
                else if (chunk_starts_with_cstr(st.fSource, "/"))
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

                return elem;
            }
            break;

            default:
                // Just advance to next character
                st.fSource++;
                break;

            }
        }

        return elem;
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
        const XmlElement & next() 
        {
            fCurrentElement = XmlElementGenerator(fParams, fState);
            return fCurrentElement;
        }
    };
}


