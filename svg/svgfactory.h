#pragma once

#include "svgdocument.h"

namespace waavs {
    struct SVGFactory
    {
        // Disallow copying and assignment
        SVGFactory(const SVGFactory&) = delete;
        SVGFactory& operator=(const SVGFactory&) = delete;

        SVGFactory()
        {
            registerNodeTypes();
        }

        void registerNodeTypes()
        {
            // Register attributes
            SVGPaintOrderAttribute::registerFactory();

            SVGOpacity::registerFactory();

            SVGFillPaint::registerFactory();
            SVGFillOpacity::registerFactory();
            SVGFillRuleAttribute::registerFactory();

            SVGStrokePaint::registerFactory();
            SVGStrokeOpacity::registerFactory();

            SVGStrokeLineCap::registerFactory();
            SVGStrokeLineJoin::registerFactory();
            SVGStrokeMiterLimit::registerFactory();
            SVGStrokeWidth::registerFactory();


            SVGMarkerAttribute::registerMarkerFactory();
            SVGVectorEffectAttribute::registerFactory();

            // Typography Attributes
            SVGTextAnchorAttribute::registerFactory();

            // Fontography Attributes
            SVGFontFamily::registerFactory();
            SVGFontSize::registerFactory();
            SVGFontStyleAttribute::registerFactory();
            SVGFontWeightAttribute::registerFactory();
            SVGFontStretchAttribute::registerFactory();

            //SVGClipPathAttribute::registerFactory();
            //SVGTransform::registerFactory();



            // Register shape nodes
            SVGCircleElement::registerFactory();               // 'circle'
            SVGEllipseElement::registerFactory();              // 'ellipse'
            SVGLineElement::registerFactory();                 // 'line'
            SVGPolygonElement::registerFactory();              // 'polygon'
            SVGPolylineElement::registerFactory();             // 'polyline'
            SVGPathElement::registerFactory();                 // 'path'
            SVGRectElement::registerFactory();                 // 'rect'

            // Register Structural nodes
            SVGAElement::registerFactory();             // 'a'
            SVGGElement::registerFactory();             // 'g'
            SVGImageElement::registerFactory();         // 'image'
            SVGSVGElement::registerFactory();           // 'svg'
            SVGStyleNode::registerFactory();            // 'style'
            SVGSwitchElement::registerFactory();        // 'switch'
            SVGTextNode::registerFactory();             // 'text'
            //SVGTSpanNode::registerFactory();            // 'tspan'
            SVGUseElement::registerFactory();           // 'use'

            // Non-Structural Nodes
            SVGSolidColorElement::registerFactory();    // 'solidColor'
            SVGClipPathElement::registerFactory();             // 'clipPath'
            SVGDefsNode::registerFactory();             // 'defs'
            SVGConicGradient::registerFactory();        // 'conicGradient'
            SVGLinearGradient::registerFactory();       // 'linearGradient'
            SVGMarkerElement::registerFactory();           // 'marker'
            SVGMaskElement::registerFactory();             // 'mask'
            SVGPatternElement::registerFactory();          // 'pattern'
            SVGRadialGradient::registerFactory();       // 'radialGradient'
            SVGSymbolNode::registerFactory();           // 'symbol'


            // Filter node registrations
            SVGFilterElement::registerFactory();            // 'filter'
            SVGFeBlendElement::registerFactory();           // 'feBlend'
            SVGFeColorMatrixElement::registerFactory();     // 'feColorMatrix'
            SVGFeCompositeElement::registerFactory();       // 'feComposite'
            SVGFeComponentTransferElement::registerFactory();       // 'feComponentTransfer'
            SVGFeConvolveMatrixElement::registerFactory();  // 'feConvolveMatrix'
            SVGFeDiffuseLightingElement::registerFactory(); // 'feDiffuseLighting'
            SVGFeDisplacementMapElement::registerFactory(); // 'feDisplacementMap'
            SVGFeDistantLightElement::registerFactory();    // 'feDistantLightMap'
            SVGFeFloodElement::registerFactory();           // 'feFlood'
            SVGFeGaussianBlurElement::registerFactory();    // 'feGaussianBlur'
            SVGFeOffsetElement::registerFactory();          // 'feOffset'
            SVGFeTurbulenceElement::registerFactory();      // 'feTurbulence'


            // Font node registrations
            // These are obsolete and deprecated
            SVGFontNode::registerFactory();             // 'font'
            SVGFontFaceNode::registerFactory();         // 'font-face'
            SVGGlyphNode::registerFactory();            // 'glyph'
            SVGMissingGlyphNode::registerFactory();     // 'missing-glyph'
            SVGFontFaceNameNode::registerFactory();     // 'font-face-name'
            SVGFontFaceSrcNode::registerFactory();      // 'font-face-src'

            // Miscellaneous
            SVGDescNode::registerFactory();             // 'desc'
            SVGTitleNode::registerFactory();            // 'title'



        }

    };
}
