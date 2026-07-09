// xml_scan_element.h

#pragma once

#include <cstring>

#include "lang_grammar.h"
#include "xmltypes.h"
#include "xml_scan.h"


namespace waavs
{
    static bool scan_to_tag_end_direct(ByteSpan& src, ByteSpan& attrSpan, bool& selfClosing) noexcept
    {
        attrSpan.reset();
        selfClosing = false;

        ByteSpan raw{};
        if (!bspan_read_until_unquoted(src, '>', raw))
            return false;

        bspan_rtrim(raw, chrWspChars);

        if (raw && *(raw.end() - 1) == '/')
        {
            selfClosing = true;
            raw.resetEnd(raw.end() - 1);
            bspan_rtrim(raw, chrWspChars);
        }

        attrSpan = raw;
        return true;
    }



    static bool read_xml_start_tag_direct(ByteSpan& src, XmlElement& elem) noexcept
    {
        ByteSpan tagName{};
        if (!xmlName_read(src, tagName))
            return false;

        ByteSpan attrs{};
        bool selfClosing = false;

        if (!scan_to_tag_end_direct(src, attrs, selfClosing))
            return false;

        elem.reset(
            selfClosing ? XML_ELEMENT_TYPE_SELF_CLOSING : XML_ELEMENT_TYPE_START_TAG,
            tagName,
            attrs);

        return true;
    }



    static bool read_xml_end_tag_direct(ByteSpan& src, XmlElement& elem) noexcept
    {
        bspan_skip_spaces(src);
        
        ByteSpan tagName{};
        if (!xmlName_read(src, tagName))
            return false;

        bspan_skip_spaces(src);

        if (!src || *src != '>')
            return false;

        ++src;

        elem.reset(XML_ELEMENT_TYPE_END_TAG, tagName, {});
        return true;
    }

    static bool read_xml_pi_direct(ByteSpan& src, XmlElement& elem) noexcept
    {
        // src points just after "<?"

        ByteSpan target{};
        if (!xmlName_read(src, target))
            return false;

        bspan_skip_spaces(src);

        const uint8_t* contentStart = src.begin();
        const uint8_t* p = contentStart;
        const uint8_t* end = src.end();

        while (p < end)
        {
            const uint8_t* q = static_cast<const uint8_t*>(
                std::memchr(p, '?', size_t(end - p)));

            if (!q)
                return false;

            if ((q + 1) < end && q[1] == '>')
            {
                ByteSpan content = ByteSpan::fromPointers(contentStart, q);
                src.resetStart(q + 2);

                int kind = XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION;
                if (target == "xml")
                    kind = XML_ELEMENT_TYPE_XMLDECL;

                elem.reset(kind, target, content);
                return true;
            }

            p = q + 1;
        }

        return false;
    }

    static bool read_xml_bang_construct_direct(ByteSpan& src, XmlElement& elem) noexcept
    {
        // src points just after "<!"
        // Existing readComment/readCData/readDoctype/readEntityDeclaration
        // expect the span to start at '!'.

        ByteSpan bangSrc = ByteSpan::fromPointers(src.begin() - 1, src.end());

        ByteSpan data{};

        if (bspan_starts_with(bangSrc, "!--"))
        {
            if (!readComment(bangSrc, data))
                return false;

            src.resetStart(bangSrc.begin());
            elem.reset(XML_ELEMENT_TYPE_COMMENT, {}, data);
            return true;
        }

        if (bspan_starts_with(bangSrc, "![CDATA["))
        {
            if (!readCData(bangSrc, data))
                return false;

            src.resetStart(bangSrc.begin());
            elem.reset(XML_ELEMENT_TYPE_CDATA, {}, data);
            return true;
        }

        if (bspan_starts_with(bangSrc, "!DOCTYPE"))
        {
            if (!readDoctype(bangSrc, data))
                return false;

            src.resetStart(bangSrc.begin());
            elem.reset(XML_ELEMENT_TYPE_DOCTYPE, {}, data);
            return true;
        }

        if (bspan_starts_with(bangSrc, "!ENTITY"))
        {
            if (!readEntityDeclaration(bangSrc, data))
                return false;

            src.resetStart(bangSrc.begin());
            elem.reset(XML_ELEMENT_TYPE_ENTITY, {}, data);
            return true;
        }

        return false;
    }

    // read_next_xml_element_direct
    //
    // Read the next xml element from the span
    // BUGBUG:
    // It's useful that this returns a bool value, because
    // a while() loop is easily constructed, but, it would
    // be even more useful if the return value was an XmlElement
    // instead, and just check the bool value for bool,
    // Then this can participate in chaining
    static bool read_next_xml_element_direct(const XmlIteratorParams &params, ByteSpan& input, XmlElement& elem) noexcept
    {
        elem.reset();

        ByteSpan src = input;

        while (src)
        {
            const uint8_t* start = src.begin();

            const uint8_t* lt = static_cast<const uint8_t*>(
                std::memchr(start, '<', src.size()));

            if (!lt)
                lt = src.end();

            if (lt != start)
            {
                ByteSpan text = ByteSpan::fromPointers(start, lt);
                src.resetStart(lt);

                if (params.fSkipWhitespace && isAllXmlWhitespace(text))
                    continue;

                elem.reset(XML_ELEMENT_TYPE_CONTENT, {}, text);

                input = src;
                return true;
            }

            // src is positioned at '<'
            ++src;

            if (!src)
                return false;

            bool ok = false;

            if (*src == '/')
            {
                ++src;
                ok = read_xml_end_tag_direct(src, elem);
            }
            else if (*src == '?')
            {
                ++src;
                ok = read_xml_pi_direct(src, elem);
            }
            else if (*src == '!')
            {
                ++src;
                ok = read_xml_bang_construct_direct(src, elem);
            }
            else if (xmlNameStartChars(*src))
            {
                ok = read_xml_start_tag_direct(src, elem);
            }
            else
            {
                return false;
            }

            if (!ok)
                return false;

            input = src;
            return true;
        }

        return false;
    }


}
