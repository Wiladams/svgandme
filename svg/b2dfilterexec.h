#pragma once

#include <memory>
#include <unordered_map>
#include <cstdint>
#include <cmath>

#include "nametable.h"
#include "imageproc.h"
#include "filterprogramexec.h"   // FilterProgramExecutor + IAmFroot<T>
#include "svgb2ddriver.h"

namespace waavs
{
    struct IAmGroot;
}

namespace waavs
{
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

        // feBlend modes (extend as you add support)
        static INLINE InternedKey kBlend_normal()   noexcept { static InternedKey k = PSNameTable::INTERN("normal");   return k; }
        static INLINE InternedKey kBlend_multiply() noexcept { static InternedKey k = PSNameTable::INTERN("multiply"); return k; }
        static INLINE InternedKey kBlend_screen()   noexcept { static InternedKey k = PSNameTable::INTERN("screen");   return k; }
        static INLINE InternedKey kBlend_overlay()  noexcept { static InternedKey k = PSNameTable::INTERN("overlay");  return k; }
        static INLINE InternedKey kBlend_darken()   noexcept { static InternedKey k = PSNameTable::INTERN("darken");   return k; }
        static INLINE InternedKey kBlend_lighten()  noexcept { static InternedKey k = PSNameTable::INTERN("lighten");  return k; }

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

        // --------------------------------------------------------
        // Registry storage
        // --------------------------------------------------------
        struct KeyHash {
            size_t operator()(InternedKey k) const noexcept { return (size_t)k; }
        };
        struct KeyEq { bool operator()(InternedKey a, InternedKey b) const noexcept { return a == b; } };

        using Handle = typename IAmFroot<Surface>::ImageHandle;

        std::unordered_map<InternedKey, Handle, KeyHash, KeyEq> fImages{};
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

        bool putImage(InternedKey key, Handle&& img) noexcept override
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

        Surface& createSurface(InternedKey key, size_t w, size_t h) noexcept override
        {
            auto img = std::make_unique<Surface>((int)w, (int)h);
            //(void)img->reset((size_t)w, (size_t)h); // reset() returns bool; keep minimal reference impl.
            Surface& ref = *img;
            fImages[key] = std::move(img);
            return ref;
        }

        Surface& createLikeSurface(InternedKey key, const Surface& like) noexcept override
        {
            uint32_t lwidth = (uint32_t)like.width();
            uint32_t lheight = (uint32_t)like.height();

            return createSurface(key, like.width(), like.height());
        }

        // --------------------------------------------------------
        // Key resolution helpers for ops
        // --------------------------------------------------------
        INLINE InternedKey resolveInKey(InternedKey k, InternedKey fallback) const noexcept
        {
            InternedKey r = resolveKey(k);
            return r ? r : fallback;
        }
        /*
        InternedKey resolveOutKey(const FilterIO& io) const noexcept
        {
            // If 'out' is missing => implicit last.
            if (!io.hasOut)
                return kFilter_Last();

            // If author provided empty or "__last__" => last.
            InternedKey r = resolveKey(io.out);
            return r ? r : kFilter_Last();
        }
        */
        INLINE InternedKey resolveOutKeyStrict(const FilterIO& io) const noexcept
        {
            if (!io.hasOut) return kFilter_Last();
            if (!io.out)    return kFilter_Last();
            if (io.out == kFilter_Last()) return kFilter_Last();
            return io.out;
        }


        // --------------------------------------------------------
        // applyFilter()
        //
        //
        // SubtreeT requirements:
        //   WGRectD objectBoundingBox() const noexcept;
        //   void draw(IRenderSVG*, IAmGroot*);
        // --------------------------------------------------------


        template<class SubtreeT>
        bool applyFilter(IRenderSVG* ctx,
            IAmGroot* groot,
            SubtreeT* subtree,
            const WGRectD& filterRectUS,
            const FilterProgramStream& program) noexcept
        {
            if (!ctx || !subtree)
                return false;

            const BLMatrix2D ctm = ctx->getTransform();

            // Object bounding box in current drawing user space
            const WGRectD objBB = subtree->calculateObjectBoundingBox(ctx, groot);
            if (!(objBB.w > 0.0) || !(objBB.h > 0.0))
                return true;

            if (!(filterRectUS.w > 0.0) || !(filterRectUS.h > 0.0))
                return true;

            // Map filter region to device pixel space
            const WGRectD filterRectPX = mapRectAABB(ctm, filterRectUS);

            const int tileX = (int)std::floor(filterRectPX.x);
            const int tileY = (int)std::floor(filterRectPX.y);
            const int tileW = (int)std::ceil(filterRectPX.x + filterRectPX.w) - tileX;
            const int tileH = (int)std::ceil(filterRectPX.y + filterRectPX.h) - tileY;

            if (tileW <= 0 || tileH <= 0)
                return true;

            const uint32_t W = (uint32_t)tileW;
            const uint32_t H = (uint32_t)tileH;

            // Reset executor state
            clearSurfaces();
            setLastKey({});

            fRunState.filterUnits = program.filterUnits;
            fRunState.primitiveUnits = program.primitiveUnits;
            fRunState.objectBBoxUS = objBB;
            fRunState.filterRectUS = filterRectUS;

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

            // --------------------------------------------------
            // Render SourceGraphic into tile-local surface
            // --------------------------------------------------

            Surface& srcGraphic = createSurface(kFilter_SourceGraphic(), W, H);

            {
                BLMatrix2D off = ctm;
                off.postTranslate(-double(tileX), -double(tileY));

                SVGB2DDriver tmp{};
                tmp.attach(srcGraphic, 1);
                tmp.renew();

                // Establish correct device mapping
                tmp.transform(off);

                // Root object frame must match main rendering path
                tmp.setObjectFrame(objBB);

                // Render subtree content only (avoid recursive filter call)
                subtree->drawContent(&tmp, groot);

                tmp.detach();
            }

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


        static INLINE BLCompOp compositeOpToCompOp(FilterCompositeOp op) noexcept
        {
            switch (op)
            {
            case FILTER_COMPOSITE_OVER: return BL_COMP_OP_SRC_OVER;
            case FILTER_COMPOSITE_IN:   return BL_COMP_OP_SRC_IN;
            case FILTER_COMPOSITE_OUT:  return BL_COMP_OP_SRC_OUT;
            case FILTER_COMPOSITE_ATOP: return BL_COMP_OP_SRC_ATOP;
            case FILTER_COMPOSITE_XOR:  return BL_COMP_OP_XOR;

            default:
                return BL_COMP_OP_SRC_OVER;
            }
        }

        bool onComposite(const FilterIO& io, const WGRectD* subr,
            FilterCompositeOp op,
            float k1, float k2, float k3, float k4) noexcept override
        {
            (void)subr;

            Surface* in1 = getImage(resolveKey(io.in1));
            Surface* in2 = getImage(resolveKey(io.in2));

            if (!in1 || !in2)
                return false;

            InternedKey outKey = resolveOutKeyStrict(io);
            Surface& out = createLikeSurface(outKey, *in1);

            if (op == FILTER_COMPOSITE_ARITHMETIC)
            {
                // arithmetic pixel loop
            }
            else
            {
                SVGB2DDriver ctx{};
                ctx.attach(out, 1);
                ctx.renew();

                ctx.blendMode(BL_COMP_OP_SRC_COPY);
                ctx.image(*in1, 0, 0);

                BLCompOp compOp = compositeOpToCompOp(op);
                ctx.blendMode(compOp);
                ctx.image(*in2, 0, 0);

                ctx.detach();
            }

            setLastKey(outKey);
            return true;
        }


        bool onGaussianBlur(const FilterIO& io, const WGRectD* subr,
            float sx, float sy) noexcept override
        {
            InternedKey inKey = resolveInKey(io.in1, (lastKey() ? lastKey() : kFilter_SourceGraphic()));
            Surface* in = getImage(inKey);
            if (!in)
                return false;

            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = kFilter_Last();

            const int W = (int)in->width();
            const int H = (int)in->height();

            Surface& out = createSurface(outKey, (uint32_t)W, (uint32_t)H);

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

            if (!(sxPx > 0.0) && !(syPx > 0.0))
            {
                SVGB2DDriver bctx{};
                bctx.attach(out, 0);
                bctx.renew();
                bctx.blendMode(BL_COMP_OP_SRC_COPY);
                bctx.image(*in, 0, 0);
                bctx.detach();

                setLastKey(outKey);
                return true;
            }

            const double sigma = (sxPx > syPx) ? sxPx : syPx;
            if (!(sigma > 0.0))
                return false;

            WGRectI area = { 0, 0, W, H };

            if (subr)
            {
                WGRectD ur{};

                if (fRunState.primitiveUnits == SpaceUnitsKind::SVG_SPACE_OBJECT)
                {
                    const double bx = fRunState.objectBBoxUS.x;
                    const double by = fRunState.objectBBoxUS.y;
                    const double bw = fRunState.objectBBoxUS.w;
                    const double bh = fRunState.objectBBoxUS.h;

                    ur = WGRectD(
                        bx + (double)subr->x * bw,
                        by + (double)subr->y * bh,
                        (double)subr->w * bw,
                        (double)subr->h * bh
                    );
                }
                else
                {
                    ur = WGRectD(
                        (double)subr->x,
                        (double)subr->y,
                        (double)subr->w,
                        (double)subr->h
                    );
                }

                // Map primitive subregion USER rect through the same CTM used to build
                // the filter tile, then shift into tile-local pixel coordinates.
                const WGRectD subrPX = mapRectAABB(fSpace.ctm, ur);

                int ix0 = (int)std::floor(subrPX.x - fSpace.filterRectPX.x);
                int iy0 = (int)std::floor(subrPX.y - fSpace.filterRectPX.y);
                int ix1 = (int)std::ceil((subrPX.x + subrPX.w) - fSpace.filterRectPX.x);
                int iy1 = (int)std::ceil((subrPX.y + subrPX.h) - fSpace.filterRectPX.y);

                int bx0 = 0;
                int by0 = 0;
                int bx1 = W;
                int by1 = H;

                int rx0 = (ix0 > bx0) ? ix0 : bx0;
                int ry0 = (iy0 > by0) ? iy0 : by0;
                int rx1 = (ix1 < bx1) ? ix1 : bx1;
                int ry1 = (iy1 < by1) ? iy1 : by1;

                area.x = rx0;
                area.y = ry0;
                area.w = rx1 - rx0;
                area.h = ry1 - ry0;
            }

            if (area.w <= 0 || area.h <= 0)
            {
                SVGB2DDriver bctx{};
                bctx.attach(out, 1);
                bctx.renew();
                bctx.blendMode(BL_COMP_OP_SRC_COPY);
                bctx.image(*in, 0, 0);
                bctx.detach();

                setLastKey(outKey);
                return true;
            }

            {
                SVGB2DDriver bctx{};
                bctx.attach(out, 1);
                bctx.renew();
                bctx.blendMode(BL_COMP_OP_SRC_COPY);
                bctx.image(*in, 0, 0);
                bctx.detach();
            }

            int box[3];
            waavs::boxesForGauss(sigma, 3, box);

            waavs::Surface tmp0;
            waavs::Surface tmp1;

            if (!tmp0.reset(in->width(), in->height()))
                return false;
            if (!tmp1.reset(in->width(), in->height()))
                return false;

            const waavs::Surface* curSrc = in;
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

            {
                const waavs::Surface& blurred = *curSrc;
                for (int y = area.y; y < area.y + area.h; ++y) {
                    const uint32_t* srow = (const uint32_t*)blurred.rowPointer((size_t)y);
                    uint32_t* drow = (uint32_t*)out.rowPointer((size_t)y);
                    std::memcpy(drow + area.x, srow + area.x, (size_t)area.w * 4u);
                }
            }

            setLastKey(outKey);
            return true;
        }

        // feFlood
// feFlood
        bool onFlood(const FilterIO& io, const WGRectD* subr, uint32_t rgbaPremul) noexcept override
        {
            // Choose a size reference for allocation.
            // feFlood does not use input pixels, but it still needs an output image
            // the size of the current filter tile. Prefer SourceGraphic if present.
            Surface* like = getImage(kFilter_SourceGraphic());

            // Fallback to current last image if SourceGraphic is unavailable.
            if (!like) {
                InternedKey lk = lastKey();
                if (lk)
                    like = getImage(lk);
            }

            if (!like)
                return false;

            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = kFilter_Last();

            Surface& out = createLikeSurface(outKey, *like);

            if (out.width() == 0 || out.height() == 0) {
                setLastKey(outKey);
                return true;
            }

            // No primitive subregion: fill the entire surface.
            if (!subr) {
                out.fillAll(rgbaPremul);
                setLastKey(outKey);
                return true;
            }

            // With a primitive subregion, the flood only affects that region.
            // Everything else remains transparent.
            out.clearAll();
            out.fillRect(*subr, rgbaPremul);
            setLastKey(outKey);

            return true;
        }


        // feOffset
        bool onOffset(const FilterIO& io, const WGRectD* subr, float dx, float dy) noexcept override
        {
            (void)subr;

            // Resolve input.
            InternedKey inKey = resolveInKey(io.in1, (lastKey() ? lastKey() : kFilter_SourceGraphic()));
            Surface* in = getImage(inKey);
            if (!in)
                return false;

            // Resolve output.
            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = kFilter_Last();

            // Create output same size as input tile.
            Surface& out = createLikeSurface(outKey, *in);

            // ------------------------------------------------------------
            // 1) Convert dx/dy from primitive units to USER SPACE
            // ------------------------------------------------------------
            double dxUS = (double)dx;
            double dyUS = (double)dy;

            switch (fRunState.primitiveUnits)
            {
            default:
            case SpaceUnitsKind::SVG_SPACE_USER:
                // already in user units
                break;

            case SpaceUnitsKind::SVG_SPACE_OBJECT:
                dxUS *= (double)fRunState.objectBBoxUS.w;
                dyUS *= (double)fRunState.objectBBoxUS.h;
                break;

            case SpaceUnitsKind::SVG_SPACE_STROKEWIDTH:
                // Placeholder until stroke width is carried in run state.
                // For now, treat as user units.
                break;
            }

            // ------------------------------------------------------------
            // 2) Map USER SPACE offset vector to TILE PIXEL vector
            // ------------------------------------------------------------
            // Use the linear part of the CTM, not sx/sy magnitudes.
            // This correctly handles rotation and shear.
            const BLMatrix2D& m = fSpace.ctm;

            const double dxPx = dxUS * m.m00 + dyUS * m.m10;
            const double dyPx = dxUS * m.m01 + dyUS * m.m11;

            // ------------------------------------------------------------
            // 3) Render shifted input into transparent output
            // ------------------------------------------------------------
            SVGB2DDriver bctx{};
            bctx.attach(out, 1);
            bctx.renew();

            // Start from transparent output, then copy translated input.
            bctx.blendMode(BL_COMP_OP_SRC_COPY);
            bctx.transform(BLMatrix2D::makeTranslation(dxPx, dyPx));
            bctx.image(*in, 0, 0);

            bctx.detach();

            // ------------------------------------------------------------
            // 4) Primitive subregion handling
            // ------------------------------------------------------------
            // Still omitted until subregion mapping is finalized.

            setLastKey(outKey);
            return true;
        }

        
        // feBlend
        bool onBlend(const FilterIO& io, const WGRectD*, FilterBlendMode mode) noexcept override
        {
            InternedKey in1Key = resolveInKey(io.in1, (lastKey() ? lastKey() : kFilter_SourceGraphic()));
            Surface* in1 = getImage(in1Key);
            if (!in1)
                return false;

            Surface* in2 = nullptr;
            if (io.hasIn2) {
                InternedKey in2Key = resolveInKey(io.in2, lastKey());
                in2 = getImage(in2Key);
                if (!in2)
                    return false;
            }

            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = kFilter_Last();

            Surface& out = createLikeSurface(outKey, *in1);

            SVGB2DDriver bctx{};
            bctx.attach(out, 1);
            bctx.renew();

            bctx.image(*in1, 0, 0);

            // out = in2 (blend)
            if (in2) {
                bctx.blendMode(blendModeToCompOp(mode));
                bctx.image(*in2, 0, 0);
            }

            bctx.detach();

            setLastKey(outKey);

            return true;
        }

        // feMerge
        bool onMerge(const FilterIO& io, const WGRectD*, KeySpan inputs) noexcept override
        {
            if (!inputs.n)
                return true;

            Surface* base = nullptr;
            for (uint32_t i = 0; i < inputs.n; ++i) {
                InternedKey k = resolveInKey(inputs.p[i], lastKey());
                Surface* img = getImage(k);
                if (img) { base = img; break; }
            }
            if (!base)
                return false;

            InternedKey outKey = resolveOutKeyStrict(io);
            if (!outKey)
                outKey = kFilter_Last();

            Surface& out = createLikeSurface(outKey, *base);
            memset_l(out.data(), 0, (size_t)out.width() * (size_t)out.height());

            SVGB2DDriver bctx{};
            bctx.attach(out, 1);
            bctx.renew();
            bctx.blendMode(BL_COMP_OP_SRC_OVER);


            for (uint32_t i = 0; i < inputs.n; ++i) {
                InternedKey k = resolveInKey(inputs.p[i], lastKey());
                Surface* img = getImage(k);
                if (!img)
                    continue;
                bctx.image(*img, 0,0);
            }

            bctx.detach();

            setLastKey(outKey);

            return true;
        }
        
    };
}
