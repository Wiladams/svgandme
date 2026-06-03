#pragma once

#include "gcdl_program_authored.h"
#include "xmlscan.h"

namespace waavs
{
    static bool gcdl_load_authored_start_tag(
        const XmlElement& elem,
        GCDLAuthoredNode& out) noexcept
    {
        out.clear();

        out.elem = elem.nameAtom();
        if (!out.elem)
            return false;

        if (!scanAttributes(out.attrs, elem.data()))
            return false;

        ByteSpan idSpan{};
        if (out.attrs.getValueBySpan("id", idSpan))
            out.id = PSNameTable::INTERN(idSpan);

        return true;
    }

    static bool gcdl_load_authored_self_closing(
        const XmlElement& elem,
        GCDLAuthoredNode& out) noexcept
    {
        return gcdl_load_authored_start_tag(elem, out);
    }

    static bool gcdl_parse_authored_xml_pull(
        XmlPull& pull,
        GCDLAuthoredProgram& out) noexcept
    {
        std::vector<GCDLAuthoredNode*> stack;

        while (pull.next())
        {
            const XmlElement& elem = *pull;

            if (elem.isStart())
            {
                GCDLAuthoredNode node;
                if (!gcdl_load_authored_start_tag(elem, node))
                    return false;

                if (stack.empty())
                {
                    if (out.rootNode.elem)
                        return false;

                    out.rootNode = node;
                    stack.push_back(&out.rootNode);
                }
                else
                {
                    GCDLAuthoredNode& parent = *stack.back();

                    if (!parent.addChild(node))
                        return false;

                    stack.push_back(&parent.children.back());
                }

                continue;
            }

            if (elem.isSelfClosing())
            {
                GCDLAuthoredNode node;
                if (!gcdl_load_authored_self_closing(elem, node))
                    return false;

                if (stack.empty())
                {
                    if (out.rootNode.elem)
                        return false;

                    out.rootNode = node;
                }
                else
                {
                    if (!stack.back()->addChild(node))
                        return false;
                }

                continue;
            }

            if (elem.isEnd())
            {
                if (stack.empty())
                    return false;

                if (stack.back()->elem != elem.nameAtom())
                    return false;

                stack.pop_back();
                continue;
            }

            // Ignore content, comments, PI, XML decl, doctype, CDATA, etc.
        }

        return out.rootNode.elem != nullptr && stack.empty();
    }

    static bool gcdl_create_authored_program_from_xml(
        const ByteSpan& src,
        GCDLAuthoredProgram& out) noexcept
    {
        out.clear();

        if (!out.resetSource(src))
            return false;

        XmlPull pull(out.source(), false);

        return gcdl_parse_authored_xml_pull(pull, out);
    }
}