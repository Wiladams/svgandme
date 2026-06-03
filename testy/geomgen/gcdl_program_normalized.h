#pragma once

#include "gcdl_types.h"

namespace waavs
{
    struct GCDLNormalizedNode
    {
        InternedKey id = nullptr;
        InternedKey op = nullptr;

        std::vector<GeoRef> refs;
        std::vector<double> nums;

        GCDLNodeMeta meta{};
    };
}