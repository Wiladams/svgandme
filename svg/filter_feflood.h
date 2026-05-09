#pragma once

#include "filter_exec.h"

namespace waavs
{
    struct FloodTraits
    {
        static constexpr FilterExecPolicy policy{
            FilterArity::Generator,
            FilterOutsidePolicy::ClearOutput,
            FilterAccessPattern::Procedural,
            false,
            false,
            false,
            true,
            0,
            0
        };
    };

    struct FloodPayload
    {
        ColorSRGB color;
        float opacity;
    };

    struct FloodExecutor final
        : FilterPrimitiveExecutorBase<FloodExecutor,
        FloodTraits,
        FloodPayload>
    {
        FilterOpId opId() const noexcept override
        {
            return FilterOpId::FOP_FLOOD;
        }

        bool executeTyped(FilterExecContext& ctx,
            const FilterResolvedIO& rio,
            const FloodPayload& p) const noexcept
        {
            if (rio.out.empty())
                return false;

            if (rio.writeAreaPx.w <= 0 || rio.writeAreaPx.h <= 0)
                return ctx.putImage(rio.outKey, std::move(rio.out));

            ColorSRGB c = p.color;
            c.a *= p.opacity;

            WGResult r = wg_flood_rect(
                *rio.out,
                rio.writeAreaPx,
                c,
                rio.colorSpace);

            if (r != WG_SUCCESS)
                return false;

            return ctx.putImage(rio.outKey, std::move(rio.out));
        }
    };
}