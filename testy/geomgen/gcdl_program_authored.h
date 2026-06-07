#pragma once

#include "gcdl_types.h"
#include "membuff.h"
#include "xmlscan.h"

#include <vector>

namespace waavs
{
    using GCDLAttributeCollection = XmlAttributeCollection;


    struct GCDLAuthoredNode
    {
        InternedKey id = nullptr;
        InternedKey elem = nullptr;

        XmlAttributeCollection attrs{};
        std::vector<GCDLAuthoredNode> children;

        void clear()
        {
            id = nullptr;
            elem = nullptr;
            attrs.clear();
            children.clear();
        }

        bool addChild(const GCDLAuthoredNode& child)
        {
            if (!child.elem)
                return false;

            children.push_back(child);
            return true;
        }
    };



    static bool gcdl_find_authored_node_by_id(
        const GCDLAuthoredNode& rootNode,
        InternedKey id,
        const GCDLAuthoredNode*& out) noexcept
    {
        if (!id)
            return false;

        if (rootNode.id == id) {
            out = &rootNode;
            return true;
        }

        for (const auto& child : rootNode.children) {
            if (gcdl_find_authored_node_by_id(child, id, out))
                return true;
        }

        return false;
    }


    struct GCDLAuthoredProgram
    {
        MemBuff sourceMem{};
        GCDLAuthoredNode rootNode{};

        void clear()
        {
            sourceMem.reset();
            rootNode.clear();
        }

        bool empty() const
        {
            return rootNode.elem == nullptr;
        }

        ByteSpan source() const
        {
            return sourceMem.span();
        }

        bool resetSource(const ByteSpan& src)
        {
            clear();

            if (!sourceMem.resetFromSpan(src))
                return false;

            return true;
        }
    };
}