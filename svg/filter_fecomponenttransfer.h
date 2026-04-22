#pragma once

#include "filter_types.h"
//#include "nametable.h"

namespace waavs
{
    static float applyTransferFunc(const ComponentFunc& f, float x) noexcept
    {
        x = clamp01f(x);

        switch (f.type)
        {
        default:
        case FILTER_TRANSFER_IDENTITY:
            return x;

        case FILTER_TRANSFER_LINEAR:
            return clamp01f(f.p0 * x + f.p1);

        case FILTER_TRANSFER_GAMMA:
            return clamp01f(f.p0 * std::pow(x, f.p1) + f.p2);

        case FILTER_TRANSFER_TABLE:
        {
            if (!f.table.p || f.table.n == 0)
                return x;

            if (f.table.n == 1)
                return clamp01f(f.table.p[0]);

            if (x >= 1.0f)
                return clamp01f(f.table.p[f.table.n - 1]);

            const float pos = x * float(f.table.n - 1);
            uint32_t i0 = (uint32_t)std::floor(pos);
            if (i0 >= f.table.n - 1)
                i0 = f.table.n - 2;

            const uint32_t i1 = i0 + 1;
            const float t = pos - float(i0);

            return clamp01f(f.table.p[i0] * (1.0f - t) + f.table.p[i1] * t);
        }

        case FILTER_TRANSFER_DISCRETE:
        {
            if (!f.table.p || f.table.n == 0)
                return x;

            if (x >= 1.0f)
                return clamp01f(f.table.p[f.table.n - 1]);

            uint32_t idx = (uint32_t)std::floor(x * float(f.table.n));
            if (idx >= f.table.n)
                idx = f.table.n - 1;

            return clamp01f(f.table.p[idx]);
        }
        }
    }


 
}
