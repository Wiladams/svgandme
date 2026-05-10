#pragma once

#include "definitions.h"
#include "filter_program.h"
#include "filter_types.h"
#include "svgenums.h"   // BUGBUG - for SpaceUnitsKind
#include "surface.h"

#include <memory>


namespace waavs
{
    // Filter execution policy descriptors. These are used 
    // by the executor to determine how to handle various 
    // aspects of filter execution,

    // Determines how many input images the filter reads. 
    // This is used by the executor to determine how to 
    // resolve missing inputs (e.g. default to "last" or "sourceGraphic") 
    // and how to manage intermediate results.
    enum class FilterArity : uint8_t
    {
        Generator,  // no inputs, produces output from "thin air"
        Unary,      // one input, e.g. feGaussianBlur
        Binary,     // two inputs, e.g. feBlend
        MultiInput  // more than two inputs, e.g. feMerge
    };

    // Determines how the filter handles pixels outside the bounds of its input(s).
    enum class FilterOutsidePolicy : uint8_t
    {
        ClearOutput,        // pixels outside input bounds are treated as transparent black
        CopyInput,          // pixels outside input bounds are treated as the nearest edge pixel of the input
        GenerateEverywhere, // filter is expected to produce output even outside the bounds of its input (e.g. feTurbulence)
        PreserveDestination // for compositing filters: pixels outside input bounds are treated as the corresponding pixel from the destination (composite-atop semantics)
    };

    // Describes the expected access pattern of the filter during 
    // execution, which can inform optimization strategies.
    enum class FilterAccessPattern : uint8_t
    {
        Pointwise,      // e.g. feColorMatrix, which reads/writes each pixel independently
        Neighborhood,   // e.g. feGaussianBlur, which reads a local neighborhood of pixels for each output pixel
        Global,         // e.g. feBlend with a non-separable mode, which may read all pixels of both inputs before writing any output
        Procedural      // e.g. feTurbulence, which may read/write all pixels in an unpredictable pattern
    };

    // A descriptor that filter implementations provide to the executor,
    struct FilterExecPolicy
    {
        FilterArity arity;
        FilterOutsidePolicy outsidePolicy;
        FilterAccessPattern accessPattern;

        bool readsAlphaOnly;
        bool preservesAlpha;
        bool writesOpaque;
        bool requiresColorSpaceConversion;

        int haloX;
        int haloY;
    };

    // Well known filter traits
    struct ColorMatrixTraits
    {
        static constexpr FilterExecPolicy policy{
            FilterArity::Unary,
            FilterOutsidePolicy::ClearOutput,
            FilterAccessPattern::Pointwise,
            false,  // readsAlphaOnly
            false,  // preservesAlpha: depends on matrix, conservative false
            false,  // writesOpaque
            true,   // requiresColorSpaceConversion
            0,
            0
        };
    };

    struct ComponentTransferTraits
    {
        static constexpr FilterExecPolicy policy{
            FilterArity::Unary,
            FilterOutsidePolicy::ClearOutput,
            FilterAccessPattern::Pointwise,
            false,
            false,
            false,
            true,
            0,
            0
        };
    };



    struct GaussianBlurTraits
    {
        static constexpr FilterExecPolicy policy{
            FilterArity::Unary,
            FilterOutsidePolicy::ClearOutput,
            FilterAccessPattern::Neighborhood,
            false,
            false,
            false,
            true,
            0, // computed from stdDeviation later
            0
        };
    };


    // ------------------------------------------
    // Core data structures for filter execution
    // ------------------------------------------

    using SurfaceHolder = Surface;

    struct FilterIO
    {
        FilterColorInterpolation colorInterp;
        InternedKey in1{};
        InternedKey in2{};
        InternedKey out{};     // nullptr means "implicit last"
        bool hasIn2{ false };
        bool hasOut{ false };
    };

    struct FilterResolvedIO
    {
        InternedKey in1Key{};
        InternedKey in2Key{};
        InternedKey outKey{};

        Surface in1{};
        Surface in2{};
        Surface out;

        bool hasIn1 = false;
        bool hasIn2 = false;


        WGRectI primitiveAreaPx{};
        WGRectI writeAreaPx{};
        WGRectI readArea1Px{};
        WGRectI readArea2Px{};

        FilterColorInterpolation colorSpace{};
        FilterExecPolicy policy{};
    };







    // State associated with a running program
    struct FilterRunState
    {
        FilterColorInterpolation colorInterpolation{ FilterColorInterpolation::FILTER_COLOR_INTERPOLATION_LINEAR_RGB };

        // Filter-level coordinate systems
        SpaceUnitsKind filterUnits{ SpaceUnitsKind::SVG_SPACE_OBJECT };
        SpaceUnitsKind primitiveUnits{ SpaceUnitsKind::SVG_SPACE_USER };

        // Geometry resolved at apply time
        WGRectD objectBBoxUS{};
        WGRectD filterRectUS;
    };

    // Provide space conversion to primitives (dx/dy, stdDeviation, etc.).
    struct FilterSpace
    {
        WGMatrix3x3 ctm{};
        WGMatrix3x3 invCtm{};

        WGRectD filterRectUS{};     // Filter region in user space (float)
        WGRectI filterRectPX{};     // filter region in pixel space (int)

        // Local filter-surface user extents only
        // Origin should be normalized to 0,0
        WGRectD filterExtentUS{};


        double sx{ 1.0 }; // user->px scale estimate
        double sy{ 1.0 };

        double toPxX(double du) const noexcept { return du * sx; }
        double toPxY(double du) const noexcept { return du * sy; }
    };


    class FilterSpaceResolver
    {
    private:
        FilterSpace fSpace;
        FilterRunState* fRun;

    public:
        const FilterSpace& space() const noexcept { return fSpace; }

        WGRectD objectBBoxUS() const noexcept;
        SpaceUnitsKind primitiveUnits() const noexcept;

        WGRectD primitiveSubregionToUser(const WGRectD* subr) const noexcept;

        WGRectI userRectToPixelRect(const WGRectD& ur) const noexcept;

        WGRectI primitiveSubregionToPixelRect(const WGRectD* subr,
            double padUserX = 0.0,
            double padUserY = 0.0) const noexcept
        {
            WGRectD ur{};

            if (!subr)
            {
                ur = fRun->filterRectUS;
            }
            else if (fRun->primitiveUnits == SpaceUnitsKind::SVG_SPACE_OBJECT)
            {
                const auto& bb = fRun->objectBBoxUS;

                ur = WGRectD{
                    bb.x + subr->x * bb.w,
                    bb.y + subr->y * bb.h,
                    subr->w * bb.w,
                    subr->h * bb.h
                };
            }
            else
            {
                ur = *subr;
            }

            if (!(ur.w > 0.0) || !(ur.h > 0.0))
                return WGRectI{};

            if (padUserX || padUserY)
            {
                ur.x -= padUserX;
                ur.y -= padUserY;
                ur.w += 2.0 * padUserX;
                ur.h += 2.0 * padUserY;
            }

            const WGRectD px = mapRectAABB(fSpace.ctm, ur);

            int ix0 = int(std::floor(px.x - fSpace.filterRectPX.x));
            int iy0 = int(std::floor(px.y - fSpace.filterRectPX.y));
            int ix1 = int(std::ceil((px.x + px.w) - fSpace.filterRectPX.x));
            int iy1 = int(std::ceil((px.y + px.h) - fSpace.filterRectPX.y));

            return WGRectI{ ix0, iy0, ix1 - ix0, iy1 - iy0 };
        }


        WGPointD pixelCenterToFilterUser(int x, int y) const noexcept;


    };


    // FilterExecContext
    // 
    // Provides the execution context for filter primitives, 
    // including access to input/output images,
    //

    using SurfaceTable = std::unordered_map<InternedKey, Surface, InternedKeyHash, InternedKeyEquivalent>;


    struct FilterExecContext
    {
        FilterRunState& run;
        SurfaceTable fImages;
        FilterSpaceResolver& resolver;

        FilterColorInterpolation defaultColorInterp;


        Surface getImage(InternedKey key) noexcept
        {
            auto it = fImages.find(key);
            if (it != fImages.end())
                return it->second;
            return Surface{};
        }

        bool putImage(InternedKey key, Surface image) noexcept
        {
            if (!key || image.empty())
                return false;

            fImages[key] = image;

            return true;
        }

        // These implementations were not provided by default
        // I hope the following are the intended implementations

        InternedKey resolveInputKey(const FilterIO& io, uint32_t index) noexcept
        {
            if (index == 0)
                return io.in1;
            else if (index == 1)
                return io.in2;
            else
                return nullptr;
        }

        InternedKey resolveOutputKey(const FilterIO& io) noexcept
        {
            if (io.hasOut)
                return io.out;
            else
                return nullptr;
        }

        WGRectI resolvePrimitiveSubregionPx(const FilterIO& io, const WGRectD* subr) noexcept
        {
            WGRectI rc = {(int)(subr->x), (int)(subr->y), (int)(subr->w), (int)(subr->h)};
            return rc;
        }

        FilterColorInterpolation resolveColorSpace(const FilterIO& io) noexcept
        {
            // Color interpolation was resolved at parse time
            // so, we just return it here.
            return io.colorInterp;

        }
    };

    // FilterExecuteFn
    // 
    // This function type presents a normalized view of the execution
    // environment to anything that wants to implement a filter primitive.
    // 
    // Note:  The cool thing about this design is that the filter primitive 
    // implementations are completely decoupled from the executor.  
    // They can be implemented as objects, lambdas, provided as plugins, etc.
    // They don't need to know anything about the execution environment,
    // other than the information provided in the FilterExecContext and FilterResolvedIO.
    //
    using FilterExecuteFn = bool (*)(FilterExecContext& ctx,
        const FilterResolvedIO& rio,
        const void* payload) noexcept;



    struct IFilterPrimitiveExecutor
    {
        virtual ~IFilterPrimitiveExecutor() = default;

        virtual FilterOpId opId() const noexcept = 0;

        virtual FilterExecPolicy policy(const void* payload) const noexcept = 0;

        virtual bool execute(FilterExecContext& ctx,
            const FilterResolvedIO& rio,
            const void* payload) const noexcept = 0;
    };

    template<class Derived, class Traits, class PayloadT>
    struct FilterPrimitiveExecutorBase : IFilterPrimitiveExecutor
    {
        FilterExecPolicy policy(const void*) const noexcept override
        {
            return Traits::policy;
        }

        bool execute(FilterExecContext& ctx,
            const FilterResolvedIO& rio,
            const void* payload) const noexcept override
        {
            return static_cast<const Derived*>(this)->executeTyped(
                ctx,
                rio,
                *static_cast<const PayloadT*>(payload));
        }
    };

    static INLINE Surface createSurfaceHolder(size_t w, size_t h) noexcept
    {
        Surface out;
        (void)out.reset(w, h);
        return out;

    }

    static INLINE Surface createLikeSurfaceHolder(const Surface& like) noexcept
    {
        Surface out;
        (void)out.reset(like.width(), like.height());
        return out;
    }


    // filter_exec_prepare
    // 
    // Perform the common preparation steps for executing 
    // a filter primitive, including:
    //   - Resolving input and output keys to actual images
    //   - Determining the primitive area in pixel coordinates
    //   - Allocating the output surface
    //   - Determining the read/write areas based on the primitive area and halo
    //   - Handling the outside policy (e.g. clearing output, copying input, etc.)
    // Returns WG_SUCCESS if preparation succeeded and the primitive is ready to execute,
    // or some error if preparation failed (e.g. missing input image, allocation failure, etc.)
    static WGResult filter_exec_prepare(
        FilterExecContext& ctx,
        const FilterIO& io,
        const WGRectD* subr,
        const FilterExecPolicy& policy,
        FilterResolvedIO& rio) noexcept
    {
        rio.policy = policy;
        rio.colorSpace = ctx.resolveColorSpace(io);

        rio.outKey = ctx.resolveOutputKey(io);
        if (!rio.outKey)
            rio.outKey = filter::Filter_Last();

        rio.primitiveAreaPx = ctx.resolvePrimitiveSubregionPx(io, subr);

        switch (policy.arity)
        {
        case FilterArity::Generator:
            break;

        case FilterArity::Unary:
            rio.in1Key = ctx.resolveInputKey(io, 0);
            rio.in1 = ctx.getImage(rio.in1Key);
            if (rio.in1.empty())
                return false;
            break;

        case FilterArity::Binary:
            rio.in1Key = ctx.resolveInputKey(io, 0);
            rio.in2Key = ctx.resolveInputKey(io, 1);
            rio.in1 = ctx.getImage(rio.in1Key);
            rio.in2 = ctx.getImage(rio.in2Key);
            if (rio.in1.empty() || rio.in2.empty())
                return false;
            break;

        case FilterArity::MultiInput:
            // Merge can use a specialized path initially.
            return false;
        }

        const Surface model = !rio.in1.empty() ? rio.in1 : ctx.getImage(filter::SourceGraphic());
        if (model.empty())
            return false;

        rio.out = createLikeSurfaceHolder(model);
        if (rio.out.empty())
            return false;

        switch (policy.outsidePolicy)
        {
        case FilterOutsidePolicy::ClearOutput:
            rio.out.clearAll();
            break;

        case FilterOutsidePolicy::CopyInput:
            if (!rio.in1.empty())
                rio.out.blit(rio.in1, 0, 0);
            else
                rio.out.clearAll();
            break;

        case FilterOutsidePolicy::GenerateEverywhere:
            // Usually no clear needed, but clear is safe during migration.
            rio.out.clearAll();
            break;

        case FilterOutsidePolicy::PreserveDestination:
            break;
        }

        rio.writeAreaPx = intersection(rio.primitiveAreaPx, rio.out.boundsI());

        rio.readArea1Px = rio.writeAreaPx;
        rio.readArea2Px = rio.writeAreaPx;

        if (policy.haloX || policy.haloY)
        {
            rio.readArea1Px = wg_rectI_inflate(rio.readArea1Px, policy.haloX, policy.haloY);
            rio.readArea1Px = intersection(rio.readArea1Px, rio.in1.boundsI());
        }

        return true;
    }
}