// xmlscan.h

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

#include "lang_grammar.h"

#include "xmltypes.h"
#include "xmlschema.h"



namespace waavs {
    // Constants for sizes of prefixes
    static constexpr size_t kCommentPrefixLen = 3;  // !--
    static constexpr size_t kCDataPrefixLen = 8;    // ![CDATA[
    static constexpr size_t kCDataPostfixLen = 3;   // ]]>
    static constexpr size_t kDoctypePrefixLen = 8;  // !DOCTYPE
    static constexpr size_t kEntityPrefixLen = 7;   // !ENTITY


    static constexpr charset xmlwsp(" \t\r\n");
    static constexpr charset xmlalpha("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    static constexpr charset xmldigit("0123456789");

    // isAllXmlWhitespace
    // 
    // Return true if all characters in the span are 
    // XML whitespace characters : " \t\r\n"
    //
    inline bool isAllXmlWhitespace(const ByteSpan& span) noexcept
    {
        bool allWhiteSpace = bspan_is_all(span, xmlwsp);
        return allWhiteSpace;
    }
}

namespace waavs
{
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
        //bool fAutoScanAttributes = false;
    };


    /*
    struct XmlElementIterator
    {
        ByteSpan input{};
        XmlIteratorParams params{};

        XmlElementIterator() = default;
        explicit XmlElementIterator(const ByteSpan& src) noexcept
            : input(src) {}
    };
    */
}

namespace waavs 
{

    static INLINE bool xmlName_read(ByteSpan& src, ByteSpan& out) noexcept
    {
        return identifier_read(src, out, xmlNameStartChars, xmlNameChars);
    }


    // Helper for bracketed parsing
    static INLINE bool readBangDelimited(
        ByteSpan& src,
        size_t prefixLen,
        const char* suffix,
        ByteSpan& dataChunk) noexcept
    {
        if (src.size() < prefixLen)
            return false;

        src.advance(prefixLen);

        ByteSpan endMark;
        if (!bspan_find_span(src, suffix, endMark))
            return false;

        dataChunk = ByteSpan::fromPointers(src.begin(), endMark.begin());

        src.resetStart(endMark.begin() + std::strlen(suffix));
        return true;
    }

    // readBangUntilGtQuoted
    // 
    // Helper for parsing constructs that start with '!' and end with '>', 
    // but can have quoted sections that contain '>' characters 
    // that should be ignored.

    static bool readBangUntilGtQuoted(
        ByteSpan& src,
        size_t prefixLen,
        ByteSpan& dataChunk) noexcept
    {
        if (src.size() < prefixLen)
            return false;

        src.advance(prefixLen);
        return bspan_read_until_unquoted(src, '>', dataChunk);
    }



    //============================================================
    // readCData()
    // starting: ![CDATA[
    //============================================================
    static bool readCData(ByteSpan& src, ByteSpan& dataChunk) noexcept
    {
        return readBangDelimited(src, kCDataPrefixLen, "]]>", dataChunk);
    }


    //============================================================
    // readComment()
    // starting: !--
    //============================================================
    
    static bool readComment(ByteSpan &src, ByteSpan &dataChunk) noexcept
    {
        return readBangDelimited(src, kCommentPrefixLen, "-->", dataChunk);
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
        return readBangUntilGtQuoted(src, kEntityPrefixLen, dataChunk);
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
        dataChunk.reset();

        if (src.size() < kDoctypePrefixLen)
            return false;

        src.advance(kDoctypePrefixLen);

        ByteSpan payload = src;
        const uint8_t* start = payload.begin();

        uint8_t quote = 0;
        int bracketDepth = 0;

        while (payload)
        {
            uint8_t c = *payload;
            ++payload;

            if (quote)
            {
                if (c == quote)
                    quote = 0;
                continue;
            }

            if (c == '"' || c == '\'')
            {
                quote = c;
                continue;
            }

            if (c == '[')
            {
                ++bracketDepth;
                continue;
            }

            if (c == ']')
            {
                if (bracketDepth > 0)
                    --bracketDepth;
                continue;
            }

            if (c == '>' && bracketDepth == 0)
            {
                dataChunk = ByteSpan::fromPointers(start, payload.begin() - 1);
                src = payload;
                return true;
            }
        }

        return false;
    }



    // parseDocTypeDecl()
    //
    // Read the internal subset of the doctype declaration, 
    // if it exists, and return it as a XmlDocTypeDecl
    static bool docTypeDecl_parse(ByteSpan payload, XmlDocTypeDecl& out) noexcept
    {
        bspan_skip_spaces(payload);

        out.rootName = bspan_read_until(payload, chrWspChars);
        if (!out.rootName)
            return false;

        bspan_skip_spaces(payload);

        if (!payload)
            return true;

        if (*payload == '[')
        {
            return bspan_read_enclosed(payload, out.internalSubset, '[', ']');
        }

        if (bspan_starts_with(payload, "PUBLIC"))
        {
            out.externalKind = ByteSpan("PUBLIC");
            payload.advance(6);

            if (!bspan_read_quoted(payload, out.publicId))
                return false;

            if (!bspan_read_quoted(payload, out.systemId))
                return false;
        }
        else if (bspan_starts_with(payload, "SYSTEM"))
        {
            out.externalKind = ByteSpan("SYSTEM");
            payload.advance(6);

            if (!bspan_read_quoted(payload, out.systemId))
                return false;
        }

        bspan_skip_spaces(payload);

        if (payload && *payload == '[')
        {
            return bspan_read_enclosed(payload, out.internalSubset, '[', ']');
        }

        return true;
    }

}


namespace waavs 
{

    // scanToTagEnd()
    // 
    // Skip past the attributes to the end of the tag '>' or '/>' as quickly as possible.
    // 
    // Preconditions:
    // - iter.fState.inTag == true
    // - iter.fState.input.fStart is positioned right after the tag name
    //   (i.e. at the beginning of attributes / whitespace)
    //
    /*
    static bool scanToTagEnd(XmlElementIterator& iter, ByteSpan& attrSpan, bool& selfClosing) noexcept
    {

        const unsigned char* p = iter.input.begin();
        const unsigned char* end = iter.input.end();

        const unsigned char* attrStart = p;

        selfClosing = false;
        unsigned char quote = 0;

        while (p < end)
        {
            unsigned char c = *p++;

            if (quote)
            {
                if (c == quote)
                    quote = 0;
                continue;
            }

            if (c == '"' || c == '\'')
            {
                quote = c;
                continue;
            }

            if (c == '>')
            {
                // Tag ends at p-1 (the '>')
                const unsigned char* gt = p - 1;

                // Self-close if immediately preceded by '/' (ignoring trailing whitespace).
                // This matches XML behavior like: <tag ... />
                const unsigned char* q = gt;
                while (q > attrStart && chrWspChars(*(q - 1)))
                    --q;

                if (q > attrStart && *(q - 1) == '/')
                {
                    selfClosing = true;
                    // Exclude that '/' from the attribute span (also exclude any whitespace before '>' if you want).
                    attrSpan = ByteSpan::fromPointers( attrStart, q - 1 );
                }
                else
                {
                    attrSpan = ByteSpan::fromPointers( attrStart, gt );
                }

                // Advance iterator past '>'
                iter.input.resetStart(p);
                return true;
            }
        }

        return false; // EOF before closing '>'
    }
    */
}


