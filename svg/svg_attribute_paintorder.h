#pragma once

#include "twobitpu.h"
#include "svgstructuretypes.h"

namespace waavs
{

    template <typename StorageT>
    struct PaintOrderProgram
    {
    public:
        using TBPU = TwoBitProcessingUnit<StorageT>;

        enum class Strictness {
            StrictSVG,
            Permissive
        };

        PaintOrderProgram() = default;
        explicit PaintOrderProgram(StorageT code) noexcept : fTPU(code) {}

        const TBPU& unit() const noexcept { return fTPU; }
        StorageT code() const noexcept { return fTPU.code(); }

        // SVG Paint Order Instructions builder
        // https://www.w3.org/TR/SVG2/render.html#PaintOrderProperty
        // Build up a paint order program from an SVG paint-order attribute span
        static PaintOrderProgram buildSVG(const ByteSpan& bs) noexcept
        {
            PaintOrderProgram prog;
            TBPU& tpu = prog.fTPU;

            // Default program is fill, stroke, markers
            // so, create that code if the attribute is 'normal' or empty
            if (!bs || bs == "normal")
            {
                // fill, stroke, markers
                tpu.emit(TBPU::OP_1, 0);    // Fill
                tpu.emit(TBPU::OP_2, 1);    // Stroke
                tpu.emit(TBPU::OP_3, 2);    // Markers
                tpu.terminate(3);

                return prog;
            }

            uint32_t usedMask = 0;
            uint32_t slot = 0;

            // A helper to ensure we only emit unique ops
            auto emit_unique_op = [&](uint32_t op) noexcept {
                if (op < TBPU::OP_1 || op > TBPU::OP_3) return;
                if (slot >= 3) return;

                const uint32_t bit = 1u << (op - 1u); // op=1..3 -> bit 0..2
                if (usedMask & bit) return;

                usedMask |= bit;
                tpu.emit(op, slot++);
                };

            // 1) parse the user provided tokens
            // (strict: max 3 unique recognized tokens
            ByteSpan p = bs;
            while (p && slot < 3)
            {
                ByteSpan tok = bspan_read_until(p, chrWspChars);
                if (tok.empty())
                    break;

                uint32_t kind = 0;
                if (getEnumValue(SVGPaintOrderEnum, tok, kind))
                {
                    emit_unique_op(kind);
                }

                // unknown tokens ignored
            }

            // 2) append missing in canonical order
            emit_unique_op(TBPU::OP_1);     // SVG_PAINT_ORDER_FILL
            emit_unique_op(TBPU::OP_2);     // SVG_PAINT_ORDER_STROKE
            emit_unique_op(TBPU::OP_3);     // SVG_PAINT_ORDER_MARKERS

            tpu.terminate(slot);
            return prog;
        }

        // Execute the paint order program on the given context
        template <class Renderer>
        void run(Renderer& r) const noexcept
        {
            struct Executor
            {
                Renderer& r;

                void execute(uint32_t op) const noexcept
                {
                    switch (op) {
                    case TBPU::OP_1: r.onFill();  break;
                    case TBPU::OP_2: r.onStroke(); break;
                    case TBPU::OP_3: r.onMarkers(); break;
                    default: break;
                    }
                }
            };

            Executor exec{ r };
            fTPU.run(exec);
        }

    private:
        TBPU fTPU;
    };
}

namespace waavs {
    struct SVGPaintOrderAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::paint_order(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGPaintOrderAttribute>(nullptr);
                node->loadFromAttributes(attrs, groot);
                return node;
                });
        }

        ByteSpan fValue{};
        uint8_t fInstruct = PaintOrderKind::SVG_PAINT_ORDER_NORMAL;

        SVGPaintOrderAttribute(IAmGroot* iMap) :SVGVisualProperty(iMap)
        {
            setName(svgattr::paint_order());
        }

        uint32_t instructions() const noexcept { return fInstruct; }



        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!inChunk)
                return false;

            fValue = inChunk;
            PaintOrderProgram<uint8_t> prog = PaintOrderProgram<uint8_t>::buildSVG(inChunk);
            fInstruct = prog.code();

            set(true);
            setNeedsBinding(false);

            return true;
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {

            ctx->paintOrder(fInstruct);
        }

    };
}
