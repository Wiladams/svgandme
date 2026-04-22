#pragma once

#include <memory>
#include <unordered_map>
#include <cstdint>
#include <cmath>

#include "nametable.h"
#include "svgb2ddriver.h"

#include "svgstructuretypes.h"
#include "filter_types.h"
#include "filter_program_exec.h"   
#include "filter_noise.h"
#include "viewport.h"


#include "filter_feblend.h"
#include "filter_fecolormatrix.h"
#include "filter_fecomponenttransfer.h"
#include "filter_fecomposite.h"
#include "filter_fediffuselight.h"
#include "filter_fegaussian.h"
#include "filter_femorphology.h"
#include "filter_fespecularlight.h"

namespace waavs
{
    struct IAmGroot;

    static INLINE BLCompOp blendModeToCompOp(FilterBlendMode mode) noexcept
    {
        static constexpr BLCompOp map[] =
        {
            BL_COMP_OP_SRC_OVER,      // NORMAL
            BL_COMP_OP_MULTIPLY,
            BL_COMP_OP_SCREEN,
            BL_COMP_OP_DARKEN,
            BL_COMP_OP_LIGHTEN,
            BL_COMP_OP_OVERLAY,
            BL_COMP_OP_COLOR_DODGE,
            BL_COMP_OP_COLOR_BURN,
            BL_COMP_OP_HARD_LIGHT,
            BL_COMP_OP_SOFT_LIGHT,
            BL_COMP_OP_DIFFERENCE,
            BL_COMP_OP_EXCLUSION
        };

        uint32_t i = (uint32_t)mode;
        if (i >= sizeof(map) / sizeof(map[0]))
            return BL_COMP_OP_SRC_OVER;

        return map[i];
    }



}

namespace waavs
{
    template<class SurfaceT>
    struct B2DFilterResourceResolver : public IFilterResourceResolver<SurfaceT>
    {
        //using Surface = SurfaceT;
        using ResourceHandle = std::unique_ptr<SurfaceT>;

        IAmGroot* fGroot{ nullptr };
        IRenderSVG* fRender{ nullptr };
        IAmFroot<SurfaceT>* fFroot{ nullptr };


        B2DFilterResourceResolver(
            IAmGroot* groot,
            IRenderSVG* render,
            IAmFroot<SurfaceT>* froot) noexcept
            : fGroot(groot)
            , fRender(render)
            , fFroot(froot)
        {
        }

        static INLINE bool isValidRect(const WGRectD& r) noexcept
        {
            return (r.w > 0.0) && (r.h > 0.0);
        }

        static INLINE WGRectD resolveDestRect(
            const FilterRunState& runState,
            const WGRectD* subr) noexcept
        {
            if (subr && isValidRect(*subr))
                return *subr;

            return runState.filterRectUS;
        }

        std::unique_ptr<Surface> createFilterSurface(const FilterRunState& runState) noexcept
        {
            if (!fFroot)
                return {};

            const int w = (int)std::ceil(runState.filterRectUS.w);
            const int h = (int)std::ceil(runState.filterRectUS.h);

            if (w <= 0 || h <= 0)
                return {};

            return fFroot->createSurfaceHandle((size_t)w, (size_t)h);
        }

        WGRectD computeReferencedViewBox(IViewable* elem) noexcept
        {
            if (!elem || !fRender || !fGroot)
                return {};

            // First-pass policy:
            // use object bounding box of referenced subtree as the source viewBox.
            return elem->objectBoundingBox();
        }

        static INLINE PreserveAspectRatio makePAR(
            AspectRatioAlignKind align,
            AspectRatioMeetOrSliceKind mos) noexcept
        {
            PreserveAspectRatio par;
            par.setAlign(align);
            par.setMeetOrSlice(mos);
            return par;
        }

        static INLINE WGMatrix3x3 makeUserToSurfaceMatrix(const FilterRunState& runState) noexcept
        {
            WGMatrix3x3 m = WGMatrix3x3::makeIdentity();
            m.translate(-runState.filterRectUS.x, -runState.filterRectUS.y);
            return m;
        }

        bool renderReferencedSubtree(SVGB2DDriver& ctx, IViewable* elem) noexcept
        {
            if (!elem || !fGroot)
                return false;

            elem->draw(&ctx, fGroot);

            return true;
        }

        std::unique_ptr<Surface> resolveFeImage(
            InternedKey imageKey,
            const FilterRunState& runState,
            const WGRectD* subr,
            AspectRatioAlignKind align,
            AspectRatioMeetOrSliceKind meetOrSlice) noexcept override
        {
            if (!fGroot || !fRender || !fFroot)
                return {};

            if (!imageKey)
                return {};

            ByteSpan href = imageKey;


            // First-pass implementation: local fragment refs only.
            if (href[0] != '#')
                return {};

            auto node = fGroot->findNodeByHref(href);
            if (!node)
                return {};

            auto elem = std::dynamic_pointer_cast<SVGGraphicsElement>(node);
            if (!elem)
                return {};

            if (!isValidRect(runState.filterRectUS))
                return {};

            WGRectD dstRect = resolveDestRect(runState, subr);
            if (!isValidRect(dstRect))
                return {};

            //WGRectD srcViewBox = dstRect;
            //WGRectD srcViewBox = computeReferencedViewBox(elem.get());
            //if (!isValidRect(srcViewBox))
            //    return {};

            auto out = createFilterSurface(runState);
            if (!out)
                return {};
            out->clearAll();

            //PreserveAspectRatio par = makePAR(align, meetOrSlice);
            WGMatrix3x3 fit = WGMatrix3x3::makeIdentity();

            //if (!computeViewBoxToViewport(dstRect, srcViewBox, par, fit))
            //    return {};

            WGMatrix3x3 userToSurface = makeUserToSurfaceMatrix(runState);

            SVGB2DDriver ctx{};
            ctx.attach(*out, 1);
            ctx.renew();

            //ctx.blendMode(BL_COMP_OP_SRC_OVER);

            ctx.push();

            // Shift user-space filter region origin to surface origin.
            ctx.transform(userToSurface);

            // Fit referenced content into the destination viewport.
            ctx.transform(fit);

            if (!renderReferencedSubtree(ctx, elem.get()))
            {
                ctx.pop();
                ctx.detach();
                return {};
            }

            ctx.pop();
            ctx.detach();

            return out;
        }
    };
}

namespace waavs {
    //============================================================
    // B2DFilterExecutor
    // Header-only reference implementation:
    //   - owns a key->Surface registry (unique_ptr)
    //   - renders SourceGraphic into offscreen Surface
    //   - executes FilterProgramStream using FilterProgramExecutor decoding
    //   - blits final (lastKey) result back into destination
    //============================================================
        // Provide space conversion to primitives (dx/dy, stdDeviation, etc.).
    struct FilterSpace {
        // semantic filter region as authored/resolved 
        // in user space.  Use this if you need the actual 
        // authored region.
        WGRectD filterRectUS{};

        // Pixel allocation region in device/surface pixels
        // This is the actual pixel area that we have allocated 
        // for the filter to render into.
        WGRectI filterRectPX{}; // device bbox (float)

        // Local filter-surface user extents only
        // Origin should be normalized to 0,0
        WGRectD filterExtentUS{};

        WGMatrix3x3 ctm{};
        WGMatrix3x3 invCtm{};
        double sx{ 1.0 }; // user->px scale estimate
        double sy{ 1.0 };

        double toPxX(double du) const noexcept { return du * sx; }
        double toPxY(double du) const noexcept { return du * sy; }
    };

    struct B2DFilterExecutor final : FilterProgramExecutor, IAmFroot<Surface>
    {
        std::unique_ptr<B2DFilterResourceResolver<Surface> > fResolver{nullptr};

        // --------------------------------------------------------
        // Registry storage
        // --------------------------------------------------------
        struct KeyHash {
            size_t operator()(InternedKey k) const noexcept { return (size_t)k; }
        };
        struct KeyEq { bool operator()(InternedKey a, InternedKey b) const noexcept { return a == b; } };

        using Handle = typename IAmFroot<Surface>::SurfaceHandle;

        std::unordered_map<InternedKey, SurfaceHandle, KeyHash, KeyEq> fImages{};
        InternedKey fLastKey{};

        // ----------------------------------------------
        // Space management
        // ----------------------------------------------
        FilterSpace fSpace{};


        // --------------------------------------------------------
        // IAmFrootBase / IAmFroot<PixelArray>
        // --------------------------------------------------------
        InternedKey lastKey() const noexcept override { return fLastKey; }
        void setLastKey(InternedKey k) noexcept override { fLastKey = k; }

        bool hasImage(InternedKey key) const noexcept override
        {
            return fImages.find(key) != fImages.end();
        }

        Surface* getImage(InternedKey key) noexcept override
        {
            auto it = fImages.find(key);
            if (it == fImages.end())
                return nullptr;
            return it->second.get();
        }

        const Surface* getImage(InternedKey key) const noexcept override
        {
            auto it = fImages.find(key);
            if (it == fImages.end())
                return nullptr;
            return it->second.get();
        }

        Surface* getInputImage(InternedKey inKey) noexcept
        {
            Surface* in = getImage(inKey);
            if (!in)
            {
                inKey = lastKey();
                in = getImage(lastKey());
            }
            return in;
        }

        bool putImage(InternedKey key, SurfaceHandle&& img) noexcept override
        {
            fImages[key] = std::move(img);
            return true;
        }

        void eraseImage(InternedKey key) noexcept override
        {
            fImages.erase(key);
            if (fLastKey == key)
                fLastKey = {};
        }

        void clearSurfaces() noexcept override
        {
            fImages.clear();
            fLastKey = {};
        }

        SurfaceHandle createSurfaceHandle(size_t w, size_t h) noexcept override
        {
            auto img = std::make_unique<Surface>();
            if (!img->reset((int)w, (int)h))
                return {};

            return img;
        }

        SurfaceHandle createLikeSurfaceHandle(const Surface& like) noexcept override
        {
            auto img = createSurfaceHandle(like.width(), like.height());
            return img;
        }

        SurfaceHandle copySurfaceHandle(const Surface& src) noexcept override
        {
            auto img = createLikeSurfaceHandle(src);
            if (!img)
                return {};

            img->blit(src, 0, 0);

            return img;
        }

        // --------------------------------------------------------
        // Helpers
        // --------------------------------------------------------

        WGRectI resolveSubregionPx(
            const WGRectD* subr,
            const Surface& like,
            double padUserX = 0.0,
            double padUserY = 0.0) const noexcept
        {
            const int W = (int)like.width();
            const int H = (int)like.height();

            WGRectI surfArea{ 0, 0, W, H };

            WGRectD ur{};

            if (!subr)
            {
                // No primitive subregion means the whole filter region.
                ur = fRunState.filterRectUS;
            }
            else if (fRunState.primitiveUnits == SpaceUnitsKind::SVG_SPACE_OBJECT)
            {
                const double bx = fRunState.objectBBoxUS.x;
                const double by = fRunState.objectBBoxUS.y;
                const double bw = fRunState.objectBBoxUS.w;
                const double bh = fRunState.objectBBoxUS.h;

                // Primitive subregion is in objectBoundingBox space.
                ur = WGRectD(
                    bx + subr->x * bw,
                    by + subr->y * bh,
                    subr->w * bw,
                    subr->h * bh
                );
            }
            else
            {
                // Primitive subregion is already in current user space.
                ur = *subr;
            }

            if (!(ur.w > 0.0) || !(ur.h > 0.0))
                return WGRectI{};

            // Inflate in user space for sampling kernels such as blur.
            if (padUserX > 0.0 || padUserY > 0.0)
            {
                ur.x -= padUserX;
                ur.y -= padUserY;
                ur.w += 2.0 * padUserX;
                ur.h += 2.0 * padUserY;
            }

            const WGRectD subrPX = mapRectAABB(fSpace.ctm, ur);

            int ix0 = (int)std::floor(subrPX.x - fSpace.filterRectPX.x);
            int iy0 = (int)std::floor(subrPX.y - fSpace.filterRectPX.y);
            int ix1 = (int)std::ceil((subrPX.x + subrPX.w) - fSpace.filterRectPX.x);
            int iy1 = (int)std::ceil((subrPX.y + subrPX.h) - fSpace.filterRectPX.y);

            WGRectI subArea{ ix0, iy0, ix1 - ix0, iy1 - iy0 };
            return intersection(subArea, surfArea);
        }



        // --------------------------------------
        // SourceAlpha implementation
        // --------------------------------------

        static  void extractAlpha_row_scalar( uint32_t* dst, const uint32_t* src, size_t n) noexcept
        {
            for (size_t i = 0; i < n; ++i)
            {
                // extract alpha to all channels, 
                // so that we can treat it as a single-channel 
                // heightfield for lighting calculations.
                auto alpha = argb32_unpack_alpha_u32(src[i]);
                uint32_t newValue = alpha;
                if (alpha > 0)
                {
                    newValue *= 0x01010101u; // replicate alpha to all channels
                }
                dst[i] = newValue;
            }
        }

#if defined(__ARM_NEON) || defined(__aarch64__)
        static INLINE void extractAlpha_row_neon(
            uint32_t* dst,
            const uint32_t* src,
            size_t n) noexcept
        {
            const uint32x4_t kAlphaMask = vdupq_n_u32(0xFF000000u);

            size_t i = 0;
            for (; i + 4 <= n; i += 4)
            {
                const uint32x4_t px = vld1q_u32(src + i);
                const uint32x4_t a = vandq_u32(px, kAlphaMask);
                vst1q_u32(dst + i, a);
            }

            // cleanup tail
            for (; i < n; ++i)
                dst[i] = src[i] & 0xFF000000u;
        }
#endif
        static  void extractAlpha_row(
            uint32_t* dst,
            const uint32_t* src,
            size_t n) noexcept
        {
#if defined(__ARM_NEON) || defined(__aarch64__)
            extractAlpha_row_scalar(dst, src, n);
#else
            extractAlpha_row_scalar(dst, src, n);
#endif
        }


        std::unique_ptr<Surface> makeSourceAlpha(const Surface& src)
        {
            auto out = createLikeSurfaceHandle(src);
            if (!out)
                return nullptr;

            const size_t w = src.width();
            const size_t h = src.height();

            // If they're both contiguous, we can treat the source
            // as one giant row
            if (src.isContiguous() && out->isContiguous())
            {
                const uint32_t* s = (const uint32_t*)src.data();
                uint32_t* d = (uint32_t*)out->data();

                extractAlpha_row(d, s, w * h);
                return out;
            }

            // If not contiguous, we have to go row by row
            for (size_t y = 0; y < h; ++y)
            {
                const uint32_t* s = (const uint32_t*)src.rowPointer(y);
                uint32_t* d = (uint32_t*)out->rowPointer(y);

                extractAlpha_row(d, s, w);
            }

            return out;
        }


        // --------------------------------------------------------
        // applyFilter()
        //
        //
        // SubtreeT requirements:
        //   WGRectD objectBoundingBox() const noexcept;
        //   void draw(IRenderSVG*, IAmGroot*);
        // --------------------------------------------------------




        // -------------------------------------------
        template<class SubtreeT>
        bool applyFilter(IRenderSVG* ctx,
            IAmGroot* groot,
            SubtreeT* subtree,
            const WGRectD& objectBBoxUS,
            const WGRectD& filterRectUS,
            const FilterProgramStream& program) noexcept
        {
            if (!ctx || !subtree)
                return false;

            // bail if the visible bounding box of the subtree
            // is empty, since the filter result won't be visible anyway.
            if (!(objectBBoxUS.w > 0.0) || !(objectBBoxUS.h > 0.0))
                return true;

            if (!(filterRectUS.w > 0.0) || !(filterRectUS.h > 0.0))
                return true;


            // Setup runstate as soon as possible
            fRunState.filterUnits = program.filterUnits;
            fRunState.primitiveUnits = program.primitiveUnits;
            fRunState.colorInterpolation = program.colorInterpolation;
            fRunState.filterRectUS = filterRectUS;
            fRunState.objectBBoxUS = objectBBoxUS;

            // get the current CTM from the context, 
            // which we will need for mapping the filter region 
            // to pixel space, and for providing space conversion 
            // to primitives that need it.
            const WGMatrix3x3 ctm = wgMatrix_from_BLMatrix2D(ctx->getTransform());

            // Map the full authored filter region to device space.
            const WGRectD nominalFilterRectPX = mapRectAABB(ctm, filterRectUS);

            // Clip the mapped filter region to the currently visible viewport.
            // This is the key step that prevents pathological allocations for
            // huge background geometry.
            //const WGRectD visibleUserClipUS = ctx->getViewportUserSpace();
            //const WGRectD visibleClipPX = mapRectAABB(ctm, visibleUserClipUS);
            //const WGRectD allocRectPX = intersection(nominalFilterRectPX, visibleClipPX);
            const WGRectD allocRectPX = nominalFilterRectPX;

            if (!(allocRectPX.w > 0.0) || !(allocRectPX.h > 0.0))
                return true;

            const int tileX = (int)std::floor(allocRectPX.x);
            const int tileY = (int)std::floor(allocRectPX.y);
            const int tileW = (int)std::ceil(allocRectPX.x + allocRectPX.w) - tileX;
            const int tileH = (int)std::ceil(allocRectPX.y + allocRectPX.h) - tileY;

            if (tileW <= 0 || tileH <= 0)
                return true;

            const uint32_t W = (uint32_t)tileW;
            const uint32_t H = (uint32_t)tileH;

            // Reset executor state
            clearSurfaces();
            setLastKey({});

            fSpace = {};
            fSpace.filterRectUS = filterRectUS;
            fSpace.filterExtentUS = WGRectD{ 0.0, 0.0, filterRectUS.w, filterRectUS.h };
            fSpace.filterRectPX = WGRectI{ tileX, tileY, tileW, tileH };
            fSpace.ctm = ctm;
            fSpace.invCtm = ctm;
            (void)fSpace.invCtm.invert();

            // Extract CTM scale magnitudes (user->pixel scale)
            {
                const double xvx = ctm.m00;
                const double xvy = ctm.m01;
                const double yvx = ctm.m10;
                const double yvy = ctm.m11;

                fSpace.sx = std::sqrt(xvx * xvx + xvy * xvy);
                fSpace.sy = std::sqrt(yvx * yvx + yvy * yvy);

                if (!(fSpace.sx > 0.0)) fSpace.sx = 1.0;
                if (!(fSpace.sy > 0.0)) fSpace.sy = 1.0;
            }


            // Setup resolver
            fResolver = std::make_unique<B2DFilterResourceResolver<Surface>>(groot, ctx, this);


            // --------------------------------------------------
            // Render SourceGraphic into tile-local surface
            // --------------------------------------------------

            auto srcGraphic = createSurfaceHandle(W, H);
            if (!srcGraphic)
                return false;
            
            // Start out with a blank slate
            //srcGraphic->clearAll();

            {
                WGMatrix3x3 off = ctm;
                off.postTranslate(-double(tileX), -double(tileY));

                SVGB2DDriver tmp{};
                tmp.attach(*srcGraphic, 1);
                tmp.renew();

                // DEBUG
                // draw a border to start
                //tmp.background(BLRgba32(0xFFff0000));
                //tmp.clearToBackground();
                //tmp.clear();

                // Establish correct device mapping
                tmp.transform(off);

                // Root object frame must match main rendering path
                tmp.setObjectFrame(objectBBoxUS);

                // Render subtree content only (avoid recursive filter call)
                subtree->drawContent(&tmp, groot);

                tmp.detach();
            }

            // DEBUG
            // blit immediately to check drawing
            //ctx->push();
            //ctx->background(BLRgba32(0xFF00ff00));
            //ctx->clearToBackground();
            //ctx->transform(BLMatrix2D::makeIdentity());
            //ctx->image(*srcGraphic, 0, 0);
            //ctx->pop();

            // Prime the pump by placing the image we just drew into 
            // the registry as the SourceGraphic.
            InternedKey srcGraphicKey = filter::Filter_SourceGraphic();
            if (!putImage(srcGraphicKey, std::move(srcGraphic)))
                return false;

            // make the alpha channel available as SourceAlpha, for primitives that need it.
            // BUGBUG - maybe this can be delayed and the filter primitives that need
            // it can request it on demand.
            auto srcGraphicPtr = getImage(srcGraphicKey);

            auto srcAlpha = makeSourceAlpha(*srcGraphicPtr);
            if (!srcAlpha)
                return false;

            if (!putImage(filter::Filter_SourceAlpha(), std::move(srcAlpha)))
                return false;

            // --------------------------------------------------
            // Execute filter primitives
            // --------------------------------------------------

            if (!FilterProgramExecutor::execute(program, *this))
                return false;

            // Resolve final output surface
            InternedKey outKey = lastKey();
            if (!outKey)
                outKey = filter::Filter_Last();

            Surface* outImg = getImage(outKey);
            if (!outImg)
                outImg = getImage(filter::Filter_SourceGraphic());
            if (!outImg)
                return false;

            // --------------------------------------------------
            // Composite result back to main context
            // --------------------------------------------------


            ctx->push();
            
            // Reset transform so tile is drawn in device space
            ctx->transform(WGMatrix3x3::makeIdentity());

            ctx->image(*outImg, double(tileX), double(tileY));
            ctx->flush();

            ctx->pop();

            return true;
        }


        // --------------------------------------------------------
        // FilterProgramExecutor hooks
        // --------------------------------------------------------
        bool onBeginProgram(const FilterProgramStream&) noexcept override
        {
            // If caller didn't set last, default to SourceGraphic if present.
            if (!lastKey() && hasImage(filter::Filter_SourceGraphic()))
                setLastKey(filter::Filter_SourceGraphic());
            return true;
        }

        // =============================================
        // feBlend
        // =============================================

        bool onBlend(const FilterIO& io, const WGRectD* subr, FilterBlendMode mode) noexcept override
        {
            InternedKey in1Key = resolveBinaryInput1Key(io);
            InternedKey in2Key = resolveBinaryInput2Key(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in1 = getInputImage(in1Key);
            Surface* in2 = getInputImage(in2Key);
            if (!in1 || !in2)
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(*in1);
            if (!out)
                return false;

            out->clearAll();

            WGRectI area = resolveSubregionPx(subr, *in1);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            const int W = min((int)in1->width(), (int)in2->width());
            const int H = min((int)in1->height(), (int)in2->height());

            const int x0 = max(area.x, 0);
            const int y0 = max(area.y, 0);
            const int x1 = min(area.x + area.w, W);
            const int y1 = min(area.y + area.h, H);

            if (x0 >= x1 || y0 >= y1)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            const WGRectI clipped{ x0, y0, x1 - x0, y1 - y0 };

            const WGBlendMode wgMode = to_wg_blend_mode(mode);



            const WGFilterColorSpace wgColorSpace =
                to_WGFilterColorSpace(io.colorInterp);

            if (wg_blend_rect(
                out->fInfo,
                in1->fInfo,
                in2->fInfo,
                clipped,
                wgMode,
                wgColorSpace) != WG_SUCCESS)
            {
                return false;
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }





        //==================================================
        // ------------------------------------------
        // onColorMatrix
        // unary
        // integer / half way to NEON
        // -------------------------------------------

        bool onColorMatrix(const FilterIO& io, const WGRectD* subr,
            FilterColorMatrixType type, float param, F32Span matrix) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getInputImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            // Preserve existing behavior:
            // copy the full input, then only rewrite the effected subregion.
            //out->blit(*in, 0, 0);
            out->clearAll();

            WGRectI area = resolveSubregionPx(subr, *in);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            const int W = (int)in->width();
            const int H = (int)in->height();

            const int x0 = max(area.x, 0);
            const int y0 = max(area.y, 0);
            const int x1 = min(area.x + area.w, W);
            const int y1 = min(area.y + area.h, H);

            if (x0 >= x1 || y0 >= y1)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            const WGRectI clipped{ x0, y0, x1 - x0, y1 - y0 };

            const WGFilterColorSpace cs = to_WGFilterColorSpace(io.colorInterp);

            ColorMatrixPrepared M{};
            if (!prepare_colormatrix(M, type, param, matrix, cs))
                return false;

            if (wg_colormatrix_rect(out->fInfo, in->fInfo, clipped, M) != WG_SUCCESS)
                return false;

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }

        // ----------------------------------------
        // onComponentTransfer()
        // unary
        // ----------------------------------------
        bool onComponentTransfer(const FilterIO& io, const WGRectD* subr,
            const ComponentFunc& rF,
            const ComponentFunc& gF,
            const ComponentFunc& bF,
            const ComponentFunc& aF) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getInputImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            // Current executor model for this primitive:
            // preserve the full input, then rewrite only the primitive region.
            out->blit(*in, 0, 0);

            WGRectI area = resolveSubregionPx(subr, *in);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            const int W = (int)in->width();
            const int H = (int)in->height();

            const int x0 = max(area.x, 0);
            const int y0 = max(area.y, 0);
            const int x1 = min(area.x + area.w, W);
            const int y1 = min(area.y + area.h, H);

            if (x0 >= x1 || y0 >= y1)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            const WGFilterColorSpace cs = to_WGFilterColorSpace(io.colorInterp);
            const bool useLinearRGB = (cs == WG_FILTER_COLORSPACE_LINEAR_RGB);

            for (int y = y0; y < y1; ++y)
            {
                const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)y);
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

                for (int x = x0; x < x1; ++x)
                {
                    const uint32_t px = srow[x];

                    // Stored surface pixels are premultiplied ARGB32.
                    const float a_p = dequantize0_255((px >> 24) & 0xFFu);
                    const float r_p = dequantize0_255((px >> 16) & 0xFFu);
                    const float g_p = dequantize0_255((px >> 8) & 0xFFu);
                    const float b_p = dequantize0_255((px >> 0) & 0xFFu);

                    // feComponentTransfer operates on non-premultiplied values.
                    float a = a_p;
                    float r = 0.0f;
                    float g = 0.0f;
                    float b = 0.0f;

                    if (a_p > 0.0f)
                    {
                        const float invA = 1.0f / a_p;
                        r = clamp01f(r_p * invA);
                        g = clamp01f(g_p * invA);
                        b = clamp01f(b_p * invA);
                    }

                    // Convert straight RGB into the primitive working color space.
                    if (useLinearRGB)
                    {
                        r = clamp01f(coloring_srgb_component_to_linear(r));
                        g = clamp01f(coloring_srgb_component_to_linear(g));
                        b = clamp01f(coloring_srgb_component_to_linear(b));
                    }

                    // Apply transfer functions in straight RGBA working space.
                    float rr = applyTransferFunc(rF, r);
                    float gg = applyTransferFunc(gF, g);
                    float bb = applyTransferFunc(bF, b);
                    float aa = applyTransferFunc(aF, a);

                    rr = clamp01f(rr);
                    gg = clamp01f(gg);
                    bb = clamp01f(bb);
                    aa = clamp01f(aa);

                    // Convert RGB back from working space to storage sRGB space.
                    if (useLinearRGB)
                    {
                        rr = clamp01f(coloring_linear_component_to_srgb(rr));
                        gg = clamp01f(coloring_linear_component_to_srgb(gg));
                        bb = clamp01f(coloring_linear_component_to_srgb(bb));
                    }

                    // Re-premultiply for storage.
                    rr *= aa;
                    gg *= aa;
                    bb *= aa;

                    drow[x] = argb32_pack_u8(
                        quantize0_255(aa),
                        quantize0_255(rr),
                        quantize0_255(gg),
                        quantize0_255(bb));
                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }

        /*
        bool onComponentTransfer(const FilterIO& io, const WGRectD* subr,
            const ComponentFunc& rF,
            const ComponentFunc& gF,
            const ComponentFunc& bF,
            const ComponentFunc& aF) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getInputImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            // In the case of component transfer, the entirety
            // of the input should be transferred to the output
            // and only pixels within the subarea should be modified
            // this is different than other primitives, where 
            // clearAll() is the preferred behavior.
            out->blit(*in, 0, 0);


            WGRectI area = resolveSubregionPx(subr, *in);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            // go row by row, applying the transfer functions to each pixel. 
            // This is not the fastest way to do this, but it's straightforward 
            // and serves as a reference implementation for now. 
            // If this becomes a bottleneck, we can optimize it with SIMD 
            // or by precomputing lookup tables for common transfer function types.
            for (int y = area.y; y < area.y + area.h; ++y)
            {
                const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)y);
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

                for (int x = area.x; x < area.x + area.w; ++x)
                {
                    const uint32_t px = srow[x];

                    // Stored pixels are premultiplied.

                    const float a_p = dequantize0_255((px >> 24) & 0xFF);
                    const float r_p = dequantize0_255((px >> 16) & 0xFF);
                    const float g_p = dequantize0_255((px >> 8) & 0xFF);
                    const float b_p = dequantize0_255((px >> 0) & 0xFF);


                    // feComponentTransfer operates on non-premultiplied values.
                    float a = a_p;
                    float r = 0.0f;
                    float g = 0.0f;
                    float b = 0.0f;

                    if (a_p > 0.0f)
                    {
                        const float invA = 1.0f / a_p;
                        r = clamp01f(r_p * invA);
                        g = clamp01f(g_p * invA);
                        b = clamp01f(b_p * invA);
                    }

                    // Apply transfer functions in straight RGBA space.
                    float rr = applyTransferFunc(rF, r);
                    float gg = applyTransferFunc(gF, g);
                    float bb = applyTransferFunc(bF, b);
                    float aa = applyTransferFunc(aF, a);

                    rr = clamp01f(rr);
                    gg = clamp01f(gg);
                    bb = clamp01f(bb);
                    aa = clamp01f(aa);

                    // Re-premultiply for storage.
                    rr *= aa;
                    gg *= aa;
                    bb *= aa;

                    //drow[x] = pack_argb32(aa, rr, gg, bb);
                    drow[x] = argb32_pack_u8(
                        quantize0_255(aa),
                        quantize0_255(rr),
                        quantize0_255(gg),
                        quantize0_255(bb));
                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }
        */


        //-----------------------------------------
        WGCompositeOp WGCompositeOpFromFilterCompositeOp(FilterCompositeOp op) noexcept
        {
            switch (op)
            {
            case FILTER_COMPOSITE_OVER: return WG_COMP_SRC_OVER; break;
            case FILTER_COMPOSITE_IN:   return WG_COMP_SRC_IN;   break;
            case FILTER_COMPOSITE_OUT:  return WG_COMP_SRC_OUT;  break;
            case FILTER_COMPOSITE_ATOP: return WG_COMP_SRC_ATOP; break;
            case FILTER_COMPOSITE_XOR:  return WG_COMP_SRC_XOR;  break;
            default: return WG_COMP_SRC_COPY;
            }

            return WG_COMP_SRC_COPY;
        }

        bool onComposite(const FilterIO& io, const WGRectD* subr,
            FilterCompositeOp op,
            float k1, float k2, float k3, float k4) noexcept override
        {
            // There are two fallback modes for inputs
            // 1) no input is specified: in this case, 
            // the input defaults to the current last output 
            // (or SourceGraphic if last is not set).
            // 2) The input is specified, but not found 
            // in the registry: in this case, the input will default
            // to the last output, or SourceGraphic

            // binary Operator Prolog
            // -------------------------------------
            InternedKey in1Key = resolveBinaryInput1Key(io);
            Surface* in1 = getInputImage(in1Key);

            InternedKey in2Key = resolveBinaryInput2Key(io);
            Surface* in2 = getInputImage(in2Key);

            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = filter::Filter_Last();

            if (!in1 || !in2)
                return false;

            auto out = createLikeSurfaceHandle(*in1);
            if (!out)
                return false;


            out->clearAll();

            WGRectI area = resolveSubregionPx(subr, *in1);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);
                return true;
            }

            const int W = min((int)in1->width(), (int)in2->width());
            const int H = min((int)in1->height(), (int)in2->height());

            const int x0 = max(area.x, 0);
            const int y0 = max(area.y, 0);
            const int x1 = min(area.x + area.w, W);
            const int y1 = min(area.y + area.h, H);

            if (x0 >= x1 || y0 >= y1)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);
                return true;
            }

            //-------------------------------------
            // Main body

            if (op == FILTER_COMPOSITE_ARITHMETIC)
            {
                const ArithmeticCompositeKind arithmeticKind = classifyArithmetic(k1, k2, k3, k4);
                const ArithmeticCoeffFx fx = makeArithmeticCoeffFx(k1, k2, k3, k4);

                for (int y = y0; y < y1; ++y)
                {
                    const uint32_t* s1 = (const uint32_t*)in1->rowPointer((size_t)y);
                    const uint32_t* s2 = (const uint32_t*)in2->rowPointer((size_t)y);
                    uint32_t* d = (uint32_t*)out->rowPointer((size_t)y);

                    switch (arithmeticKind)
                    {
                    case ARITH_ZERO:
                        arithmetic_fill_prgb32_row(d + x0, size_t(x1 - x0), 0);
                        //arithmetic_zero_prgb32_row(d + x0, size_t(x1 - x0));
                        break;

                    case ARITH_K1_ONLY:
                        arithmetic_k1_only_prgb32_row(d + x0, s1 + x0, s2 + x0, size_t(x1 - x0), fx);
                        break;

                    case ARITH_K2_ONLY:
                        if (arithmetic_is_zero(fx.k2))
                            arithmetic_zero_prgb32_row(d + x0, size_t(x1 - x0));
                        else if (arithmetic_is_one(fx.k2, fx.shift))
                            copy_prgb32_row(d + x0, s1 + x0, size_t(x1 - x0));
                        else
                            arithmetic_k2_only_prgb32_row(d + x0, s1 + x0, size_t(x1 - x0), fx);
                        break;

                    case ARITH_K3_ONLY:
                        if (arithmetic_is_zero(fx.k3))
                            arithmetic_zero_prgb32_row(d + x0, size_t(x1 - x0));
                        else if (arithmetic_is_one(fx.k3, fx.shift))
                            copy_prgb32_row(d + x0, s2 + x0, size_t(x1 - x0));
                        else
                            arithmetic_k3_only_prgb32_row(d + x0, s2 + x0, size_t(x1 - x0), fx);
                        break;
                    
                    case ARITH_K4_ONLY:
                    {
                        const uint8_t c = arith_k4_only_u8(fx.k4, fx.shift);
                        const uint32_t px = argb32_pack_u8(c, c, c, c);
                        arithmetic_fill_prgb32_row(d + x0, size_t(x1 - x0), px);
                        break;
                    }

                    case ARITH_K2_K3:
                        if (arithmetic_is_zero(fx.k2) && arithmetic_is_zero(fx.k3))
                            arithmetic_zero_prgb32_row(d + x0, size_t(x1 - x0));
                        else if (arithmetic_is_one(fx.k2, fx.shift) && arithmetic_is_zero(fx.k3))
                            copy_prgb32_row(d + x0, s1 + x0, size_t(x1 - x0));
                        else if (arithmetic_is_zero(fx.k2) && arithmetic_is_one(fx.k3, fx.shift))
                            copy_prgb32_row(d + x0, s2 + x0, size_t(x1 - x0));
                        else
                            arithmetic_k2_k3_prgb32_row(d + x0, s1 + x0, s2 + x0, size_t(x1 - x0), fx);
                        break;

                    case ARITH_K1_K2_K3:
                        arithmetic_k1_k2_k3_prgb32_row(d + x0, s1 + x0, s2 + x0, size_t(x1 - x0), fx);
                        break;

                    case ARITH_GENERAL:
                    default:
                        arithmetic_general_prgb32_row(d + x0, s1 + x0, s2 + x0, size_t(x1 - x0), fx);
                        break;
                    }
                }
            }
            else
            {
                WGCompositeOp surfOp = WGCompositeOpFromFilterCompositeOp(op);
                // Restrict work to the subregion via views
                Surface outArea;
                Surface in1Area;
                Surface in2Area;

                if (out->getSubSurface(WGRectI{ x0, y0, x1 - x0, y1 - y0 }, outArea) != WG_SUCCESS)
                    return false;
                if (in1->getSubSurface(WGRectI{ x0, y0, x1 - x0, y1 - y0 }, in1Area) != WG_SUCCESS)
                    return false;
                if (in2->getSubSurface(WGRectI{ x0, y0, x1 - x0, y1 - y0 }, in2Area) != WG_SUCCESS)
                    return false;

                outArea.blit(in2Area, 0, 0);
                outArea.compositeBlit(in1Area, 0, 0, surfOp);

                /*
                for (int y = y0; y < y1; ++y)
                {
                    const uint32_t* s1 = (const uint32_t*)in1->rowPointer((size_t)y);
                    const uint32_t* s2 = (const uint32_t*)in2->rowPointer((size_t)y);
                    uint32_t* d = (uint32_t*)out->rowPointer((size_t)y);

                    switch (op)
                    {
                    case FILTER_COMPOSITE_OVER:
                        composite_over_prgb32_row(d + x0, s1 + x0, s2 + x0, x1 - x0);
                        break;

                    case FILTER_COMPOSITE_IN:
                        composite_in_prgb32_row(d + x0, s1 + x0, s2 + x0, x1 - x0);
                        break;

                    case FILTER_COMPOSITE_OUT:
                        composite_out_prgb32_row(d + x0, s1 + x0, s2 + x0, x1 - x0);
                        break;

                    case FILTER_COMPOSITE_ATOP:
                        composite_atop_prgb32_row(d + x0, s1 + x0, s2 + x0, x1 - x0);
                        break;

                    case FILTER_COMPOSITE_XOR:
                        composite_xor_prgb32_row(d + x0, s1 + x0, s2 + x0, x1 - x0);
                        break;

                    default:
                        return false;
                    }
                }
                */
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }


        // ------------------------------------------
        // onConvolveMatrix
        // -------------------------------------------
        static INLINE uint32_t samplePixelEdge(const Surface& s, int x, int y, FilterEdgeMode edgeMode) noexcept
        {
            const int W = (int)s.width();
            const int H = (int)s.height();

            if (W <= 0 || H <= 0)
                return 0u;

            switch (edgeMode)
            {
            case FILTER_EDGE_WRAP:
                // Wrap coordinates periodically into the image extent.
                x %= W;
                y %= H;

                if (x < 0) x += W;
                if (y < 0) y += H;
                break;

            case FILTER_EDGE_NONE:
                // Outside image contributes transparent black.
                if (x < 0 || y < 0 || x >= W || y >= H)
                    return 0u;
                break;

            case FILTER_EDGE_DUPLICATE:
            default:
                // Clamp to nearest valid edge pixel.
                if (x < 0) x = 0;
                else if (x >= W) x = W - 1;

                if (y < 0) y = 0;
                else if (y >= H) y = H - 1;
                break;
            }

            const uint32_t* row = (const uint32_t*)s.rowPointer((size_t)y);
            return row[x];
        }

        bool onConvolveMatrix(const FilterIO& io, const WGRectD* subr,
            uint32_t orderX, uint32_t orderY,
            F32Span kernel,
            float divisor,
            float bias,
            uint32_t targetX,
            uint32_t targetY,
            FilterEdgeMode edgeMode,
            float kernelUnitLengthX,
            float kernelUnitLengthY,
            bool preserveAlpha) noexcept override
        {
            (void)kernelUnitLengthX;
            (void)kernelUnitLengthY;

            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getInputImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            if (!orderX || !orderY)
                return false;
            if (!kernel.p || kernel.n != orderX * orderY)
                return false;
            if (targetX >= orderX || targetY >= orderY)
                return false;

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            // Primitive result exists only inside its write area.
            out->clearAll();

            WGRectI area = resolveSubregionPx(subr, *in);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);
                return true;
            }

            // Spec default: if divisor is 0, use sum(kernel), except use 1 if sum is 0.
            if (divisor == 0.0f)
            {
                float sum = 0.0f;
                for (uint32_t i = 0; i < kernel.n; ++i)
                    sum += kernel.p[i];

                divisor = (sum != 0.0f) ? sum : 1.0f;
            }

            auto clamp_to_premul_invariant = [](float& a, float& r, float& g, float& b) noexcept
                {
                    a = clamp01f(a);
                    r = clamp01f(r);
                    g = clamp01f(g);
                    b = clamp01f(b);

                    if (r > a) r = a;
                    if (g > a) g = a;
                    if (b > a) b = a;
                };

            for (int y = area.y; y < area.y + area.h; ++y)
            {
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

                for (int x = area.x; x < area.x + area.w; ++x)
                {
                    float accR = 0.0f;
                    float accG = 0.0f;
                    float accB = 0.0f;
                    float accA = 0.0f;

                    uint32_t kidx = 0;
                    for (uint32_t ky = 0; ky < orderY; ++ky)
                    {
                        const int sy = y + int(ky) - int(targetY);

                        for (uint32_t kx = 0; kx < orderX; ++kx, ++kidx)
                        {
                            const int sx = x + int(kx) - int(targetX);
                            const uint32_t px = samplePixelEdge(*in, sx, sy, edgeMode);
                            const float w = kernel.p[kidx];

                            const float a = dequantize0_255((px >> 24) & 0xFFu);
                            const float rp = dequantize0_255((px >> 16) & 0xFFu);
                            const float gp = dequantize0_255((px >> 8) & 0xFFu);
                            const float bp = dequantize0_255((px >> 0) & 0xFFu);

                            if (preserveAlpha)
                            {
                                // Spec: temporarily unpremultiply color channels, convolve only color.
                                float r = 0.0f;
                                float g = 0.0f;
                                float b = 0.0f;

                                if (a > 0.0f)
                                {
                                    const float invA = 1.0f / a;
                                    r = clamp01f(rp * invA);
                                    g = clamp01f(gp * invA);
                                    b = clamp01f(bp * invA);
                                }

                                accR += r * w;
                                accG += g * w;
                                accB += b * w;
                            }
                            else
                            {
                                // Convolve stored premultiplied channels directly, including alpha.
                                accR += rp * w;
                                accG += gp * w;
                                accB += bp * w;
                                accA += a * w;
                            }
                        }
                    }

                    float aa;
                    float rr;
                    float gg;
                    float bb;

                    if (preserveAlpha)
                    {
                        const uint32_t src = ((const uint32_t*)in->rowPointer((size_t)y))[x];
                        aa = dequantize0_255((src >> 24) & 0xFFu);

                        rr = accR / divisor + bias;
                        gg = accG / divisor + bias;
                        bb = accB / divisor + bias;

                        rr = clamp01f(rr);
                        gg = clamp01f(gg);
                        bb = clamp01f(bb);
                        aa = clamp01f(aa);

                        // Re-premultiply once for storage.
                        rr *= aa;
                        gg *= aa;
                        bb *= aa;
                    }
                    else
                    {
                        // These are already premultiplied results in storage space.
                        rr = accR / divisor + bias;
                        gg = accG / divisor + bias;
                        bb = accB / divisor + bias;
                        aa = accA / divisor + bias;

                        clamp_to_premul_invariant(aa, rr, gg, bb);
                    }

                    drow[x] = argb32_pack_u8(
                        quantize0_255(aa),
                        quantize0_255(rr),
                        quantize0_255(gg),
                        quantize0_255(bb));
                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }

/*
        bool onConvolveMatrix(const FilterIO& io, const WGRectD* subr,
            uint32_t orderX, uint32_t orderY,
            F32Span kernel,
            float divisor,
            float bias,
            uint32_t targetX,
            uint32_t targetY,
            FilterEdgeMode edgeMode,
            float kernelUnitLengthX,
            float kernelUnitLengthY,
            bool preserveAlpha) noexcept override
        {
            (void)kernelUnitLengthX;
            (void)kernelUnitLengthY;

            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getInputImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

            if (!orderX || !orderY)
                return false;
            if (!kernel.p || kernel.n != orderX * orderY)
                return false;

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            WGRectI area = resolveSubregionPx(subr, *in);
            if (area.w <= 0 || area.h <= 0) {
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);
                return true;
            }

            if (divisor == 0.0f)
                divisor = 1.0f;

            for (int y = area.y; y < area.y + area.h; ++y)
            {
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

                for (int x = area.x; x < area.x + area.w; ++x)
                {
                    float accR = 0.0f;
                    float accG = 0.0f;
                    float accB = 0.0f;
                    float accA = 0.0f;

                    uint32_t kidx = 0;
                    for (uint32_t ky = 0; ky < orderY; ++ky)
                    {
                        const int sy = y + int(ky) - int(targetY);

                        for (uint32_t kx = 0; kx < orderX; ++kx, ++kidx)
                        {
                            const int sx = x + int(kx) - int(targetX);
                            const uint32_t px = samplePixelEdge(*in, sx, sy, edgeMode);
                            const float w = kernel.p[kidx];

                            const float a = float((px >> 24) & 0xFF) / 255.0f;
                            const float r = float((px >> 16) & 0xFF) / 255.0f;
                            const float g = float((px >> 8) & 0xFF) / 255.0f;
                            const float b = float((px >> 0) & 0xFF) / 255.0f;

                            accR += r * w;
                            accG += g * w;
                            accB += b * w;
                            if (!preserveAlpha)
                                accA += a * w;
                        }
                    }

                    float rr = accR / divisor + bias;
                    float gg = accG / divisor + bias;
                    float bb = accB / divisor + bias;
                    float aa;

                    if (preserveAlpha) {
                        const uint32_t src = ((const uint32_t*)in->rowPointer((size_t)y))[x];
                        aa = float((src >> 24) & 0xFF) / 255.0f;
                    }
                    else {
                        aa = accA / divisor + bias;
                    }

                    rr *= aa;
                    gg *= aa;
                    bb *= aa;

                    drow[x] = pack_argb32(aa, rr, gg, bb);

                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }
        */

        // ------------------------------------------
        // onDiffuseLighting
        // Type: lighting
        // 
		// LightingRGBA is coming in as a pre-multiplied color, 
        // It should always be fully opaque, so we should be 
        // able to ignore the alpha?
        // -------------------------------------------

        bool onDiffuseLighting(const FilterIO& io, const WGRectD* subr,
            const ColorSRGB & lightingColor, float surfaceScale, float diffuseConstant,
            float kernelUnitLengthX, float kernelUnitLengthY,
            uint32_t lightType, const LightPayload& light) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            // DEBUG - check input and output
            //printf("feDiffuseLighting inKey=%s outKey=%s\n", inKey, outKey);

            Surface* in = getInputImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            out->clearAll();

            WGRectI area = resolveSubregionPx(subr, *in);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

			// Turn the lightingRGBA into separate R,G,B components in [0,1] range.
			// We can use ColorSRGB here since the lighting color is in sRGB space, 
            float lcR, lcG, lcB;
            resolveColorInterpolationRGB(lightingColor, io.colorInterp, lcR, lcG, lcB);


            PixelToFilterUserMap map;
            map.surfaceW = int(in->width());
            map.surfaceH = int(in->height());
            
            //map.filterExtentUS = fSpace.filterExtentUS;
            map.filterExtentUS = fSpace.filterRectUS;

            map.uxPerPixel = (map.surfaceW > 0)

                ? float(map.filterExtentUS.w / double(map.surfaceW))
                : 1.0f;
            map.uyPerPixel = (map.surfaceH > 0)
                ? float(map.filterExtentUS.h / double(map.surfaceH))
                : 1.0f;

            float defaultDux = 1.0f;
            float defaultDuy = 1.0f;
            computeSpecularLocalPixelStep(map, area.x, area.y, defaultDux, defaultDuy);

            const float dux = (kernelUnitLengthX > 0.0f) ? kernelUnitLengthX : defaultDux;
            const float duy = (kernelUnitLengthY > 0.0f) ? kernelUnitLengthY : defaultDuy;

            LightPayload localLight = light;

            if (lightType == FILTER_LIGHT_POINT || lightType == FILTER_LIGHT_SPOT)
            {
                localLight.L[0] -= float(fSpace.filterRectUS.x);
                localLight.L[1] -= float(fSpace.filterRectUS.y);
            }

            if (lightType == FILTER_LIGHT_SPOT)
            {
                localLight.L[3] -= float(fSpace.filterRectUS.x);
                localLight.L[4] -= float(fSpace.filterRectUS.y);
            }

            const int W = int(in->width());
            const int H = int(in->height());

            const int yBeg = area.y;
            const int yEnd = area.y + area.h;
            const int xBeg = area.x;
            const int xCount = area.w;

            for (int y = yBeg; y < yEnd; ++y)
            {
                const int y0 = clamp(y - 1, 0, H - 1);
                const int y1 = clamp(y, 0, H - 1);
                const int y2 = clamp(y + 1, 0, H - 1);

                const uint32_t* row0 = (const uint32_t*)in->rowPointer((size_t)y0);
                const uint32_t* row1 = (const uint32_t*)in->rowPointer((size_t)y1);
                const uint32_t* row2 = (const uint32_t*)in->rowPointer((size_t)y2);
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

                DiffuseLightingRowParams p{};
                p.surfaceScale = surfaceScale;
                p.diffuseConstant = diffuseConstant;
                p.lcR = lcR;
                p.lcG = lcG;
                p.lcB = lcB;
                p.dux = dux;
                p.duy = duy;
                p.uxPerPixel = map.uxPerPixel;
                p.uyPerPixel = map.uyPerPixel;
                p.rowUy = (float(y) + 0.5f) * map.uyPerPixel;
                p.lightType = lightType;
                p.localLight = localLight;

                diffuseLighting_row(
                    drow,
                    row0,
                    row1,
                    row2,
                    xBeg,
                    xCount,
                    W,
                    p);
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }


        // ------------------------------------------
        // onDisplacementMap
        // Type: binary
        // -------------------------------------------

        // unpack a pre-multiplied ARGB32 pixel to straight Color4f. 
        // The RGB channels are divided by alpha, and clamped to [0,1]. 
        // If alpha is zero, RGB are set to zero.

        static INLINE void argb32_premul_unpack_channels_to_straight(
            uint32_t px,
            FilterChannelSelector xChannel,
            FilterChannelSelector yChannel,
            float& mxv,
            float& myv) noexcept
        {
            //const float k = 1.0f / 255.0f;

            const float a = dequantize0_255((px >> 24) & 0xFFu);

            auto decodeOne = [&](FilterChannelSelector ch) noexcept -> float
                {
                    switch (ch)
                    {
                    default:
                    case FILTER_CHANNEL_A:
                        return a;

                    case FILTER_CHANNEL_R:
                        if (a <= 0.0f)
                            return 0.0f;
                        return clamp01f((dequantize0_255((px >> 16) & 0xFFu)) / a);

                    case FILTER_CHANNEL_G:
                        if (a <= 0.0f)
                            return 0.0f;
                        return clamp01((dequantize0_255((px >> 8) & 0xFFu)) / a);

                    case FILTER_CHANNEL_B:
                        if (a <= 0.0f)
                            return 0.0f;
                        return clamp01((dequantize0_255((px >> 0) & 0xFFu)) / a);
                    }
                };

            if (xChannel == yChannel)
            {
                mxv = myv = decodeOne(xChannel);
                return;
            }

            mxv = decodeOne(xChannel);
            myv = decodeOne(yChannel);
        }

        /*
        static INLINE float selectMapChannel(const Color4f& c, FilterChannelSelector ch) noexcept
        {
            switch (ch)
            {
            default:
            case FILTER_CHANNEL_A: return c.a;
            case FILTER_CHANNEL_R: return c.r;
            case FILTER_CHANNEL_G: return c.g;
            case FILTER_CHANNEL_B: return c.b;
            }
        }
        */

        static INLINE int iroundf_fast(float v) noexcept
        {
            return int(v + (v >= 0.0f ? 0.5f : -0.5f));
        }

        bool onDisplacementMap(const FilterIO& io, const WGRectD* subr,
            float scale, FilterChannelSelector xChannel, FilterChannelSelector yChannel) noexcept override
        {
            InternedKey in1Key = resolveBinaryInput1Key(io);
            InternedKey in2Key = resolveBinaryInput2Key(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in1 = getInputImage(in1Key);
            Surface* in2 = getInputImage(in2Key);
            if (!in1 || !in2)
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(*in1);
            if (!out)
                return false;

            // start with transparent black
            out->clearAll();


            double scaleUS = (double)scale;
            switch (fRunState.primitiveUnits)
            {
            default:
            case SpaceUnitsKind::SVG_SPACE_USER:
                break;

            case SpaceUnitsKind::SVG_SPACE_OBJECT:
                scaleUS *= 0.5 * (fRunState.objectBBoxUS.w + fRunState.objectBBoxUS.h);
                break;

            case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                break;
            }

            const float scaleX = float(scaleUS * fSpace.sx);
            const float scaleY = float(scaleUS * fSpace.sy);


            WGRectI area = resolveSubregionPx(subr, *in1);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            const int W1 = (int)in1->width();
            const int H1 = (int)in1->height();
            const int W2 = (int)in2->width();
            const int H2 = (int)in2->height();

            const int W = min(W1, W2);
            const int H = min(H1, H2);

            if (W <= 0 || H <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }


            const Surface_ARGB32 surfIn1Info = in1->info();
            const Surface_ARGB32 surfIn2Info = in2->info();
            const Surface_ARGB32 surfOutInfo = out->info();

            const float biasX = -0.5f * scaleX;
            const float biasY = -0.5f * scaleY;

            // Compute the intersection of the primitive 
            // area and the surface bounds, to avoid unnecessary 
            // looping over out-of-bounds pixels.
            const int x0 = max(area.x, 0);
            const int y0 = max(area.y, 0);
            const int x1 = min(area.x + area.w, W);
            const int y1 = min(area.y + area.h, H);


            for (int y = y0; y < y1; ++y)
            {

                uint32_t* drow = Surface_ARGB32_row_pointer(&surfOutInfo,y);
                const uint32_t* mapRow = Surface_ARGB32_row_pointer_const(&surfIn2Info, y);

                for (int x = x0; x < x1; ++x)
                {
                    const uint32_t mp = mapRow[x];

                    // feDisplacementMap channel selectors conceptually operate on the
                    // channel values, not on premultiplied storage bytes. Recover
                    // straight RGB from PRGB storage before using them as map values.
                    float mxv, myv;
                    argb32_premul_unpack_channels_to_straight(mp, xChannel, yChannel, mxv, myv);

                    const float fx = float(x) + biasX + mxv * scaleX;
                    const float fy = float(y) + biasY + myv * scaleY;

                    const int sx = iroundf_fast(fx);
                    const int sy = iroundf_fast(fy);


                    if ((unsigned)sx < (unsigned)W1 && (unsigned)sy < (unsigned)H1)
                    {
                        const uint32_t* srow = Surface_ARGB32_row_pointer_const(&surfIn1Info, sy);

                        drow[x] = srow[sx];
                    }
                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }

        // ------------------------------------------
        // onDropShadow
        // Type: drop shadow
        // -------------------------------------------

        bool onDropShadow(const FilterIO& io, const WGRectD* subr,
            float dx, float dy, float sx, float sy,
            ColorSRGB srgb) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getInputImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            auto shadow0 = createLikeSurfaceHandle(*in);
            if (!shadow0)
                return false;

            auto shadow1 = createLikeSurfaceHandle(*in);
            if (!shadow1)
                return false;

            out->clearAll();
            shadow0->clearAll();
            shadow1->clearAll();

            const int W = (int)in->width();
            const int H = (int)in->height();

            auto intersectRect = [](const WGRectI& a, const WGRectI& b) noexcept -> WGRectI
                {
                    const int x0 = (a.x > b.x) ? a.x : b.x;
                    const int y0 = (a.y > b.y) ? a.y : b.y;
                    const int x1 = ((a.x + a.w) < (b.x + b.w)) ? (a.x + a.w) : (b.x + b.w);
                    const int y1 = ((a.y + a.h) < (b.y + b.h)) ? (a.y + a.h) : (b.y + b.h);

                    WGRectI r{};
                    r.x = x0;
                    r.y = y0;
                    r.w = x1 - x0;
                    r.h = y1 - y0;
                    if (r.w < 0) r.w = 0;
                    if (r.h < 0) r.h = 0;
                    return r;
                };

            auto expandRect = [](const WGRectI& r, int ex, int ey) noexcept -> WGRectI
                {
                    WGRectI out{};
                    out.x = r.x - ex;
                    out.y = r.y - ey;
                    out.w = r.w + ex * 2;
                    out.h = r.h + ey * 2;
                    return out;
                };

            const WGRectI surfaceRect{ 0, 0, W, H };

            WGRectI area = resolveSubregionPx(subr, *in);
            area = intersectRect(area, surfaceRect);

            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);
                return true;
            }

            // Build the colored shadow source from the input alpha.
            {
                Pixel_ARGB32 px = Pixel_ARGB32_premultiplied_from_ColorSRGB(srgb);

                uint8_t floodA, floodR, floodG, floodB;
                argb32_unpack_u8(px, floodA, floodR, floodG, floodB);
                //const uint8_t floodA = (uint8_t)((px >> 24) & 0xFFu);
                //const uint8_t floodR = (uint8_t)((px >> 16) & 0xFFu);
                //const uint8_t floodG = (uint8_t)((px >> 8) & 0xFFu);
                //const uint8_t floodB = (uint8_t)((px >> 0) & 0xFFu);

                for (int y = area.y; y < area.y + area.h; ++y)
                {
                    const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)y);
                    uint32_t* drow = (uint32_t*)shadow0->rowPointer((size_t)y);

                    for (int x = area.x; x < area.x + area.w; ++x)
                    {
                        const uint8_t sa = (uint8_t)((srow[x] >> 24) & 0xFFu);

                        const uint8_t a = (uint8_t)(mul255_round_u8(uint32_t(sa), uint32_t(floodA)));
                        const uint8_t r = (uint8_t)(mul255_round_u8(uint32_t(sa), uint32_t(floodR)));
                        const uint8_t g = (uint8_t)(mul255_round_u8(uint32_t(sa), uint32_t(floodG)));
                        const uint8_t b = (uint8_t)(mul255_round_u8(uint32_t(sa), uint32_t(floodB)));

                        drow[x] = argb32_pack_u8(a, r, g, b);
                    }
                }
            }

            // Resolve stdDeviation into user units.
            double stdXUS = sx;
            double stdYUS = sy;

            switch (fRunState.primitiveUnits)
            {
            default:
            case SpaceUnitsKind::SVG_SPACE_USER:
                break;
            case SpaceUnitsKind::SVG_SPACE_OBJECT:
                stdXUS *= fRunState.objectBBoxUS.w;
                stdYUS *= fRunState.objectBBoxUS.h;
                break;
            case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                break;
            }

            // Convert stdDeviation to pixel space.
            const double stdXPx = stdXUS * fSpace.sx;
            const double stdYPx = stdYUS * fSpace.sy;

            Surface* finalShadow = shadow0.get();

            if (stdXPx > 0.0 || stdYPx > 0.0)
            {
                int boxX[3] = { 1, 1, 1 };
                int boxY[3] = { 1, 1, 1 };

                boxesForGauss((stdXPx > 0.0) ? stdXPx : 0.0, 3, boxX);
                boxesForGauss((stdYPx > 0.0) ? stdYPx : 0.0, 3, boxY);

                int spreadX = 0;
                int spreadY = 0;

                for (int i = 0; i < 3; ++i)
                {
                    spreadX += (boxX[i] - 1) / 2;
                    spreadY += (boxY[i] - 1) / 2;
                }

                WGRectI blurArea = expandRect(area, spreadX, spreadY);
                blurArea = intersectRect(blurArea, surfaceRect);

                const Surface* curSrc = shadow0.get();
                Surface* curDst = shadow1.get();

                for (int pass = 0; pass < 3; ++pass)
                {
                    const int rx = (boxX[pass] - 1) / 2;
                    const int ry = (boxY[pass] - 1) / 2;

                    if (rx > 0)
                    {
                        curDst->clearAll();
                        boxBlurH_PRGB32(*curDst, *curSrc, rx, blurArea);
                        curSrc = curDst;
                        curDst = (curDst == shadow0.get()) ? shadow1.get() : shadow0.get();
                    }

                    if (ry > 0)
                    {
                        curDst->clearAll();
                        boxBlurV_PRGB32(*curDst, *curSrc, ry, blurArea);
                        curSrc = curDst;
                        curDst = (curDst == shadow0.get()) ? shadow1.get() : shadow0.get();
                    }
                }

                finalShadow = const_cast<Surface*>(curSrc);
            }

            // Resolve offset into user units.
            double dxUS = dx;
            double dyUS = dy;

            switch (fRunState.primitiveUnits)
            {
            default:
            case SpaceUnitsKind::SVG_SPACE_USER:
                break;
            case SpaceUnitsKind::SVG_SPACE_OBJECT:
                dxUS *= fRunState.objectBBoxUS.w;
                dyUS *= fRunState.objectBBoxUS.h;
                break;
            case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                break;
            }

            // Map the offset through the CTM linear part.
            const WGMatrix3x3& m = fSpace.ctm;
            const double dxPx = dxUS * m.m00 + dyUS * m.m10;
            const double dyPx = dxUS * m.m01 + dyUS * m.m11;

            // Draw the blurred shadow at its pixel offset, then draw the source over it.
            {
                SVGB2DDriver bctx{};
                bctx.attach(*out, 1);
                bctx.renew();

                out->clearAll();

                const double offX = std::floor(dxPx + 0.5);
                const double offY = std::floor(dyPx + 0.5);

                bctx.blendMode(BL_COMP_OP_SRC_COPY);
                bctx.image(*finalShadow, offX, offY);

                bctx.blendMode(BL_COMP_OP_SRC_OVER);
                bctx.image(*in, 0, 0);

                bctx.detach();
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }

        /*
        bool onDropShadow(const FilterIO& io, const WGRectD* subr,
            float dx, float dy, float sx, float sy,
            uint32_t rgbaPremul) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getInputImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            auto shadow = createLikeSurfaceHandle(*in);
            if (!shadow)
                return false;

            shadow->clearAll();
            out->clearAll();

            const int W = (int)in->width();
            const int H = (int)in->height();

            WGRectI area = resolveSubregionPx(subr, *in);
            if (area.w <= 0 || area.h <= 0) {
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);
                return true;
            }

            // Build colored shadow source from input alpha.
            {
                const uint8_t floodA = (uint8_t)((rgbaPremul >> 24) & 0xFF);
                const uint8_t floodR = (uint8_t)((rgbaPremul >> 16) & 0xFF);
                const uint8_t floodG = (uint8_t)((rgbaPremul >> 8) & 0xFF);
                const uint8_t floodB = (uint8_t)((rgbaPremul >> 0) & 0xFF);

                for (int y = area.y; y < area.y + area.h; ++y)
                {
                    const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)y);
                    uint32_t* drow = (uint32_t*)shadow->rowPointer((size_t)y);

                    for (int x = area.x; x < area.x + area.w; ++x)
                    {
                        const uint8_t sa = (uint8_t)((srow[x] >> 24) & 0xFF);

                        const uint8_t a = (uint8_t)((uint32_t(sa) * uint32_t(floodA) + 127u) / 255u);
                        const uint8_t r = (uint8_t)((uint32_t(sa) * uint32_t(floodR) + 127u) / 255u);
                        const uint8_t g = (uint8_t)((uint32_t(sa) * uint32_t(floodG) + 127u) / 255u);
                        const uint8_t b = (uint8_t)((uint32_t(sa) * uint32_t(floodB) + 127u) / 255u);

                        drow[x] = pack_argb32(a, r, g, b);
                    }
                }
            }

            // Blur the shadow if needed.
            double stdXUS = sx;
            double stdYUS = sy;

            switch (fRunState.primitiveUnits)
            {
            default:
            case SpaceUnitsKind::SVG_SPACE_USER:
                break;
            case SpaceUnitsKind::SVG_SPACE_OBJECT:
                stdXUS *= fRunState.objectBBoxUS.w;
                stdYUS *= fRunState.objectBBoxUS.h;
                break;
            case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                break;
            }

            const double stdXPx = stdXUS * fSpace.sx;
            const double stdYPx = stdYUS * fSpace.sy;
            const double sigma = (stdXPx > stdYPx) ? stdXPx : stdYPx;

            if (sigma > 0.0)
            {
                int box[3];
                waavs::boxesForGauss(sigma, 3, box);

                waavs::Surface tmp0;
                waavs::Surface tmp1;

                if (!tmp0.reset(shadow->width(), shadow->height()))
                    return false;
                if (!tmp1.reset(shadow->width(), shadow->height()))
                    return false;

                const waavs::Surface* curSrc = shadow.get();
                waavs::Surface* curDst = &tmp0;

                for (int pass = 0; pass < 3; ++pass)
                {
                    const int r = (box[pass] - 1) / 2;

                    waavs::boxBlurH_PRGB32(*curDst, *curSrc, r, area);
                    curSrc = curDst;
                    curDst = (curDst == &tmp0) ? &tmp1 : &tmp0;

                    waavs::boxBlurV_PRGB32(*curDst, *curSrc, r, area);
                    curSrc = curDst;
                    curDst = (curDst == &tmp0) ? &tmp1 : &tmp0;
                }

                shadow->clearAll();
                for (int y = area.y; y < area.y + area.h; ++y)
                {
                    const uint32_t* srow = (const uint32_t*)curSrc->rowPointer((size_t)y);
                    uint32_t* drow = (uint32_t*)shadow->rowPointer((size_t)y);
                    std::memcpy(drow + area.x, srow + area.x, (size_t)area.w * 4u);
                }
            }

            // Offset in user space mapped through CTM linear part.
            double dxUS = dx;
            double dyUS = dy;

            switch (fRunState.primitiveUnits)
            {
            default:
            case SpaceUnitsKind::SVG_SPACE_USER:
                break;
            case SpaceUnitsKind::SVG_SPACE_OBJECT:
                dxUS *= fRunState.objectBBoxUS.w;
                dyUS *= fRunState.objectBBoxUS.h;
                break;
            case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                break;
            }

            const WGMatrix3x3 & m = fSpace.ctm;
            const double dxPx = dxUS * m.m00 + dyUS * m.m10;
            const double dyPx = dxUS * m.m01 + dyUS * m.m11;

            {
                SVGB2DDriver bctx{};
                bctx.attach(*out, 1);
                bctx.renew();

                out->clearAll();

                bctx.blendMode(BL_COMP_OP_SRC_COPY);
                WGMatrix3x3 tmx = WGMatrix3x3::makeTranslation(dxPx, dyPx);
                bctx.transform(tmx);
                bctx.image(*shadow, 0, 0);

                bctx.transform(WGMatrix3x3::makeIdentity());
                bctx.blendMode(BL_COMP_OP_SRC_OVER);
                bctx.image(*in, 0, 0);

                bctx.detach();
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }
        */


        // ------------------------------------------
        // onFlood
        // Type: generator
        // -------------------------------------------

        bool onFlood(const FilterIO& io, const WGRectD* subr, const ColorSRGB &srgb) noexcept override
        {
            InternedKey outKey = resolveOutKeyStrict(io);

            // feFlood is a generator, so pick a surface that 
            // represents the current tile size.
            Surface* like = getInputImage(lastKey());

            if (!like)
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(*like);
            if (!out)
                return false;

            const int W = (int)out->width();
            const int H = (int)out->height();

            if (W <= 0 || H <= 0) {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);

                return true;
            }

            // No primitive subregion means flood the entire tile.
            // create the premultiplied value we'll use to 
            // fill in the surface
            Pixel_ARGB32 px = Pixel_ARGB32_premultiplied_from_ColorSRGB(srgb);
            if (!subr) {
                out->fillAll(px);
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);

                return true;
            }

            // Convert primitive subregion to tile-local pixel coordinates.
            const WGRectI area = resolveSubregionPx(subr, *out);

            // Outside the subregion remains transparent.
            out->clearAll();

            if (area.w > 0 && area.h > 0)
                out->fillRect(area, px);

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);

            return true;
        }

        // -----------------------------------------
        // onGaussianBlur()
        // unary
        // -----------------------------------------

        bool onGaussianBlur(const FilterIO& io, const WGRectD* subr,
            float sx, float sy) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            Surface* in = getInputImage(inKey);
            if (!in)
                return false;

            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            auto primLenToUser = [&](double v, double range) noexcept -> double {
                switch (fRunState.primitiveUnits) {
                default:
                case SpaceUnitsKind::SVG_SPACE_USER:
                    return v;
                case SpaceUnitsKind::SVG_SPACE_OBJECT:
                    return v * range;
                case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                    return v;
                }
                };

            if (sx < 0.0f) sx = 0.0f;
            if (sy < 0.0f) sy = 0.0f;

            const double sxUS = primLenToUser((double)sx, (double)fRunState.objectBBoxUS.w);
            const double syUS = primLenToUser((double)sy, (double)fRunState.objectBBoxUS.h);

            const double sxPx = sxUS * std::abs((double)fSpace.sx);
            const double syPx = syUS * std::abs((double)fSpace.sy);

            WGRectI writeArea = resolveSubregionPx(subr, *in);

            out->clearAll();

            if (writeArea.w <= 0 || writeArea.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);
                return true;
            }

            const bool doX = sxPx > 0.0;
            const bool doY = syPx > 0.0;

            if (!doX && !doY)
            {
                Surface srcView;
                if (!in->getSubSurface(writeArea, srcView))
                    return false;

                out->blit(srcView, writeArea.x, writeArea.y);

                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            int boxX[3] = { 1, 1, 1 };
            int boxY[3] = { 1, 1, 1 };

            if (doX)
                boxesForGauss(sxPx, 3, boxX);
            if (doY)
                boxesForGauss(syPx, 3, boxY);

            int rx[3] = { 0, 0, 0 };
            int ry[3] = { 0, 0, 0 };

            if (doX)
            {
                rx[0] = (boxX[0] - 1) / 2;
                rx[1] = (boxX[1] - 1) / 2;
                rx[2] = (boxX[2] - 1) / 2;
            }

            if (doY)
            {
                ry[0] = (boxY[0] - 1) / 2;
                ry[1] = (boxY[1] - 1) / 2;
                ry[2] = (boxY[2] - 1) / 2;
            }

            const int padPxX = rx[0] + rx[1] + rx[2];
            const int padPxY = ry[0] + ry[1] + ry[2];

            WGRectI sampleArea = writeArea;
            sampleArea.x -= padPxX;
            sampleArea.y -= padPxY;
            sampleArea.w += padPxX * 2;
            sampleArea.h += padPxY * 2;

            WGRectI surfaceBounds{ 0, 0, (int)in->width(), (int)in->height() };
            sampleArea = intersection(sampleArea, surfaceBounds);

            if (sampleArea.w <= 0 || sampleArea.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);
                return true;
            }

            Surface tmp0;
            Surface tmp1;

            if (!tmp0.reset(in->width(), in->height()))
                return false;
            if (!tmp1.reset(in->width(), in->height()))
                return false;

            tmp0.clearAll();
            tmp1.clearAll();

            const Surface* curSrc = in;
            Surface* curDst = &tmp0;

            for (int pass = 0; pass < 3; ++pass)
            {
                if (doX && rx[pass] > 0)
                {
                    boxBlurH_PRGB32(*curDst, *curSrc, rx[pass], sampleArea);
                    curSrc = curDst;
                    curDst = (curDst == &tmp0) ? &tmp1 : &tmp0;
                }

                if (doY && ry[pass] > 0)
                {
                    boxBlurV_PRGB32(*curDst, *curSrc, ry[pass], sampleArea);
                    curSrc = curDst;
                    curDst = (curDst == &tmp0) ? &tmp1 : &tmp0;
                }
            }

            const bool didAnyPass =
                (doX && (rx[0] > 0 || rx[1] > 0 || rx[2] > 0)) ||
                (doY && (ry[0] > 0 || ry[1] > 0 || ry[2] > 0));

            if (!didAnyPass)
            {
                Surface srcView;
                if (!in->getSubSurface(writeArea, srcView))
                    return false;

                out->blit(srcView, writeArea.x, writeArea.y);

                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            // Copy only the primitive result area to the output.
            {
                const Surface& blurred = *curSrc;
                for (int y = writeArea.y; y < writeArea.y + writeArea.h; ++y)
                {
                    const uint32_t* srow = (const uint32_t*)blurred.rowPointer((size_t)y);
                    uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);
                    std::memcpy(drow + writeArea.x, srow + writeArea.x, (size_t)writeArea.w * 4u);
                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }

        /*
        bool onGaussianBlur(const FilterIO& io, const WGRectD* subr,
            float sx, float sy) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            Surface* in = getInputImage(inKey);
            if (!in)
                return false;

            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            auto primLenToUser = [&](double v, double range) noexcept -> double {
                switch (fRunState.primitiveUnits) {
                default:
                case SpaceUnitsKind::SVG_SPACE_USER:
                    return v;
                case SpaceUnitsKind::SVG_SPACE_OBJECT:
                    return v * range;
                case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                    return v;
                }
                };


            if (sx < 0.0f) sx = 0.0f;
            if (sy < 0.0f) sy = 0.0f;

            const double sxUS = primLenToUser((double)sx, (double)fRunState.objectBBoxUS.w);
            const double syUS = primLenToUser((double)sy, (double)fRunState.objectBBoxUS.h);

            // Blur radius is a magnitude. Reflection / negative scale must not make sigma negative.
            const double sxPx = sxUS * std::abs((double)fSpace.sx);
            const double syPx = syUS * std::abs((double)fSpace.sy);

            // Primitive result is written only inside the authored subregion.
            WGRectI writeArea = resolveSubregionPx(subr, *in);

            out->clearAll();

            if (writeArea.w <= 0 || writeArea.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);
                return true;
            }

            // Fully degenerate blur: copy only the write area.
            const bool doX = sxPx > 0.0;
            const bool doY = syPx > 0.0;

            if (!doX && !doY)
            {
                for (int y = writeArea.y; y < writeArea.y + writeArea.h; ++y)
                {
                    const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)y);
                    uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);
                    std::memcpy(drow + writeArea.x, srow + writeArea.x, (size_t)writeArea.w * 4u);
                }

                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            // Build separate box approximations for X and Y.
            int boxX[3] = { 1, 1, 1 };
            int boxY[3] = { 1, 1, 1 };

            if (doX)
                boxesForGauss(sxPx, 3, boxX);
            if (doY)
                boxesForGauss(syPx, 3, boxY);

            int rx[3] = { 0, 0, 0 };
            int ry[3] = { 0, 0, 0 };

            if (doX)
            {
                rx[0] = (boxX[0] - 1) / 2;
                rx[1] = (boxX[1] - 1) / 2;
                rx[2] = (boxX[2] - 1) / 2;
            }

            if (doY)
            {
                ry[0] = (boxY[0] - 1) / 2;
                ry[1] = (boxY[1] - 1) / 2;
                ry[2] = (boxY[2] - 1) / 2;
            }

            // Compute exact pixel halo implied by the three box passes.
            const int padPxX = rx[0] + rx[1] + rx[2];
            const int padPxY = ry[0] + ry[1] + ry[2];

            WGRectI sampleArea = writeArea;
            sampleArea.x -= padPxX;
            sampleArea.y -= padPxY;
            sampleArea.w += padPxX * 2;
            sampleArea.h += padPxY * 2;

            WGRectI surfaceBounds{ 0, 0, (int)in->width(), (int)in->height() };
            sampleArea = intersection(sampleArea, surfaceBounds);

            if (sampleArea.w <= 0 || sampleArea.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);
                return true;
            }

            Surface tmp0;
            Surface tmp1;

            if (!tmp0.reset(in->width(), in->height()))
                return false;
            if (!tmp1.reset(in->width(), in->height()))
                return false;

            tmp0.clearAll();
            tmp1.clearAll();

            const Surface* curSrc = in;
            Surface* curDst = &tmp0;

            for (int pass = 0; pass < 3; ++pass)
            {
                if (doX && rx[pass] > 0)
                {
                    boxBlurH_PRGB32(*curDst, *curSrc, rx[pass], sampleArea);
                    curSrc = curDst;
                    curDst = (curDst == &tmp0) ? &tmp1 : &tmp0;
                }

                if (doY && ry[pass] > 0)
                {
                    boxBlurV_PRGB32(*curDst, *curSrc, ry[pass], sampleArea);
                    curSrc = curDst;
                    curDst = (curDst == &tmp0) ? &tmp1 : &tmp0;
                }
            }

            // If one axis had sigma > 0 but boxes collapsed to zero radii,
            // fall back to copying the write area from the input.
            const bool didAnyPass =
                (doX && (rx[0] > 0 || rx[1] > 0 || rx[2] > 0)) ||
                (doY && (ry[0] > 0 || ry[1] > 0 || ry[2] > 0));

            if (!didAnyPass)
            {
                for (int y = writeArea.y; y < writeArea.y + writeArea.h; ++y)
                {
                    const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)y);
                    uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);
                    std::memcpy(drow + writeArea.x, srow + writeArea.x, (size_t)writeArea.w * 4u);
                }

                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            // Copy only the primitive result area to the output.
            {
                const Surface& blurred = *curSrc;
                for (int y = writeArea.y; y < writeArea.y + writeArea.h; ++y)
                {
                    const uint32_t* srow = (const uint32_t*)blurred.rowPointer((size_t)y);
                    uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);
                    std::memcpy(drow + writeArea.x, srow + writeArea.x, (size_t)writeArea.w * 4u);
                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }
        */



        // -----------------------------------------
        // onImage()
        // Type: generator
        // -----------------------------------------
        bool onImage(const FilterIO& io, 
            const WGRectD* subr, 
            InternedKey imageKey,
            AspectRatioAlignKind align, 
            AspectRatioMeetOrSliceKind mos) noexcept override
        {
            if (!fResolver)
                return false;

            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = fResolver->resolveFeImage(imageKey, fRunState, subr, align, mos);
            if (!out)
                return false;

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }

        // ------------------------------------------
        // onMerge
        // ------------------------------------------
        bool onMerge(const FilterIO& io, const WGRectD* subr, KeySpan inputs) noexcept override
        {
            if (!inputs.n)
                return true;

            // Check that at least one of the inputs resolves to an actual image,
            // choose the first valid image as the basis for output image parameters.
            Surface* base = nullptr;
            for (uint32_t i = 0; i < inputs.n; ++i)
            {
                InternedKey k = resolveInKey(inputs.p[i]);
                Surface* img = getImage(k);
                if (img)
                {
                    base = img;
                    break;
                }
            }

            // if we didn't find at leastt one valid input image,
            // we have nothing to do, so fail.
            if (!base)
                return false;

            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(*base);
            if (!out)
                return false;

            out->clearAll();

            WGRectI area = resolveSubregionPx(subr, *base);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;
                
                setLastKey(outKey);

                return true;
            }

            // Setup a drawing context to compose the inputs
            // one by one.
            // 
            {
                SVGB2DDriver bctx{};
                bctx.attach(*out, 1);
                //bctx.renew();

                bctx.push();
                //bctx.clipRect(WGRectD(area.x, area.y, area.w, area.h));
                bctx.blendMode(BL_COMP_OP_SRC_OVER);

                for (uint32_t i = 0; i < inputs.n; ++i)
                {
                    InternedKey k = resolveInKey(inputs.p[i]);
                    Surface* img = getImage(k);
                    if (!img)
                        continue;

                    bctx.image(*img, 0, 0);
                }

                bctx.pop();
                bctx.detach();
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }

        // -----------------------------------------
        // onMorphology()
        // Type: unary
        // -----------------------------------------

        bool onMorphology(const FilterIO& io, const WGRectD* subr,
            FilterMorphologyOp op, float rx, float ry) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getInputImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;



            double rxUS = (double)rx;
            double ryUS = (double)ry;

            switch (fRunState.primitiveUnits)
            {
            default:
            case SpaceUnitsKind::SVG_SPACE_USER:
                break;
            case SpaceUnitsKind::SVG_SPACE_OBJECT:
                rxUS *= fRunState.objectBBoxUS.w;
                ryUS *= fRunState.objectBBoxUS.h;
                break;
            case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                break;
            }

            int rpx = (int)std::floor(rxUS * fSpace.sx + 0.5);
            int rpy = (int)std::floor(ryUS * fSpace.sy + 0.5);

            if (rpx < 0) rpx = 0;
            if (rpy < 0) rpy = 0;

            WGRectI area = resolveSubregionPx(subr, *in);
            if (area.w <= 0 || area.h <= 0)
            {
                //out->clearAll();

                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            const int W = (int)in->width();
            const int H = (int)in->height();

            auto tmp = createLikeSurfaceHandle(*in);
            if (!tmp)
                return false;

            out->clearAll();
            tmp->clearAll();

            const int x0 = area.x;
            const int x1 = area.x + area.w - 1;
            const int y0 = area.y;
            const int y1 = area.y + area.h - 1;

            const int tmpY0 = (y0 - rpy < 0) ? 0 : (y0 - rpy);
            const int tmpY1 = (y1 + rpy >= H) ? (H - 1) : (y1 + rpy);

            MorphDequeScratch rowScratch;
            MorphDequeScratch colScratch;

            if (!rowScratch.ensureCapacity((x1 - x0 + 1) + (rpx * 2)))
                return false;

            if (!colScratch.ensureCapacity((y1 - y0 + 1) + (rpy * 2)))
                return false;

            if (op == FILTER_MORPHOLOGY_ERODE)
            {
                for (int y = tmpY0; y <= tmpY1; ++y)
                {
                    uint32_t* drow = (uint32_t*)tmp->rowPointer((size_t)y);
                    const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)y);

                    if (!erode_row(drow, srow, x0, x1, W, rpx, rowScratch))
                        return false;
                }

                if (!erode_col(*out, *tmp, x0, x1, y0, y1, H, rpy, colScratch))
                    return false;
            }
            else
            {
                for (int y = tmpY0; y <= tmpY1; ++y)
                {
                    uint32_t* drow = (uint32_t*)tmp->rowPointer((size_t)y);
                    const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)y);

                    if (!dilate_row(drow, srow, x0, x1, W, rpx, rowScratch))
                        return false;
                }

                if (!dilate_col(*out, *tmp, x0, x1, y0, y1, H, rpy, colScratch))
                    return false;
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);

            return true;
        }


        // -----------------------------------------
        // onOffset
        // unary
        // -----------------------------------------

        bool onOffset(const FilterIO& io, const WGRectD* subr, float dx, float dy) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getInputImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            out->clearAll();

            const WGRectI area = resolveSubregionPx(subr, *in);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);
                return true;
            }

            // Convert dx/dy from primitive units to user space.
            double dxUS = (double)dx;
            double dyUS = (double)dy;

            switch (fRunState.primitiveUnits)
            {
            default:
            case SpaceUnitsKind::SVG_SPACE_USER:
                break;

            case SpaceUnitsKind::SVG_SPACE_OBJECT:
                dxUS *= (double)fRunState.objectBBoxUS.w;
                dyUS *= (double)fRunState.objectBBoxUS.h;
                break;

            case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                break;
            }

            // Map the offset vector through the linear part of the CTM.
            const WGMatrix3x3& m = fSpace.ctm;
            const int offX = (int)std::lround(dxUS * m.m00 + dyUS * m.m10);
            const int offY = (int)std::lround(dxUS * m.m01 + dyUS * m.m11);

            // Restrict writes to the resolved primitive subregion.
            // This preserves the behavior of the existing implementation:
            // output is clear everywhere except where the translated image lands
            // inside 'area'.
            Surface outArea;
            if (out->getSubSurface(area, outArea) != WG_SUCCESS)
                return false;

            // 'outArea' has local coordinates [0..area.w, 0..area.h), but maps to
            // global destination coordinates [area.x..area.x+area.w, area.y..area.y+area.h).
            //
            // A global blit at (offX, offY) becomes a local blit at:
            //   (offX - area.x, offY - area.y)
            //
            // wg_blit()/Surface::blit() will clip source and destination automatically.
            outArea.blit(*in, offX - area.x, offY - area.y);

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }

        /*
bool onOffset(const FilterIO& io, const WGRectD* subr, float dx, float dy) noexcept override
{
    InternedKey inKey = resolveUnaryInputKey(io);
    InternedKey outKey = resolveOutKeyStrict(io);

    Surface* in = getInputImage(inKey);
    if (!in)
        return false;

    if (!outKey)
        outKey = filter::Filter_Last();

    auto out = createLikeSurfaceHandle(*in);
    if (!out)
        return false;

    out->clearAll();

    WGRectI area = resolveSubregionPx(subr, *in);
    if (area.w <= 0 || area.h <= 0)
    {
        if (!putImage(outKey, std::move(out)))
            return false;
        setLastKey(outKey);
        return true;
    }

    // Convert dx/dy from primitive units to user space.
    double dxUS = (double)dx;
    double dyUS = (double)dy;

    switch (fRunState.primitiveUnits)
    {
    default:
    case SpaceUnitsKind::SVG_SPACE_USER:
        break;

    case SpaceUnitsKind::SVG_SPACE_OBJECT:
        dxUS *= (double)fRunState.objectBBoxUS.w;
        dyUS *= (double)fRunState.objectBBoxUS.h;
        break;

    case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
        break;
    }

    // Map the offset vector through the linear part of the CTM.
    const WGMatrix3x3 & m = fSpace.ctm;
    const int offX = (int)std::lround(dxUS * m.m00 + dyUS * m.m10);
    const int offY = (int)std::lround(dxUS * m.m01 + dyUS * m.m11);

    const int W = (int)in->width();
    const int H = (int)in->height();

    const int dstX0 = area.x;
    const int dstX1 = area.x + area.w;

    // figure out a clip rectangle so we can use 
    // get a subarea of the source image
    // to copy from.
    WGRectI srcArea = area;
    srcArea.x += offX;
    srcArea.y += offY;
    //WGRectI dstArea = { 0,0,area.w, area.h };
    WGRectI srcClip = intersection(srcArea, WGRectI(0, 0, W, H));

    for (int y = area.y; y < area.y + area.h; ++y)
    {
        const int sy = y - offY;
        if ((unsigned)sy >= (unsigned)H)
            continue;

        // Destination span [dstX0, dstX1) maps to source span [dstX0-offX, dstX1-offX).
        // Clip that mapping so source stays within [0, W).
        const int copyDstX0 = std::max(dstX0, offX);
        const int copyDstX1 = std::min(dstX1, W + offX);

        if (copyDstX1 <= copyDstX0)
            continue;

        const int sx0 = copyDstX0 - offX;
        const size_t count = (size_t)(copyDstX1 - copyDstX0);

        uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);
        const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)sy);

        std::memcpy(drow + copyDstX0, srow + sx0, count * sizeof(uint32_t));
    }

    if (!putImage(outKey, std::move(out)))
        return false;

    setLastKey(outKey);
    return true;
}
        */



        // -----------------------------------------
        // onSpecularLighting
        // ------------------------------------------
        //     // pixelHeightFromAlpha()
    //
    // Interprets the alpha channel of a pixel as a height value in [0,1].
    //
static INLINE float pixelHeightFromAlpha(const Surface& s, int x, int y) noexcept
{
    const int W = (int)s.width();
    const int H = (int)s.height();

    if (W <= 0 || H <= 0)
        return 0.0f;

    if (x < 0) x = 0;
    else if (x >= W) x = W - 1;

    if (y < 0) y = 0;
    else if (y >= H) y = H - 1;

    const uint32_t* row = (const uint32_t*)s.rowPointer((size_t)y);
    const uint32_t px = row[x];

    return argb32_unpack_alpha_norm(px);
}


    // computeHeightNormal()
    //
    // Computes the normal vector at pixel (x,y) by sampling the alpha channel of
    //
static INLINE void computeHeightNormal(const Surface& s,
    int x, int y,
    float surfaceScale,
    float dux, float duy,
    float& nx, float& ny, float& nz) noexcept
{
    const float hL = pixelHeightFromAlpha(s, x - 1, y);
    const float hR = pixelHeightFromAlpha(s, x + 1, y);
    const float hU = pixelHeightFromAlpha(s, x, y - 1);
    const float hD = pixelHeightFromAlpha(s, x, y + 1);

    float dHx = 0.0f;
    float dHy = 0.0f;

    if (dux > 0.0f)
        dHx = (hR - hL) / (2.0f * dux);

    if (duy > 0.0f)
        dHy = (hD - hU) / (2.0f * duy);

    nx = -surfaceScale * dHx;
    ny = -surfaceScale * dHy;
    nz = 1.0f;

    const float len2 = nx * nx + ny * ny + nz * nz;
    if (len2 > 1e-20f)
    {
        const float invLen = 1.0f / std::sqrt(len2);
        nx *= invLen;
        ny *= invLen;
        nz *= invLen;
    }
    else
    {
        nx = 0.0f;
        ny = 0.0f;
        nz = 1.0f;
    }
}


bool onSpecularLighting(const FilterIO& io, const WGRectD* subr,
    const ColorSRGB& lightingColor,
    float surfaceScale,
    float specularConstant, float specularExponent,
    float kernelUnitLengthX, float kernelUnitLengthY,
    uint32_t lightType, const LightPayload& light) noexcept override
{
    InternedKey inKey = resolveUnaryInputKey(io);
    InternedKey outKey = resolveOutKeyStrict(io);

    Surface* in = getInputImage(inKey);
    if (!in)
        return false;

    if (!outKey)
        outKey = filter::Filter_Last();

    auto out = createLikeSurfaceHandle(*in);
    if (!out)
        return false;

    out->clearAll();

    WGRectI area = resolveSubregionPx(subr, *in);
    if (area.w <= 0 || area.h <= 0)
    {
        if (!putImage(outKey, std::move(out)))
            return false;

        setLastKey(outKey);
        return true;
    }

    float lcR, lcG, lcB;
    resolveColorInterpolationRGB(lightingColor, io.colorInterp, lcR, lcG, lcB);



    specularExponent = clamp(specularExponent, 1.0f, 128.0f);

    PixelToFilterUserMap map;
    map.surfaceW = int(in->width());
    map.surfaceH = int(in->height());
    
    map.filterExtentUS = fSpace.filterExtentUS;
    //map.filterExtentUS = fSpace.filterRectUS;
    map.uxPerPixel = (map.surfaceW > 0)
        ? float(map.filterExtentUS.w / double(map.surfaceW))
        : 1.0f;
    map.uyPerPixel = (map.surfaceH > 0)
        ? float(map.filterExtentUS.h / double(map.surfaceH))
        : 1.0f;

    float defaultDux = 1.0f;
    float defaultDuy = 1.0f;
    computeSpecularLocalPixelStep(map, area.x, area.y, defaultDux, defaultDuy);

    const float dux = (kernelUnitLengthX > 0.0f) ? kernelUnitLengthX : defaultDux;
    const float duy = (kernelUnitLengthY > 0.0f) ? kernelUnitLengthY : defaultDuy;

    // Convert light into the same local filter-space used by pixelCenterToFilterUserStandalone().
    LightPayload localLight = light;

    if (lightType == FILTER_LIGHT_POINT || lightType == FILTER_LIGHT_SPOT)
    {
        localLight.L[0] -= float(fSpace.filterRectUS.x);
        localLight.L[1] -= float(fSpace.filterRectUS.y);

        // z stays as-is
        // localLight.L[2] unchanged
    }

    if (lightType == FILTER_LIGHT_SPOT)
    {
        localLight.L[3] -= float(fSpace.filterRectUS.x);
        localLight.L[4] -= float(fSpace.filterRectUS.y);

        // pointsAtZ stays as-is
        // localLight.L[5] unchanged
    }

    for (int y = area.y; y < area.y + area.h; ++y)
    {
        uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

        for (int x = area.x; x < area.x + area.w; ++x)
        {
            const float h00 = pixelHeightFromAlpha(*in, x - 1, y - 1);
            const float h10 = pixelHeightFromAlpha(*in, x, y - 1);
            const float h20 = pixelHeightFromAlpha(*in, x + 1, y - 1);

            const float h01 = pixelHeightFromAlpha(*in, x - 1, y);
            const float h11 = pixelHeightFromAlpha(*in, x, y);
            const float h21 = pixelHeightFromAlpha(*in, x + 1, y);

            const float h02 = pixelHeightFromAlpha(*in, x - 1, y + 1);
            const float h12 = pixelHeightFromAlpha(*in, x, y + 1);
            const float h22 = pixelHeightFromAlpha(*in, x + 1, y + 1);

            float nx, ny, nz;
            computeSpecularNormalFromHeights(
                h00, h10, h20,
                h01, h11, h21,
                h02, h12, h22,
                surfaceScale,
                dux, duy,
                nx, ny, nz);

            float ux, uy;
            pixelCenterToFilterUserStandalone(map, x, y, ux, uy);

            const float h = surfaceScale * h11;

            float lx, ly, lz;
            computeSpecularLightVector(lightType, localLight, ux, uy, h, lx, ly, lz);

            float lightFactor = 1.0f;
            if (lightType == FILTER_LIGHT_SPOT)
                lightFactor = computeSpecularSpotFactor(localLight, ux, uy, h);

            const float lit = computeSpecularTerm(
                nx, ny, nz,
                lx, ly, lz,
                specularConstant,
                specularExponent,
                lightFactor);

            drow[x] = packSpecularLightingPixel(lcR, lcG, lcB, lit);
        }
    }

    if (!putImage(outKey, std::move(out)))
        return false;

    setLastKey(outKey);
    return true;
}






        // -----------------------------------------
        // onTile
        // Type: unary
        // 
        // This essentially acts as a 'pattern' routine.
        // If the subr is smaller than the source graphic
        // then it will repeat
        // -----------------------------------------

bool onTile(const FilterIO& io, const WGRectD* subr) noexcept override
{
    InternedKey inKey = resolveUnaryInputKey(io);
    InternedKey outKey = resolveOutKeyStrict(io);

    Surface* in = getInputImage(inKey);
    if (!in)
        return false;

    if (!outKey)
        outKey = filter::Filter_Last();

    auto out = createLikeSurfaceHandle(*in);
    if (!out)
        return false;

    out->clearAll();

    const WGRectI area = resolveSubregionPx(subr, *in);
    if (area.w <= 0 || area.h <= 0)
    {
        if (!putImage(outKey, std::move(out)))
            return false;

        setLastKey(outKey);
        return true;
    }

    Surface outArea;
    if (out->getSubSurface(area, outArea) != WG_SUCCESS)
        return false;

    const int tileW = (int)in->width();
    const int tileH = (int)in->height();

    // Compute phase so tiling is stable
    int startX = area.x % tileW;
    if (startX < 0) startX += tileW;

    int startY = area.y % tileH;
    if (startY < 0) startY += tileH;

    // Fill using repeated blits
    for (int y = -startY; y < area.h; y += tileH)
    {
        for (int x = -startX; x < area.w; x += tileW)
        {
            outArea.blit(*in, x, y);
        }
    }

    if (!putImage(outKey, std::move(out)))
        return false;

    setLastKey(outKey);
    return true;
}

/*
        bool onTile(const FilterIO& io, const WGRectD* subr) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getInputImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            out->clearAll();

            WGRectI srcArea = resolveSubregionPx(subr, *in);
            if (srcArea.w <= 0 || srcArea.h <= 0) {
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);

                return true;
            }

            const int W = (int)out->width();
            const int H = (int)out->height();

            for (int y = 0; y < H; ++y)
            {
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);
                const int sy = srcArea.y + ((y - srcArea.y) % srcArea.h + srcArea.h) % srcArea.h;
                const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)sy);

                for (int x = 0; x < W; ++x)
                {
                    const int sx = srcArea.x + ((x - srcArea.x) % srcArea.w + srcArea.w) % srcArea.w;
                    drow[x] = srow[sx];
                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);

            return true;
        }
        */

        // -----------------------------------------
        // onTurbulence
        // -----------------------------------------


        bool onTurbulence(
            const FilterIO& io,
            const WGRectD* subr,
            FilterTurbulenceType typeKey,
            float baseFreqX,
            float baseFreqY,
            uint32_t numOctaves,
            float seed,
            bool stitchTiles) noexcept override
        {
            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = filter::Filter_Last();

            // feTurbulence is a generator. Use the filter tile size.
            Surface* like = getImage(lastKey());


            if (!like)
                return false;

            auto out = createLikeSurfaceHandle(*like);
            if (!out)
                return false;

            out->clearAll();

            WGRectI area = resolveSubregionPx(subr, *out);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            PixelToFilterUserMap map;
            map.surfaceW = int(out->width());
            map.surfaceH = int(out->height());
            map.filterExtentUS = fSpace.filterExtentUS;
            map.uxPerPixel = (map.surfaceW > 0)
                ? float(map.filterExtentUS.w / double(map.surfaceW))
                : 1.0f;
            map.uyPerPixel = (map.surfaceH > 0)
                ? float(map.filterExtentUS.h / double(map.surfaceH))
                : 1.0f;

            TurbulenceNoiseParams params{};
            params.baseFreqX = baseFreqX;
            params.baseFreqY = baseFreqY;
            params.seed = seed;
            params.octaves = numOctaves;

            TurbulenceState turb{};
            buildTurbulenceState(turb, (int32_t)seed);

            auto userToPrimitive = [&](float ux, float uy, float& px, float& py) noexcept
                {
                    switch (fRunState.primitiveUnits)
                    {
                    default:
                    case SpaceUnitsKind::SVG_SPACE_USER:
                        px = ux;
                        py = uy;
                        break;

                    case SpaceUnitsKind::SVG_SPACE_OBJECT:
                    {
                        // Convert local filter-space user coordinates back into
                        // absolute user-space before objectBoundingBox normalization.
                        const double absUx = double(ux) + fSpace.filterRectUS.x;
                        const double absUy = double(uy) + fSpace.filterRectUS.y;

                        const double bx = fRunState.objectBBoxUS.x;
                        const double by = fRunState.objectBBoxUS.y;
                        const double bw = fRunState.objectBBoxUS.w;
                        const double bh = fRunState.objectBBoxUS.h;

                        if (bw > 0.0 && bh > 0.0)
                        {
                            px = float((absUx - bx) / bw);
                            py = float((absUy - by) / bh);
                        }
                        else
                        {
                            px = 0.0f;
                            py = 0.0f;
                        }
                        break;
                    }

                    case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                        px = ux;
                        py = uy;
                        break;
                    }
                };

            auto primitiveSubregionRect = [&]() noexcept -> WGRectD
                {
                    if (subr)
                    {
                        if (fRunState.primitiveUnits == SpaceUnitsKind::SVG_SPACE_OBJECT)
                            return *subr;

                        // For USER units, convert authored user-space subregion to local filter-space.
                        return WGRectD(
                            subr->x - fSpace.filterRectUS.x,
                            subr->y - fSpace.filterRectUS.y,
                            subr->w,
                            subr->h);
                    }

                    if (fRunState.primitiveUnits == SpaceUnitsKind::SVG_SPACE_OBJECT)
                    {
                        const double bx = fRunState.objectBBoxUS.x;
                        const double by = fRunState.objectBBoxUS.y;
                        const double bw = fRunState.objectBBoxUS.w;
                        const double bh = fRunState.objectBBoxUS.h;

                        if (bw > 0.0 && bh > 0.0)
                        {
                            return WGRectD(
                                (fRunState.filterRectUS.x - bx) / bw,
                                (fRunState.filterRectUS.y - by) / bh,
                                fRunState.filterRectUS.w / bw,
                                fRunState.filterRectUS.h / bh);
                        }

                        return WGRectD{};
                    }

                    // USER units: stitch against local filter-space extent, not absolute filterRectUS.
                    return fSpace.filterExtentUS;
                };

            TurbulenceStitchInfo stitchInfo{};
            const TurbulenceStitchInfo* stitchPtr = nullptr;

            if (stitchTiles)
            {
                WGRectD tileRect = primitiveSubregionRect();

                if (tileRect.w > 0.0 && tileRect.h > 0.0)
                {
                    adjust_base_frequencies_for_stitch(
                        (float)tileRect.w,
                        (float)tileRect.h,
                        params.baseFreqX,
                        params.baseFreqY);

                    stitchInfo = prepare_stitch_info(
                        (float)tileRect.x,
                        (float)tileRect.y,
                        (float)tileRect.w,
                        (float)tileRect.h,
                        params.baseFreqX,
                        params.baseFreqY);

                    stitchPtr = &stitchInfo;
                }
            }

            const bool fractalNoise = (typeKey == FILTER_TURBULENCE_FRACTAL_NOISE);

            for (int y = area.y; y < area.y + area.h; ++y)
            {
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

                for (int x = area.x; x < area.x + area.w; ++x)
                {
                    float ux, uy;
                    pixelCenterToFilterUserStandalone(map, x, y, ux, uy);

                    float tx, ty;
                    userToPrimitive(ux, uy, tx, ty);

                    float r, g, b, a;

                    if (fractalNoise)
                    {
                        r = sampleFractalChannel(tx, ty, params, turb, 0, stitchTiles, stitchPtr);
                        g = sampleFractalChannel(tx, ty, params, turb, 1, stitchTiles, stitchPtr);
                        b = sampleFractalChannel(tx, ty, params, turb, 2, stitchTiles, stitchPtr);
                        a = sampleFractalChannel(tx, ty, params, turb, 3, stitchTiles, stitchPtr);
                    }
                    else
                    {
                        r = sampleTurbulenceChannel(tx, ty, params, turb, 0, stitchTiles, stitchPtr);
                        g = sampleTurbulenceChannel(tx, ty, params, turb, 1, stitchTiles, stitchPtr);
                        b = sampleTurbulenceChannel(tx, ty, params, turb, 2, stitchTiles, stitchPtr);
                        a = sampleTurbulenceChannel(tx, ty, params, turb, 3, stitchTiles, stitchPtr);
                    }

                    r = clamp01f(r);
                    g = clamp01f(g);
                    b = clamp01f(b);
                    a = clamp01f(a);

                    // Store in premultiplied surface format.
                    r *= a;
                    g *= a;
                    b *= a;

                    drow[x] = argb32_pack_u8(
                        quantize0_255(a),
                        quantize0_255(r),
                        quantize0_255(g),
                        quantize0_255(b));
                    //drow[x] = pack_argb32(a, r, g, b);
                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }

    };
}
