#pragma once


#include "lang_grammar.h"
#include "converters.h"
#include "core_geometry.h"
#include "wg_matrix3x3.h"
#include "svgscan.h"
#include "svgatoms.h"
#include "svgstructuretypes.h"


// It may be a bit of overkill doing all this structured
// approach to parsing the transform functions, but it is
// a good way to go to exercise function parsing.
// Using a call frame can also be used when parsing color functions,
// and possibly CSS functions in the future.  So, might as well
// exercise it here and see how it goes.

namespace waavs
{

    // Given a span that contains a set of numbers separated
    // by whitespace and/or commas, read the numbers into an array
    // 'body' - the span containing the numbers to be read
    // 'args' - an array where the numbers will be stored
    // 'maxArgs' - the maximum number of arguments that can be stored in the array
    // 'na' - returns the number of arguments actually read
    static bool readNumericArgsBody(
        ByteSpan body,
        double* args,
        int maxArgs,
        int& nArgs) noexcept
    {
        if ( maxArgs <= 0)
            return false;

        nArgs = 0;
        bspan_skip_spaces(body);

        while (body) 
        {
            bspan_skip_spaces(body);

            if (body && *body == ',') {
                ++body;
                bspan_skip_spaces(body);
            }

            if (!body)
                break;

            if (nArgs >= maxArgs)
                return false;

            if (!number_list_read_next(body, args[nArgs]))
                return false;

            nArgs++;
        }

        return true;
    }



    // parseTransformArgs
    // 
    //
    // parse a number of numeric arguments from a chunk.  The numbers
    // are delimited by whitspace, and/or ',' characters.
    // Parameters
    //   'args' is an array where the values will be stored
    //   'minNa' is the minimum number of arguments required for a successful parse
    //   'maxNa' is the maximum number of arguments
    //   'na' - returns the number of arguments actually retrieved
    static bool transformFromInvocation(const Invocation& call, WGMatrix3x3& tm) noexcept
    {
        double args[6]{};
        int nArgs = 0;

        if (!readNumericArgsBody(call.payload, args, 6, nArgs))
            return false;

        tm.reset();

        if (call.name == svgfunc::matrix())
        {
            if (nArgs != 6)
                return false;

            tm.resetAffine(args[0], args[1], args[2], args[3], args[4], args[5]);
            return true;
        }

        if (call.name == svgfunc::translate())
        {
            if (nArgs < 1 || nArgs > 2)
                return false;

            tm.resetToTranslation(args[0], nArgs == 2 ? args[1] : 0.0);
            return true;
        }

        if (call.name == svgfunc::scale())
        {
            if (nArgs < 1 || nArgs > 2)
                return false;

            tm.resetToScaling(args[0], nArgs == 2 ? args[1] : args[0]);
            return true;
        }

        if (call.name == svgfunc::rotate())
        {
            if (nArgs != 1 && nArgs != 3)
                return false;

            const double angle = radians(args[0]);

            if (nArgs == 1)
                tm.resetToRotation(angle);
            else
                tm.resetToRotation(angle, args[1], args[2]);

            return true;
        }

        if (call.name == svgfunc::skewX())
        {
            if (nArgs != 1)
                return false;

            tm.resetToSkewing(radians(args[0]), 0.0);
            return true;
        }

        if (call.name == svgfunc::skewY())
        {
            if (nArgs != 1)
                return false;

            tm.resetToSkewing(0.0, radians(args[0]));
            return true;
        }

        return false;
    }



    // parseTransform()
    // 
    // Parse a transform attribute, stuffing the results
    // into a single WGMatrix3x3 structure
    // This will repeatedly apply the portions that are parsed
    //
    
    static bool parseTransform(const ByteSpan& src, WGMatrix3x3& xform) noexcept
    {
        ByteSpan s = src;
        bspan_skip_spaces(s);

        if (!s)
            return false;

        xform = WGMatrix3x3::makeIdentity();

        while (s)
        {
            bspan_skip_spaces(s);

            if (!s)
                break;

            Invocation call{};

            if (!readInvocation(s, call))
                return false;

            WGMatrix3x3 tm = WGMatrix3x3::makeIdentity();

            if (!transformFromInvocation(call, tm))
                return false;

            // SVG lists transforms in coordinate-system order.
            // WGMatrix3x3 maps row vectors, so each new transform
            // is multiplied on the left.
            xform.transform(tm);
        }

        return true;
    }

}


namespace waavs {

    //================================================
    // SVGTransform
    // Transformation matrix
    //================================================
    struct SVGTransform : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::transform(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGTransform>(); 
                node->loadFromAttributes(attrs, groot);
                return node; });
        }

        WGMatrix3x3 fMatrix = WGMatrix3x3::makeIdentity();

        SVGTransform() : SVGVisualProperty(nullptr)
        {
            setName(svgattr::transform());
            setAutoDraw(false);
        }
        SVGTransform(const SVGTransform& other) = delete;
        SVGTransform& operator=(const SVGTransform& rhs) = delete;



        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!parseTransform(inChunk, fMatrix)) {
                printf("Failed to parse transform: '%.*s'\n", (int)inChunk.size(), inChunk.data());
                return false;
            }

            set(true);
            setNeedsBinding(false);

            return true;
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            //if ((fMatrix.type() != BLTransformType::BL_TRANSFORM_TYPE_INVALID) &&
            //    (fMatrix.type() != BLTransformType::BL_TRANSFORM_TYPE_IDENTITY))
            ctx->applyTransform(fMatrix);
        }
    };
}
