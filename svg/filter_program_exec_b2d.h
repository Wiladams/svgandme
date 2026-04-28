// filter_program_exec_b2d.h

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



        Surface createFilterSurface(const FilterRunState& runState) noexcept
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

        Surface resolveFeImage(
            InternedKey imageKey,
            const FilterRunState& runState,
            const WGRectD& subr,
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

            if (!wg_rectD_is_valid(runState.filterRectUS))
                return {};

            WGRectD dstRect = subr; 
            if (!wg_rectD_is_valid(dstRect))
                return {};

            auto out = createFilterSurface(runState);
            if (out.empty())
                return {};
            out.clearAll();

            //PreserveAspectRatio par = makePAR(align, meetOrSlice);
            WGMatrix3x3 fit = WGMatrix3x3::makeIdentity();

            //if (!computeViewBoxToViewport(dstRect, srcViewBox, par, fit))
            //    return {};

            WGMatrix3x3 userToSurface = makeUserToSurfaceMatrix(runState);

            SVGB2DDriver ctx{};
            ctx.attach(out, 1);
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


    struct B2DFilterExecutor final : FilterProgramExecutor, IAmFroot<Surface>
    {
        std::unique_ptr<B2DFilterResourceResolver<Surface> > fResolver{nullptr};

        // --------------------------------------------------------
        // Registry storage
        // --------------------------------------------------------
        //using Handle = typename IAmFroot<Surface>::ImageHandle;

        std::unordered_map<InternedKey, Surface, InternedKeyHash, InternedKeyEquivalent> fImages{};
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

        Surface getImage(InternedKey key) noexcept override
        {
            auto it = fImages.find(key);
            if (it == fImages.end())
                return Surface{};
            return it->second;
        }

        const Surface getImage(InternedKey key) const noexcept override
        {
            auto it = fImages.find(key);
            if (it == fImages.end())
                return Surface{};
            return it->second;
        }

        Surface getInputImage(InternedKey inKey) noexcept
        {
            Surface in = getImage(inKey);
            
            // If we did not find the image, then try to get the 
            // last one.  This happens in some cases where the 
            // filter has been edited to remove an intermediary 
            // step, but the ids were not updated.
            if (in.empty())
            {
                inKey = lastKey();
                in = getImage(lastKey());
            }
            return in;
        }

        bool putImage(InternedKey key, Surface img) noexcept override
        {
            fImages[key] = img;
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

        Surface createSurfaceHandle(size_t w, size_t h) noexcept override
        {
            Surface img{};

            if (!img.reset((int)w, (int)h))
                return {};

            return img;
        }

        Surface createLikeSurfaceHandle(const Surface& like) noexcept override
        {
            auto img = createSurfaceHandle(like.width(), like.height());
            return img;
        }

        // Make a copy of the surface
        Surface copySurfaceHandle(const Surface& src) noexcept override
        {
            auto img = createLikeSurfaceHandle(src);
            if (img.empty())
                return {};

            img.blit(src, 0, 0);

            return img;
        }

        // --------------------------------------------------------
        // Helpers
        // --------------------------------------------------------
        WGRectD resolveSubregionUS(
            const FilterPrimitiveSubregion& subr,
            double padUserX = 0.0,
            double padUserY = 0.0) const noexcept
        {
            WGRectD ur{};

            if (!subr.isValid)
            {
                ur = fRunState.filterRectUS;
            }
            else
            {
                const auto& x = subr.x;
                const auto& y = subr.y;
                const auto& w = subr.w;
                const auto& h = subr.h;

                switch (fRunState.primitiveUnits)
                {
                default:
                case SpaceUnitsKind::SVG_SPACE_USER:
                {
                    const double fx = x.isPercent()
                        ? fRunState.filterRectUS.x + x.calculatedValue() * fRunState.filterRectUS.w
                        : x.value();

                    const double fy = y.isPercent()
                        ? fRunState.filterRectUS.y + y.calculatedValue() * fRunState.filterRectUS.h
                        : y.value();

                    const double fw = w.isPercent()
                        ? w.calculatedValue() * fRunState.filterRectUS.w
                        : w.value();

                    const double fh = h.isPercent()
                        ? h.calculatedValue() * fRunState.filterRectUS.h
                        : h.value();

                    ur = WGRectD(fx, fy, fw, fh);
                    break;
                }

                case SpaceUnitsKind::SVG_SPACE_OBJECT:
                {
                    const double bx = fRunState.objectBBoxUS.x;
                    const double by = fRunState.objectBBoxUS.y;
                    const double bw = fRunState.objectBBoxUS.w;
                    const double bh = fRunState.objectBBoxUS.h;

                    const double fx = bx + x.calculatedValue() * bw;
                    const double fy = by + y.calculatedValue() * bh;
                    const double fw = w.calculatedValue() * bw;
                    const double fh = h.calculatedValue() * bh;

                    ur = WGRectD(fx, fy, fw, fh);
                    break;
                }

                case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                {
                    ur = WGRectD(
                        x.calculatedValue(),
                        y.calculatedValue(),
                        w.calculatedValue(),
                        h.calculatedValue());
                    break;
                }
                }
            }

            if (!(ur.w > 0.0) || !(ur.h > 0.0))
                return WGRectD{};

            if (padUserX > 0.0 || padUserY > 0.0)
            {
                ur.x -= padUserX;
                ur.y -= padUserY;
                ur.w += 2.0 * padUserX;
                ur.h += 2.0 * padUserY;
            }

            return ur;
        }

        WGRectI resolveSubregionPx(
            const FilterPrimitiveSubregion& subr,
            const Surface& like,
            double padUserX = 0.0,
            double padUserY = 0.0) const noexcept
        {
            const int W = int(like.width());
            const int H = int(like.height());

            const WGRectI surfArea{ 0, 0, W, H };

            const WGRectD ur =
                resolveSubregionUS(subr, padUserX, padUserY);

            if (!(ur.w > 0.0) || !(ur.h > 0.0))
                return WGRectI{};

            const WGRectD subrPX = mapRectAABB(fSpace.ctm, ur);

            const int ix0 = int(std::floor(subrPX.x - fSpace.filterRectPX.x));
            const int iy0 = int(std::floor(subrPX.y - fSpace.filterRectPX.y));
            const int ix1 = int(std::ceil((subrPX.x + subrPX.w) - fSpace.filterRectPX.x));
            const int iy1 = int(std::ceil((subrPX.y + subrPX.h) - fSpace.filterRectPX.y));

            const WGRectI subArea{ ix0, iy0, ix1 - ix0, iy1 - iy0 };

            return intersection(subArea, surfArea);
        }
        /*
        WGRectI resolveSubregionPx(
            const FilterPrimitiveSubregion& subr,
            const Surface& like,
            double padUserX = 0.0,
            double padUserY = 0.0) const noexcept
        {
            const int W = (int)like.width();
            const int H = (int)like.height();

            WGRectI surfArea{ 0, 0, W, H };

            WGRectD ur{};

            if (!subr.isValid)
            {
                // Default: entire filter region
                ur = fRunState.filterRectUS;
            }
            else
            {
                const auto& x = subr.x;
                const auto& y = subr.y;
                const auto& w = subr.w;
                const auto& h = subr.h;

                switch (fRunState.primitiveUnits)
                {
                default:
                case SpaceUnitsKind::SVG_SPACE_USER:
                {
                    // Percentages relative to filter region
                    const double fx = x.isPercent()
                        ? fRunState.filterRectUS.x + x.calculatedValue() * fRunState.filterRectUS.w
                        : x.value();

                    const double fy = y.isPercent()
                        ? fRunState.filterRectUS.y + y.calculatedValue() * fRunState.filterRectUS.h
                        : y.value();

                    const double fw = w.isPercent()
                        ? w.calculatedValue() * fRunState.filterRectUS.w
                        : w.value();

                    const double fh = h.isPercent()
                        ? h.calculatedValue() * fRunState.filterRectUS.h
                        : h.value();

                    ur = WGRectD(fx, fy, fw, fh);
                    break;
                }

                case SpaceUnitsKind::SVG_SPACE_OBJECT:
                {
                    const double bx = fRunState.objectBBoxUS.x;
                    const double by = fRunState.objectBBoxUS.y;
                    const double bw = fRunState.objectBBoxUS.w;
                    const double bh = fRunState.objectBBoxUS.h;

                    // BOTH percent and number are bbox-relative
                    const double fx = bx + x.calculatedValue() * bw;
                    const double fy = by + y.calculatedValue() * bh;
                    const double fw = w.calculatedValue() * bw;
                    const double fh = h.calculatedValue() * bh;

                    ur = WGRectD(fx, fy, fw, fh);
                    break;
                }

                case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                {
                    // Leave as raw values (you may refine later)
                    ur = WGRectD(
                        x.value(),
                        y.value(),
                        w.value(),
                        h.value()
                    );
                    break;
                }
                }
            }

            if (!(ur.w > 0.0) || !(ur.h > 0.0))
                return WGRectI{};

            // Kernel padding (blur, morphology, etc.)
            if (padUserX > 0.0 || padUserY > 0.0)
            {
                ur.x -= padUserX;
                ur.y -= padUserY;
                ur.w += 2.0 * padUserX;
                ur.h += 2.0 * padUserY;
            }

            // Map to pixel space
            const WGRectD subrPX = mapRectAABB(fSpace.ctm, ur);

            int ix0 = (int)std::floor(subrPX.x - fSpace.filterRectPX.x);
            int iy0 = (int)std::floor(subrPX.y - fSpace.filterRectPX.y);
            int ix1 = (int)std::ceil((subrPX.x + subrPX.w) - fSpace.filterRectPX.x);
            int iy1 = (int)std::ceil((subrPX.y + subrPX.h) - fSpace.filterRectPX.y);

            WGRectI subArea{ ix0, iy0, ix1 - ix0, iy1 - iy0 };

            return intersection(subArea, surfArea);
        }
        */





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



        Surface makeSourceAlpha(const Surface& src)
        {
            auto out = createLikeSurfaceHandle(src);
            if (out.empty())
                return {};

            Surface_ARGB32 srcInfo = src.info();
            Surface_ARGB32 outInfo = out.info();

            if (srcInfo.contiguous && outInfo.contiguous)
            {
                extractAlpha_row(
                    reinterpret_cast<uint32_t*>(outInfo.data),
                    reinterpret_cast<const uint32_t*>(srcInfo.data),
                    src.width() * src.height());
                return out;
            }

            wg_surface_rows_apply_unary_unchecked(
                outInfo,
                srcInfo,
                [](uint32_t* d, const uint32_t* s, int w) noexcept
                {
                    extractAlpha_row(d, s, size_t(w));
                });

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
        bool getBackgroundLocal(
            IRenderSVG* ctx,
            const WGRectI& filterRectPX,
            Surface& out) noexcept
        {
            out = {};

            if (!ctx)
                return false;

            if (filterRectPX.w <= 0 || filterRectPX.h <= 0)
                return false;

            if (!out.reset(filterRectPX.w, filterRectPX.h))
                return false;

            out.clearAll();

            // Make sure pending renderer work is visible before reading the target.
            ctx->flush();

            BLImage* bkgImage = ctx->currentTarget();
            if (!bkgImage)
                return true; // Valid transparent background.

            Surface bkgSurf = surfaceFromBLImage(*bkgImage);
            if (bkgSurf.empty())
                return true; // Valid transparent background.

            const WGRectI srcBounds = bkgSurf.boundsI();
            const WGRectI clippedSrc = intersection(srcBounds, filterRectPX);

            if (clippedSrc.w <= 0 || clippedSrc.h <= 0)
                return true; // Filter tile lies outside current target.

            Surface srcView;
            if (bkgSurf.getSubSurface(clippedSrc, srcView) != WG_SUCCESS)
                return true;

            const int dstX = clippedSrc.x - filterRectPX.x;
            const int dstY = clippedSrc.y - filterRectPX.y;

            out.blit(srcView, dstX, dstY);

            return true;
        }


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
            const WGMatrix3x3 ctm = ctx->getTransform();

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
            if (srcGraphic.empty())
                return false;

            {
                WGMatrix3x3 off = ctm;
                off.postTranslate(-double(tileX), -double(tileY));

                SVGB2DDriver tmp{};
                tmp.attach(srcGraphic, 1);
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
            InternedKey srcGraphicKey = filter::SourceGraphic();
            if (!putImage(srcGraphicKey, std::move(srcGraphic)))
                return false;


            // make the alpha channel available as SourceAlpha, for primitives that need it.
            // BUGBUG - maybe this can be delayed and the filter primitives that need
            // it can request it on demand.
            auto srcGraphicPtr = getImage(srcGraphicKey);

            // Add background image as well, since some primitives need it (e.g. feBlend)
            InternedKey backgroundKey = filter::BackgroundImage();
            Surface backgroundLocal;
            if (!getBackgroundLocal(ctx, fSpace.filterRectPX, backgroundLocal))
                return false;

            if (!backgroundLocal.empty())
            {
                if (!putImage(backgroundKey, backgroundLocal))
                    return false;
            }



            auto srcAlpha = makeSourceAlpha(srcGraphicPtr);
            if (srcAlpha.empty())
                return false;

            if (!putImage(filter::SourceAlpha(), std::move(srcAlpha)))
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

            Surface outImg = getImage(outKey);
            if (outImg.empty())
                outImg = getImage(filter::SourceGraphic());
            if (outImg.empty())
                return false;

            // --------------------------------------------------
            // Composite result back to main context
            // --------------------------------------------------


            ctx->push();
            
            // Reset transform so tile is drawn in device space
            ctx->transform(WGMatrix3x3::makeIdentity());

            ctx->image(outImg, double(tileX), double(tileY));
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
            if (!lastKey() && hasImage(filter::SourceGraphic()))
                setLastKey(filter::SourceGraphic());
            return true;
        }

        // =============================================
        // feBlend
        // =============================================

        bool onBlend(
            const FilterIO& io,
            const FilterPrimitiveSubregion& subr,
            FilterBlendMode mode) noexcept override
        {
            InternedKey in1Key = resolveBinaryInput1Key(io);
            InternedKey in2Key = resolveBinaryInput2Key(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface in1 = getInputImage(in1Key);
            Surface in2 = getInputImage(in2Key);
            if (in1.empty() || in2.empty())
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(in1);
            if (out.empty())
                return false;

            out.clearAll();

            const WGRectI area = resolveSubregionPx(subr, in1);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, out))
                    return false;

                setLastKey(outKey);
                return true;
            }

            const WGBlendMode wgMode = to_wg_blend_mode(mode);
            const WGFilterColorSpace wgColorSpace =
                to_WGFilterColorSpace(io.colorInterp);

            Surface_ARGB32 outInfo = out.info();
            Surface_ARGB32 in1Info = in1.info();
            Surface_ARGB32 in2Info = in2.info();

            WGResult res = wg_blit_blend_rect(
                outInfo,
                in1Info,
                in2Info,
                area,
                wgMode,
                wgColorSpace);

            if (res != WG_SUCCESS)
                return false;

            if (!putImage(outKey, out))
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
        
        bool onColorMatrix(
            const FilterIO& io,
            const FilterPrimitiveSubregion& subr,
            FilterColorMatrixType type,
            float param,
            F32Span matrix) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface in = getInputImage(inKey);
            if (in.empty())
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(in);
            if (out.empty())
                return false;

            Surface_ARGB32 outInfo = out.info();
            Surface_ARGB32 inInfo = in.info();

            // Preserve behavior: copy full input first
            //if (wg_blit_copy(outInfo, inInfo, 0, 0) != WG_SUCCESS)
            //    return false;
            out.clearAll();

            const WGRectI area = resolveSubregionPx(subr, in);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, out))
                    return false;

                setLastKey(outKey);
                return true;
            }

            const WGFilterColorSpace cs =
                to_WGFilterColorSpace(io.colorInterp);

            ColorMatrixPrepared M{};
            if (!prepare_colormatrix(M, type, param, matrix, cs))
                return false;

            if (wg_colormatrix_rect(out, in, area, M) != WG_SUCCESS)
                return false;

            if (!putImage(outKey, out))
                return false;

            setLastKey(outKey);
            return true;
        }




        // ----------------------------------------
        // onComponentTransfer()
        // unary
        // ----------------------------------------

        bool onComponentTransfer(
            const FilterIO& io,
            const FilterPrimitiveSubregion& subr,
            const ComponentFunc& rF,
            const ComponentFunc& gF,
            const ComponentFunc& bF,
            const ComponentFunc& aF) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface in = getInputImage(inKey);
            if (in.empty())
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(in);
            if (out.empty())
                return false;

            Surface_ARGB32 outInfo = out.info();
            Surface_ARGB32 inInfo = in.info();

            // Preserve input, modify only the primitive subregion.
            //if (wg_blit_copy(outInfo, inInfo, 0, 0) != WG_SUCCESS)
            //    return false;
            out.clearAll();


            const WGRectI area = resolveSubregionPx(subr, in);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, out))
                    return false;

                setLastKey(outKey);
                return true;
            }

            const WGFilterColorSpace cs =
                to_WGFilterColorSpace(io.colorInterp);

            if (wg_rect_componenttransfer(
                out,
                in,
                area,
                rF,
                gF,
                bF,
                aF,
                cs) != WG_SUCCESS)
            {
                return false;
            }

            if (!putImage(outKey, out))
                return false;

            setLastKey(outKey);
            return true;
        }


        //-----------------------------------------
        // onComposite()
        // binary
        // -----------------------------------------
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

        bool onComposite(
            const FilterIO& io,
            const FilterPrimitiveSubregion& subr,
            FilterCompositeOp op,
            float k1,
            float k2,
            float k3,
            float k4) noexcept override
        {
            InternedKey in1Key = resolveBinaryInput1Key(io);
            InternedKey in2Key = resolveBinaryInput2Key(io);

            Surface in1 = getInputImage(in1Key);
            Surface in2 = getInputImage(in2Key);

            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = filter::Filter_Last();

            if (in1.empty() || in2.empty())
                return false;

            auto out = createLikeSurfaceHandle(in1);
            if (out.empty())
                return false;

            out.clearAll();

            WGRectI area = resolveSubregionPx(subr, in1);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, out))
                    return false;

                setLastKey(outKey);
                return true;
            }

            Surface_ARGB32 outInfo = out.info();
            Surface_ARGB32 in1Info = in1.info();
            Surface_ARGB32 in2Info = in2.info();

            Surface_ARGB32 outView{};
            Surface_ARGB32 in1View{};
            Surface_ARGB32 in2View{};

            if (wg_surface_resolve_rect_binary(outInfo, in1Info, in2Info, area, outView, in1View, in2View) != WG_SUCCESS)
            {
                return false;
            }

            //WGRectI clipped = intersection(area, Surface_ARGB32_bounds(&outInfo));
            //clipped = intersection(clipped, Surface_ARGB32_bounds(&in1Info));
            //clipped = intersection(clipped, Surface_ARGB32_bounds(&in2Info));

            //if (clipped.w <= 0 || clipped.h <= 0)
            //{
            //    if (!putImage(outKey, std::move(out)))
            //        return false;

            //    setLastKey(outKey);
            //    return true;
            //}

            //if (Surface_ARGB32_get_subarea(outInfo, clipped, outView) != WG_SUCCESS)
            //    return false;

            //if (Surface_ARGB32_get_subarea(in1Info, clipped, in1View) != WG_SUCCESS)
            //    return false;

            //if (Surface_ARGB32_get_subarea(in2Info, clipped, in2View) != WG_SUCCESS)
            //    return false;

            if (op == FILTER_COMPOSITE_ARITHMETIC)
            {
                const ArithmeticCompositeKind arithmeticKind =
                    classifyArithmetic(k1, k2, k3, k4);

                const ArithmeticCoeffFx fx =
                    makeArithmeticCoeffFx(k1, k2, k3, k4);

                WGResult res = wg_surface_rows_apply_binary_unchecked(
                    outView,
                    in1View,
                    in2View,
                    [arithmeticKind, fx](
                        uint32_t* d,
                        const uint32_t* s1,
                        const uint32_t* s2,
                        int w) noexcept
                    {
                        const size_t n = size_t(w);

                        switch (arithmeticKind)
                        {
                        case ARITH_ZERO:
                            arithmetic_fill_prgb32_row(d, n, 0);
                            break;

                        case ARITH_K1_ONLY:
                            arithmetic_k1_only_prgb32_row(d, s1, s2, n, fx);
                            break;

                        case ARITH_K2_ONLY:
                            if (arithmetic_is_zero(fx.k2))
                                arithmetic_zero_prgb32_row(d, n);
                            else if (arithmetic_is_one(fx.k2, fx.shift))
                                copy_prgb32_row(d, s1, n);
                            else
                                arithmetic_k2_only_prgb32_row(d, s1, n, fx);
                            break;

                        case ARITH_K3_ONLY:
                            if (arithmetic_is_zero(fx.k3))
                                arithmetic_zero_prgb32_row(d, n);
                            else if (arithmetic_is_one(fx.k3, fx.shift))
                                copy_prgb32_row(d, s2, n);
                            else
                                arithmetic_k3_only_prgb32_row(d, s2, n, fx);
                            break;

                        case ARITH_K4_ONLY:
                        {
                            const uint8_t c = arith_k4_only_u8(fx.k4, fx.shift);
                            const uint32_t px = argb32_pack_u8(c, c, c, c);
                            arithmetic_fill_prgb32_row(d, n, px);
                            break;
                        }

                        case ARITH_K2_K3:
                            if (arithmetic_is_zero(fx.k2) &&
                                arithmetic_is_zero(fx.k3))
                            {
                                arithmetic_zero_prgb32_row(d, n);
                            }
                            else if (arithmetic_is_one(fx.k2, fx.shift) &&
                                arithmetic_is_zero(fx.k3))
                            {
                                copy_prgb32_row(d, s1, n);
                            }
                            else if (arithmetic_is_zero(fx.k2) &&
                                arithmetic_is_one(fx.k3, fx.shift))
                            {
                                copy_prgb32_row(d, s2, n);
                            }
                            else
                            {
                                arithmetic_k2_k3_prgb32_row(d, s1, s2, n, fx);
                            }
                            break;

                        case ARITH_K1_K2_K3:
                            arithmetic_k1_k2_k3_prgb32_row(d, s1, s2, n, fx);
                            break;

                        case ARITH_GENERAL:
                        default:
                            arithmetic_general_prgb32_row(d, s1, s2, n, fx);
                            break;
                        }
                    });

                if (res != WG_SUCCESS)
                    return false;
            }
            else
            {
                const WGCompositeOp surfOp =
                    WGCompositeOpFromFilterCompositeOp(op);

                if (wg_blit_copy_unchecked(outView, in2View) != WG_SUCCESS)
                    return false;

                if (wg_blit_composite_unchecked(outView, in1View, surfOp) != WG_SUCCESS)
                    return false;
            }

            if (!putImage(outKey, out))
                return false;

            setLastKey(outKey);
            return true;
        }




        // ----------------------------------------
        // onConvolveMatrix
        // ----------------------------------------
        bool onConvolveMatrix(
            const FilterIO& io,
            const FilterPrimitiveSubregion& subr,
            uint32_t orderX,
            uint32_t orderY,
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

            Surface in = getInputImage(inKey);
            if (in.empty())
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            if (!orderX || !orderY)
                return false;

            if (!kernel.p || kernel.n != orderX * orderY)
                return false;

            if (targetX >= orderX || targetY >= orderY)
                return false;

            auto out = createLikeSurfaceHandle(in);
            if (out.empty())
                return false;

            out.clearAll();

            const WGRectI area = resolveSubregionPx(subr, in);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, out))
                    return false;

                setLastKey(outKey);
                return true;
            }

            if (divisor == 0.0f)
            {
                float sum = 0.0f;
                for (uint32_t i = 0; i < kernel.n; ++i)
                    sum += kernel.p[i];

                divisor = (sum != 0.0f) ? sum : 1.0f;
            }

            Surface_ARGB32 outInfo = out.info();
            Surface_ARGB32 inInfo = in.info();

            Surface_ARGB32 outView{};
            Surface_ARGB32 inView{};

            if (wg_surface_resolve_rect_unary(
                outInfo,
                inInfo,
                area,
                outView,
                inView) != WG_SUCCESS)
            {
                return false;
            }

            if (outView.width <= 0 || outView.height <= 0)
            {
                if (!putImage(outKey, out))
                    return false;

                setLastKey(outKey);
                return true;
            }

            PixelNeighborhood_ARGB32 nb{};
            nb.src = &inInfo;

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

            WGResult res = wg_surface_rows_apply_unchecked(
                outView,
                [&](uint32_t* drow, int w, int yLocal) noexcept
                {
                    const int y = area.y + yLocal;

                    for (int xLocal = 0; xLocal < w; ++xLocal)
                    {
                        const int x = area.x + xLocal;

                        float accR = 0.0f;
                        float accG = 0.0f;
                        float accB = 0.0f;
                        float accA = 0.0f;

                        uint32_t kidx = 0;

                        for (uint32_t ky = 0; ky < orderY; ++ky)
                        {
                            const int sy = y + int(targetY) - int(ky);

                            for (uint32_t kx = 0; kx < orderX; ++kx, ++kidx)
                            {
                                const int sx = x + int(targetX) - int(kx);
                                const uint32_t px =
                                    sample_edge_mode(nb, sx, sy, edgeMode);

                                const float kw = kernel.p[kidx];

                                float a, r, g, b;

                                if (preserveAlpha)
                                {
                                    argb32_unpack_dequantized_straight(px, a, r, g, b);
                                    accR += r * kw;
                                    accG += g * kw;
                                    accB += b * kw;
                                }
                                else {
                                    argb32_unpack_dequantized_prgba(px, a, r, g, b);
                                    accR += r * kw;
                                    accG += g * kw;
                                    accB += b * kw;
                                    accA += a * kw;
                                }

                            }
                        }

                        float aa;
                        float rr;
                        float gg;
                        float bb;

                        if (preserveAlpha)
                        {
                            const uint32_t srcPx =
                                nb.sample_clamp(x, y);

                            aa = dequantize0_255((srcPx >> 24) & 0xFFu);

                            rr = accR / divisor + bias;
                            gg = accG / divisor + bias;
                            bb = accB / divisor + bias;

                            rr = clamp01f(rr);
                            gg = clamp01f(gg);
                            bb = clamp01f(bb);
                            aa = clamp01f(aa);

                            rr *= aa;
                            gg *= aa;
                            bb *= aa;
                        }
                        else
                        {
                            rr = accR / divisor + bias;
                            gg = accG / divisor + bias;
                            bb = accB / divisor + bias;
                            aa = accA / divisor + bias;

                            clamp_to_premul_invariant(aa, rr, gg, bb);
                        }

                        drow[xLocal] = argb32_pack_u8(
                            quantize0_255(aa),
                            quantize0_255(rr),
                            quantize0_255(gg),
                            quantize0_255(bb));
                    }
                });

            if (res != WG_SUCCESS)
                return false;

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }




        // ------------------------------------------
        // onDiffuseLighting
        // Type: lighting
        // 
		// LightingRGBA is coming in as a pre-multiplied color, 
        // It should always be fully opaque, so we should be 
        // able to ignore the alpha?
        // -------------------------------------------

        bool onDiffuseLighting(const FilterIO& io, const FilterPrimitiveSubregion& subr,
            const ColorSRGB & lightingColor, 
            float surfaceScale, 
            float diffuseConstant,
            float kernelUnitLengthX, float kernelUnitLengthY,
            uint32_t lightType, 
            const LightPayload& light) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            // DEBUG - check input and output
            //printf("feDiffuseLighting inKey=%s outKey=%s\n", inKey, outKey);

            Surface in = getInputImage(inKey);
            if (in.empty())
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(in);
            if (out.empty())
                return false;

            out.clearAll();

            WGRectI area = resolveSubregionPx(subr, in);
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
            map.surfaceW = int(in.width());
            map.surfaceH = int(in.height());
            
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

            const int W = int(in.width());
            const int H = int(in.height());

            const int yBeg = area.y;
            const int yEnd = area.y + area.h;
            const int xBeg = area.x;
            const int xCount = area.w;

            for (int y = yBeg; y < yEnd; ++y)
            {
                const int y0 = clamp(y - 1, 0, H - 1);
                const int y1 = clamp(y, 0, H - 1);
                const int y2 = clamp(y + 1, 0, H - 1);

                const uint32_t* row0 = (const uint32_t*)in.rowPointer((size_t)y0);
                const uint32_t* row1 = (const uint32_t*)in.rowPointer((size_t)y1);
                const uint32_t* row2 = (const uint32_t*)in.rowPointer((size_t)y2);
                uint32_t* drow = (uint32_t*)out.rowPointer((size_t)y);

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
                    drow+xBeg,
                    row0,
                    row1,
                    row2,
                    xBeg,
                    xCount,
                    W,
                    p);
            }

            if (!putImage(outKey, out))
                return false;

            setLastKey(outKey);
            return true;
        }


        // ------------------------------------------
        // onDisplacementMap
        // Type: binary
        // -------------------------------------------
        struct DisplacementMapRowParams
        {
            const Surface_ARGB32* srcImage;
            int srcW;
            int srcH;

            int x0Global;
            int yGlobal;

            float scaleX;
            float scaleY;
            float biasX;
            float biasY;

            FilterChannelSelector xChannel;
            FilterChannelSelector yChannel;
        };

        static INLINE void displacementmap_prgb32_row_scalar(
            uint32_t* dst,
            const uint32_t* map,
            int count,
            const DisplacementMapRowParams& p) noexcept
        {
            for (int i = 0; i < count; ++i)
            {
                const int x = p.x0Global + i;
                const uint32_t mp = map[i];

                float mxv;
                float myv;

                argb32_premul_unpack_channels_to_straight(
                    mp,
                    p.xChannel,
                    p.yChannel,
                    mxv,
                    myv);

                const float fx = float(x) + p.biasX + mxv * p.scaleX;
                const float fy = float(p.yGlobal) + p.biasY + myv * p.scaleY;

                const int sx = iroundf_fast(fx);
                const int sy = iroundf_fast(fy);

                if ((unsigned)sx < (unsigned)p.srcW &&
                    (unsigned)sy < (unsigned)p.srcH)
                {
                    const uint32_t* srcRow =
                        Surface_ARGB32_row_pointer_const(p.srcImage, sy);

                    dst[i] = srcRow[sx];
                }
            }
        }


        static INLINE void argb32_premul_unpack_channels_to_straight(
            uint32_t px,
            FilterChannelSelector xChannel,
            FilterChannelSelector yChannel,
            float& mxv,
            float& myv) noexcept
        {
            // Get the alpha channel as a straight value in [0, 1]
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

        bool onDisplacementMap(
            const FilterIO& io,
            const FilterPrimitiveSubregion& subr,
            float scale,
            FilterChannelSelector xChannel,
            FilterChannelSelector yChannel) noexcept override
        {
            InternedKey in1Key = resolveBinaryInput1Key(io);
            InternedKey in2Key = resolveBinaryInput2Key(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface in1 = getInputImage(in1Key);
            Surface in2 = getInputImage(in2Key);
            if (in1.empty() || in2.empty())
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(in1);
            if (out.empty())
                return false;

            out.clearAll();

            double scaleUS = double(scale);

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

            const WGRectI area = resolveSubregionPx(subr, in1);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, out))
                    return false;

                setLastKey(outKey);
                return true;
            }

            Surface_ARGB32 outInfo = out.info();
            Surface_ARGB32 mapInfo = in2.info();
            Surface_ARGB32 srcInfo = in1.info();

            WGRectI clipped = intersection(area, Surface_ARGB32_bounds(&outInfo));
            clipped = intersection(clipped, Surface_ARGB32_bounds(&mapInfo));

            if (clipped.w <= 0 || clipped.h <= 0)
            {
                if (!putImage(outKey, out))
                    return false;

                setLastKey(outKey);
                return true;
            }

            Surface_ARGB32 outView{};
            Surface_ARGB32 mapView{};

            if (Surface_ARGB32_get_subarea(outInfo, clipped, outView) != WG_SUCCESS)
                return false;

            if (Surface_ARGB32_get_subarea(mapInfo, clipped, mapView) != WG_SUCCESS)
                return false;

            const float biasX = -0.5f * scaleX;
            const float biasY = -0.5f * scaleY;

            WGResult res = wg_surface_rows_apply_unary_unchecked(
                outView,
                mapView,
                [&](uint32_t* d, const uint32_t* mapRow, int w) noexcept
                {
                    const ptrdiff_t rowOffsetBytes =
                        reinterpret_cast<const uint8_t*>(mapRow) - mapView.data;

                    const int localY =
                        int(rowOffsetBytes / mapView.stride);

                    DisplacementMapRowParams p{};
                    p.srcImage = &srcInfo;
                    p.srcW = srcInfo.width;
                    p.srcH = srcInfo.height;
                    p.x0Global = clipped.x;
                    p.yGlobal = clipped.y + localY;
                    p.scaleX = scaleX;
                    p.scaleY = scaleY;
                    p.biasX = biasX;
                    p.biasY = biasY;
                    p.xChannel = xChannel;
                    p.yChannel = yChannel;

                    displacementmap_prgb32_row_scalar(d, mapRow, w, p);
                });

            if (res != WG_SUCCESS)
                return false;

            if (!putImage(outKey, out))
                return false;

            setLastKey(outKey);
            return true;
        }




        // ------------------------------------------
        // onDropShadow
        // Type: drop shadow
        // -------------------------------------------
        bool onDropShadow(
            const FilterIO& io,
            const FilterPrimitiveSubregion& subr,
            float dx, float dy,
            float sx, float sy,
            ColorSRGB srgb) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface in = getInputImage(inKey);
            if (in.empty())
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(in);
            if (out.empty())
                return false;

            auto shadow0 = createLikeSurfaceHandle(in);
            if (shadow0.empty())
                return false;

            auto shadow1 = createLikeSurfaceHandle(in);
            if (shadow1.empty())
                return false;

            out.clearAll();
            shadow0.clearAll();
            shadow1.clearAll();

            // -------------------------------------------------
            // Resolve subregion (ONLY authority)
            // -------------------------------------------------
            const WGRectI area = resolveSubregionPx(subr, in);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, out))
                    return false;

                setLastKey(outKey);
                return true;
            }

            const int W = int(in.width());
            const int H = int(in.height());

            // -------------------------------------------------
            // Build shadow from alpha
            // -------------------------------------------------
            {
                const Pixel_ARGB32 px =
                    Pixel_ARGB32_premultiplied_from_ColorSRGB(srgb);

                uint8_t floodA, floodR, floodG, floodB;
                argb32_unpack_u8(px, floodA, floodR, floodG, floodB);

                for (int y = area.y; y < area.y + area.h; ++y)
                {
                    const uint32_t* srow =
                        (const uint32_t*)in.rowPointer((size_t)y);

                    uint32_t* drow =
                        (uint32_t*)shadow0.rowPointer((size_t)y);

                    for (int x = area.x; x < area.x + area.w; ++x)
                    {
                        const uint8_t sa = (uint8_t)((srow[x] >> 24) & 0xFFu);

                        const uint8_t a = (uint8_t)(mul255_round_u8(sa, floodA));
                        const uint8_t r = (uint8_t)(mul255_round_u8(sa, floodR));
                        const uint8_t g = (uint8_t)(mul255_round_u8(sa, floodG));
                        const uint8_t b = (uint8_t)(mul255_round_u8(sa, floodB));

                        drow[x] = argb32_pack_u8(a, r, g, b);
                    }
                }
            }

            // -------------------------------------------------
            // Resolve stdDeviation -> pixel space
            // -------------------------------------------------
            double stdXUS = sx;
            double stdYUS = sy;

            switch (fRunState.primitiveUnits)
            {
            case SpaceUnitsKind::SVG_SPACE_OBJECT:
                stdXUS *= fRunState.objectBBoxUS.w;
                stdYUS *= fRunState.objectBBoxUS.h;
                break;
            default:
                break;
            }

            const double stdXPx = stdXUS * fSpace.sx;
            const double stdYPx = stdYUS * fSpace.sy;

            Surface finalShadow = shadow0;

            // -------------------------------------------------
            // Blur (unchanged pipeline, but region-aware)
            // -------------------------------------------------
            if (stdXPx > 0.0 || stdYPx > 0.0)
            {
                int boxX[3] = { 1, 1, 1 };
                int boxY[3] = { 1, 1, 1 };

                boxesForGauss(stdXPx, 3, boxX);
                boxesForGauss(stdYPx, 3, boxY);

                int spreadX = 0;
                int spreadY = 0;

                for (int i = 0; i < 3; ++i)
                {
                    spreadX += (boxX[i] - 1) / 2;
                    spreadY += (boxY[i] - 1) / 2;
                }

                // Expand via resolver (pixel padding -> user padding)
                const double padUserX = spreadX / fSpace.sx;
                const double padUserY = spreadY / fSpace.sy;

                const WGRectI sampleArea =
                    resolveSubregionPx(subr, in, padUserX, padUserY);

                if (sampleArea.w > 0 && sampleArea.h > 0)
                {
                    Surface curSrc = shadow0;
                    Surface curDst = shadow1;

                    for (int pass = 0; pass < 3; ++pass)
                    {
                        const int rx = (boxX[pass] - 1) / 2;
                        const int ry = (boxY[pass] - 1) / 2;

                        if (rx > 0)
                        {
                            curDst.clearAll();
                            boxBlurH_PRGB32(curDst, curSrc, rx, sampleArea);
                            curSrc = curDst;
                            curDst = (curDst == shadow0) ? shadow1 : shadow0;
                        }

                        if (ry > 0)
                        {
                            curDst.clearAll();
                            boxBlurV_PRGB32(curDst, curSrc, ry, sampleArea);
                            curSrc = curDst;
                            curDst = (curDst == shadow0) ? shadow1 : shadow0;
                        }
                    }

                    finalShadow = curSrc;
                }
            }

            // -------------------------------------------------
            // Offset 
            // -------------------------------------------------
            double dxUS = dx;
            double dyUS = dy;

            switch (fRunState.primitiveUnits)
            {
            case SpaceUnitsKind::SVG_SPACE_OBJECT:
                dxUS *= fRunState.objectBBoxUS.w;
                dyUS *= fRunState.objectBBoxUS.h;
                break;
            default:
                break;
            }

            const WGMatrix3x3& m = fSpace.ctm;

            const double dxPx = dxUS * m.m00 + dyUS * m.m10;
            const double dyPx = dxUS * m.m01 + dyUS * m.m11;

            // -------------------------------------------------
            // Composite
            // -------------------------------------------------
            {
                SVGB2DDriver bctx{};
                bctx.attach(out, 1);
                bctx.renew();

                const double offX = std::floor(dxPx + 0.5);
                const double offY = std::floor(dyPx + 0.5);

                bctx.blendMode(BL_COMP_OP_SRC_COPY);
                bctx.image(finalShadow, offX, offY);

                bctx.blendMode(BL_COMP_OP_SRC_OVER);
                bctx.image(in, 0, 0);

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

        bool onFlood(const FilterIO& io, const FilterPrimitiveSubregion& subr,
            const ColorSRGB& srgb) noexcept override
        {
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface like = getInputImage(lastKey());
            if (like.empty())
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(like);
            if (out.empty())
                return false;

            Surface_ARGB32 outInfo = out.info();

            if (!outInfo.data || outInfo.width <= 0 || outInfo.height <= 0)
            {
                if (!putImage(outKey, out))
                    return false;

                setLastKey(outKey);
                return true;
            }

            // create the pixel we will use to flood
            const Pixel_ARGB32 px =
                Pixel_ARGB32_premultiplied_from_ColorSRGB(srgb);

            // If there is no specified subregion, we can fill the whole 
            // surface directly. Otherwise, we will fill only the subregion, 
            // which requires a bit more work to avoid disturbing pixels 
            // outside the subregion.
            if (!subr.isValid)
            {
                if (wg_surface_fill(outInfo, px) != WG_SUCCESS)
                    return false;

                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            // Clear the whole surface first, to ensure pixels 
            // outside the subregion are fully transparent.
            if (wg_surface_clear(outInfo) != WG_SUCCESS)
                return false;

            // get the specified subregion as a pixel rectangle
            const WGRectI area = resolveSubregionPx(subr, out);

            if (area.w > 0 && area.h > 0)
            {
                Surface_ARGB32 areaView{};

                if (Surface_ARGB32_get_subarea(outInfo, area, areaView) == WG_SUCCESS)
                {
                    if (wg_surface_fill(areaView, px) != WG_SUCCESS)
                        return false;
                }
            }

            if (!putImage(outKey, out))
                return false;

            setLastKey(outKey);
            return true;
        }



        // -----------------------------------------
        // onGaussianBlur()
        // unary
        // -----------------------------------------

        bool onGaussianBlur(
            const FilterIO& io,
            const FilterPrimitiveSubregion& subr,
            float sx,
            float sy) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            Surface in = getInputImage(inKey);
            if (in.empty())
                return false;

            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(in);
            if (out.empty())
                return false;

            auto primLenToUser = [&](double v, double range) noexcept -> double
                {
                    switch (fRunState.primitiveUnits)
                    {
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

            const double sxUS =
                primLenToUser(double(sx), double(fRunState.objectBBoxUS.w));

            const double syUS =
                primLenToUser(double(sy), double(fRunState.objectBBoxUS.h));

            const double sxPx = sxUS * std::abs(double(fSpace.sx));
            const double syPx = syUS * std::abs(double(fSpace.sy));

            const WGRectI writeArea = resolveSubregionPx(subr, in);

            out.clearAll();

            if (writeArea.w <= 0 || writeArea.h <= 0)
            {
                if (!putImage(outKey, std::move(out)))
                    return false;

                setLastKey(outKey);
                return true;
            }

            auto copyWriteArea = [&](const Surface& src) noexcept -> bool
                {
                    Surface_ARGB32 dstInfo = out.info();
                    Surface_ARGB32 srcInfo = src.info();

                    Surface_ARGB32 dstView{};
                    Surface_ARGB32 srcView{};

                    if (Surface_ARGB32_get_subarea(dstInfo, writeArea, dstView) != WG_SUCCESS)
                        return false;

                    if (Surface_ARGB32_get_subarea(srcInfo, writeArea, srcView) != WG_SUCCESS)
                        return false;

                    return wg_blit_copy_unchecked(dstView, srcView) == WG_SUCCESS;
                };

            const bool doX = sxPx > 0.0;
            const bool doY = syPx > 0.0;

            if (!doX && !doY)
            {
                if (!copyWriteArea(in))
                    return false;

                if (!putImage(outKey, out))
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

            const double padUserX = padPxX / fSpace.sx;
            const double padUserY = padPxY / fSpace.sy;

            const WGRectI sampleArea = resolveSubregionPx(subr, in, padUserX, padUserY);


            if (sampleArea.w <= 0 || sampleArea.h <= 0)
            {
                if (!putImage(outKey, out))
                    return false;

                setLastKey(outKey);
                return true;
            }

            Surface tmp0;
            Surface tmp1;

            if (!tmp0.reset(in.width(), in.height()))
                return false;

            if (!tmp1.reset(in.width(), in.height()))
                return false;

            tmp0.clearAll();
            tmp1.clearAll();

            Surface curSrc = in;
            Surface curDst = tmp0;

            for (int pass = 0; pass < 3; ++pass)
            {
                if (doX && rx[pass] > 0)
                {
                    boxBlurH_PRGB32(curDst, curSrc, rx[pass], sampleArea);
                    curSrc = curDst;
                    curDst = (curDst == tmp0) ? tmp1 : tmp0;
                }

                if (doY && ry[pass] > 0)
                {
                    boxBlurV_PRGB32(curDst, curSrc, ry[pass], sampleArea);
                    curSrc = curDst;
                    curDst = (curDst == tmp0) ? tmp1 : tmp0;
                }
            }

            const bool didAnyPass =
                (doX && (rx[0] > 0 || rx[1] > 0 || rx[2] > 0)) ||
                (doY && (ry[0] > 0 || ry[1] > 0 || ry[2] > 0));

            if (!didAnyPass)
            {
                if (!copyWriteArea(in))
                    return false;

                if (!putImage(outKey, out))
                    return false;

                setLastKey(outKey);
                return true;
            }

            if (!copyWriteArea(curSrc))
                return false;

            if (!putImage(outKey, out))
                return false;

            setLastKey(outKey);
            return true;
        }

 




        // -----------------------------------------
        // onImage()
        // Type: generator
        // -----------------------------------------
        bool onImage(const FilterIO& io, 
            const FilterPrimitiveSubregion& subr,
            InternedKey imageKey,
            AspectRatioAlignKind align, 
            AspectRatioMeetOrSliceKind mos) noexcept override
        {
            if (!fResolver)
                return false;

            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = filter::Filter_Last();

            //WGRectI area = resolveSubregionPx(subr, *in);
            const  WGRectD dstRectUS = resolveSubregionUS(subr);
            if (!(dstRectUS.w > 0.0) || !(dstRectUS.h > 0.0))
                return false;

            auto out = fResolver->resolveFeImage(imageKey, fRunState, dstRectUS, align, mos);
            if (out.empty())
                return false;

            if (!putImage(outKey, out))
                return false;

            setLastKey(outKey);
            return true;
        }

        // ------------------------------------------
        // onMerge
        // ------------------------------------------

        bool onMerge(const FilterIO& io, const FilterPrimitiveSubregion& subr, KeySpan inputs) noexcept override
        {
            if (!inputs.n)
                return true;

            Surface base{};
            for (uint32_t i = 0; i < inputs.n; ++i)
            {
                InternedKey k = resolveInKey(inputs.p[i]);
                Surface img = getImage(k);
                if (!img.empty())
                {
                    base = img;
                    break;
                }
            }

            if (base.empty())
                return false;

            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(base);
            if (out.empty())
                return false;

            out.clearAll();

            const WGRectI area = resolveSubregionPx(subr, base);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, out))
                    return false;

                setLastKey(outKey);
                return true;
            }

            Surface_ARGB32 outInfo = out.info();

            Surface_ARGB32 outView{};
            if (Surface_ARGB32_get_subarea(outInfo, area, outView) != WG_SUCCESS)
                return false;

            for (uint32_t i = 0; i < inputs.n; ++i)
            {
                InternedKey k = resolveInKey(inputs.p[i]);
                Surface img = getImage(k);
                if (img.empty())
                    continue;

                Surface_ARGB32 imgInfo = img.info();

                Surface_ARGB32 imgView{};
                if (Surface_ARGB32_get_subarea(imgInfo, area, imgView) != WG_SUCCESS)
                    continue;

                if (wg_blit_composite_unchecked(
                    outView,
                    imgView,
                    WG_COMP_SRC_OVER) != WG_SUCCESS)
                {
                    return false;
                }
            }

            if (!putImage(outKey, out))
                return false;

            setLastKey(outKey);
            return true;
        }





        // -----------------------------------------
        // onMorphology()
        // Type: unary
        // -----------------------------------------

        bool onMorphology(const FilterIO& io, const FilterPrimitiveSubregion& subr,
            FilterMorphologyOp op, float rx, float ry) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface in = getInputImage(inKey);
            if (in.empty())
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(in);
            if (out.empty())
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

            WGRectI area = resolveSubregionPx(subr, in);
            if (area.w <= 0 || area.h <= 0)
            {
                //out->clearAll();

                if (!putImage(outKey, out))
                    return false;

                setLastKey(outKey);
                return true;
            }

            const int W = (int)in.width();
            const int H = (int)in.height();

            auto tmp = createLikeSurfaceHandle(in);
            if (tmp.empty())
                return false;

            out.clearAll();
            tmp.clearAll();

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
                    uint32_t* drow = (uint32_t*)tmp.rowPointer((size_t)y);
                    const uint32_t* srow = (const uint32_t*)in.rowPointer((size_t)y);

                    if (!erode_row(drow, srow, x0, x1, W, rpx, rowScratch))
                        return false;
                }

                if (!erode_col(out, tmp, x0, x1, y0, y1, H, rpy, colScratch))
                    return false;
            }
            else
            {
                for (int y = tmpY0; y <= tmpY1; ++y)
                {
                    uint32_t* drow = (uint32_t*)tmp.rowPointer((size_t)y);
                    const uint32_t* srow = (const uint32_t*)in.rowPointer((size_t)y);

                    if (!dilate_row(drow, srow, x0, x1, W, rpx, rowScratch))
                        return false;
                }

                if (!dilate_col(out, tmp, x0, x1, y0, y1, H, rpy, colScratch))
                    return false;
            }

            if (!putImage(outKey, out))
                return false;

            setLastKey(outKey);

            return true;
        }


        // -----------------------------------------
        // onOffset
        // unary
        // -----------------------------------------

        bool onOffset(
            const FilterIO& io,
            const FilterPrimitiveSubregion& subr,
            float dx,
            float dy) noexcept override
        {
            InternedKey inKey = resolveUnaryInputKey(io);
            InternedKey outKey = resolveOutKeyStrict(io);

            Surface in = getInputImage(inKey);
            if (in.empty())
                return false;

            if (!outKey)
                outKey = filter::Filter_Last();

            auto out = createLikeSurfaceHandle(in);
            if (out.empty())
                return false;

            out.clearAll();

            const WGRectI area = resolveSubregionPx(subr, in);
            if (area.w <= 0 || area.h <= 0)
            {
                if (!putImage(outKey, out))
                    return false;

                setLastKey(outKey);
                return true;
            }

            double dxUS = double(dx);
            double dyUS = double(dy);

            switch (fRunState.primitiveUnits)
            {
            default:
            case SpaceUnitsKind::SVG_SPACE_USER:
                break;

            case SpaceUnitsKind::SVG_SPACE_OBJECT:
                dxUS *= double(fRunState.objectBBoxUS.w);
                dyUS *= double(fRunState.objectBBoxUS.h);
                break;

            case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                break;
            }

            const WGMatrix3x3& m = fSpace.ctm;

            const int offX =
                int(std::lround(dxUS * m.m00 + dyUS * m.m10));

            const int offY =
                int(std::lround(dxUS * m.m01 + dyUS * m.m11));

            Surface_ARGB32 outInfo = out.info();
            Surface_ARGB32 inInfo = in.info();

            Surface_ARGB32 outArea{};

            if (Surface_ARGB32_get_subarea(outInfo, area, outArea) != WG_SUCCESS)
                return false;

            if (wg_blit_copy(
                outArea,
                inInfo,
                offX - area.x,
                offY - area.y) != WG_SUCCESS)
            {
                return false;
            }

            if (!putImage(outKey, std::move(out)))
                return false;

            setLastKey(outKey);
            return true;
        }




        // -----------------------------------------
        // onSpecularLighting
        // ------------------------------------------
        // pixelHeightFromAlpha()
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


bool onSpecularLighting(const FilterIO& io, const FilterPrimitiveSubregion& subr,
    const ColorSRGB& lightingColor,
    float surfaceScale,
    float specularConstant, float specularExponent,
    float kernelUnitLengthX, float kernelUnitLengthY,
    uint32_t lightType, const LightPayload& light) noexcept override
{
    InternedKey inKey = resolveUnaryInputKey(io);
    InternedKey outKey = resolveOutKeyStrict(io);

    Surface in = getInputImage(inKey);
    if (in.empty())
        return false;

    if (!outKey)
        outKey = filter::Filter_Last();

    auto out = createLikeSurfaceHandle(in);
    if (out.empty())
        return false;

    out.clearAll();

    WGRectI area = resolveSubregionPx(subr, in);
    if (area.w <= 0 || area.h <= 0)
    {
        if (!putImage(outKey, out))
            return false;

        setLastKey(outKey);
        return true;
    }

    float lcR, lcG, lcB;
    resolveColorInterpolationRGB(lightingColor, io.colorInterp, lcR, lcG, lcB);



    specularExponent = clamp(specularExponent, 1.0f, 128.0f);

    PixelToFilterUserMap map;
    map.surfaceW = int(in.width());
    map.surfaceH = int(in.height());
    
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
        uint32_t* drow = (uint32_t*)out.rowPointer((size_t)y);

        for (int x = area.x; x < area.x + area.w; ++x)
        {
            const float h00 = pixelHeightFromAlpha(in, x - 1, y - 1);
            const float h10 = pixelHeightFromAlpha(in, x, y - 1);
            const float h20 = pixelHeightFromAlpha(in, x + 1, y - 1);

            const float h01 = pixelHeightFromAlpha(in, x - 1, y);
            const float h11 = pixelHeightFromAlpha(in, x, y);
            const float h21 = pixelHeightFromAlpha(in, x + 1, y);

            const float h02 = pixelHeightFromAlpha(in, x - 1, y + 1);
            const float h12 = pixelHeightFromAlpha(in, x, y + 1);
            const float h22 = pixelHeightFromAlpha(in, x + 1, y + 1);

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

        bool onTile(const FilterIO& io, const FilterPrimitiveSubregion& subr) noexcept override
        {
    InternedKey inKey = resolveUnaryInputKey(io);
    InternedKey outKey = resolveOutKeyStrict(io);

    Surface in = getInputImage(inKey);
    if (in.empty())
        return false;

    if (!outKey)
        outKey = filter::Filter_Last();

    auto out = createLikeSurfaceHandle(in);
    if (out.empty())
        return false;

    out.clearAll();

    const WGRectI area = resolveSubregionPx(subr, in);
    if (area.w <= 0 || area.h <= 0)
    {
        if (!putImage(outKey, out))
            return false;

        setLastKey(outKey);
        return true;
    }

    Surface outArea;
    if (out.getSubSurface(area, outArea) != WG_SUCCESS)
        return false;

    const int tileW = (int)in.width();
    const int tileH = (int)in.height();

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
            outArea.blit(in, x, y);
        }
    }

    if (!putImage(outKey, out))
        return false;

    setLastKey(outKey);
    return true;
        }



        // -----------------------------------------
        // onTurbulence
        // -----------------------------------------

        bool onTurbulence(
    const FilterIO& io,
    const FilterPrimitiveSubregion& subr,
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

    Surface like = getImage(lastKey());
    if (like.empty())
        return false;

    auto out = createLikeSurfaceHandle(like);
    if (out.empty())
        return false;

    out.clearAll();

    WGRectI area = resolveSubregionPx(subr, out);
    if (area.w <= 0 || area.h <= 0)
    {
        if (!putImage(outKey, out))
            return false;

        setLastKey(outKey);
        return true;
    }

    // --- Pixel mapping ---
    PixelToFilterUserMap map;
    map.surfaceW = int(out.width());
    map.surfaceH = int(out.height());
    map.filterExtentUS = fSpace.filterExtentUS;
    map.uxPerPixel = (map.surfaceW > 0)
        ? float(map.filterExtentUS.w / double(map.surfaceW))
        : 1.0f;
    map.uyPerPixel = (map.surfaceH > 0)
        ? float(map.filterExtentUS.h / double(map.surfaceH))
        : 1.0f;

    // --- Noise params ---
    TurbulenceNoiseParams params{};
    params.baseFreqX = baseFreqX;
    params.baseFreqY = baseFreqY;
    params.seed = seed;
    params.octaves = numOctaves;

    TurbulenceState turb{};
    buildTurbulenceState(turb, (int32_t)seed);

    // --- User -> primitive mapping ---
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

    // --- Stitch setup (derived from resolved area) ---
    TurbulenceStitchInfo stitchInfo{};
    const TurbulenceStitchInfo* stitchPtr = nullptr;

    if (stitchTiles)
    {
        // Convert resolved pixel area back to user space
        const WGRectD areaUS = resolveSubregionUS(subr);

        if (areaUS.w > 0.0 && areaUS.h > 0.0)
        {
            adjust_base_frequencies_for_stitch(
                (float)areaUS.w,
                (float)areaUS.h,
                params.baseFreqX,
                params.baseFreqY);

            stitchInfo = prepare_stitch_info(
                (float)areaUS.x,
                (float)areaUS.y,
                (float)areaUS.w,
                (float)areaUS.h,
                params.baseFreqX,
                params.baseFreqY);

            stitchPtr = &stitchInfo;
        }
    }

    const bool fractalNoise = (typeKey == FILTER_TURBULENCE_FRACTAL_NOISE);

    // --- Main loop ---
    for (int y = area.y; y < area.y + area.h; ++y)
    {
        uint32_t* drow = (uint32_t*)out.rowPointer((size_t)y);

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

            r *= a;
            g *= a;
            b *= a;

            drow[x] = argb32_pack_u8(
                quantize0_255(a),
                quantize0_255(r),
                quantize0_255(g),
                quantize0_255(b));
        }
    }

    if (!putImage(outKey, out))
        return false;

    setLastKey(outKey);
    return true;
            }



    };
}
