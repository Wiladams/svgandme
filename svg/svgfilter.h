// svgfilter.h 
#pragma once


#include <array>
#include <functional>
#include <unordered_map>
#include <memory>

#include "svggraphicselement.h"
#include "svgattributes.h"
#include "filter_program_builder.h"



// Elements related to filters
// filter			- compound


// feBlend			- single
// feColorMatrix	- single
// feComponentTransfer- single
// feComposite		- single
// feConvolveMatrix	- single
// feDiffuseLighting- single
// feDisplacementMap- single
// feFlood			- single
// feGaussianBlur	- single
// feImage			- single
// feMerge			- compound
// feMorphology		- single
// feOffset			- single
// feSpecularLighting- single
// feTile			- single
// feTurbulence		- single

// feMergeNode		- single
// feDistantLight	- single
// fePointLight		- single
// feSpotLight		- single


// feFuncR			- single
// feFuncG			- single
// feFuncB			- single
// feFuncA			- single
//


namespace waavs
{



    static INLINE bool lengthToFilterNumberOrPercent(const SVGLengthValue& inVal,
        SVGNumberOrPercent& outVal,
        double dpi = 96.0,
        const BLFont* font = nullptr) noexcept
    {
        return lengthValueToNumberOrPercent(inVal, outVal, dpi, font, true);
    }

    struct SVGNumberPair
    {
        float a{ 0.0f };
        float b{ 0.0f };
        bool hasB{ false }; // if false, b is implied = a

        void set(float v) { a = v; b = v; hasB = false; }
    };

    // Parse: <number> [<number>]
    // Note: negative numbers are treated as 0, per SVG spec.
    // 
    // Uses readNextNumber(), so it naturally accepts comma/space separators.
    static INLINE bool parseNumberPair(ByteSpan s, SVGNumberPair& out) noexcept
    {
        s.skipSpaces();

        if (!s) { out.set(0.0f); return false; }

        double x = 0.0;
        if (!readNextNumber(s, x)) { out.set(0.0f); return false; }

        double y = 0.0;
        if (readNextNumber(s, y)) {
            out.a = (float)x;
            out.b = (float)y;
            out.hasB = true;
        }
        else {
            out.set((float)x);
        }

        // SVG behavior: negative treated as 0
        if (out.a < 0.0f) out.a = 0.0f;
        if (out.b < 0.0f) out.b = 0.0f;

        return true;
    }
}

// Helpers to make parsing various filter attributes easier
//
namespace waavs
{
    static INLINE bool parseF32Attr(const ByteSpan& s, float& out) noexcept
    {
        double v = 0.0;
        if (!parseNumber(s, v))
            return false;
        out = (float)v;
        return true;
    }

    static INLINE bool parseU32Attr(const ByteSpan& s, uint32_t& out) noexcept
    {
        double v = 0.0;
        if (!parseNumber(s, v))
            return false;

        if (v < 0.0)
            v = 0.0;

        out = (uint32_t)v;
        return true;
    }

    static INLINE bool parseFloatPairAttr(const ByteSpan& s, float& a, float& b) noexcept
    {
        SVGTokenListView tv(s);
        double x = 0.0;
        double y = 0.0;

        if (!tv.readANumber(x))
            return false;

        if (!tv.readANumber(y))
            y = x;

        a = (float)x;
        b = (float)y;
        return true;
    }

    static INLINE bool parseU32PairAttr(const ByteSpan& s, uint32_t& a, uint32_t& b) noexcept
    {
        SVGTokenListView tv(s);
        double x = 0.0;
        double y = 0.0;

        if (!tv.readANumber(x))
            return false;

        if (!tv.readANumber(y))
            y = x;

        if (x < 1.0) x = 1.0;
        if (y < 1.0) y = 1.0;

        a = (uint32_t)x;
        b = (uint32_t)y;
        return true;
    }

    static INLINE bool parseFloatListAttr(const ByteSpan& s, std::vector<float>& out) noexcept
    {
        out.clear();

        SVGTokenListView tv(s);
        double v = 0.0;
        while (tv.readANumber(v))
            out.push_back((float)v);

        return !out.empty();
    }

    static INLINE bool parseFloatListExact(const ByteSpan& s, float* dst, size_t n) noexcept
    {
        SVGTokenListView tv(s);
        double v = 0.0;

        for (size_t i = 0; i < n; ++i)
        {
            if (!tv.readANumber(v))
                return false;
            dst[i] = (float)v;
        }

        return true;
    }

    static INLINE bool parseBoolAttr(const ByteSpan& s, bool& out) noexcept
    {
        if (!s)
            return false;

        static InternedKey kTrue = PSNameTable::INTERN("true");
        static InternedKey kFalse = PSNameTable::INTERN("false");
        InternedKey k = PSNameTable::INTERN(s);

        if (k == kTrue) {
            out = true;
            return true;
        }

        if (k == kFalse) {
            out = false;
            return true;
        }

        uint32_t v = 0;
        if (!parseU32Attr(s, v))
            return false;

        out = (v != 0);
        return true;
    }

    static INLINE bool parseStitchTilesAttr(const ByteSpan& s, bool& out) noexcept
    {
        if (!s)
            return false;

        static InternedKey kStitch = PSNameTable::INTERN("stitch");
        static InternedKey kNoStitch = PSNameTable::INTERN("noStitch");
        InternedKey k = PSNameTable::INTERN(s);

        if (k == kStitch) {
            out = true;
            return true;
        }

        if (k == kNoStitch) {
            out = false;
            return true;
        }

        return false;
    }

    /*
    static INLINE bool colorMatrixTypeHasSingleValue(FilterColorMatrixType t) noexcept
    {
        return t == FILTER_COLOR_MATRIX_SATURATE || t == FILTER_COLOR_MATRIX_HUE_ROTATE;
    }



    static INLINE bool transferFuncTypeUsesTable(FilterTransferFuncType t) noexcept
    {
        return t == FILTER_TRANSFER_TABLE || t == FILTER_TRANSFER_DISCRETE;
    }
    */
}

namespace waavs
{
    //
    // feDistantLight
    // This is a sub-component of other lighting components
    // so don't emit a program for it.
    //
    struct SVGFeDistantLightElement : public SVGGraphicsElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName(filter::feDistantLight(), [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeDistantLightElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName(filter::feDistantLight(), [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeDistantLightElement>();
                node->loadFromXmlPull(iter, groot);

                return node;
                });

            registerSingularNode();
        }


        SVGFeDistantLightElement()
            : SVGGraphicsElement()
        {
        }


    };

    // feSpotLight
    // This is a sub-component of other lighting components
    // so don't emit a program for it.
    //
    struct SVGFePointLightElement : public SVGGraphicsElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName(filter::fePointLight(), [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFePointLightElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName(filter::fePointLight(), [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFePointLightElement>();
                node->loadFromXmlPull(iter, groot);

                return node;
                });

            registerSingularNode();
        }

        SVGFePointLightElement()
            : SVGGraphicsElement()
        {
        }

    };


    // feSpotLight
    // This is a sub-component of other lighting components
    // so don't emit a program for it.
    //
    struct SVGFeSpotLightElement : public SVGGraphicsElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feSpotLight", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeSpotLightElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feSpotLight", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeSpotLightElement>();
                node->loadFromXmlPull(iter, groot);

                return node;
                });

            registerSingularNode();
        }

        SVGFeSpotLightElement()
            : SVGGraphicsElement()
        {
        }

    };
}

// All the filter primitives
namespace waavs
{
    // Base class for filter primitives.
    struct SVGFilterPrimitiveElement : public SVGGraphicsElement
    {
        FilterOpId fOperator{ FOP_END };

        // Common attributes
        FilterColorInterpolation fColorInterpolation{ FilterColorInterpolation::FILTER_COLOR_INTERPOLATION_AUTO };
        InternedKey fIn{nullptr};
        InternedKey fIn2{nullptr};
        InternedKey fResult{nullptr};  // optional

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
        void emitCommonIO(FilterProgramStream& out,const InternedKey in1,const InternedKey in2, const uint8_t flags) const noexcept
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
            if (getAttribute(filter::in(), fInAttr)) { fIn = PSNameTable::INTERN(fInAttr);}
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

    //
    // feBlend
    //
    struct SVGFeBlendElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feBlend", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeBlendElement>(groot);
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feBlend", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeBlendElement>(groot);
                node->loadFromXmlPull(iter, groot);
                
                return node;
                });

            registerSingularNode();
        }

        //-----------------------------------
        FilterBlendMode fMode{ FilterBlendMode::FILTER_BLEND_NORMAL };

        SVGFeBlendElement(IAmGroot* )
            : SVGFilterPrimitiveElement(FOP_BLEND)
        {

        }


        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {
            emit_u32(out, (uint32_t)fMode);

            return true;
        }

        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            (void)groot;
            ByteSpan modeAttr{};
            if (getAttribute(filter::mode(), modeAttr)) 
                fMode = parseFilterBlendMode(PSNameTable::INTERN(modeAttr));
            else
                fMode = FilterBlendMode::FILTER_BLEND_NORMAL;
        }
    };
    

    // ----------------------------------------------
    // feComponentTransfer
    // ----------------------------------------------

    struct SVGComponentTransferFunc
    {
        FilterTransferFuncType fType{ FILTER_TRANSFER_IDENTITY };
        float fP0{ 1.0f };
        float fP1{ 0.0f };
        float fP2{ 0.0f };
        std::vector<float> fTable{};
    };



    // feFuncR, feFuncG, feFuncB, feFuncA are all the same except for the channel they represent.  
    // We can use a single class for all of them, and just check the name to see which channel it is.
    struct SVGFeFuncElement : public SVGGraphicsElement
    {


    public:
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feFuncA", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeFuncElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
            registerSVGSingularNodeByName("feFuncR", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeFuncElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
            registerSVGSingularNodeByName("feFuncG", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeFuncElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
            registerSVGSingularNodeByName("feFuncB", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeFuncElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        
        SVGComponentTransferFunc fFunc{};


        // -----------------------------------------------
        // Implementation
        // ------------------------------------------------

        SVGFeFuncElement()
            : SVGGraphicsElement()
        {
        }

        const SVGComponentTransferFunc& func() const noexcept
        {
            return fFunc;
        }

        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            fFunc = {};

            ByteSpan typeAttr{};
            if (getAttribute(filter::type_(), typeAttr))
                fFunc.fType = parseFilterTransferFuncType(PSNameTable::INTERN(typeAttr));
            else
                fFunc.fType = FILTER_TRANSFER_IDENTITY;

            switch (fFunc.fType)
            {
            case FILTER_TRANSFER_LINEAR:
                fFunc.fP0 = 1.0f;
                fFunc.fP1 = 0.0f;
                parseF32Attr(getAttribute(filter::slope()), fFunc.fP0);
                parseF32Attr(getAttribute(filter::intercept()), fFunc.fP1);
                break;

            case FILTER_TRANSFER_GAMMA:
                fFunc.fP0 = 1.0f;
                fFunc.fP1 = 1.0f;
                fFunc.fP2 = 0.0f;
                parseF32Attr(getAttribute(filter::amplitude()), fFunc.fP0);
                parseF32Attr(getAttribute(filter::exponent()), fFunc.fP1);
                parseF32Attr(getAttribute(filter::offset()), fFunc.fP2);
                break;

            case FILTER_TRANSFER_TABLE:
            case FILTER_TRANSFER_DISCRETE:
                fFunc.fTable.clear();
                parseFloatListAttr(getAttribute(filter::tableValues()), fFunc.fTable);
                break;

            case FILTER_TRANSFER_IDENTITY:
            default:
                break;
            }

        }


    };


    struct SVGFeComponentTransferElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feComponentTransfer", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeComponentTransferElement>(groot);
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feComponentTransfer", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeComponentTransferElement>(groot);
                node->loadFromXmlPull(iter, groot);
                return node;
                });

            registerSingularNode();
        }

        // ----------------------------------------------
        SVGComponentTransferFunc fR{};
        SVGComponentTransferFunc fG{};
        SVGComponentTransferFunc fB{};
        SVGComponentTransferFunc fA{};


        SVGFeComponentTransferElement(IAmGroot* )
            : SVGFilterPrimitiveElement(FOP_COMPONENT_TRANSFER)
        {
        }

        static INLINE void emitTransferFunc(FilterProgramStream& out, const SVGComponentTransferFunc& f) noexcept
        {
            emit_u32(out, (uint32_t)f.fType);
            emit_f32(out, f.fP0);
            emit_f32(out, f.fP1);
            emit_f32(out, f.fP2);
            emit_counted_f32_list(out, f.fTable.empty() ? nullptr : f.fTable.data(), (uint32_t)f.fTable.size());
        }

        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {
            emitTransferFunc(out, fR);
            emitTransferFunc(out, fG);
            emitTransferFunc(out, fB);
            emitTransferFunc(out, fA);

            return true;
        }


        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            (void)groot;

            fR = {};
            fG = {};
            fB = {};
            fA = {};

            for (auto& n : fNodes)
            {
                if (!n)
                    continue;

                auto g = std::dynamic_pointer_cast<SVGFeFuncElement>(n);
                if (!g)
                    continue;

                // Make sure the sub-node is fixed up, 
                // so that its fFunc is valid when we read it.
                g->fixupStyleAttributes(groot);

                // Get the details of the transferfunc
                const auto & func = g->func();

                if (g->nameAtom() == filter::feFuncA())
                    fA = func;
                else if (g->nameAtom() == filter::feFuncR())
                    fR = func;
                else if (g->nameAtom() == filter::feFuncG())
                    fG = func;
                else if (g->nameAtom() == filter::feFuncB())
                    fB = func;
            }
        }

    };
    
    
    //
    // feComposite
    //
    struct SVGFeCompositeElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feComposite", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeCompositeElement>(groot);
                node->loadFromXmlElement(elem, groot);
                
                return node;
            });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feComposite", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeCompositeElement>(groot);
                node->loadFromXmlPull(iter, groot);
                
                return node;
            });
            
            registerSingularNode();
        }

        // -------------------------------------------------
        FilterCompositeOp fCompOp{ FILTER_COMPOSITE_OVER };
        float fK1{ 0.0f };
        float fK2{ 0.0f };
        float fK3{ 0.0f };
        float fK4{ 0.0f };


        SVGFeCompositeElement(IAmGroot* )
            : SVGFilterPrimitiveElement(FOP_COMPOSITE)
        {
            setIsVisible(false);
        }

        static INLINE bool compositeOpNeedsArithmeticCoeffs(FilterCompositeOp op) noexcept
        {
            return op == FILTER_COMPOSITE_ARITHMETIC;
        }

        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {
            (void)last;

            emit_u32(out, (uint32_t)fCompOp);

            if (compositeOpNeedsArithmeticCoeffs(fCompOp)) {
                emit_f32(out, fK1);
                emit_f32(out, fK2);
                emit_f32(out, fK3);
                emit_f32(out, fK4);
            }

            return true;
        }

        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            (void)groot;

            ByteSpan opAttr{};
            if (getAttribute(filter::operator_(), opAttr))
                fCompOp = parseFilterCompositeOp(PSNameTable::INTERN(opAttr));
            else
                fCompOp = FILTER_COMPOSITE_OVER;

            double v = 0.0;
            if (parseNumber(getAttribute(filter::k1()), v)) fK1 = (float)v;
            if (parseNumber(getAttribute(filter::k2()), v)) fK2 = (float)v;
            if (parseNumber(getAttribute(filter::k3()), v)) fK3 = (float)v;
            if (parseNumber(getAttribute(filter::k4()), v)) fK4 = (float)v;
        }
    };

    //
    // feColorMatrix
    //
    struct SVGFeColorMatrixElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feColorMatrix", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeColorMatrixElement>();
                node->loadFromXmlElement(elem, groot);
                
                return node;
            });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feColorMatrix", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeColorMatrixElement>();
                node->loadFromXmlPull(iter, groot);
                
                return node;
            });

            registerSingularNode();
        }

        // -------------------------------------------------
        
        FilterColorMatrixType fType{ FILTER_COLOR_MATRIX_MATRIX };
        float fValue{ 1.0f };
        std::array<float, 20> fMatrix{
            1,0,0,0,0,
            0,1,0,0,0,
            0,0,1,0,0,
            0,0,0,1,0
        };

        SVGFeColorMatrixElement()
            : SVGFilterPrimitiveElement(FOP_COLOR_MATRIX)
        {
        }

        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {
            emit_u32(out, (uint32_t)fType);

            switch (fType)
            {
            case FILTER_COLOR_MATRIX_MATRIX:
                emit_f32_list(out, fMatrix.data(), 20);
                break;

            case FILTER_COLOR_MATRIX_SATURATE:
            case FILTER_COLOR_MATRIX_HUE_ROTATE:
                emit_f32(out, fValue);
                break;

            case FILTER_COLOR_MATRIX_LUMINANCE_TO_ALPHA:
            default:
                break;
            }

            return true;
        }

        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            (void)groot;

            ByteSpan typeAttr{};
            if (getAttribute(filter::type_(), typeAttr))
                fType = parseFilterColorMatrixType(PSNameTable::INTERN(typeAttr));
            else
                fType = FILTER_COLOR_MATRIX_MATRIX;

            ByteSpan valuesAttr{};
            if (!getAttribute(filter::values(), valuesAttr))
                return;

            switch (fType)
            {
            case FILTER_COLOR_MATRIX_MATRIX:
            {
                float tmp[20]{};
                if (parseFloatListExact(valuesAttr, tmp, 20))
                {
                    for (size_t i = 0; i < 20; ++i)
                        fMatrix[i] = tmp[i];
                }
            } break;

            case FILTER_COLOR_MATRIX_SATURATE:
            case FILTER_COLOR_MATRIX_HUE_ROTATE:
                parseF32Attr(valuesAttr, fValue);
                break;

            case FILTER_COLOR_MATRIX_LUMINANCE_TO_ALPHA:
            default:
                break;
            }
        }
    };

    //
    // feConvolveMatrix
    //
    struct SVGFeConvolveMatrixElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feConvolveMatrix", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeConvolveMatrixElement>(groot);
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feConvolveMatrix", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeConvolveMatrixElement>(groot);
                node->loadFromXmlPull(iter, groot);
                return node;
                });

            registerSingularNode();
        }

        // -------------------------------------------------
        uint32_t fOrderX{ 3 };
        uint32_t fOrderY{ 3 };
        std::vector<float> fKernel{};
        float fDivisor{ 1.0f };
        float fBias{ 0.0f };
        uint32_t fTargetX{ 1 };
        uint32_t fTargetY{ 1 };
        FilterEdgeMode fEdgeMode{ FILTER_EDGE_DUPLICATE };
        float fKernelUnitLengthX{ 0.0f };
        float fKernelUnitLengthY{ 0.0f };
        bool fPreserveAlpha{ false };


        SVGFeConvolveMatrixElement(IAmGroot* )
            : SVGFilterPrimitiveElement(FOP_CONVOLVE_MATRIX)
        {
            setIsVisible(false);
        }

        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {
            (void)last;

            emit_u32x2(out, fOrderX, fOrderY);

            const uint32_t kernelCount = fOrderX * fOrderY;

            if (!fKernel.empty())
                emit_f32_list(out, fKernel.data(), kernelCount);

            emit_f32(out, fDivisor);
            emit_f32(out, fBias);
            emit_u32x2(out, fTargetX, fTargetY);
            emit_u32(out, (uint32_t)fEdgeMode);
            emit_f32(out, fKernelUnitLengthX);
            emit_f32(out, fKernelUnitLengthY);
            emit_u32(out, fPreserveAlpha ? 1u : 0u);

            return true;
        }


        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            (void)groot;

            ByteSpan orderAttr{};
            if (getAttribute(filter::order(), orderAttr))
                parseU32PairAttr(orderAttr, fOrderX, fOrderY);
            else
                fOrderX = fOrderY = 3;

            parseFloatListAttr(getAttribute(filter::kernelMatrix()), fKernel);

            // Ensure kernel size matches orderX * orderY.  
            // If too small, pad with 0s.  
            // If too big, truncate.
            const uint32_t kernelCount = fOrderX * fOrderY;
            if (fKernel.size() < kernelCount)
                fKernel.resize(kernelCount, 0.0);
            else if (fKernel.size() > kernelCount)
                fKernel.resize(kernelCount);

            ByteSpan divisorAttr{};
            if (getAttribute(filter::divisor(), divisorAttr))
            {
                parseF32Attr(divisorAttr, fDivisor);
            }
            else
            {
                if (!fKernel.empty())
                {
                    double sum = 0.0;
                    for (float v : fKernel)
                        sum += v;

                    fDivisor = (sum == 0.0) ? 1.0f : (float)sum;
                }
                else
                {
                    fDivisor = 1.0f;
                }
            }

            parseF32Attr(getAttribute(filter::bias()), fBias);

            ByteSpan targetXAttr{};
            ByteSpan targetYAttr{};
            if (getAttribute(filter::targetX(), targetXAttr))
                parseU32Attr(targetXAttr, fTargetX);
            else
                fTargetX = fOrderX / 2;

            if (getAttribute(filter::targetY(), targetYAttr))
                parseU32Attr(targetYAttr, fTargetY);
            else
                fTargetY = fOrderY / 2;

            ByteSpan edgeAttr{};
            if (getAttribute(filter::edgeMode(), edgeAttr))
                fEdgeMode = parseFilterEdgeMode(PSNameTable::INTERN(edgeAttr));
            else
                fEdgeMode = FILTER_EDGE_DUPLICATE;

            ByteSpan kulAttr{};
            if (getAttribute(filter::kernelUnitLength(), kulAttr))
            {
                parseFloatPairAttr(kulAttr, fKernelUnitLengthX, fKernelUnitLengthY);
            }

            ByteSpan paAttr{};
            if (getAttribute(filter::preserveAlpha(), paAttr))
                parseBoolAttr(paAttr, fPreserveAlpha);
            else
                fPreserveAlpha = false;
        }
    };


    //
    // feDiffuseLighting
    //
    struct SVGFeDiffuseLightingElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feDiffuseLighting", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeDiffuseLightingElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feDiffuseLighting", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeDiffuseLightingElement>();
                node->loadFromXmlPull(iter, groot);
                return node;
                });

            registerSingularNode();
        }

        // -------------------------------------------------
        SVGColor fLightingColor{ filter::lighting_color(), nullptr };
        float fSurfaceScale{ 1.0f };
        float fDiffuseConstant{ 1.0f };
        float fKernelUnitLengthX{ 0.0f };
        float fKernelUnitLengthY{ 0.0f };
        FilterLightType fLightType{ FILTER_LIGHT_DISTANT };
        float fLight[8]{};


        SVGFeDiffuseLightingElement( )
            : SVGFilterPrimitiveElement(FOP_DIFFUSE_LIGHTING)
        {
        }

        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {
            emit_ColorSRGB(out, fLightingColor.value());
            emit_f32(out, fSurfaceScale);
            emit_f32(out, fDiffuseConstant);
            emit_f32(out, fKernelUnitLengthX);
            emit_f32(out, fKernelUnitLengthY);
            emit_u32(out, (uint32_t)fLightType);
            emit_f32_list(out, fLight, 8);

            return true;
        }

        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            parseF32Attr(getAttribute(filter::surfaceScale()), fSurfaceScale);
            parseF32Attr(getAttribute(filter::diffuseConstant()), fDiffuseConstant);

            ByteSpan kulAttr{};
            if (getAttribute(filter::kernelUnitLength(), kulAttr))
                parseFloatPairAttr(kulAttr, fKernelUnitLengthX, fKernelUnitLengthY);

            // get the lighting color
            if (!fLightingColor.loadFromAttributes(fAttributes))
            {
                fLightingColor.setValue(1.0f, 1.0f, 1.0f, 1.0f);
            }

            fLightType = FILTER_LIGHT_DISTANT;
            for (int i = 0; i < 8; ++i)
                fLight[i] = 0.0f;

            for (auto& n : fNodes)
            {
                if (!n)
                    continue;

                auto g = std::dynamic_pointer_cast<SVGGraphicsElement>(n);
                if (!g)
                    continue;

                auto nm = g->nameAtom();

                n->fixupStyleAttributes(groot);

                // If feDistantLight element
                if (nm == filter::feDistantLight())
                {
                    fLightType = FILTER_LIGHT_DISTANT;
                    parseF32Attr(g->getAttribute(filter::azimuth()), fLight[0]);
                    parseF32Attr(g->getAttribute(filter::elevation()), fLight[1]);
                    break;
                }
                else if (nm == filter::fePointLight())
                {
                    fLightType = FILTER_LIGHT_POINT;
                    parseF32Attr(g->getAttribute(filter::x()), fLight[0]);
                    parseF32Attr(g->getAttribute(filter::y()), fLight[1]);
                    parseF32Attr(g->getAttribute(filter::z()), fLight[2]);
                    break;
                }
                else if (nm == filter::feSpotLight())
                {
                    fLightType = FILTER_LIGHT_SPOT;
                    fLight[6] = 1.0f; // default specular exponent
                    parseF32Attr(g->getAttribute(filter::x()), fLight[0]);
                    parseF32Attr(g->getAttribute(filter::y()), fLight[1]);
                    parseF32Attr(g->getAttribute(filter::z()), fLight[2]);
                    parseF32Attr(g->getAttribute(filter::pointsAtX()), fLight[3]);
                    parseF32Attr(g->getAttribute(filter::pointsAtY()), fLight[4]);
                    parseF32Attr(g->getAttribute(filter::pointsAtZ()), fLight[5]);
                    parseF32Attr(g->getAttribute(filter::specularExponent()), fLight[6]);
                    parseF32Attr(g->getAttribute(filter::limitingConeAngle()), fLight[7]);
                    break;
                }
            }
        }
    };
    
    
    //
    // feDisplacementMap
    //
    struct SVGFeDisplacementMapElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feDisplacementMap", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeDisplacementMapElement>(groot);
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feDisplacementMap", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeDisplacementMapElement>(groot);
                node->loadFromXmlPull(iter, groot);
                return node;
                });

            registerSingularNode();
        }

        // -------------------------------------------------
        float fScale{ 0.0f };
        FilterChannelSelector fXChannel{ FILTER_CHANNEL_A };
        FilterChannelSelector fYChannel{ FILTER_CHANNEL_A };


        SVGFeDisplacementMapElement(IAmGroot* )
            : SVGFilterPrimitiveElement(FOP_DISPLACEMENT_MAP)
        {
            setIsVisible(false);
        }

        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {

            emit_f32(out, fScale);
            emit_u32(out, (uint32_t)fXChannel);
            emit_u32(out, (uint32_t)fYChannel);

            //last = finishLast(fResult);
            return true;
        }

        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            (void)groot;

            parseF32Attr(getAttribute(filter::scale()), fScale);

            ByteSpan xSelAttr{};
            ByteSpan ySelAttr{};

            if (getAttribute(filter::xChannelSelector(), xSelAttr))
                fXChannel = parseFilterChannelSelector(PSNameTable::INTERN(xSelAttr));
            else
                fXChannel = FILTER_CHANNEL_A;

            if (getAttribute(filter::yChannelSelector(), ySelAttr))
                fYChannel = parseFilterChannelSelector(PSNameTable::INTERN(ySelAttr));
            else
                fYChannel = FILTER_CHANNEL_A;
        }
    };

    // ----------------------------------------------
    // Light related elements.
    // ----------------------------------------------

    



    //
    // feDropShadow
    //
    struct SVGFeDropShadowElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feDropShadow", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeDropShadowElement>(groot);
                node->loadFromXmlElement(elem, groot);
                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feDropShadow", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeDropShadowElement>(groot);
                node->loadFromXmlPull(iter, groot);
                return node;
                });

            registerSingularNode();
        }

        float fDx{ 2.0f };
        float fDy{ 2.0f };
        SVGNumberPair fStdDeviation{};
        SVGColor fFloodColor{ filter::flood_color(), filter::flood_opacity() };

        SVGFeDropShadowElement(IAmGroot*)
            : SVGFilterPrimitiveElement(FOP_DROP_SHADOW)
        {
            // SVG default stdDeviation is 2 in common authoring practice for drop shadow?
            // Spec default is 0 if omitted. Keep the builder literal to spec behavior.
            fStdDeviation.set(0.0f);
        }

        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {
            (void)last;

            emit_f32(out, fDx);
            emit_f32(out, fDy);
            emit_f32(out, fStdDeviation.a);
            emit_f32(out, fStdDeviation.hasB ? fStdDeviation.b : fStdDeviation.a);
            //emit_u32(out, fFloodRGBA32Premul);
            emit_ColorSRGB(out, fFloodColor.value());

            return true;
        }

        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            // dx / dy
            {
                ByteSpan dxAttr{};
                if (getAttribute(svgattr::dx(), dxAttr))
                {
                    double v = 0.0;
                    if (parseNumber(dxAttr, v))
                        fDx = (float)v;
                    else
                        fDx = 2.0f;
                }
                else
                {
                    fDx = 2.0f;
                }
            }

            {
                ByteSpan dyAttr{};
                if (getAttribute(svgattr::dy(), dyAttr))
                {
                    double v = 0.0;
                    if (parseNumber(dyAttr, v))
                        fDy = (float)v;
                    else
                        fDy = 2.0f;
                }
                else
                {
                    fDy = 2.0f;
                }
            }

            // stdDeviation
            {
                ByteSpan sdAttr{};
                if (getAttribute(filter::stdDeviation(), sdAttr))
                    parseNumberPair(sdAttr, fStdDeviation);
                else
                    fStdDeviation.set(0.0f);
            }

            // flood-color + flood-opacity
            // Same approach as feFlood.
            if (!fFloodColor.loadFromAttributes(fAttributes) || !fFloodColor.isColor())
            {
                // default to black on failure
                fFloodColor.setValue(0.0f, 0.0f, 0.0f, 1.0f);
            }

        }
    };

    // -----------------------------------------------
    // feFlood
    // ------------------------------------------------
    struct SVGFeFloodElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feFlood", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeFloodElement>(groot);
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feFlood", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeFloodElement>(groot);
                node->loadFromXmlPull(iter, groot);
                
                return node;
                });

            registerSingularNode();
        }

        // -------------------------------------------------
        SVGColor fFloodColor{ filter::flood_color(), filter::flood_opacity() };
        //uint32_t fFloodRGBA32Premul{ 0xFF000000u };

        SVGFeFloodElement(IAmGroot* )
            : SVGFilterPrimitiveElement(FOP_FLOOD)
        {
        }

        InternedKey resolveIn1(InternedKey last) const noexcept override
        {
            (void)last;

            // feFlood doesn't have an input, so ignore 'last' and always return null.
            return nullptr;
        }

        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {
            emit_ColorSRGB(out, fFloodColor.value());
            //emit_u32(out, fFloodRGBA32Premul);

            return true;
        }

        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            if (fFloodColor.loadFromAttributes(fAttributes))
            {
                // nothing to do, we were successful in loading the color
            }
        }
    };

    
    //
    // feGaussianBlur
    //


    struct SVGFeGaussianBlurElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feGaussianBlur", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeGaussianBlurElement>(groot);
                node->loadFromXmlElement(elem, groot);
                
                return node;
            });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feGaussianBlur", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeGaussianBlurElement>(groot);
                node->loadFromXmlPull(iter, groot);
                
                return node;
            });
            
            registerSingularNode();
        }
        
        // -------------------------------------------------

        SVGNumberPair fStdDeviation;
        
        SVGFeGaussianBlurElement(IAmGroot* )
            : SVGFilterPrimitiveElement(FOP_GAUSSIAN_BLUR)
        {
            setIsVisible(false);
        }

        virtual bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept
        {
            //const uint8_t flags = flagsForIO(*this, false);
            //out.ops.push_back(packOp(FOP_GAUSSIAN_BLUR, flags));

            //const InternedKey in1 = resolveIn1(*this, last);
            //emitCommonIO(out, *this, in1, {}, flags);

            emit_f32(out, fStdDeviation.a); // stdDeviationX
            emit_f32(out, fStdDeviation.hasB ? fStdDeviation.b : fStdDeviation.a); // stdDeviationY (if not specified, same as X)

            //last = finishLast(fResult);
            return true;
        }


        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            parseNumberPair(getAttribute(filter::stdDeviation()), fStdDeviation);
        }


    };

    //
    // feImage
    //
    struct SVGFeImageElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feImage", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeImageElement>(groot);
                node->loadFromXmlElement(elem, groot);
                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feImage", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeImageElement>(groot);
                node->loadFromXmlPull(iter, groot);
                return node;
                });

            registerSingularNode();
        }

        InternedKey fImageKey{ nullptr };
        AspectRatioAlignKind fAlign{ AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMID };
        AspectRatioMeetOrSliceKind fMeetOrSlice{ AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_MEET };


        SVGFeImageElement(IAmGroot*)
            : SVGFilterPrimitiveElement(FOP_IMAGE)
        {
            setIsVisible(false);
        }

        virtual InternedKey resolveIn1(InternedKey last) const noexcept override
        {
            (void)last;
            return nullptr;
        }

        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {
            (void)last;
            emit_key(out, fImageKey);
            emit_u32(out, (uint32_t)fAlign);
            emit_u32(out, (uint32_t)fMeetOrSlice);

            return true;
        }

        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            (void)groot;

            ByteSpan hrefAttr{};
            hrefAttr = getAttribute(filter::href());
            if (!hrefAttr)
                hrefAttr = getAttribute(filter::xlink_href());

            if (hrefAttr)
                fImageKey = PSNameTable::INTERN(hrefAttr);
            else
                fImageKey = nullptr;

            ByteSpan parAttr = getAttribute(svgattr::preserveAspectRatio());
            if (parAttr)
                parsePreserveAspectRatio(parAttr, fAlign, fMeetOrSlice);
        }
    };


    //
    // feMergeNode
    // Child-only payload for feMerge
    //
    struct SVGFeMergeNodeElement : public SVGGraphicsElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feMergeNode", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeMergeNodeElement>();
                node->loadFromXmlElement(elem, groot);
                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feMergeNode", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeMergeNodeElement>();
                node->loadFromXmlPull(iter, groot);
                return node;
                });

            registerSingularNode();
        }

        SVGFeMergeNodeElement()
            : SVGGraphicsElement()
        {
        }



    };

    //
    // feMerge
    //
    struct SVGFeMergeElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feMerge", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeMergeElement>(groot);
                node->loadFromXmlElement(elem, groot);
                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feMerge", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeMergeElement>(groot);
                node->loadFromXmlPull(iter, groot);
                return node;
                });

            registerSingularNode();
        }

        std::vector<InternedKey> fInputs{};

        SVGFeMergeElement(IAmGroot*)
            : SVGFilterPrimitiveElement(FOP_MERGE)
        {
        }

        // Generator-like wrt common "in1": ABI says it is unused.
        InternedKey resolveIn1(InternedKey last) const noexcept
        {
            (void)last;
            return nullptr;
        }

        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {
            (void)last;
            emit_counted_key_list(out, fInputs.empty() ? nullptr : fInputs.data(), (uint32_t)fInputs.size());
            return true;
        }

        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            (void)groot;

            fInputs.clear();

            for (auto& n : fNodes)
            {
                if (!n)
                    continue;

                // Only accept feMergeNode children.  Ignore anything else, 
                // even if it's a graphics element that has an "in" attribute.
                auto mn = std::dynamic_pointer_cast<SVGFeMergeNodeElement>(n);
                if (!mn)
                    continue;
                
                // Make sure the fixup occurs so we can get the "in" attribute if specified, 
                // and also so that any feMergeNode children are processed before we query their "in".
                mn->fixupStyleAttributes(groot);

                InternedKey k{};
                ByteSpan inAttr{};

                // BUGBUG: feMergeNode "in" default is previous result if omitted.
                // But inside feMerge, that would be surprising and not especially useful.
                // Usually authors provide it explicitly. Still, use the same fallback rule.

                // If "in" is not specified, SVG spec says the default is the result of the previous filter primitive.
                if (!mn->getAttribute(filter::in(), inAttr))
                {
                    k = PSNameTable::INTERN("__last__");
                }else
                    k = PSNameTable::INTERN(inAttr);


                fInputs.push_back(k);
            }
        }
    };


    //
    // feMorphology
    //
    struct SVGFeMorphologyElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feMorphology", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeMorphologyElement>(groot);
                node->loadFromXmlElement(elem, groot);
                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feMorphology", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeMorphologyElement>(groot);
                node->loadFromXmlPull(iter, groot);
                return node;
                });

            registerSingularNode();
        }

        FilterMorphologyOp fMorphOp{ FILTER_MORPHOLOGY_ERODE };
        float fRadiusX{ 0.0f };
        float fRadiusY{ 0.0f };

        SVGFeMorphologyElement(IAmGroot*)
            : SVGFilterPrimitiveElement(FOP_MORPHOLOGY)
        {
            setIsVisible(false);
        }

        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {
            (void)last;

            emit_u32(out, (uint32_t)fMorphOp);
            emit_f32(out, fRadiusX);
            emit_f32(out, fRadiusY);

            return true;
        }

        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            (void)groot;

            ByteSpan opAttr{};
            if (getAttribute(filter::operator_(), opAttr))
                fMorphOp = parseFilterMorphologyOp(PSNameTable::INTERN(opAttr));
            else
                fMorphOp = FILTER_MORPHOLOGY_ERODE;

            ByteSpan radiusAttr{};
            if (getAttribute(filter::radius(), radiusAttr))
            {
                parseFloatPairAttr(radiusAttr, fRadiusX, fRadiusY);

                if (fRadiusX < 0.0f) fRadiusX = 0.0f;
                if (fRadiusY < 0.0f) fRadiusY = 0.0f;
            }
            else
            {
                fRadiusX = 0.0f;
                fRadiusY = 0.0f;
            }
        }
    };


    //
    // feOffset
    //
    struct SVGFeOffsetElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feOffset", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeOffsetElement>(groot);
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feOffset", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeOffsetElement>(groot);
                node->loadFromXmlPull(iter, groot);
                
                return node;
                });

            registerSingularNode();
        }


        // feOffset attributes
        float fDx{ 0.0f };
        float fDy{ 0.0f };


        SVGFeOffsetElement(IAmGroot* )
            : SVGFilterPrimitiveElement(FOP_OFFSET)
        {
        }

        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {
            emit_f32(out, fDx);
            emit_f32(out, fDy);

            return true;
        }

        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            ByteSpan dxAttr{}, dyAttr{};
            if (getAttribute(svgattr::dx(), dxAttr)) {
                double dx = 0.0;
                if (parseNumber(dxAttr, dx))
                    fDx = dx;
            }

            if (getAttribute(svgattr::dy(), dyAttr)) {
                double dy = 0.0;
                if (parseNumber(dyAttr, dy))
                    fDy = dy;
            }
        }

    };
    
    //
    // feSpecularLighting
    //
    struct SVGFeSpecularLightingElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feSpecularLighting", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeSpecularLightingElement>();
                node->loadFromXmlElement(elem, groot);
                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feSpecularLighting", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeSpecularLightingElement>();
                node->loadFromXmlPull(iter, groot);
                return node;
                });

            registerSingularNode();
        }

        SVGColor fLightingColor{ filter::lighting_color(), nullptr };
        float fSurfaceScale{ 1.0f };
        float fSpecularConstant{ 1.0f };
        float fSpecularExponent{ 1.0f };
        float fKernelUnitLengthX{ 0.0f };
        float fKernelUnitLengthY{ 0.0f };
        FilterLightType fLightType{ FILTER_LIGHT_DISTANT };
        float fLight[8]{};

        SVGFeSpecularLightingElement()
            : SVGFilterPrimitiveElement(FOP_SPECULAR_LIGHTING)
        {
            printf("feSpecularLighting\n");
        }

        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {
            (void)last;

            //emit_u32(out, fLightingColorRGBA32);
            emit_ColorSRGB(out, fLightingColor.value());
            emit_f32(out, fSurfaceScale);
            emit_f32(out, fSpecularConstant);
            emit_f32(out, fSpecularExponent);
            emit_f32(out, fKernelUnitLengthX);
            emit_f32(out, fKernelUnitLengthY);
            emit_u32(out, (uint32_t)fLightType);
            emit_f32_list(out, fLight, 8);

            return true;
        }

        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            parseF32Attr(getAttribute(filter::surfaceScale()), fSurfaceScale);
            parseF32Attr(getAttribute(filter::specularConstant()), fSpecularConstant);
            parseF32Attr(getAttribute(filter::specularExponent()), fSpecularExponent);

            ByteSpan kulAttr{};
            if (getAttribute(filter::kernelUnitLength(), kulAttr))
                parseFloatPairAttr(kulAttr, fKernelUnitLengthX, fKernelUnitLengthY);

            if (!fLightingColor.loadFromAttributes(fAttributes))
            {
                fLightingColor.setValue(1.0f, 1.0f, 1.0f, 1.0f);
            }



            fLightType = FILTER_LIGHT_DISTANT;
            for (int i = 0; i < 8; ++i)
                fLight[i] = 0.0f;

            for (auto& n : fNodes)
            {
                
                if (!n)
                    continue;

                auto g = std::dynamic_pointer_cast<SVGGraphicsElement>(n);
                if (!g)
                    continue;

                n->fixupStyleAttributes(groot);

                InternedKey nm = g->nameAtom();

                if (nm == filter::feDistantLight())
                {
                    fLightType = FILTER_LIGHT_DISTANT;

                    parseF32Attr(g->getAttribute(filter::azimuth()), fLight[0]);
                    parseF32Attr(g->getAttribute(filter::elevation()), fLight[1]);
                    break;
                }
                else if (nm == filter::fePointLight())
                {
                    fLightType = FILTER_LIGHT_POINT;
                    parseF32Attr(g->getAttribute(filter::x()), fLight[0]);
                    parseF32Attr(g->getAttribute(filter::y()), fLight[1]);
                    parseF32Attr(g->getAttribute(filter::z()), fLight[2]);
                    break;
                }
                else if (nm == filter::feSpotLight())
                {
                    fLightType = FILTER_LIGHT_SPOT;
                    fLight[6] = 1.0f; // default specular exponent
                    parseF32Attr(g->getAttribute(filter::x()), fLight[0]);
                    parseF32Attr(g->getAttribute(filter::y()), fLight[1]);
                    parseF32Attr(g->getAttribute(filter::z()), fLight[2]);
                    parseF32Attr(g->getAttribute(filter::pointsAtX()), fLight[3]);
                    parseF32Attr(g->getAttribute(filter::pointsAtY()), fLight[4]);
                    parseF32Attr(g->getAttribute(filter::pointsAtZ()), fLight[5]);
                    parseF32Attr(g->getAttribute(filter::specularExponent()), fLight[6]);
                    parseF32Attr(g->getAttribute(filter::limitingConeAngle()), fLight[7]);
                    break;
                }
            }
        }
    };


    //
    // feTile
    //
    struct SVGFeTileElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feTile", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeTileElement>(groot);
                node->loadFromXmlElement(elem, groot);
                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feTile", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeTileElement>(groot);
                node->loadFromXmlPull(iter, groot);
                return node;
                });

            registerSingularNode();
        }

        SVGFeTileElement(IAmGroot*)
            : SVGFilterPrimitiveElement(FOP_TILE)
        {
            setIsVisible(false);
        }

        // feTile is essentially a no-op that just passes through its input, 
        // so emitSelf doesn't need to do anything.
        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {
            (void)out;
            (void)last;
            return true;
        }

        // No additional attributes for feTile, so this is a no-op.
        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            (void)groot;
        }
    };


    //
    // feTurbulence
    //
    struct SVGFeTurbulenceElement : public SVGFilterPrimitiveElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feTurbulence", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeTurbulenceElement>(groot);
                node->loadFromXmlElement(elem, groot);

                return node;
            });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feTurbulence", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeTurbulenceElement>(groot);
                node->loadFromXmlPull(iter, groot);
                
                return node;
            });

            registerSingularNode();
        }

        // --------------------------------------------
        FilterTurbulenceType fType{ FILTER_TURBULENCE_TURBULENCE };
        float fBaseFrequencyX{ 0.0f };
        float fBaseFrequencyY{ 0.0f };
        uint32_t fNumOctaves{ 1 };
        float fSeed{ 0.0f };
        bool fStitchTiles{ false };


        SVGFeTurbulenceElement(IAmGroot* )
            : SVGFilterPrimitiveElement(FOP_TURBULENCE)
        {
        }

        InternedKey resolveIn1(InternedKey last) const noexcept override
        {
            (void)last;

            // feTurbulence doesn't have an input, so ignore 'last' and always return null.
            return nullptr;
        }

        bool emitSelf(FilterProgramStream& out, InternedKey& last) const noexcept override
        {

            emit_u32(out, (uint32_t)fType);
            emit_f32(out, fBaseFrequencyX);
            emit_f32(out, fBaseFrequencyY);
            emit_u32(out, fNumOctaves);
            emit_f32(out, fSeed);
            emit_u32(out, fStitchTiles ? 1u : 0u);

            //last = finishLast(fResult);
            return true;
        }

        void fixupFilterSpecificAttributes(IAmGroot* groot) noexcept override
        {
            (void)groot;

            ByteSpan typeAttr{};
            if (getAttribute(filter::type_(), typeAttr))
                fType = parseFilterTurbulenceType(PSNameTable::INTERN(typeAttr));
            else
                fType = FILTER_TURBULENCE_TURBULENCE;

            ByteSpan bfAttr{};
            if (getAttribute(filter::baseFrequency(), bfAttr))
                parseFloatPairAttr(bfAttr, fBaseFrequencyX, fBaseFrequencyY);

            parseU32Attr(getAttribute(filter::numOctaves()), fNumOctaves);
            parseF32Attr(getAttribute(filter::seed()), fSeed);

            ByteSpan stAttr{};
            if (getAttribute(filter::stitchTiles(), stAttr))
                parseStitchTilesAttr(stAttr, fStitchTiles);
            else
                fStitchTiles = false;
        }

    };
}

namespace waavs {

    //
    // filter
    //
    struct SVGFilterElement : public SVGGraphicsElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName(svgtag::tag_filter(), [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFilterElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName(svgtag::tag_filter(), [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFilterElement>();
                node->loadFromXmlPull(iter, groot);

                return node;
                });

            registerSingularNode();
        }

        // Authored attributes we want to hold onto  
        SVGLengthValue fX{};
        SVGLengthValue fY{};
        SVGLengthValue fWidth{};
        SVGLengthValue fHeight{};

        // filterUnits
        SpaceUnitsKind fFilterUnits{ SpaceUnitsKind::SVG_SPACE_OBJECT };  // default is 'objectBoundingBox'

        // primitiveUnits
        SpaceUnitsKind fPrimitiveUnits{  SpaceUnitsKind::SVG_SPACE_USER}; // default is 'userSpaceOnUse'

        FilterColorInterpolation fColorInterpolation{ FilterColorInterpolation::FILTER_COLOR_INTERPOLATION_LINEAR_RGB };

        // href
        std::shared_ptr<SVGFilterElement> fContentSource{}; // if this filter references another filter via href, this points to the base filter

        // Cached compiled program
        FilterProgramStream fProgram{};
        bool fProgCompiled{ false };


        SVGFilterElement()
            : SVGGraphicsElement()
        {
            setIsVisible(false);
        }


        // Retrieve a filter program stream for this element
        std::shared_ptr<FilterProgramStream> getFilterProgramStream(IAmGroot* groot) noexcept override
        {
            if (!fProgCompiled)
            {
                // Rebuild the program stream from our filter primitives
                buildFilterProgramStream();
                fProgCompiled = true;
            }

            return std::make_shared<FilterProgramStream>(fProgram);
        }

        const WGRectD resolveFilterRegion(IRenderSVG* ctx, IAmGroot* groot, const WGRectD &bbox) noexcept override
        {
            if (!ctx)
                return {};

            // The filter region is resolved against the element being filtered,
            // not against the <filter> element itself.
            if (!(bbox.w > 0.0) || !(bbox.h > 0.0))
                return {};

            // Use current context info as much as is available.
            const double dpi = groot? groot->dpi() : 96.0;
            const BLFont* font = ctx ? &ctx->getFont() : nullptr;

            // SVG defaults on <filter>:
            //   x="-10%" y="-10%" width="120%" height="120%"
            //
            // resolveLengthOr() returns the fallback directly if the value is not set.
            // For objectBoundingBox space, fallback values are fractions of bbox size.
            // For user space, the spec for percentages is more subtle, but using the same
            // bbox-relative fallback is a practical reference implementation for now.

            LengthResolveCtx xCtx = makeLengthCtxUser(bbox.w, bbox.x, dpi, font, fFilterUnits);
            LengthResolveCtx yCtx = makeLengthCtxUser(bbox.h, bbox.y, dpi, font, fFilterUnits);
            LengthResolveCtx wCtx = makeLengthCtxUser(bbox.w, 0.0, dpi, font, fFilterUnits);
            LengthResolveCtx hCtx = makeLengthCtxUser(bbox.h, 0.0, dpi, font, fFilterUnits);

            WGRectD r{};
            r.x = resolveLengthOr(fX, xCtx, bbox.x - 0.10 * bbox.w);
            r.y = resolveLengthOr(fY, yCtx, bbox.y - 0.10 * bbox.h);
            r.w = resolveLengthOr(fWidth, wCtx, 1.20 * bbox.w);
            r.h = resolveLengthOr(fHeight, hCtx, 1.20 * bbox.h);

            if (!(r.w > 0.0) || !(r.h > 0.0))
                return {};

            return r;
        }

        bool hasHref() const { return !href().empty(); }
        ByteSpan href() const {
            ByteSpan svgHref{};
            svgHref = getAttribute(svgattr::href());
            if (!svgHref)
                svgHref = getAttribute(svgattr::xlink_href());    // support legacy xlink:href for compatibility

            return svgHref;
        }

        bool filterHasPrimitiveChildren() const noexcept
        {
            for (auto& n : fNodes) {
                if (!n) continue;
                if (std::dynamic_pointer_cast<SVGFilterPrimitiveElement>(n))
                    return true;
            }
            return false;
        }

        const SVGFilterElement* resolvePrimitiveSource() const noexcept
        {
            if (filterHasPrimitiveChildren()) return this;
            if (fContentSource) return fContentSource.get();
            return this;
        }

        void fixupCommonAttributes(IAmGroot* groot)
        {
            // Invalidate the current program
            fProgram.clear();
            fProgCompiled = false;


            // filter region (optional)
            parseLengthValue(getAttribute(svgattr::x()), fX);
            parseLengthValue(getAttribute(svgattr::y()), fY);
            parseLengthValue(getAttribute(svgattr::width()), fWidth);
            parseLengthValue(getAttribute(svgattr::height()), fHeight);

            ByteSpan fuA{}, puA{};
            if (getAttribute(svgattr::filterUnits(), fuA)) {
                uint32_t v{};
                if (getEnumValue(SVGSpaceUnits, fuA, v))
                    fFilterUnits = (SpaceUnitsKind)v;
            }

            if (getAttribute(svgattr::primitiveUnits(), puA)) {
                uint32_t v{};
                if (getEnumValue(SVGSpaceUnits, puA, v))
                    fPrimitiveUnits = (SpaceUnitsKind)v;
            }

            ByteSpan interpAttr{};
            if (getAttribute(filter::color_interpolation_filters(), interpAttr))
            {
                InternedKey interpKey = PSNameTable::INTERN(interpAttr);
                fColorInterpolation = parseFilterColorInterpolation(interpKey, FilterColorInterpolation::FILTER_COLOR_INTERPOLATION_LINEAR_RGB);
            }
            
        }

        // Inherit properties from a reference node
        // only inherit the ones we don't already set
        virtual void inheritProperties(const SVGFilterElement* elem)
        {
            if (!elem) return;

            // Filter region
            setAttributeIfAbsent(elem, svgattr::x());
            setAttributeIfAbsent(elem, svgattr::y());
            setAttributeIfAbsent(elem, svgattr::width());
            setAttributeIfAbsent(elem, svgattr::height());

            // Coordinate system controls
            setAttributeIfAbsent(elem, svgattr::filterUnits());
            setAttributeIfAbsent(elem, svgattr::primitiveUnits());

            // Optional: commonly used in authoring tools
            // setAttributeIfAbsent(elem, svgattr::filterRes());

            // Optional, but real SVG filters often use these
            // setAttributeIfAbsent(elem, svgattr::color_interpolation_filters());
        }


        // ------------------------------------------------------------
        // Inheritance
        // ------------------------------------------------------------
        void resolveReferenceChain(IAmGroot* groot)
        {
            if (!groot) return;
            if (!hasHref()) return;

            ByteSpan hrefSpan = href();

            const SVGFilterElement* visited[kMaxHrefDepth]{};
            uint32_t visitedCount = 0;

            for (uint32_t depth = 0; depth < kMaxHrefDepth; ++depth)
            {
                if (!hrefSpan) break;

                auto node = groot->findNodeByHref(hrefSpan);
                if (!node) break;

                auto fnode = std::dynamic_pointer_cast<SVGFilterElement>(node);
                if (!fnode) break;

                const SVGFilterElement* ref = fnode.get();

                // cycle detection (including self)
                bool seen = (ref == this);
                for (uint32_t i = 0; i < visitedCount && !seen; ++i)
                    if (visited[i] == ref) seen = true;

                if (seen) {
                    WAAVS_ASSERT(false && "SVGFilterElement href cycle detected");
                    break;
                }

                if (visitedCount < kMaxHrefDepth)
                    visited[visitedCount++] = ref;

                // Ensure referenced subtree has been resolved (important for forward refs)
                fnode->resolveStyleSubtree(groot);

                // Merge from nearest first: direct reference wins.
                inheritProperties(ref);

                // If we have no primitives, inherit primitive source pointer.
                if (!filterHasPrimitiveChildren() && fnode->filterHasPrimitiveChildren() && !fContentSource)
                    fContentSource = fnode;

                // follow chain
                hrefSpan = ref->href();
            }
        }


        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            fContentSource = nullptr;

            // We need to read the href so we can resolve inheritance

            // We already have our attributes (unresolved) sitting on our
            // element.  Since resolving references depends on attributes
            // it's safe to do that first.
            resolveReferenceChain(groot);


            // After all attributes are inherited, we can now convert them
            // to the intermediary values that will later be bound.
            fixupCommonAttributes(groot);

            setNeedsBinding(true);
        }

        void buildFilterProgramStream() noexcept
        {
            fProgram.clear();

            // push filter level units into program
            fProgram.filterUnits = fFilterUnits;
            fProgram.primitiveUnits = fPrimitiveUnits;
            fProgram.colorInterpolation = fColorInterpolation;

            const SVGFilterElement* src = resolvePrimitiveSource();
            InternedKey last = filter::SourceGraphic();

            if (src)
            {
                for (auto& n : src->fNodes)
                {
                    if (!n) continue;
                    auto prim = std::dynamic_pointer_cast<SVGFilterPrimitiveElement>(n);
                    if (!prim) continue;

                    // Primitive decides if it can emit; unsupported ones are skipped for now.
                    prim->emitProgram(fProgram, last);
                }
            }

            fProgram.ops.push_back(packOp(FOP_END));
        }

        void bindSelfToContext(IRenderSVG*, IAmGroot*) override
        { 
            // this is where we build the program and do anything
            // else needed for actual use
            // 
            if (!fProgCompiled)
            {
                buildFilterProgramStream();
                fProgCompiled = true;
            }
        }

    };
}



