#pragma once

#include <memory>
#include <unordered_map>
#include <cstdint>
#include <cmath>

#include "nametable.h"
#include "svgb2ddriver.h"

#include "imageproc.h"
#include "filterprogramexec.h"   // FilterProgramExecutor + IAmFroot<T>
#include "filter_noise.h"
#include "svggraphicselement.h"
#include "viewport.h"



namespace waavs
{
    struct IAmGroot;

    // Some decoding helpers that are common enough to 
    // be shared across primitives and executors.
    static INLINE uint8_t clamp_u8(int v) noexcept
    {
        if (v < 0) return 0;
        if (v > 255) return 255;
        return (uint8_t)v;
    }

    static INLINE uint8_t getChannelU8(uint32_t px, FilterChannelSelector ch) noexcept
    {
        switch (ch)
        {
        case FILTER_CHANNEL_R: return (uint8_t)((px >> 16) & 0xFF);
        case FILTER_CHANNEL_G: return (uint8_t)((px >> 8) & 0xFF);
        case FILTER_CHANNEL_B: return (uint8_t)((px >> 0) & 0xFF);
        case FILTER_CHANNEL_A:
        default:               return (uint8_t)((px >> 24) & 0xFF);
        }
    }



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


    // Resolver interface for filter primitives that need to fetch images by key (e.g. feImage).

    template<class ImageT>
    struct IFilterResourceResolver
    {
        using SurfaceHandle = std::unique_ptr<ImageT>;

        virtual ~IFilterResourceResolver() = default;

        virtual SurfaceHandle resolveFeImage(
            InternedKey imageKey,
            const FilterRunState& runState,
            const WGRectD* subr,
            AspectRatioAlignKind align,
            AspectRatioMeetOrSliceKind meetOrSlice) noexcept = 0;
    };


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

        // --------------------------------------------------------
        // applyFilter()
        //
        //
        // SubtreeT requirements:
        //   WGRectD objectBoundingBox() const noexcept;
        //   void draw(IRenderSVG*, IAmGroot*);
        // --------------------------------------------------------

        std::unique_ptr<Surface> makeSourceAlpha(const Surface& src)
        {
            auto out = createLikeSurfaceHandle(src);
            if (!out)
                return nullptr;

            uint32_t* dst = (uint32_t*)out->data();
            const uint32_t* s = (const uint32_t *)src.data();

            size_t n = src.width() * src.height();

            for (size_t i = 0; i < n; ++i) {
                uint8_t a = s[i] >> 24;
                dst[i] = pack_argb32(a, 0, 0, 0);
            }

            return out;
        }

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

            float M[20]{};

            if (type == FILTER_COLOR_MATRIX_MATRIX)
            {
                if (matrix.n != 20 || !matrix.p) {
                    return false;
                }


                for (int i = 0; i < 20; ++i)
                    M[i] = matrix.p[i];
            }
            else if (type == FILTER_COLOR_MATRIX_SATURATE)
            {
                const float s = param;
                M[0] = 0.213f + 0.787f * s; M[1] = 0.715f - 0.715f * s; M[2] = 0.072f - 0.072f * s; M[3] = 0; M[4] = 0;
                M[5] = 0.213f - 0.213f * s; M[6] = 0.715f + 0.285f * s; M[7] = 0.072f - 0.072f * s; M[8] = 0; M[9] = 0;
                M[10] = 0.213f - 0.213f * s; M[11] = 0.715f - 0.715f * s; M[12] = 0.072f + 0.928f * s; M[13] = 0; M[14] = 0;
                M[15] = 0;                   M[16] = 0;                   M[17] = 0;                   M[18] = 1; M[19] = 0;
            }
            else if (type == FILTER_COLOR_MATRIX_HUE_ROTATE)
            {
                const float a = param * 3.14159265358979323846f / 180.0f;
                const float c = std::cos(a);
                const float s = std::sin(a);

                M[0] = 0.213f + c * 0.787f + s * -0.213f;
                M[1] = 0.715f + c * -0.715f + s * -0.715f;
                M[2] = 0.072f + c * -0.072f + s * 0.928f;
                M[3] = 0; M[4] = 0;

                M[5] = 0.213f + c * -0.213f + s * 0.143f;
                M[6] = 0.715f + c * 0.285f + s * 0.140f;
                M[7] = 0.072f + c * -0.072f + s * -0.283f;
                M[8] = 0; M[9] = 0;

                M[10] = 0.213f + c * -0.213f + s * -0.787f;
                M[11] = 0.715f + c * -0.715f + s * 0.715f;
                M[12] = 0.072f + c * 0.928f + s * 0.072f;
                M[13] = 0; M[14] = 0;

                M[15] = 0; M[16] = 0; M[17] = 0; M[18] = 1; M[19] = 0;
            }
            else
            {
                // luminanceToAlpha
                M[0] = M[1] = M[2] = M[3] = M[4] = 0;
                M[5] = M[6] = M[7] = M[8] = M[9] = 0;
                M[10] = M[11] = M[12] = M[13] = M[14] = 0;
                M[15] = 0.2125f; M[16] = 0.7154f; M[17] = 0.0721f; M[18] = 0.0f; M[19] = 0.0f;
            }

            for (int y = area.y; y < area.y + area.h; ++y)
            {
                const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)y);
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

                for (int x = area.x; x < area.x + area.w; ++x)
                {
                    const uint32_t px = srow[x];

                    const float a = float((px >> 24) & 0xFF) / 255.0f;
                    const float r = float((px >> 16) & 0xFF) / 255.0f;
                    const float g = float((px >> 8) & 0xFF) / 255.0f;
                    const float b = float((px >> 0) & 0xFF) / 255.0f;

                    float rr = M[0] * r + M[1] * g + M[2] * b + M[3] * a + M[4];
                    float gg = M[5] * r + M[6] * g + M[7] * b + M[8] * a + M[9];
                    float bb = M[10] * r + M[11] * g + M[12] * b + M[13] * a + M[14];
                    float aa = M[15] * r + M[16] * g + M[17] * b + M[18] * a + M[19];

                    rr = clamp01(rr);
                    gg = clamp01(gg);
                    bb = clamp01(bb);
                    aa = clamp01(aa);

                    rr *= aa;
                    gg *= aa;
                    bb *= aa;

                    drow[x] = pack_argb32(aa, rr, gg, bb);
                    //drow[x] = pack_argb32(
                    //    clamp_u8((int)std::lround(aa * 255.0f)),
                    //    clamp_u8((int)std::lround(rr * 255.0f)),
                    //    clamp_u8((int)std::lround(gg * 255.0f)),
                    //    clamp_u8((int)std::lround(bb * 255.0f)));
                }
            }

            putImage(outKey, std::move(out));
            setLastKey(outKey);
            
            return true;
        }

        // ----------------------------------------
        // onComponentTransfer()
        // unary
        // ----------------------------------------
        static float applyTransferFunc(const ComponentFunc& f, float x) noexcept
        {
            x = clamp01(x);

            switch (f.type)
            {
            default:
            case FILTER_TRANSFER_IDENTITY:
                return x;

            case FILTER_TRANSFER_LINEAR:
                return clamp01(f.p0 * x + f.p1);

            case FILTER_TRANSFER_GAMMA:
                return clamp01(f.p0 * std::pow(x, f.p1) + f.p2);

            case FILTER_TRANSFER_TABLE:
            {
                if (!f.table.p || f.table.n == 0)
                    return x;
                if (f.table.n == 1)
                    return clamp01(f.table.p[0]);

                const float pos = x * float(f.table.n - 1);
                const uint32_t i0 = (uint32_t)std::floor(pos);
                const uint32_t i1 = (i0 + 1 < f.table.n) ? (i0 + 1) : i0;
                const float t = pos - float(i0);
                return clamp01(f.table.p[i0] * (1.0f - t) + f.table.p[i1] * t);
            }

            case FILTER_TRANSFER_DISCRETE:
            {
                if (!f.table.p || f.table.n == 0)
                    return x;

                uint32_t idx = (uint32_t)std::floor(x * float(f.table.n));
                if (idx >= f.table.n)
                    idx = f.table.n - 1;
                return clamp01(f.table.p[idx]);
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

            for (int y = area.y; y < area.y + area.h; ++y)
            {
                const uint32_t* srow = (const uint32_t*)in->rowPointer((size_t)y);
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

                for (int x = area.x; x < area.x + area.w; ++x)
                {
                    const uint32_t px = srow[x];

                    const float a = float((px >> 24) & 0xFF) / 255.0f;
                    const float r = float((px >> 16) & 0xFF) / 255.0f;
                    const float g = float((px >> 8) & 0xFF) / 255.0f;
                    const float b = float((px >> 0) & 0xFF) / 255.0f;

                    float rr = applyTransferFunc(rF, r);
                    float gg = applyTransferFunc(gF, g);
                    float bb = applyTransferFunc(bF, b);
                    const float aa = applyTransferFunc(aF, a);

                    rr *= aa;
                    gg *= aa;
                    bb *= aa;
                    drow[x] = pack_argb32(aa, rr, gg, bb);
                    //drow[x] = pack_argb32(
                    //    clamp_u8((int)std::lround(aa * 255.0f)),
                    //    clamp_u8((int)std::lround(rr * 255.0f)),
                    //    clamp_u8((int)std::lround(gg * 255.0f)),
                    //    clamp_u8((int)std::lround(bb * 255.0f)));
                }
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);

            return true;
        }

        //-----------------------------------------
        // onComposite() - supports feBlend and feComposite (except arithmetic)
        // Type: binary
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
                for (int y = y0; y < y1; ++y)
                {
                    const uint32_t* s1 = (const uint32_t*)in1->rowPointer((size_t)y);
                    const uint32_t* s2 = (const uint32_t*)in2->rowPointer((size_t)y);
                    uint32_t* d = (uint32_t*)out->rowPointer((size_t)y);

                    for (int x = x0; x < x1; ++x)
                    {
                        const uint32_t p1 = s1[x];
                        const uint32_t p2 = s2[x];

                        const float a1 = float((p1 >> 24) & 0xFF) * (1.0f / 255.0f);
                        const float r1 = float((p1 >> 16) & 0xFF) * (1.0f / 255.0f);
                        const float g1 = float((p1 >> 8) & 0xFF) * (1.0f / 255.0f);
                        const float b1 = float((p1 >> 0) & 0xFF) * (1.0f / 255.0f);

                        const float a2 = float((p2 >> 24) & 0xFF) * (1.0f / 255.0f);
                        const float r2 = float((p2 >> 16) & 0xFF) * (1.0f / 255.0f);
                        const float g2 = float((p2 >> 8) & 0xFF) * (1.0f / 255.0f);
                        const float b2 = float((p2 >> 0) & 0xFF) * (1.0f / 255.0f);

                        const float aa = clamp01(k1 * a1 * a2 + k2 * a1 + k3 * a2 + k4);
                        float rr = clamp01(k1 * r1 * r2 + k2 * r1 + k3 * r2 + k4);
                        float gg = clamp01(k1 * g1 * g2 + k2 * g1 + k3 * g2 + k4);
                        float bb = clamp01(k1 * b1 * b2 + k2 * b1 + k3 * b2 + k4);

                        rr *= aa;
                        gg *= aa;
                        bb *= aa;

                        d[x] = pack_argb32(
                            clamp_u8((int)std::lround(aa * 255.0f)),
                            clamp_u8((int)std::lround(rr * 255.0f)),
                            clamp_u8((int)std::lround(gg * 255.0f)),
                            clamp_u8((int)std::lround(bb * 255.0f)));
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

            {
                SVGB2DDriver bctx{};
                bctx.attach(*out, 1);
                bctx.renew();
                bctx.blendMode(BL_COMP_OP_SRC_COPY);
                bctx.image(*in1, 0, 0);
                bctx.detach();
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
            if (area.w <= 0 || area.h <= 0) {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);

                return true;
            }

            const int W = (int)in1->width();
            const int H = (int)in1->height();

            for (int y = area.y; y < area.y + area.h; ++y)
            {
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);
                const uint32_t* mapRow = (const uint32_t*)in2->rowPointer((size_t)y);

                for (int x = area.x; x < area.x + area.w; ++x)
                {
                    const uint32_t mp = mapRow[x];

                    const double mx = (double(getChannelU8(mp, xChannel)) / 255.0 - 0.5) * scaleX;
                    const double my = (double(getChannelU8(mp, yChannel)) / 255.0 - 0.5) * scaleY;

                    int sx = (int)std::lround(double(x) + mx);
                    int sy = (int)std::lround(double(y) + my);
                    //int sx = (int)std::lround(double(x) - mx);
                    //int sy = (int)std::lround(double(y) - my);

                    if (sx < 0 || sy < 0 || sx >= W || sy >= H) {
                        drow[x] = 0u;   // transparent black
                    }
                    else {
                        const uint32_t* srow = (const uint32_t*)in1->rowPointer((size_t)sy);
                        drow[x] = srow[sx];
                    }

                    //if (sx < 0) sx = 0;
                    //if (sy < 0) sy = 0;
                    //if (sx >= W) sx = W - 1;
                    //if (sy >= H) sy = H - 1;

                    //const uint32_t* srow = (const uint32_t*)in1->rowPointer((size_t)sy);
                    //drow[x] = srow[sx];
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

            auto primLenToUserX = [&](double v) noexcept -> double {
                switch (fRunState.primitiveUnits) {
                default:
                case SpaceUnitsKind::SVG_SPACE_USER:
                    return v;
                case SpaceUnitsKind::SVG_SPACE_OBJECT:
                    return v * (double)fRunState.objectBBoxUS.w;
                case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                    return v;
                }
                };

            auto primLenToUserY = [&](double v) noexcept -> double {
                switch (fRunState.primitiveUnits) {
                default:
                case SpaceUnitsKind::SVG_SPACE_USER:
                    return v;
                case SpaceUnitsKind::SVG_SPACE_OBJECT:
                    return v * (double)fRunState.objectBBoxUS.h;
                case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                    return v;
                }
                };

            if (sx < 0.0f) sx = 0.0f;
            if (sy < 0.0f) sy = 0.0f;

            const double sxUS = primLenToUserX((double)sx);
            const double syUS = primLenToUserY((double)sy);

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

            // Use the larger sigma for the box approximation you already use.
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

            {
                SVGB2DDriver bctx{};
                bctx.attach(*out, 1);
                bctx.renew();

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

            {
                SVGB2DDriver bctx{};
                bctx.attach(*out, 1);
                bctx.renew();
                bctx.blendMode(BL_COMP_OP_SRC_COPY);

                bctx.image(*in, 0, 0);
                bctx.detach();
            }

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

            specularExponent = clamp(specularExponent, 1.0f, 128.0f);

            const float dux = (in->width() > 0) ? float(fSpace.filterRectUS.w / double(in->width())) : 1.0f;
            const float duy = (in->height() > 0) ? float(fSpace.filterRectUS.h / double(in->height())) : 1.0f;

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
                    computeHeightNormal(*in, x, y, surfaceScale, dux, duy, nx, ny, nz);
                    safeNormalize3(nx, ny, nz);

                    float ux, uy;
                    pixelCenterToFilterUser(x, y, ux, uy);

                    const float h = surfaceScale * pixelHeightFromAlpha(*in, x, y);

                    // L is surface point -> light.
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

                    float ndotl = nx * lx + ny * ly + nz * lz;
                    if (ndotl < 0.0f)
                        ndotl = 0.0f;

                    float lightFactor = 1.0f;

                    if (lightType == FILTER_LIGHT_SPOT)
                    {
                        // Spotlight axis: light position -> pointsAt.
                        float ax = light.L[3] - light.L[0];
                        float ay = light.L[4] - light.L[1];
                        float az = light.L[5] - light.L[2];

                        // Spotlight ray: light position -> current surface point.
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
                            // Degenerate spotlight: fall back to point-light behavior.
                            lightFactor = 1.0f;
                        }
                    }

                    // Viewer vector. For this first-pass implementation, viewer is along +Z.
                    float vx = 0.0f;
                    float vy = 0.0f;
                    float vz = 1.0f;

                    // Half vector H = normalize(L + V)
                    float hx = lx + vx;
                    float hy = ly + vy;
                    float hz = lz + vz;
                    const bool halfOk = safeNormalize3(hx, hy, hz);

                    float ndoth = 0.0f;
                    if (halfOk)
                    {
                        ndoth = nx * hx + ny * hy + nz * hz;
                        if (ndoth < 0.0f)
                            ndoth = 0.0f;
                    }

                    float lit = 0.0f;
                    if (ndotl > 0.0f && ndoth > 0.0f)
                        lit = specularConstant * std::pow(ndoth, specularExponent) * lightFactor;

                    lit = clamp01f(lit);

                    // Keep output premultiplied and browser-like.
                    // Alpha tracks the visible light contribution.
                    const float a = lit;
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

            // feTurbulence is a generator. Use SourceGraphic as the template
            // surface when available, otherwise fall back to the last image.
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

            const int W = (int)out->width();
            const int H = (int)out->height();
            if (W <= 0 || H <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            WGRectI area{ 0, 0, W, H };
            if (subr)
                area = resolveSubregionPx(subr, *out);

            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            TurbulenceNoiseParams p;
            p.baseFreqX = baseFreqX;
            p.baseFreqY = baseFreqY;
            p.octaves = numOctaves ? numOctaves : 1;
            p.seed = (uint64_t)(seed != 0.0f ? seed : 1.0f);

            const bool isFractal =
                (typeKey == FilterTurbulenceType::FILTER_TURBULENCE_FRACTAL_NOISE);

            // Determine the generator rect in user space.
            // This is used for stitchTiles local coordinates and period derivation.
            WGRectD genRectUS = fSpace.filterRectUS;

            if (subr)
            {
                if (fRunState.primitiveUnits == SpaceUnitsKind::SVG_SPACE_OBJECT)
                {
                    const double bx = fRunState.objectBBoxUS.x;
                    const double by = fRunState.objectBBoxUS.y;

                    genRectUS = WGRectD(
                        bx + subr->x,
                        by + subr->y,
                        subr->w,
                        subr->h);
                }
                else
                {
                    genRectUS = *subr;
                }
            }

            const int32_t basePeriodX =
                safe_period_from_extent((float)genRectUS.w, p.baseFreqX);
            const int32_t basePeriodY =
                safe_period_from_extent((float)genRectUS.h, p.baseFreqY);

            // Build channel states in spec-style R,G,B,A order from one seeded RNG.
            TurbulenceChannelState chans[4];
            buildTurbulenceChannelStates(
                chans,
                (int32_t)(seed != 0.0f ? seed : 1.0f));

            for (int y = area.y; y < area.y + area.h; ++y)
            {
                uint32_t* drow = (uint32_t*)out->rowPointer((size_t)y);

                for (int x = area.x; x < area.x + area.w; ++x)
                {
                    float ux, uy;
                    pixelCenterToFilterUser(x, y, ux, uy);

                    float sx = ux;
                    float sy = uy;

                    if (stitchTiles)
                    {
                        // For stitch mode, sample in primitive-local user space
                        // so the periodic wrapping matches the primitive bounds.
                        sx = ux - (float)genRectUS.x;
                        sy = uy - (float)genRectUS.y;
                    }

                    float r, g, b, a;

                    if (isFractal)
                    {
                        r = sampleFractalChannel(
                            sx, sy, p, chans[0],
                            stitchTiles, basePeriodX, basePeriodY);
                        g = sampleFractalChannel(
                            sx, sy, p, chans[1],
                            stitchTiles, basePeriodX, basePeriodY);
                        b = sampleFractalChannel(
                            sx, sy, p, chans[2],
                            stitchTiles, basePeriodX, basePeriodY);
                        a = sampleFractalChannel(
                            sx, sy, p, chans[3],
                            stitchTiles, basePeriodX, basePeriodY);
                    }
                    else
                    {
                        r = sampleTurbulenceChannel(
                            sx, sy, p, chans[0],
                            stitchTiles, basePeriodX, basePeriodY);
                        g = sampleTurbulenceChannel(
                            sx, sy, p, chans[1],
                            stitchTiles, basePeriodX, basePeriodY);
                        b = sampleTurbulenceChannel(
                            sx, sy, p, chans[2],
                            stitchTiles, basePeriodX, basePeriodY);
                        a = sampleTurbulenceChannel(
                            sx, sy, p, chans[3],
                            stitchTiles, basePeriodX, basePeriodY);
                    }

                    // Use alpha=1 so stored RGB is not weakened by premultiplication.
                    // This matters when turbulence is later consumed as a displacement map.
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
