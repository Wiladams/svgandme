#pragma once

#include <memory>
#include <unordered_map>
#include <cstdint>
#include <cmath>

#include "nametable.h"
#include "svgb2ddriver.h"

#include "filterprogramexec.h"   // FilterProgramExecutor + IAmFroot<T>
#include "filter_noise.h"
#include "svggraphicselement.h"
#include "viewport.h"
#include "pixeling.h"

#include "filter_fecomposite.h"
#include "filter_feblend.h"
#include "filter_fegaussian.h"
#include "filter_fecolormatrix.h"
#include "filter_femorphology.h"

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

        static INLINE BLMatrix2D makeUserToSurfaceMatrix(const FilterRunState& runState) noexcept
        {
            BLMatrix2D m = BLMatrix2D::makeIdentity();
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

            WGRectD srcViewBox = dstRect;
            //WGRectD srcViewBox = computeReferencedViewBox(elem.get());
            //if (!isValidRect(srcViewBox))
            //    return {};

            auto out = createFilterSurface(runState);
            if (!out)
                return {};
            out->clearAll();

            //PreserveAspectRatio par = makePAR(align, meetOrSlice);

            BLMatrix2D fit = BLMatrix2D::makeIdentity();
            //if (!computeViewBoxToViewport(dstRect, srcViewBox, par, fit))
            //    return {};

            BLMatrix2D userToSurface = makeUserToSurfaceMatrix(runState);

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
        WGRectD filterRectUS{};
        WGRectD filterRectPX{}; // device bbox (float)
        BLMatrix2D ctm{};
        BLMatrix2D invCtm{};
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



        INLINE void pixelCenterToFilterUser(int x, int y, float& ux, float& uy) const noexcept
        {
            const double dx = fSpace.filterRectPX.x + double(x) + 0.5;
            const double dy = fSpace.filterRectPX.y + double(y) + 0.5;

            const BLMatrix2D& m = fSpace.invCtm;
            ux = float(dx * m.m00 + dy * m.m10 + m.m20);
            uy = float(dx * m.m01 + dy * m.m11 + m.m21);
        }

        // --------------------------------------
        // SourceAlpha implementation
        // --------------------------------------

        static INLINE void extractAlpha_row_scalar(
            uint32_t* dst,
            const uint32_t* src,
            size_t n) noexcept
        {
            for (size_t i = 0; i < n; ++i)
                dst[i] = src[i] & 0xFF000000u;
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
        static INLINE void extractAlpha_row(
            uint32_t* dst,
            const uint32_t* src,
            size_t n) noexcept
        {
#if defined(__ARM_NEON) || defined(__aarch64__)
            extractAlpha_row_neon(dst, src, n);
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
            fRunState.filterRectUS = filterRectUS;
            fRunState.objectBBoxUS = objectBBoxUS;

            // get the current CTM from the context, 
            // which we will need for mapping the filter region 
            // to pixel space, and for providing space conversion 
            // to primitives that need it.
            const BLMatrix2D ctm = ctx->getTransform();

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
            fSpace.filterRectPX = WGRectD{ (double)tileX, (double)tileY, (double)tileW, (double)tileH };
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

            {
                BLMatrix2D off = ctm;
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
            if (!putImage(kFilter_SourceGraphic(), std::move(srcGraphic)))
                return false;

            // make the alpha channel available as SourceAlpha, for primitives that need it.
            // BUGBUG - maybe this can be delayed and the filter primitives that need
            // it can request it on demand.
            auto srcAlpha = makeSourceAlpha(*getImage(kFilter_SourceGraphic()));
            if (!srcAlpha)
                return false;

            if (!putImage(kFilter_SourceAlpha(), std::move(srcAlpha)))
                return false;

            // --------------------------------------------------
            // Execute filter primitives
            // --------------------------------------------------

            if (!FilterProgramExecutor::execute(program, *this))
                return false;

            // Resolve final output surface
            InternedKey outKey = lastKey();
            if (!outKey)
                outKey = kFilter_Last();

            Surface* outImg = getImage(outKey);
            if (!outImg)
                outImg = getImage(kFilter_SourceGraphic());
            if (!outImg)
                return false;

            // --------------------------------------------------
            // Composite result back to main context
            // --------------------------------------------------


            ctx->push();
            
            // Reset transform so tile is drawn in device space
            ctx->transform(BLMatrix2D::makeIdentity());

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
            if (!lastKey() && hasImage(kFilter_SourceGraphic()))
                setLastKey(kFilter_SourceGraphic());
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


            Surface* in1 = getImage(in1Key);
            Surface* in2 = getImage(in2Key);
            if (!in1 || !in2)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

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

            for (int y = y0; y < y1; ++y)
            {
                const uint32_t* s1 = (const uint32_t*)in1->rowPointer((size_t)y);
                const uint32_t* s2 = (const uint32_t*)in2->rowPointer((size_t)y);
                uint32_t* d = (uint32_t*)out->rowPointer((size_t)y);

                waavs::feBlendRowPRGB32(d + x0, s1 + x0, s2 + x0, x1 - x0, mode);
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }




        // ------------------------------------------
        // onColorMatrix
        // unary
        // integer / fixed-point
        // -------------------------------------------


        /*
        // Scalar, reference implementation
        bool onColorMatrix(const FilterIO& io, const WGRectD* subr,
            FilterColorMatrixType type, float param, F32Span matrix) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            // Start from a copy so unchanged pixels outside the primitive write area
            // remain as they were in the current implementation model.
            {
                SVGB2DDriver bctx{};
                bctx.attach(*out, 1);
                bctx.renew();
                bctx.blendMode(BL_COMP_OP_SRC_COPY);
                bctx.image(*in, 0, 0);
                bctx.detach();
            }

            WGRectI area = resolveSubregionPx(subr, *in);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            float M[20]{};

            if (type == FILTER_COLOR_MATRIX_MATRIX)
            {
                if (!matrix.p || matrix.n != 20)
                    return false;

                for (int i = 0; i < 20; ++i)
                    M[i] = matrix.p[i];
            }
            else if (type == FILTER_COLOR_MATRIX_SATURATE)
            {
                const float s = param;

                M[0] = 0.213f + 0.787f * s;  M[1] = 0.715f - 0.715f * s;  M[2] = 0.072f - 0.072f * s;  M[3] = 0.0f; M[4] = 0.0f;
                M[5] = 0.213f - 0.213f * s;  M[6] = 0.715f + 0.285f * s;  M[7] = 0.072f - 0.072f * s;  M[8] = 0.0f; M[9] = 0.0f;
                M[10] = 0.213f - 0.213f * s;  M[11] = 0.715f - 0.715f * s;  M[12] = 0.072f + 0.928f * s;  M[13] = 0.0f; M[14] = 0.0f;
                M[15] = 0.0f;                 M[16] = 0.0f;                 M[17] = 0.0f;                 M[18] = 1.0f; M[19] = 0.0f;
            }
            else if (type == FILTER_COLOR_MATRIX_HUE_ROTATE)
            {
                const float a = param * 3.14159265358979323846f / 180.0f;
                const float c = std::cos(a);
                const float s = std::sin(a);

                M[0] = 0.213f + 0.787f * c - 0.213f * s;
                M[1] = 0.715f - 0.715f * c - 0.715f * s;
                M[2] = 0.072f - 0.072f * c + 0.928f * s;
                M[3] = 0.0f;
                M[4] = 0.0f;

                M[5] = 0.213f - 0.213f * c + 0.143f * s;
                M[6] = 0.715f + 0.285f * c + 0.140f * s;
                M[7] = 0.072f - 0.072f * c - 0.283f * s;
                M[8] = 0.0f;
                M[9] = 0.0f;

                M[10] = 0.213f - 0.213f * c - 0.787f * s;
                M[11] = 0.715f - 0.715f * c + 0.715f * s;
                M[12] = 0.072f + 0.928f * c + 0.072f * s;
                M[13] = 0.0f;
                M[14] = 0.0f;

                M[15] = 0.0f;
                M[16] = 0.0f;
                M[17] = 0.0f;
                M[18] = 1.0f;
                M[19] = 0.0f;
            }
            else
            {
                // luminanceToAlpha
                M[0] = 0.0f;   M[1] = 0.0f;   M[2] = 0.0f;   M[3] = 0.0f;   M[4] = 0.0f;
                M[5] = 0.0f;   M[6] = 0.0f;   M[7] = 0.0f;   M[8] = 0.0f;   M[9] = 0.0f;
                M[10] = 0.0f;   M[11] = 0.0f;   M[12] = 0.0f;   M[13] = 0.0f;   M[14] = 0.0f;
                M[15] = 0.2125f; M[16] = 0.7154f; M[17] = 0.0721f; M[18] = 0.0f; M[19] = 0.0f;
            }

            for (int y = area.y; y < area.y + area.h; ++y)
            {
                const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)y);
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

                for (int x = area.x; x < area.x + area.w; ++x)
                {
                    const uint32_t px = srow[x];

                    // Stored pixels are premultiplied.
                    const float a_p = float((px >> 24) & 0xFF) * (1.0f / 255.0f);
                    float r_p = float((px >> 16) & 0xFF) * (1.0f / 255.0f);
                    float g_p = float((px >> 8) & 0xFF) * (1.0f / 255.0f);
                    float b_p = float((px >> 0) & 0xFF) * (1.0f / 255.0f);

                    // feColorMatrix operates on non-premultiplied color values.
                    float r = 0.0f;
                    float g = 0.0f;
                    float b = 0.0f;
                    const float a = a_p;

                    if (a_p > 0.0f)
                    {
                        const float invA = 1.0f / a_p;
                        r = clamp01(r_p * invA);
                        g = clamp01(g_p * invA);
                        b = clamp01(b_p * invA);
                    }

                    // Apply the 5x4 color matrix to straight RGBA.
                    float rr = M[0] * r + M[1] * g + M[2] * b + M[3] * a + M[4];
                    float gg = M[5] * r + M[6] * g + M[7] * b + M[8] * a + M[9];
                    float bb = M[10] * r + M[11] * g + M[12] * b + M[13] * a + M[14];
                    float aa = M[15] * r + M[16] * g + M[17] * b + M[18] * a + M[19];

                    rr = clamp01(rr);
                    gg = clamp01(gg);
                    bb = clamp01(bb);
                    aa = clamp01(aa);

                    // Re-premultiply for storage.
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

    Surface* in = getImage(inKey);
    if (!in)
        return false;

    if (!outKey)
        outKey = kFilter_Last();

    auto out = createLikeSurfaceHandle(*in);
    if (!out)
        return false;

    // Preserve current executor behavior:
    // start from a copy so pixels outside primitive write area remain unchanged.
    out->blit(*in, 0, 0);

    WGRectI area = resolveSubregionPx(subr, *in);
    if (area.w <= 0 || area.h <= 0)
    {
        if (!putImage(outKey, std::move(out)))
            return false;

        setLastKey(outKey);
        return true;
    }

    float Mf[20]{};

    if (type == FILTER_COLOR_MATRIX_MATRIX)
    {
        if (!matrix.p || matrix.n != 20)
            return false;

        for (int i = 0; i < 20; ++i)
            Mf[i] = matrix.p[i];
    }
    else if (type == FILTER_COLOR_MATRIX_SATURATE)
    {
        const float s = param;

        Mf[0]  = 0.213f + 0.787f * s;  Mf[1]  = 0.715f - 0.715f * s;  Mf[2]  = 0.072f - 0.072f * s;  Mf[3]  = 0.0f;  Mf[4]  = 0.0f;
        Mf[5]  = 0.213f - 0.213f * s;  Mf[6]  = 0.715f + 0.285f * s;  Mf[7]  = 0.072f - 0.072f * s;  Mf[8]  = 0.0f;  Mf[9]  = 0.0f;
        Mf[10] = 0.213f - 0.213f * s;  Mf[11] = 0.715f - 0.715f * s;  Mf[12] = 0.072f + 0.928f * s;  Mf[13] = 0.0f;  Mf[14] = 0.0f;
        Mf[15] = 0.0f;                 Mf[16] = 0.0f;                 Mf[17] = 0.0f;                 Mf[18] = 1.0f;  Mf[19] = 0.0f;
    }
    else if (type == FILTER_COLOR_MATRIX_HUE_ROTATE)
    {
        const float a = param * 3.14159265358979323846f / 180.0f;
        const float c = std::cos(a);
        const float s = std::sin(a);

        Mf[0]  = 0.213f + 0.787f * c - 0.213f * s;
        Mf[1]  = 0.715f - 0.715f * c - 0.715f * s;
        Mf[2]  = 0.072f - 0.072f * c + 0.928f * s;
        Mf[3]  = 0.0f;
        Mf[4]  = 0.0f;

        Mf[5]  = 0.213f - 0.213f * c + 0.143f * s;
        Mf[6]  = 0.715f + 0.285f * c + 0.140f * s;
        Mf[7]  = 0.072f - 0.072f * c - 0.283f * s;
        Mf[8]  = 0.0f;
        Mf[9]  = 0.0f;

        Mf[10] = 0.213f - 0.213f * c - 0.787f * s;
        Mf[11] = 0.715f - 0.715f * c + 0.715f * s;
        Mf[12] = 0.072f + 0.928f * c + 0.072f * s;
        Mf[13] = 0.0f;
        Mf[14] = 0.0f;

        Mf[15] = 0.0f;
        Mf[16] = 0.0f;
        Mf[17] = 0.0f;
        Mf[18] = 1.0f;
        Mf[19] = 0.0f;
    }
    else
    {
        // luminanceToAlpha
        Mf[0]  = 0.0f;    Mf[1]  = 0.0f;    Mf[2]  = 0.0f;    Mf[3]  = 0.0f;    Mf[4]  = 0.0f;
        Mf[5]  = 0.0f;    Mf[6]  = 0.0f;    Mf[7]  = 0.0f;    Mf[8]  = 0.0f;    Mf[9]  = 0.0f;
        Mf[10] = 0.0f;    Mf[11] = 0.0f;    Mf[12] = 0.0f;    Mf[13] = 0.0f;    Mf[14] = 0.0f;
        Mf[15] = 0.2125f; Mf[16] = 0.7154f; Mf[17] = 0.0721f; Mf[18] = 0.0f;    Mf[19] = 0.0f;
    }

    // Fast identity bypass.
    {
        static constexpr float kId[20] =
        {
            1,0,0,0,0,
            0,1,0,0,0,
            0,0,1,0,0,
            0,0,0,1,0
        };

        bool isIdentity = true;
        for (int i = 0; i < 20; ++i)
        {
            if (std::fabs(Mf[i] - kId[i]) > 1.0e-6f)
            {
                isIdentity = false;
                break;
            }
        }

        if (isIdentity)
        {
            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }
    }

    ColorMatrixQ88 Mq{};
    for (int row = 0; row < 4; ++row)
    {
        const int b = row * 5;

        Mq.c[b + 0] = (int32_t)std::lround(Mf[b + 0] * 256.0f);
        Mq.c[b + 1] = (int32_t)std::lround(Mf[b + 1] * 256.0f);
        Mq.c[b + 2] = (int32_t)std::lround(Mf[b + 2] * 256.0f);
        Mq.c[b + 3] = (int32_t)std::lround(Mf[b + 3] * 256.0f);

        // Bias is specified in normalized color space.
        // Convert to byte domain first, then to Q8.8.
        Mq.c[b + 4] = (int32_t)std::lround(Mf[b + 4] * 255.0f * 256.0f);
    }

    static uint16_t sInvAlphaQ8[256];
    static bool sInvAlphaInit = false;

    if (!sInvAlphaInit)
    {
        sInvAlphaQ8[0] = 0;
        for (int a = 1; a < 256; ++a)
            sInvAlphaQ8[a] = (uint16_t)((255u << 8) / (uint32_t)a);

        sInvAlphaInit = true;
    }

    for (int y = area.y; y < area.y + area.h; ++y)
    {
        const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)y);
        uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

        colorMatrix_q88_row(
            drow + area.x,
            srow + area.x,
            (size_t)area.w,
            Mq,
            sInvAlphaQ8);
    }

    if (!putImage(outKey, std::move(out)))
        return false;

    setLastKey(outKey);
    return true;
}

        // ----------------------------------------
        // onComponentTransfer()
        // unary
        // ----------------------------------------
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

        bool onComponentTransfer(const FilterIO& io, const WGRectD* subr,
            const ComponentFunc& rF,
            const ComponentFunc& gF,
            const ComponentFunc& bF,
            const ComponentFunc& aF) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            // copy input first, then rewrite only the primitive area.
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
                    const float a_p = float((px >> 24) & 0xFF) * (1.0f / 255.0f);
                    const float r_p = float((px >> 16) & 0xFF) * (1.0f / 255.0f);
                    const float g_p = float((px >> 8) & 0xFF) * (1.0f / 255.0f);
                    const float b_p = float((px >> 0) & 0xFF) * (1.0f / 255.0f);

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

                    drow[x] = pack_argb32(aa, rr, gg, bb);
                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }



        //-----------------------------------------

        bool onComposite(const FilterIO& io, const WGRectD* subr,
            FilterCompositeOp op,
            float k1, float k2, float k3, float k4) noexcept override
        {

            InternedKey in1Key = resolveBinaryInput1Key(io);
            InternedKey in2Key = resolveBinaryInput2Key(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in1 = getImage(in1Key);
            Surface* in2 = getImage(in2Key);
            if (!in1 || !in2)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

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
                        const uint32_t px = pack_argb32(c, c, c, c);
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
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }


        // ------------------------------------------
        // onConvolveMatrix
        // -------------------------------------------
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

            Surface* in = getImage(inKey);
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

            {
                SVGB2DDriver bctx{};
                bctx.attach(*out, 1);
                bctx.renew();
                bctx.blendMode(BL_COMP_OP_SRC_COPY);
                bctx.image(*in, 0, 0);
                bctx.detach();
            }

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

        // ------------------------------------------
        // onDiffuseLighting
        // Type: lighting
        // -------------------------------------------
        bool onDiffuseLighting(const FilterIO& io, const WGRectD* subr,
            uint32_t lightingRGBA, float surfaceScale, float diffuseConstant,
            float kernelUnitLengthX, float kernelUnitLengthY,
            uint32_t lightType, const LightPayload& light) noexcept override
        {
            (void)kernelUnitLengthX;
            (void)kernelUnitLengthY;

            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

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

            const float lcR = float((lightingRGBA >> 16) & 0xFF) / 255.0f;
            const float lcG = float((lightingRGBA >> 8) & 0xFF) / 255.0f;
            const float lcB = float((lightingRGBA >> 0) & 0xFF) / 255.0f;

            const float kPi = 3.14159265358979323846f;

            auto safeNormalize3 = [](float& x, float& y, float& z) noexcept -> bool
                {
                    const float len2 = x * x + y * y + z * z;
                    if (len2 <= 1e-20f)
                    {
                        x = 0.0f;
                        y = 0.0f;
                        z = 1.0f;
                        return false;
                    }

                    const float invLen = 1.0f / std::sqrt(len2);
                    x *= invLen;
                    y *= invLen;
                    z *= invLen;
                    return true;
                };

            auto clamp01f = [](float v) noexcept -> float
                {
                    if (v < 0.0f)
                        return 0.0f;
                    if (v > 1.0f)
                        return 1.0f;
                    return v;
                };

            for (int y = area.y; y < area.y + area.h; ++y)
            {
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

                for (int x = area.x; x < area.x + area.w; ++x)
                {
                    float nx, ny, nz;
                    const float dux = (in->width() > 0) ? float(fSpace.filterRectUS.w / double(in->width())) : 1.0f;
                    const float duy = (in->height() > 0) ? float(fSpace.filterRectUS.h / double(in->height())) : 1.0f;

                    computeHeightNormal(*in, x, y, surfaceScale, dux, duy, nx, ny, nz);
                    safeNormalize3(nx, ny, nz);

                    float ux, uy;
                    pixelCenterToFilterUser(x, y, ux, uy);

                    const float h = surfaceScale * pixelHeightFromAlpha(*in, x, y);

                    // L is always surface point -> light for the diffuse term.
                    float lx = 0.0f;
                    float ly = 0.0f;
                    float lz = 1.0f;

                    if (lightType == FILTER_LIGHT_DISTANT)
                    {
                        const float az = light.L[0] * (kPi / 180.0f);
                        const float el = light.L[1] * (kPi / 180.0f);

                        lx = std::cos(el) * std::cos(az);
                        ly = std::cos(el) * std::sin(az);
                        lz = std::sin(el);
                        safeNormalize3(lx, ly, lz);
                    }
                    else if (lightType == FILTER_LIGHT_POINT || lightType == FILTER_LIGHT_SPOT)
                    {
                        lx = light.L[0] - ux;
                        ly = light.L[1] - uy;
                        lz = light.L[2] - h;
                        safeNormalize3(lx, ly, lz);
                    }

                    float ndotl = nx * lx + ny * ly + nz * lz;
                    if (ndotl < 0.0f)
                        ndotl = 0.0f;

                    float lightFactor = 1.0f;

                    if (lightType == FILTER_LIGHT_SPOT)
                    {
                        // Spotlight axis is light position -> pointsAt.
                        float ax = light.L[3] - light.L[0];
                        float ay = light.L[4] - light.L[1];
                        float az = light.L[5] - light.L[2];

                        // Spotlight ray is light position -> current surface point.
                        float sx = ux - light.L[0];
                        float sy = uy - light.L[1];
                        float sz = h - light.L[2];

                        const bool axisOk = safeNormalize3(ax, ay, az);
                        const bool rayOk = safeNormalize3(sx, sy, sz);

                        if (axisOk && rayOk)
                        {
                            float cosAng = ax * sx + ay * sy + az * sz;
                            cosAng = clamp01f(cosAng);

                            float spotExp = light.L[6];
                            if (!(spotExp > 0.0f))
                                spotExp = 1.0f;

                            lightFactor = std::pow(cosAng, spotExp);

                            const float limitDeg = light.L[7];
                            if (limitDeg > 0.0f)
                            {
                                const float limitCos = std::cos(limitDeg * (kPi / 180.0f));
                                if (cosAng < limitCos)
                                    lightFactor = 0.0f;
                            }
                        }
                        else
                        {
                            // Degenerate spotlight: do not black out the whole effect.
                            // Fall back to point-light behavior.
                            lightFactor = 1.0f;
                        }
                    }

                    const float lit = clamp01f(diffuseConstant * ndotl * lightFactor);

                    // Premultiplied output.
                    const float a = 1.0f; // lit;
                    const float pr = lcR * lit;
                    const float pg = lcG * lit;
                    const float pb = lcB * lit;

                    drow[x] = pack_argb32(
                        clamp_u8((int)std::lround(a * 255.0f)),
                        clamp_u8((int)std::lround(pr * 255.0f)),
                        clamp_u8((int)std::lround(pg * 255.0f)),
                        clamp_u8((int)std::lround(pb * 255.0f)));
                }
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

        bool onDisplacementMap(const FilterIO& io, const WGRectD* subr,
            float scale, FilterChannelSelector xChannel, FilterChannelSelector yChannel) noexcept override
        {
            InternedKey in1Key = resolveBinaryInput1Key(io);
            InternedKey in2Key = resolveBinaryInput2Key(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in1 = getImage(in1Key);
            Surface* in2 = getImage(in2Key);
            if (!in1 || !in2)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

            auto out = createLikeSurfaceHandle(*in1);
            if (!out)
                return false;

            // Preserve current executor behavior outside the primitive write area.
            {
                out->blit(*in1, 0, 0);
            }

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

            const double scaleX = scaleUS * fSpace.sx;
            const double scaleY = scaleUS * fSpace.sy;

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

            auto unpackMapChannelStraight = [](uint32_t px, FilterChannelSelector ch) noexcept -> float
                {
                    const float a = float((px >> 24) & 0xFF) * (1.0f / 255.0f);
                    const float r_p = float((px >> 16) & 0xFF) * (1.0f / 255.0f);
                    const float g_p = float((px >> 8) & 0xFF) * (1.0f / 255.0f);
                    const float b_p = float((px >> 0) & 0xFF) * (1.0f / 255.0f);

                    switch (ch)
                    {
                    default:
                    case FILTER_CHANNEL_A:
                        return a;

                    case FILTER_CHANNEL_R:
                        if (a > 0.0f)
                            return clamp01(r_p / a);
                        return 0.0f;

                    case FILTER_CHANNEL_G:
                        if (a > 0.0f)
                            return clamp01(g_p / a);
                        return 0.0f;

                    case FILTER_CHANNEL_B:
                        if (a > 0.0f)
                            return clamp01(b_p / a);
                        return 0.0f;
                    }
                };

            for (int y = area.y; y < area.y + area.h; ++y)
            {
                if ((unsigned)y >= (unsigned)H)
                    continue;

                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);
                const uint32_t* mapRow = (const uint32_t*)in2->rowPointer((size_t)y);

                for (int x = area.x; x < area.x + area.w; ++x)
                {
                    if ((unsigned)x >= (unsigned)W)
                    {
                        drow[x] = 0u;
                        continue;
                    }

                    const uint32_t mp = mapRow[x];

                    // feDisplacementMap channel selectors conceptually operate on the
                    // channel values, not on premultiplied storage bytes. Recover
                    // straight RGB from PRGB storage before using them as map values.
                    const double mxv = double(unpackMapChannelStraight(mp, xChannel));
                    const double myv = double(unpackMapChannelStraight(mp, yChannel));

                    const double mx = (mxv - 0.5) * scaleX;
                    const double my = (myv - 0.5) * scaleY;

                    const int sx = (int)std::lround(double(x) + mx);
                    const int sy = (int)std::lround(double(y) + my);

                    if ((unsigned)sx < (unsigned)W1 && (unsigned)sy < (unsigned)H1)
                    {
                        const uint32_t* srow = (const uint32_t*)in1->rowPointer((size_t)sy);
                        drow[x] = srow[sx];
                    }
                    else
                    {
                        drow[x] = 0u;
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
            uint32_t rgbaPremul) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getImage(inKey);
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

            const BLMatrix2D& m = fSpace.ctm;
            const double dxPx = dxUS * m.m00 + dyUS * m.m10;
            const double dyPx = dxUS * m.m01 + dyUS * m.m11;

            {
                SVGB2DDriver bctx{};
                bctx.attach(*out, 1);
                bctx.renew();

                out->clearAll();

                bctx.blendMode(BL_COMP_OP_SRC_COPY);
                bctx.transform(BLMatrix2D::makeTranslation(dxPx, dyPx));
                bctx.image(*shadow, 0, 0);

                bctx.transform(BLMatrix2D::makeIdentity());
                bctx.blendMode(BL_COMP_OP_SRC_OVER);
                bctx.image(*in, 0, 0);

                bctx.detach();
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }

        // ------------------------------------------
        // onFlood
        // Type: generator
        // -------------------------------------------

        bool onFlood(const FilterIO& io, const WGRectD* subr, uint32_t rgbaPremul) noexcept override
        {
            InternedKey outKey = resolveOutKeyStrict(io);

            // feFlood is a generator, so pick a surface that 
            // represents the current tile size.
            Surface* like = getImage(kFilter_SourceGraphic());

            if (!like) {
                InternedKey lk = lastKey();
                if (lk)
                    like = getImage(lk);
            }

            if (!like)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

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
            if (!subr) {
                out->fillAll(rgbaPremul);
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
                out->fillRect(area, rgbaPremul);
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
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            auto primLenToUser = [&](double v, double range) noexcept -> double {
                switch (fRunState.primitiveUnits) {
                default:
                case SpaceUnitsKind::SVG_SPACE_USER:
                    return v;
                case SpaceUnitsKind::SVG_SPACE_OBJECT:
                    return v * (double)range;
                case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                    return v;
                }
                };

    

            if (sx < 0.0f) sx = 0.0f;
            if (sy < 0.0f) sy = 0.0f;

            const double sxUS = primLenToUser((double)sx, (double)fRunState.objectBBoxUS.w);
            const double syUS = primLenToUser((double)sy, (double)fRunState.objectBBoxUS.h);

            const double sxPx = sxUS * fSpace.sx;
            const double syPx = syUS * fSpace.sy;

            // Primitive result should be written only inside the authored subregion.
            WGRectI writeArea = resolveSubregionPx(subr, *in);

            out->clearAll();

            if (writeArea.w <= 0 || writeArea.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);
                return true;
            }

            // Degenerate blur: copy only the write area.
            if (!(sxPx > 0.0) && !(syPx > 0.0))
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

            // Use the larger sigma for the box approximation we already use.
            const double sigma = (sxPx > syPx) ? sxPx : syPx;
            if (!(sigma > 0.0))
            {
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);
                return true;
            }

            // Blur needs an expanded sampling area.
            // 3*sigma is a practical kernel reach for Gaussian support.
            const double padUserX = 3.0 * sxUS;
            const double padUserY = 3.0 * syUS;

            WGRectI sampleArea = resolveSubregionPx(subr, *in, padUserX, padUserY);
            if (sampleArea.w <= 0 || sampleArea.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;
                setLastKey(outKey);
                return true;
            }

            int box[3];
            waavs::boxesForGauss(sigma, 3, box);

            waavs::Surface tmp0;
            waavs::Surface tmp1;

            if (!tmp0.reset(in->width(), in->height()))
                return false;
            if (!tmp1.reset(in->width(), in->height()))
                return false;

            tmp0.clearAll();
            tmp1.clearAll();

            const waavs::Surface* curSrc = in;
            waavs::Surface* curDst = &tmp0;

            for (int pass = 0; pass < 3; ++pass)
            {
                const int r = (box[pass] - 1) / 2;

                waavs::boxBlurH_PRGB32(*curDst, *curSrc, r, sampleArea);
                curSrc = curDst;
                curDst = (curDst == &tmp0) ? &tmp1 : &tmp0;

                waavs::boxBlurV_PRGB32(*curDst, *curSrc, r, sampleArea);
                curSrc = curDst;
                curDst = (curDst == &tmp0) ? &tmp1 : &tmp0;
            }

            // Copy only the primitive result area to the output.
            {
                const waavs::Surface& blurred = *curSrc;
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

        // -----------------------------------------
        // onImage()
        // Type: generator
        // -----------------------------------------
        bool onImage(const FilterIO& io, const WGRectD* subr, InternedKey imageKey,
            AspectRatioAlignKind align, AspectRatioMeetOrSliceKind mos) noexcept override
        {
            if (!fResolver)
                return false;

            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = kFilter_Last();

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
                outKey = kFilter_Last();

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

            Surface* in = getImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

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
                out->clearAll();

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

        /*
        bool onMorphology(const FilterIO& io, const WGRectD* subr,
            FilterMorphologyOp op, float rx, float ry) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

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
                out->clearAll();

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

            // We only need horizontal-pass values for x in the output area,
            // but we need rows expanded by the vertical radius because the
            // vertical pass samples neighboring rows from tmp.
            const int tmpY0 = (area.y - rpy < 0) ? 0 : (area.y - rpy);
            const int tmpY1 = (area.y + area.h - 1 + rpy >= H) ? (H - 1) : (area.y + area.h - 1 + rpy);

            const int x0 = area.x;
            const int x1 = area.x + area.w - 1;
            const int y0 = area.y;
            const int y1 = area.y + area.h - 1;

            // --------------------------------------------------------
            // Horizontal pass: in -> tmp
            // Compute only the columns we will ultimately output,
            // but for rows expanded by rpy.
            // --------------------------------------------------------
            for (int y = tmpY0; y <= tmpY1; ++y)
            {
                uint32_t* drow = (uint32_t*)tmp->rowPointer((size_t)y);
                const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)y);

                for (int x = x0; x <= x1; ++x)
                {
                    uint8_t bestA = (op == FILTER_MORPHOLOGY_ERODE) ? 255 : 0;
                    uint8_t bestR = bestA;
                    uint8_t bestG = bestA;
                    uint8_t bestB = bestA;

                    for (int kx = -rpx; kx <= rpx; ++kx)
                    {
                        int sx = x + kx;
                        if (sx < 0) sx = 0;
                        if (sx >= W) sx = W - 1;

                        const uint32_t px = srow[sx];
                        const uint8_t a = (uint8_t)((px >> 24) & 0xFF);
                        const uint8_t r = (uint8_t)((px >> 16) & 0xFF);
                        const uint8_t g = (uint8_t)((px >> 8) & 0xFF);
                        const uint8_t b = (uint8_t)((px >> 0) & 0xFF);

                        if (op == FILTER_MORPHOLOGY_ERODE)
                        {
                            if (a < bestA) bestA = a;
                            if (r < bestR) bestR = r;
                            if (g < bestG) bestG = g;
                            if (b < bestB) bestB = b;
                        }
                        else
                        {
                            if (a > bestA) bestA = a;
                            if (r > bestR) bestR = r;
                            if (g > bestG) bestG = g;
                            if (b > bestB) bestB = b;
                        }
                    }

                    drow[x] = pack_argb32(bestA, bestR, bestG, bestB);
                }
            }

            // --------------------------------------------------------
            // Vertical pass: tmp -> out
            // Only compute the actual output area.
            // --------------------------------------------------------
            for (int y = y0; y <= y1; ++y)
            {
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

                for (int x = x0; x <= x1; ++x)
                {
                    uint8_t bestA = (op == FILTER_MORPHOLOGY_ERODE) ? 255 : 0;
                    uint8_t bestR = bestA;
                    uint8_t bestG = bestA;
                    uint8_t bestB = bestA;

                    for (int ky = -rpy; ky <= rpy; ++ky)
                    {
                        int sy = y + ky;
                        if (sy < 0) sy = 0;
                        if (sy >= H) sy = H - 1;

                        const uint32_t* srow = (const uint32_t*)tmp->rowPointer((size_t)sy);
                        const uint32_t px = srow[x];

                        const uint8_t a = (uint8_t)((px >> 24) & 0xFF);
                        const uint8_t r = (uint8_t)((px >> 16) & 0xFF);
                        const uint8_t g = (uint8_t)((px >> 8) & 0xFF);
                        const uint8_t b = (uint8_t)((px >> 0) & 0xFF);

                        if (op == FILTER_MORPHOLOGY_ERODE)
                        {
                            if (a < bestA) bestA = a;
                            if (r < bestR) bestR = r;
                            if (g < bestG) bestG = g;
                            if (b < bestB) bestB = b;
                        }
                        else
                        {
                            if (a > bestA) bestA = a;
                            if (r > bestR) bestR = r;
                            if (g > bestG) bestG = g;
                            if (b > bestB) bestB = b;
                        }
                    }

                    drow[x] = pack_argb32(bestA, bestR, bestG, bestB);
                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);

            return true;
        }
        */

        /*
        bool onMorphology(const FilterIO& io, const WGRectD* subr,
            FilterMorphologyOp op, float rx, float ry) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            // Don't really need to do a copy first
            // since we'll replace every pixel that's needed
            //{
            //    out->blit(*in, 0, 0);
            //}

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
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                
                return true;
            }

            const int W = (int)in->width();
            const int H = (int)in->height();

            for (int y = area.y; y < area.y + area.h; ++y)
            {
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

                for (int x = area.x; x < area.x + area.w; ++x)
                {
                    uint8_t bestA = (op == FILTER_MORPHOLOGY_ERODE) ? 255 : 0;
                    uint8_t bestR = bestA;
                    uint8_t bestG = bestA;
                    uint8_t bestB = bestA;

                    for (int ky = -rpy; ky <= rpy; ++ky)
                    {
                        int sy = y + ky;
                        if (sy < 0) sy = 0;
                        if (sy >= H) sy = H - 1;

                        const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)sy);

                        for (int kx = -rpx; kx <= rpx; ++kx)
                        {
                            int sx = x + kx;
                            if (sx < 0) sx = 0;
                            if (sx >= W) sx = W - 1;

                            const uint32_t px = srow[sx];
                            const uint8_t a = (uint8_t)((px >> 24) & 0xFF);
                            const uint8_t r = (uint8_t)((px >> 16) & 0xFF);
                            const uint8_t g = (uint8_t)((px >> 8) & 0xFF);
                            const uint8_t b = (uint8_t)((px >> 0) & 0xFF);

                            if (op == FILTER_MORPHOLOGY_ERODE)
                            {
                                if (a < bestA) bestA = a;
                                if (r < bestR) bestR = r;
                                if (g < bestG) bestG = g;
                                if (b < bestB) bestB = b;
                            }
                            else
                            {
                                if (a > bestA) bestA = a;
                                if (r > bestR) bestR = r;
                                if (g > bestG) bestG = g;
                                if (b > bestB) bestB = b;
                            }
                        }
                    }

                    drow[x] = pack_argb32(bestA, bestR, bestG, bestB);
                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);

            return true;
        }
        */

        // -----------------------------------------
        // onOffset
        // unary
        // -----------------------------------------

        bool onOffset(const FilterIO& io, const WGRectD* subr, float dx, float dy) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

            auto out = createLikeSurfaceHandle(*in);
            if (!out)
                return false;

            // Start from a copy of the input so pixels outside the primitive subregion
            // remain unchanged.
            {
                SVGB2DDriver bctx{};
                bctx.attach(*out, 1);
                bctx.renew();
                bctx.blendMode(BL_COMP_OP_SRC_COPY);
                bctx.image(*in, 0, 0);
                bctx.detach();
            }

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
            const BLMatrix2D& m = fSpace.ctm;
            const double dxPx = dxUS * m.m00 + dyUS * m.m10;
            const double dyPx = dxUS * m.m01 + dyUS * m.m11;

            const int W = (int)in->width();
            const int H = (int)in->height();

            for (int y = area.y; y < area.y + area.h; ++y)
            {
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

                for (int x = area.x; x < area.x + area.w; ++x)
                {
                    const int sx = (int)std::lround((double)x - dxPx);
                    const int sy = (int)std::lround((double)y - dyPx);

                    if ((unsigned)sx < (unsigned)W && (unsigned)sy < (unsigned)H)
                    {
                        const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)sy);
                        drow[x] = srow[sx];
                    }
                    else
                    {
                        drow[x] = 0u;
                    }
                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }

        // -----------------------------------------
        // onSpecularLighting
        // ------------------------------------------

        bool onSpecularLighting(const FilterIO& io, const WGRectD* subr,
            uint32_t lightingRGBA, float surfaceScale,
            float specularConstant, float specularExponent,
            float kernelUnitLengthX, float kernelUnitLengthY,
            uint32_t lightType, const LightPayload& light) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

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

            const float lcR = float((lightingRGBA >> 16) & 0xFF) * (1.0f / 255.0f);
            const float lcG = float((lightingRGBA >> 8) & 0xFF) * (1.0f / 255.0f);
            const float lcB = float((lightingRGBA >> 0) & 0xFF) * (1.0f / 255.0f);

            const float kPi = 3.14159265358979323846f;

            specularExponent = clamp(specularExponent, 1.0f, 128.0f);

            auto safeNormalize3 = [](float& x, float& y, float& z) noexcept -> bool
                {
                    const float len2 = x * x + y * y + z * z;
                    if (len2 <= 1e-20f)
                    {
                        x = 0.0f;
                        y = 0.0f;
                        z = 1.0f;
                        return false;
                    }

                    const float invLen = 1.0f / std::sqrt(len2);
                    x *= invLen;
                    y *= invLen;
                    z *= invLen;
                    return true;
                };

            auto computeLocalPixelStep = [this](int px, int py, float& duxOut, float& duyOut) noexcept
                {
                    float ux0, uy0;
                    float ux1, uy1;
                    float ux2, uy2;

                    pixelCenterToFilterUser(px, py, ux0, uy0);
                    pixelCenterToFilterUser(px + 1, py, ux1, uy1);
                    pixelCenterToFilterUser(px, py + 1, ux2, uy2);

                    const float ddx0 = ux1 - ux0;
                    const float ddy0 = uy1 - uy0;
                    const float ddx1 = ux2 - ux0;
                    const float ddy1 = uy2 - uy0;

                    duxOut = std::sqrt(ddx0 * ddx0 + ddy0 * ddy0);
                    duyOut = std::sqrt(ddx1 * ddx1 + ddy1 * ddy1);

                    if (!(duxOut > 0.0f))
                        duxOut = 1.0f;
                    if (!(duyOut > 0.0f))
                        duyOut = 1.0f;
                };

            for (int y = area.y; y < area.y + area.h; ++y)
            {
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

                for (int x = area.x; x < area.x + area.w; ++x)
                {
                    float dux = 1.0f;
                    float duy = 1.0f;
                    computeLocalPixelStep(x, y, dux, duy);

                    if (kernelUnitLengthX > 0.0f)
                        dux = kernelUnitLengthX;
                    if (kernelUnitLengthY > 0.0f)
                        duy = kernelUnitLengthY;

                    const float h00 = pixelHeightFromAlpha(*in, x - 1, y - 1);
                    const float h10 = pixelHeightFromAlpha(*in, x, y - 1);
                    const float h20 = pixelHeightFromAlpha(*in, x + 1, y - 1);

                    const float h01 = pixelHeightFromAlpha(*in, x - 1, y);
                    const float h11 = pixelHeightFromAlpha(*in, x, y);
                    const float h21 = pixelHeightFromAlpha(*in, x + 1, y);

                    const float h02 = pixelHeightFromAlpha(*in, x - 1, y + 1);
                    const float h12 = pixelHeightFromAlpha(*in, x, y + 1);
                    const float h22 = pixelHeightFromAlpha(*in, x + 1, y + 1);

                    float dHx = 0.0f;
                    float dHy = 0.0f;

                    if (dux > 0.0f)
                    {
                        dHx =
                            ((h20 + 2.0f * h21 + h22) -
                                (h00 + 2.0f * h01 + h02)) / (8.0f * dux);
                    }

                    if (duy > 0.0f)
                    {
                        dHy =
                            ((h02 + 2.0f * h12 + h22) -
                                (h00 + 2.0f * h10 + h20)) / (8.0f * duy);
                    }

                    float nx = -surfaceScale * dHx;
                    float ny = -surfaceScale * dHy;
                    float nz = 1.0f;
                    safeNormalize3(nx, ny, nz);

                    float ux, uy;
                    pixelCenterToFilterUser(x, y, ux, uy);

                    const float h = surfaceScale * h11;

                    float lx = 0.0f;
                    float ly = 0.0f;
                    float lz = 1.0f;

                    if (lightType == FILTER_LIGHT_DISTANT)
                    {
                        const float az = light.L[0] * (kPi / 180.0f);
                        const float el = light.L[1] * (kPi / 180.0f);

                        lx = std::cos(el) * std::cos(az);
                        ly = std::cos(el) * std::sin(az);
                        lz = std::sin(el);
                        safeNormalize3(lx, ly, lz);
                    }
                    else if (lightType == FILTER_LIGHT_POINT || lightType == FILTER_LIGHT_SPOT)
                    {
                        lx = light.L[0] - ux;
                        ly = light.L[1] - uy;
                        lz = light.L[2] - h;
                        safeNormalize3(lx, ly, lz);
                    }
                    else
                    {
                        lx = 0.0f;
                        ly = 0.0f;
                        lz = 1.0f;
                    }

                    float lightFactor = 1.0f;

                    if (lightType == FILTER_LIGHT_SPOT)
                    {
                        float ax = light.L[3] - light.L[0];
                        float ay = light.L[4] - light.L[1];
                        float az = light.L[5] - light.L[2];

                        float sx = ux - light.L[0];
                        float sy = uy - light.L[1];
                        float sz = h - light.L[2];

                        const bool axisOk = safeNormalize3(ax, ay, az);
                        const bool rayOk = safeNormalize3(sx, sy, sz);

                        if (axisOk && rayOk)
                        {
                            float cosAng = ax * sx + ay * sy + az * sz;
                            cosAng = clamp01f(cosAng);

                            float spotExp = light.L[6];
                            if (!(spotExp > 0.0f))
                                spotExp = 1.0f;

                            lightFactor = std::pow(cosAng, spotExp);

                            const float limitDeg = light.L[7];
                            if (limitDeg > 0.0f)
                            {
                                const float limitCos = std::cos(limitDeg * (kPi / 180.0f));
                                if (cosAng < limitCos)
                                    lightFactor = 0.0f;
                            }
                        }
                        else
                        {
                            lightFactor = 1.0f;
                        }
                    }

                    const float ex = 0.0f;
                    const float ey = 0.0f;
                    const float ez = 1.0f;

                    float hx = lx + ex;
                    float hy = ly + ey;
                    float hz = lz + ez;
                    const bool halfOk = safeNormalize3(hx, hy, hz);

                    float ndoth = 0.0f;
                    if (halfOk)
                    {
                        ndoth = nx * hx + ny * hy + nz * hz;
                        if (ndoth < 0.0f)
                            ndoth = 0.0f;
                    }

                    float lit = 0.0f;
                    if (ndoth > 0.0f)
                        lit = specularConstant * std::pow(ndoth, specularExponent) * lightFactor;

                    lit = clamp01f(lit);

                    float sr = lcR * lit;
                    float sg = lcG * lit;
                    float sb = lcB * lit;

                    sr = clamp01f(sr);
                    sg = clamp01f(sg);
                    sb = clamp01f(sb);

                    float a = sr;
                    if (sg > a) a = sg;
                    if (sb > a) a = sb;

                    a = clamp01f(a);

                    float pr = 0.0f;
                    float pg = 0.0f;
                    float pb = 0.0f;

                    if (a > 0.0f)
                    {
                        pr = sr;
                        pg = sg;
                        pb = sb;
                    }

                    drow[x] = pack_argb32(a, pr, pg, pb);
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
        // -----------------------------------------

        bool onTile(const FilterIO& io, const WGRectD* subr) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface* in = getImage(inKey);
            if (!in)
                return false;

            if (!outKey)
                outKey = kFilter_Last();

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

                for (int x = 0; x < W; ++x)
                {
                    const int sx = srcArea.x + ((x - srcArea.x) % srcArea.w + srcArea.w) % srcArea.w;
                    const int sy = srcArea.y + ((y - srcArea.y) % srcArea.h + srcArea.h) % srcArea.h;

                    const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)sy);
                    drow[x] = srow[sx];
                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);

            return true;
        }

        // -----------------------------------------
        // onTurbulence
        // -----------------------------------------


        /*
        static INLINE uint32_t pack_premul_argb32(float r, float g, float b, float a) noexcept
        {
            r = clamp01(r);
            g = clamp01(g);
            b = clamp01(b);
            a = clamp01(a);

            //if (r < 0.0f) r = 0.0f; else if (r > 1.0f) r = 1.0f;
            //if (g < 0.0f) g = 0.0f; else if (g > 1.0f) g = 1.0f;
            //if (b < 0.0f) b = 0.0f; else if (b > 1.0f) b = 1.0f;
            //if (a < 0.0f) a = 0.0f; else if (a > 1.0f) a = 1.0f;

            // Premultiply for PRGB32-style storage.
            r *= a;
            g *= a;
            b *= a;

            const uint32_t A = (uint32_t)float_to_u8(a);
            const uint32_t R = (uint32_t)float_to_u8(r);
            const uint32_t G = (uint32_t)float_to_u8(g);
            const uint32_t B = (uint32_t)float_to_u8(b);

            return (A << 24) | (R << 16) | (G << 8) | B;
        }
        */



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
                outKey = kFilter_Last();

            // feTurbulence is a generator. Use the filter tile size.
            Surface* like = getImage(kFilter_SourceGraphic());
            if (!like)
            {
                InternedKey lk = lastKey();
                if (lk)
                    like = getImage(lk);
            }

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
                        const double bx = fRunState.objectBBoxUS.x;
                        const double by = fRunState.objectBBoxUS.y;
                        const double bw = fRunState.objectBBoxUS.w;
                        const double bh = fRunState.objectBBoxUS.h;

                        if (bw > 0.0 && bh > 0.0)
                        {
                            px = float((double(ux) - bx) / bw);
                            py = float((double(uy) - by) / bh);
                        }
                        else
                        {
                            px = 0.0f;
                            py = 0.0f;
                        }
                        break;
                    }

                    case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                        // No dedicated conversion implemented elsewhere either.
                        // Fall back to user space for now.
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

                        return *subr;
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

                    return fRunState.filterRectUS;
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
                    pixelCenterToFilterUser(x, y, ux, uy);

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

                    r = clamp01(r);
                    g = clamp01(g);
                    b = clamp01(b);
                    a = clamp01(a);

                    // Store in the executor's normal premultiplied surface format.
                    // This keeps downstream consumers like feDisplacementMap consistent.
                    r *= a;
                    g *= a;
                    b *= a;

                    drow[x] = pack_argb32(a, r, g, b);
                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }
        
    };
}
