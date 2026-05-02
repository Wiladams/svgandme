// filter_primitiveelement.h 
#pragma once

#include <array>
#include <functional>
#include <unordered_map>
#include <memory>

#include "svggraphicselement.h"
#include "svgattributes.h"
#include "filter_program_builder.h"

// - Defines the FilterPrimitiveElement class, which represents a single filter primitive in an SVG filter.


namespace waavs
{
    // Base class for filter primitives.
    struct SVGFilterPrimitiveElement : public SVGGraphicsElement
    {
        FilterOpId fOperator{ FOP_END };

        // Common attributes
        FilterColorInterpolation fColorInterpolation{ FilterColorInterpolation::FILTER_COLOR_INTERPOLATION_AUTO };
        InternedKey fIn{ nullptr };
        InternedKey fIn2{ nullptr };
        InternedKey fResult{ nullptr };  // optional

        SVGNumberOrPercent fX{};
        SVGNumberOrPercent fY{};
        SVGNumberOrPercent fWidth{};
        SVGNumberOrPercent fHeight{};


        SVGFilterPrimitiveElement(FilterOpId op)
            : SVGGraphicsElement()
            , fOperator(op)
        {
        }

        virtual bool hasIn2() const noexcept { return fIn2 != nullptr; }
        virtual bool hasOut() const noexcept { return fResult != nullptr; }
        virtual bool hasSubregion() const noexcept
        {
            return fX.isSet() || fY.isSet() || fWidth.isSet() || fHeight.isSet();
        }


        uint8_t flagsForIO() const noexcept
        {
            uint8_t f = 0;
            if (hasOut())       f |= FOPF_HAS_OUT;
            if (hasIn2())       f |= FOPF_HAS_IN2;
            if (hasSubregion()) f |= FOPF_HAS_SUBR;

            return f;
        }

        // Implement this on each filter primitive to build up 
        // the filter program.
        virtual bool emitSelf(FilterProgramStream& fps, InternedKey& last) const noexcept
        {
            (void)fps;
            (void)last;

            return true;
        }

        // Given the last resolved key, resolve the 'in' key for this op.
        // If the op states an explicit 'in' key, use that
        // Otherwise, if last is non-null, use last
        // Otherwise, default to SourceGraphic
        virtual InternedKey resolveIn1(InternedKey last) const noexcept
        {
            if (fIn)
                return fIn;

            return last ? last : filter::SourceGraphic();
        }


        virtual InternedKey resolveIn2(InternedKey last) const noexcept
        {
            if (fIn2)
                return fIn2;

            return filter::SourceGraphic();
        }

        // -------------------------------------------
        void emitCommonIO(FilterProgramStream& out, const InternedKey in1, const InternedKey in2, const uint8_t flags) const noexcept
        {
            // push the interpolation mode for this op,
            // so that the executor can switch modes if needed.
            if (fColorInterpolation == FilterColorInterpolation::FILTER_COLOR_INTERPOLATION_AUTO)
                emit_u32(out, out.colorInterpolation);
            else
                emit_u32(out, fColorInterpolation);

            emit_key(out, in1);

            if (flags & FOPF_HAS_IN2)
            {
                emit_key(out, in2);
                //out.mem.push_back(u64_from_key(in2));
            }

            if (flags & FOPF_HAS_OUT)
            {
                emit_key(out, fResult);
                //out.mem.push_back(u64_from_key(fResult));
            }

            if (flags & FOPF_HAS_SUBR)
            {
                emit_u64(out, packNumberOrPercent(fX));
                emit_u64(out, packNumberOrPercent(fY));
                emit_u64(out, packNumberOrPercent(fWidth));
                emit_u64(out, packNumberOrPercent(fHeight));
            }
        }


        void emitCommon(FilterProgramStream& prog, InternedKey& last) const noexcept
        {
            const uint8_t flags = flagsForIO();
            prog.ops.push_back(packOp(fOperator, flags));

            const InternedKey in1 = resolveIn1(last);
            const InternedKey in2 = resolveIn2(last);
            emitCommonIO(prog, in1, in2, flags);
        }

        bool emitProgram(FilterProgramStream& prog, InternedKey& last) const noexcept
        {
            emitCommon(prog, last);
            emitSelf(prog, last);

            last = finishLast(fResult);

            return true;
        }

        virtual void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept
        {
        }

        // Here we grap the attributes that are common to all filter
        // primitives.  we convert the subregion to NumberOrPercent
        // for easier serialization
        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            double dpi = groot ? groot->dpi() : 96.0;
            BLFont* fontOpt = nullptr;

            ByteSpan interpAttr{};
            if (getAttribute(filter::color_interpolation_filters(), interpAttr))
            {
                InternedKey interpKey = PSNameTable::INTERN(interpAttr);
                fColorInterpolation = parseFilterColorInterpolation(interpKey, FilterColorInterpolation::FILTER_COLOR_INTERPOLATION_AUTO);
            }

            ByteSpan fInAttr{}, fIn2Attr{}, fResultAttr{};
            if (getAttribute(filter::in(), fInAttr)) { fIn = PSNameTable::INTERN(fInAttr); }
            if (getAttribute(filter::in2(), fIn2Attr)) { fIn2 = PSNameTable::INTERN(fIn2Attr); }
            if (getAttribute(filter::result(), fResultAttr)) { fResult = PSNameTable::INTERN(fResultAttr); }


            // Subregion (optional)
            SVGLengthValue x{};
            SVGLengthValue y{};
            SVGLengthValue w{};
            SVGLengthValue h{};

            parseLengthValue(getAttribute(svgattr::x()), x);
            parseLengthValue(getAttribute(svgattr::y()), y);
            parseLengthValue(getAttribute(svgattr::width()), w);
            parseLengthValue(getAttribute(svgattr::height()), h);

            lengthValueToNumberOrPercent(x, fX, dpi, fontOpt, true);
            lengthValueToNumberOrPercent(y, fY, dpi, fontOpt, true);
            lengthValueToNumberOrPercent(w, fWidth, dpi, fontOpt, true);
            lengthValueToNumberOrPercent(h, fHeight, dpi, fontOpt, true);


            fixupFilterSpecificAttributes(groot);
        }
    };
}
