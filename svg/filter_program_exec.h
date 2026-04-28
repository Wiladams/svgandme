// filter_program_exec.h
#pragma once

#include <memory>

#include "filter_exec.h"
#include "filter_types.h"
#include "filter_program.h"
#include "filter_program_builder.h"


//
// FilterProgramExecutor: a base class for executing filter programs.
//
// Helpers to get enum values from u32s stored in mem[] 
// as part of the filter program ABI contract.

namespace waavs
{
    struct FilterPrimitiveSubregion
    {
        SVGNumberOrPercent x;
        SVGNumberOrPercent y;
        SVGNumberOrPercent w;
        SVGNumberOrPercent h;
        bool isValid{ false };
    };


    static INLINE WGFilterColorSpace to_WGFilterColorSpace(FilterColorInterpolation colorInterpolation) noexcept
    {
            // Adjust these enum names to match your actual ColorInterpolation enum.
            switch (colorInterpolation)
            {
            case FILTER_COLOR_INTERPOLATION_SRGB:
                return WG_FILTER_COLORSPACE_SRGB;

            case FILTER_COLOR_INTERPOLATION_LINEAR_RGB:
            default:
                return WG_FILTER_COLORSPACE_LINEAR_RGB;
            }
    }

    static  INLINE void resolveColorInterpolationRGB(
        const ColorSRGB& in,
        FilterColorInterpolation interp,
        float& r,
        float& g,
        float& b) noexcept
    {
        if (interp == FILTER_COLOR_INTERPOLATION_LINEAR_RGB)
        {
            const ColorLinear lin = coloring_srgb_to_linear(in);
            r = clamp01f(lin.r);
            g = clamp01f(lin.g);
            b = clamp01f(lin.b);
        }
        else if (interp == FILTER_COLOR_INTERPOLATION_SRGB)
        {
            r = clamp01f(in.r);
            g = clamp01f(in.g);
            b = clamp01f(in.b);
        }
    }

    // -----------------------------------------
    // Small decode helpers
    // -----------------------------------------


    template<class EnumT>
    static INLINE EnumT takeAnEnum(FilterProgramCursor& c) noexcept
    {
        static_assert(std::is_enum<EnumT>::value, "takeEnum requires enum type");

        const uint32_t v = u32_from_u64(c.take());
        return static_cast<EnumT>(v);
    }

    static INLINE InternedKey take_key(FilterProgramCursor& c) noexcept { return key_from_u64(c.take()); }
    static INLINE float      take_f32(FilterProgramCursor& c) noexcept { return f32_from_u64(c.take()); }
    static INLINE uint32_t   take_u32(FilterProgramCursor& c) noexcept { return u32_from_u64(c.take()); }
    static INLINE uint64_t   take_u64(FilterProgramCursor& c) noexcept { return c.take(); }

    INLINE ColorSRGB take_ColorSRGB(FilterProgramCursor& c)
    {
        ColorSRGB out;
        out.r = take_f32(c);
        out.g = take_f32(c);
        out.b = take_f32(c);
        out.a = take_f32(c);
        return out;
    }





    static INLINE void unpack_listRef(uint64_t ref, uint32_t& off, uint32_t& cnt) noexcept
    {
        u32x2_from_u64(ref, off, cnt);
    }


    struct IAmFrootBase
    {
        virtual ~IAmFrootBase() = default;
        virtual InternedKey lastKey() const noexcept = 0;
        virtual void setLastKey(InternedKey) noexcept = 0;
        virtual InternedKey resolveKey(InternedKey) const noexcept = 0;
    };


    // -----------------------------------------------------------------------------
    // IAmFroot<TImage>
    // Filter Root: Key->Image registry used by filter program execution.
    // A typical instantiation would be IAmFroot<Surface>, but it can be used for other 
    // image types as well.
    // -----------------------------------------------------------------------------

        template<class ImageT>
        struct IAmFroot : IAmFrootBase
        {
            //using ImageHandle = std::unique_ptr<ImageT>;

            virtual ~IAmFroot() = default;

            // Keyed storage (registry owns lifetime)
            virtual bool hasImage(InternedKey key) const noexcept = 0;

            // Non-owning access to registry contents (valid while registry holds it)
            virtual ImageT getImage(InternedKey key) noexcept = 0;
            virtual const ImageT getImage(InternedKey key) const noexcept = 0;

            // Transfer ownership into the registry.
            virtual bool putImage(InternedKey key, ImageT img) noexcept = 0;

            virtual void eraseImage(InternedKey key) noexcept = 0;
            virtual void clearSurfaces() noexcept = 0;

            // "implicit last" bookkeeping (executor uses this for missing 'out')
            virtual InternedKey lastKey() const noexcept = 0;
            virtual void setLastKey(InternedKey k) noexcept = 0;

            // Image allocation helpers (backend decides pixel format, alignment, etc.)
            virtual ImageT createSurfaceHandle(size_t w, size_t h) noexcept = 0;
            virtual ImageT createLikeSurfaceHandle(const ImageT& like) noexcept = 0;
            virtual ImageT copySurfaceHandle(const ImageT& src) noexcept = 0;
        
            // Resolve empty / "__last__" sentinel to concrete key.
            InternedKey resolveKey(InternedKey k) const noexcept override
            {
                static InternedKey kLast = PSNameTable::INTERN("__last__");

                if (!k)
                    return lastKey();

                if (k == kLast)
                    return lastKey();

                return k;
            }

            // --------------------------------------------------------
            // Key resolution helpers for ops
            // --------------------------------------------------------
            
            INLINE InternedKey implicitInputFallback() const noexcept
            {
                return lastKey() ? lastKey() : filter::SourceGraphic();
            }

            INLINE InternedKey resolveExplicitOrImplicitInputKey(InternedKey k) const noexcept
            {
                InternedKey r = resolveKey(k);
                return r ? r : implicitInputFallback();
            }

            // BUGBUG: This one should go away
            INLINE InternedKey resolveInKey(InternedKey k) const noexcept
            {
                InternedKey r = resolveKey(k);
                return r ? r : implicitInputFallback();
            }

            INLINE InternedKey resolveUnaryInputKey(const FilterIO& io) const noexcept
            {
                return resolveExplicitOrImplicitInputKey(io.in1);
            }

            INLINE InternedKey resolveBinaryInput1Key(const FilterIO& io) const noexcept
            {
                if (!io.in1)
                    return implicitInputFallback();

                InternedKey r = resolveKey(io.in1);
                return r ? r : implicitInputFallback();
            }

            INLINE InternedKey resolveBinaryInput2Key(const FilterIO& io) const noexcept
            {
                return resolveExplicitOrImplicitInputKey(io.in2);
            }

            INLINE InternedKey resolveOutKeyStrict(const FilterIO& io) const noexcept
            {
                if (!io.hasOut) return filter::Filter_Last();
                if (!io.out)    return filter::Filter_Last();
                if (io.out == filter::Filter_Last()) return filter::Filter_Last();
                return io.out;
            }

            INLINE InternedKey resolveOutputKey(const FilterIO& io) const noexcept
            {
                return resolveOutKeyStrict(io);
            }
        };
    





        // Resolver interface for filter primitives that need to fetch images by key (e.g. feImage).

        template<class ImageT>
        struct IFilterResourceResolver
        {
            using ImageHandle = std::unique_ptr<ImageT>;

            virtual ~IFilterResourceResolver() = default;

            virtual ImageT resolveFeImage(
                InternedKey imageKey,
                const FilterRunState& runState,
                const WGRectD& subr,
                AspectRatioAlignKind align,
                AspectRatioMeetOrSliceKind meetOrSlice) noexcept = 0;
        };

    // =========================================================================
    // FilterProgramExecutor
    //
    // Pattern:
    //   - execute() sets instance fields: fProg + fCur
    //   - execute() decodes ONLY the op id + flags, then dispatches to a
    //     non-virtual decoder method (gaussianBlur(), offset(), ...)
    //   - each non-virtual decoder method consumes operands according to ABI
    //     and calls the corresponding virtual onXXX() hook
    //   - all onXXX() hooks have default stub implementations (no pure virtuals)
    //
    // This guarantees ABI-correct decoding even if user overrides nothing.
    // =========================================================================
    struct FilterProgramExecutor
    {
        IAmFrootBase* fFroot{ nullptr };

        // Program execution state (set by execute())
        const FilterProgramStream* fProg{ nullptr };
        //FilterProgramCursor fCur{ *(const FilterProgramStream*)nullptr }; // set in execute via placement

        // We can't default-construct FilterProgramCursor (it needs a program ref),
        // so we store raw storage and emplace it in execute().
        alignas(FilterProgramCursor) uint8_t fCurStorage[sizeof(FilterProgramCursor)]{};
        bool fCurLive{ false };

        // Meta information about the current run, 
        // which may be useful to ops during execution 
        // (e.g. feImage needs this for resolving resources)
        FilterRunState fRunState{};


        virtual ~FilterProgramExecutor()
        {
            resetCursor();
        }

        // Access to the filter root (registry of key->image)
        INLINE IAmFrootBase& frootBase() noexcept
        {
            WAAVS_ASSERT(fFroot);
            return *fFroot;
        }

        INLINE const IAmFrootBase& frootBase() const noexcept
        {
            WAAVS_ASSERT(fFroot);
            return *fFroot;
        }

        template<class EnumT>
        INLINE EnumT takeEnum() noexcept
        {
            return takeAnEnum<EnumT>(cur());
        }

        // -----------------------------------------
        // Virtual hooks (ALL STUBS by default)
        // -----------------------------------------
        virtual bool onBeginProgram(const FilterProgramStream&) noexcept { return true; }
        virtual void onEndProgram(const FilterProgramStream&) noexcept {}

        virtual bool onGaussianBlur(const FilterIO&, const FilterPrimitiveSubregion &, float sx, float sy) noexcept
        {
            return true;
        }

        virtual bool onOffset(const FilterIO&, const FilterPrimitiveSubregion&, float dx, float dy) noexcept
        {
            return true;
        }

        virtual bool onBlend(const FilterIO&, const FilterPrimitiveSubregion&,FilterBlendMode ) noexcept
        {
            return true;
        }

        virtual bool onComposite(const FilterIO&, const FilterPrimitiveSubregion&,
            FilterCompositeOp opKey, 
            float k1, 
            float k2, 
            float k3, 
            float k4) noexcept 
        {
            return true;
        }

        virtual bool onColorMatrix(const FilterIO&, const FilterPrimitiveSubregion&,
            FilterColorMatrixType /*typeKey*/, float /*param*/,
            F32Span /*matrix*/) noexcept {
            return true;
        }

        virtual bool onComponentTransfer(const FilterIO&, const FilterPrimitiveSubregion&,
            const ComponentFunc& r,
            const ComponentFunc& g,
            const ComponentFunc& b,
            const ComponentFunc& a) noexcept 
        {
            return true;
        }

        virtual bool onConvolveMatrix(const FilterIO&, const FilterPrimitiveSubregion&,
            uint32_t /*orderX*/, uint32_t /*orderY*/,
            F32Span /*kernel*/,
            float /*divisor*/, 
            float /*bias*/,
            uint32_t /*targetX*/, 
            uint32_t /*targetY*/,
            FilterEdgeMode /*edgeModeKey*/,
            float /*kernelUnitLengthX*/, 
            float /*kernelUnitLengthY*/,
            bool /*preserveAlpha*/) noexcept {
            return true;
        }

        virtual bool onDisplacementMap(const FilterIO&, const FilterPrimitiveSubregion&,
            float /*scale*/, 
            FilterChannelSelector /*xChannel*/, 
            FilterChannelSelector /*yChannel*/) noexcept 
        {
            return true;
        }

        virtual bool onFlood(const FilterIO&, 
            const FilterPrimitiveSubregion&,
            const ColorSRGB& ) noexcept 
        {
            return true;
        }

        virtual bool onImage(const FilterIO&, const FilterPrimitiveSubregion&,
            InternedKey, 
            AspectRatioAlignKind,
            AspectRatioMeetOrSliceKind) noexcept {
            return true;
        }

        virtual bool onMerge(const FilterIO&, const FilterPrimitiveSubregion&,
            KeySpan /*inputs*/) noexcept {
            return true;
        }

        virtual bool onMorphology(const FilterIO&, const FilterPrimitiveSubregion&,
            FilterMorphologyOp /*opKey*/, 
            float /*rx*/, 
            float /*ry*/) noexcept {
            return true;
        }

        virtual bool onTile(const FilterIO&, const FilterPrimitiveSubregion&) noexcept { return true; }

        virtual bool onTurbulence(const FilterIO&, const FilterPrimitiveSubregion&,
            FilterTurbulenceType /*typeKey*/, 
            float /*baseFreqX*/, 
            float /*baseFreqY*/,
            uint32_t /*numOctaves*/, 
            float /*seed*/, 
            bool /*stitchTiles*/) noexcept 
        {
            return true;
        }

        virtual bool onDiffuseLighting(const FilterIO&, const FilterPrimitiveSubregion&,
            const ColorSRGB& ,
            float /*surfaceScale*/, 
            float /*diffuseConstant*/,
            float /*kernelUnitLengthX*/, 
            float /*kernelUnitLengthY*/,
            uint32_t /*lightType*/, 
            const LightPayload& /*light*/) noexcept {
            return true;
        }

        virtual bool onSpecularLighting(const FilterIO&, const FilterPrimitiveSubregion&,
            const ColorSRGB & /*lightingRGBA*/, 
            float /*surfaceScale*/,
            float /*specularConstant*/, 
            float /*specularExponent*/,
            float /*kernelUnitLengthX*/, 
            float /*kernelUnitLengthY*/,
            uint32_t /*lightType*/, 
            const LightPayload& /*light*/) noexcept {
            return true;
        }

        virtual bool onDropShadow(const FilterIO&, const FilterPrimitiveSubregion&,
            float /*dx*/, 
            float /*dy*/, 
            float /*sx*/, 
            float /*sy*/,
            ColorSRGB ) noexcept {
            return true;
        }

        virtual bool onUnknownOp(uint8_t /*opByte*/) noexcept { return false; }

        // -----------------------------------------
        // Cursor lifecycle
        // -----------------------------------------
        INLINE void resetCursor() noexcept
        {
            if (fCurLive) {
                // Trivial dtor, but keep symmetry.
                ((FilterProgramCursor*)fCurStorage)->~FilterProgramCursor();
                fCurLive = false;
            }
            fProg = nullptr;
        }

        INLINE FilterProgramCursor& cur() noexcept
        {
            WAAVS_ASSERT(fCurLive);
            return *(FilterProgramCursor*)fCurStorage;
        }

        INLINE const FilterProgramCursor& cur() const noexcept
        {
            WAAVS_ASSERT(fCurLive);
            return *(const FilterProgramCursor*)fCurStorage;
        }

        INLINE const FilterProgramStream& prog() const noexcept
        {
            WAAVS_ASSERT(fProg);
            return *fProg;
        }

        // -----------------------------------------
        // Decode helpers (use instance cursor)
        // -----------------------------------------
        INLINE InternedKey takeKey() noexcept { return take_key(cur()); }
        INLINE float       takeF32() noexcept { return take_f32(cur()); }
        INLINE uint32_t    takeU32() noexcept { return take_u32(cur()); }
        INLINE uint64_t    takeU64() noexcept { return take_u64(cur()); }
        INLINE ColorSRGB    takeColorSRGB() { return take_ColorSRGB(cur()); }

        void decodeCommonPrefix(uint8_t flags, FilterProgramCursor& c,
            FilterIO& io, FilterPrimitiveSubregion& subr) noexcept
        {
            io = {};

            io.colorInterp = takeEnum<FilterColorInterpolation>();
            io.in1 = take_key(c);

            if (flags & FOPF_HAS_IN2) {
                io.in2 = take_key(c);
                io.hasIn2 = true;
            }

            if (flags & FOPF_HAS_OUT) {
                io.out = take_key(c);
                io.hasOut = true;
            }

            if (flags & FOPF_HAS_SUBR) {
                // Decode the value
                subr.x = unpackNumberOrPercent(c.take());
                subr.y = unpackNumberOrPercent(c.take());
                subr.w = unpackNumberOrPercent(c.take());
                subr.h = unpackNumberOrPercent(c.take());
                subr.isValid = true;
            }
            else {
                subr = {};
            }
        }

        INLINE void decodeCommon(uint8_t flags, FilterIO& io, FilterPrimitiveSubregion& subr) noexcept
        {
            decodeCommonPrefix(flags, cur(), io, subr);
        }

        // Convert a mem list (f32 stored via u64_from_f32) to a scratch float buffer.
        // Reference implementation: malloc/free; override pattern later if you want.
        INLINE F32Span decodeF32ListToScratch(uint32_t off, uint32_t cnt) noexcept
        {
            F32Span out{};
            if (!cnt) return out;
            if (!fProg) return out;

            const auto& m = fProg->mem;
            if (off > m.size()) return out;
            if ((size_t)off + (size_t)cnt > m.size()) return out;

            float* buf = (float*)malloc(sizeof(float) * cnt);
            if (!buf) return out;

            for (uint32_t i = 0; i < cnt; ++i)
                buf[i] = f32_from_u64(m[off + i]);

            out.p = buf;
            out.n = cnt;
            return out;
        }

        static INLINE void freeF32Scratch(F32Span& s) noexcept
        {
            if (s.p) free((void*)s.p);
            s = {};
        }

        INLINE KeySpan decodeKeyListToScratch(uint32_t off, uint32_t cnt) noexcept
        {
            KeySpan out{};
            if (!cnt) return out;
            if (!fProg) return out;

            const auto& m = fProg->mem;
            if (off > m.size()) return out;
            if ((size_t)off + (size_t)cnt > m.size()) return out;

            InternedKey* buf = (InternedKey*)malloc(sizeof(InternedKey) * cnt);
            if (!buf) return out;

            for (uint32_t i = 0; i < cnt; ++i)
                buf[i] = key_from_u64(m[off + i]);

            out.p = buf;
            out.n = cnt;
            return out;
        }

        static INLINE void freeKeyScratch(KeySpan& s) noexcept
        {
            if (s.p) free((void*)s.p);
            s = {};
        }

        // -----------------------------------------
        // Per-op decode methods (non-virtual, ABI-locked)
        // -----------------------------------------
        bool gaussianBlur(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};
            decodeCommon(flags, io, subr);

            const float sx = takeF32();
            const float sy = takeF32();

            return onGaussianBlur(io, subr, sx, sy);
        }

        bool offset(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};
            decodeCommon(flags, io, subr);

            const float dx = takeF32();
            const float dy = takeF32();

            return onOffset(io, subr, dx, dy);
        }

        bool blend(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};
            decodeCommon(flags, io, subr);

            FilterBlendMode mode = takeEnum< FilterBlendMode>();
            return onBlend(io, subr, mode);
        }

        bool composite(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};
            decodeCommon(flags, io, subr);

            FilterCompositeOp op = takeEnum<FilterCompositeOp>();

            float k1 = 0, k2 = 0, k3 = 0, k4 = 0;

            if (op == FILTER_COMPOSITE_ARITHMETIC)
            {
                k1 = takeF32();
                k2 = takeF32();
                k3 = takeF32();
                k4 = takeF32();
            }

            return onComposite(io, subr, op, k1, k2, k3, k4);
        }

        bool colorMatrix(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};
            decodeCommon(flags, io, subr);

            FilterColorMatrixType type_ = takeEnum<FilterColorMatrixType>(); 

            float param = 0.0f;
            F32Span matrix{};

            float tmp[20];

            switch (type_)
            {
            case FILTER_COLOR_MATRIX_MATRIX:

                for (int i = 0; i < 20; i++)
                    tmp[i] = takeF32();

                matrix = { tmp, 20 };
                break;

            case FILTER_COLOR_MATRIX_SATURATE:
            case FILTER_COLOR_MATRIX_HUE_ROTATE:

                param = takeF32();
                break;

            default:
                break;
            }

            return onColorMatrix(io, subr, type_, param, matrix);
        }

        bool componentTransfer(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};
            decodeCommon(flags, io, subr);

            ComponentFunc ch[4]{};

            for (int i = 0; i < 4; ++i)
            {
                ch[i].type = takeEnum<FilterTransferFuncType>();
                ch[i].p0 = takeF32();
                ch[i].p1 = takeF32();
                ch[i].p2 = takeF32();

                const uint32_t cnt = takeU32();

                if (cnt)
                {
                    float* buf = (float*)malloc(sizeof(float) * cnt);
                    if (!buf) {
                        for (int k = 0; k < i; ++k)
                            freeF32Scratch(ch[k].table);
                        return false;
                    }

                    for (uint32_t j = 0; j < cnt; ++j)
                        buf[j] = takeF32();

                    ch[i].table = { buf, cnt };
                }
            }

            const bool ok = onComponentTransfer(io, subr, ch[0], ch[1], ch[2], ch[3]);

            for (int i = 0; i < 4; ++i)
                freeF32Scratch(ch[i].table);

            return ok;
        }

        bool convolveMatrix(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};
            decodeCommon(flags, io, subr);

            uint32_t orderX = 0, orderY = 0;
            u32x2_from_u64(takeU64(), orderX, orderY);

            const uint32_t kcnt = orderX * orderY;

            float* kernelBuf = nullptr;
            F32Span kernel{};

            if (kcnt)
            {
                kernelBuf = (float*)malloc(sizeof(float) * kcnt);
                if (!kernelBuf)
                    return false;

                for (uint32_t i = 0; i < kcnt; ++i)
                    kernelBuf[i] = takeF32();

                kernel.p = kernelBuf;
                kernel.n = kcnt;
            }

            const float divisor = takeF32();
            const float bias = takeF32();

            uint32_t targetX = 0, targetY = 0;
            u32x2_from_u64(takeU64(), targetX, targetY);

            const FilterEdgeMode edgeMode = takeEnum<FilterEdgeMode>();

            const float kulx = takeF32();
            const float kuly = takeF32();

            const bool preserveAlpha = takeU32() != 0;

            const bool ok = onConvolveMatrix(io, subr,
                orderX, orderY,
                kernel,
                divisor, bias,
                targetX, targetY,
                edgeMode,
                kulx, kuly,
                preserveAlpha);

            if (kernelBuf)
                free(kernelBuf);

            return ok;
        }

        bool displacementMap(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};
            decodeCommon(flags, io, subr);

            const float scale = takeF32();
            FilterChannelSelector xCh = takeEnum< FilterChannelSelector>();
            FilterChannelSelector yCh = takeEnum< FilterChannelSelector>();

            return onDisplacementMap(io, subr, scale, xCh, yCh);
        }

        bool flood(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};
            decodeCommon(flags, io, subr);

            ColorSRGB c = takeColorSRGB();
            return onFlood(io, subr, c);
        }

        bool image(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};
            
            decodeCommon(flags, io, subr);
            
            InternedKey imageKey = takeKey();
            AspectRatioAlignKind align = takeEnum<AspectRatioAlignKind>();
            AspectRatioMeetOrSliceKind mos = takeEnum<AspectRatioMeetOrSliceKind>();

            return onImage(io, subr, imageKey, align, mos);
        }

        bool merge(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};
            decodeCommon(flags, io, subr);

            const uint32_t cnt = takeU32();

            KeySpan inputs{};
            if (cnt)
            {
                InternedKey* buf = (InternedKey*)malloc(sizeof(InternedKey) * cnt);
                if (!buf)
                    return false;

                for (uint32_t i = 0; i < cnt; ++i)
                    buf[i] = takeKey();

                inputs = { buf, cnt };
            }

            const bool ok = onMerge(io, subr, inputs);
            freeKeyScratch(inputs);
            return ok;
        }

        bool morphology(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};
            decodeCommon(flags, io, subr);

            FilterMorphologyOp op = takeEnum<FilterMorphologyOp>();


            const float rx = takeF32();
            const float ry = takeF32();

            return onMorphology(io, subr, op, rx, ry);
        }

        bool tile(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};

            decodeCommon(flags, io, subr);

            return onTile(io, subr);
        }

        bool turbulence(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};
            decodeCommon(flags, io, subr);


            FilterTurbulenceType type_ = takeEnum< FilterTurbulenceType>();

            float fx = takeF32();
            float fy = takeF32();
            uint32_t oct = takeU32();
            float seed = takeF32();
            bool stitch = takeU32() != 0;

            return onTurbulence(io, subr, type_, fx, fy, oct, seed, stitch);
        }

        bool diffuseLighting(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};
            decodeCommon(flags, io, subr);

            const ColorSRGB srgb = takeColorSRGB();
            const float surfaceScale = takeF32();
            const float diffuseC = takeF32();
            const float kulx = takeF32();
            const float kuly = takeF32();
            const uint32_t lightType = takeU32();

            LightPayload L{};
            for (int i = 0; i < 8; ++i)
                L.L[i] = takeF32();

            return onDiffuseLighting(io, subr, srgb, surfaceScale, diffuseC, kulx, kuly, lightType, L);
        }

        bool specularLighting(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};

            decodeCommon(flags, io, subr);

            ColorSRGB srgb = takeColorSRGB();
            const float surfaceScale = takeF32();
            const float specC = takeF32();
            const float specExp = takeF32();
            const float kulx = takeF32();
            const float kuly = takeF32();
            const uint32_t lightType = takeU32();

            LightPayload L{};
            for (int i = 0; i < 8; ++i)
                L.L[i] = takeF32();

            return onSpecularLighting(io, subr, srgb, surfaceScale, specC, specExp, kulx, kuly, lightType, L);
        }

        bool dropShadow(uint8_t flags) noexcept
        {
            FilterIO io{};
            FilterPrimitiveSubregion subr{};

            decodeCommon(flags, io, subr);

            const float dx = takeF32();
            const float dy = takeF32();
            const float sx = takeF32();
            const float sy = takeF32();
            const ColorSRGB srgb = takeColorSRGB();

            return onDropShadow(io, subr, dx, dy, sx, sy, srgb);
        }

        // -----------------------------------------
        // execute(): stores prog + cursor as instance state; per-op routines decode operands
        // -----------------------------------------
        bool executeImpl(const FilterProgramStream& progRef) noexcept
        {
            resetCursor();
            fProg = &progRef;

            // Setup the run state
            // This should have already been done in applyTransform
            // or elsewhere, so don't undo it.
            //fRunState.filterUnits = progRef.filterUnits;
            //fRunState.primitiveUnits = progRef.primitiveUnits;
            //fRunState.objectBBoxUS = {};
            //fRunState.filterRectUS = {};

            // Emplace cursor in our storage (cannot default-construct it).
            new (fCurStorage) FilterProgramCursor(progRef);
            fCurLive = true;

            if (!onBeginProgram(progRef)) {
                onEndProgram(progRef);
                resetCursor();
                return false;
            }

            FilterOpType opByte{};
            while (cur().next(opByte))
            {
                const FilterOpId id = opId(opByte);
                const uint8_t flags = opFlags(opByte);

                if (id == FOP_END)
                    break;

                bool ok = true;

                switch (id)
                {
                case FOP_GAUSSIAN_BLUR:      ok = gaussianBlur(flags); break;
                case FOP_OFFSET:             ok = offset(flags); break;
                case FOP_BLEND:              ok = blend(flags); break;
                case FOP_COMPOSITE:          ok = composite(flags); break;
                case FOP_COLOR_MATRIX:       ok = colorMatrix(flags); break;
                case FOP_COMPONENT_TRANSFER: ok = componentTransfer(flags); break;
                case FOP_CONVOLVE_MATRIX:    ok = convolveMatrix(flags); break;
                case FOP_DISPLACEMENT_MAP:   ok = displacementMap(flags); break;
                case FOP_FLOOD:              ok = flood(flags); break;
                case FOP_IMAGE:              ok = image(flags); break;
                case FOP_MERGE:              ok = merge(flags); break;
                case FOP_MORPHOLOGY:         ok = morphology(flags); break;
                case FOP_TILE:               ok = tile(flags); break;
                case FOP_TURBULENCE:         ok = turbulence(flags); break;
                case FOP_DIFFUSE_LIGHTING:   ok = diffuseLighting(flags); break;
                case FOP_SPECULAR_LIGHTING:  ok = specularLighting(flags); break;
                case FOP_DROP_SHADOW:        ok = dropShadow(flags); break;

                default:
                    ok = onUnknownOp(opByte);
                    break;
                }

                if (!ok) {
                    printf("FilterProgramExecutor: op %u failed\n", (unsigned)id);
                    onEndProgram(progRef);
                    resetCursor();
                    return false;
                }
            }

            onEndProgram(progRef);
            resetCursor();
            return true;
        }

        bool execute(const FilterProgramStream& progRef, IAmFrootBase& fr) noexcept
        {
            fFroot = &fr;
            bool ok = executeImpl(progRef);
            fFroot = nullptr;
            return ok;
        }
    };
}
