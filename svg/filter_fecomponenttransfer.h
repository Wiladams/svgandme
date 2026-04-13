#pragma once

#include "filter_types.h"
#include "nametable.h"

namespace waavs
{

    // ============================================================
    // feComponentTransfer
    // ============================================================
    struct ComponentFunc {
        InternedKey typeKey{};
        float p0{}, p1{}, p2{};
        const float* table{};
        uint32_t tableN{};
    };

    static INLINE float applyTransferFunc(float v, const ComponentFunc& f) noexcept
    {
        static InternedKey kIdentity = PSNameTable::INTERN("identity");
        static InternedKey kTable = PSNameTable::INTERN("table");
        static InternedKey kDiscrete = PSNameTable::INTERN("discrete");
        static InternedKey kLinear = PSNameTable::INTERN("linear");
        static InternedKey kGamma = PSNameTable::INTERN("gamma");

        v = WAAVS_CLAMP01(v);
        const InternedKey t = f.typeKey;

        if (!t || t == kIdentity) return v;

        if (t == kLinear) {
            return WAAVS_CLAMP01(WAAVS_FMAF(f.p0, v, f.p1)); // slope, intercept
        }

        if (t == kGamma) {
            const float p = (v <= 0.0f) ? 0.0f : powf(v, f.p1); // exponent
            return WAAVS_CLAMP01(WAAVS_FMAF(f.p0, p, f.p2));    // amplitude, offset
        }

        if ((t == kTable || t == kDiscrete) && f.table && f.tableN) {
            if (f.tableN == 1) return WAAVS_CLAMP01(f.table[0]);

            const float tt = v * (float)(f.tableN - 1);
            if (t == kDiscrete) {
                const uint32_t idx = (uint32_t)(tt);
                const uint32_t i = (idx < f.tableN) ? idx : (f.tableN - 1);
                return WAAVS_CLAMP01(f.table[i]);
            }
            else {
                const uint32_t i0 = (uint32_t)std::floor(tt);
                const uint32_t i1 = (i0 + 1 < f.tableN) ? (i0 + 1) : i0;
                const float frac = tt - (float)i0;
                const float a = f.table[i0], b = f.table[i1];
                return WAAVS_CLAMP01(WAAVS_FMAF((b - a), frac, a));
            }
        }

        return v;
    }

    static INLINE bool feComponentTransfer(const PixelKernelCtx& k,
        const Surface_ARGB32* in,
        Surface_ARGB32* out,
        const ComponentFunc& fr,
        const ComponentFunc& fg,
        const ComponentFunc& fb,
        const ComponentFunc& fa) noexcept
    {
        (void)k;
        const int W = in->width;
        const int H = in->height;

        for (int y = 0; y < H; ++y) {
            const uint32_t* srow = Surface_ARGB32_row_pointer_const(in, y);
            uint32_t* drow = Surface_ARGB32_row_pointer(out, y);

            for (int x = 0; x < W; ++x) {
                const ColorPRGBA sp = pixeling_ARGB32_unpack_prgba(srow[x]);
                const ColorLinear s = coloring_linear_unpremultiply(sp);

                ColorLinear o{};
                o.r = applyTransferFunc(s.r, fr);
                o.g = applyTransferFunc(s.g, fg);
                o.b = applyTransferFunc(s.b, fb);
                o.a = applyTransferFunc(s.a, fa);

                drow[x] = pixeling_prgba_pack_ARGB32(coloring_linear_premultiply(o));
            }
        }

        return true;
    }
}
