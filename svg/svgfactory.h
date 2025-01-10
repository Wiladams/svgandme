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

        // We have a singleton factory object for the entire application
        static SVGFactory* getFactory()
        {
			static std::unique_ptr<SVGFactory> sFactory = std::make_unique<SVGFactory>();

			return sFactory.get();
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


        // createDOM
		// Create a new SVGDocument object
        // This document is not bound to a drawing context, so a lot of things are not going
        // to be resolved, particularly relative sizing, and fonts
        // But, tree visitors can be used to turn the DOM into something useful, like 
        // a graphics rendering tree.
        static std::shared_ptr<SVGDocument> createDOM(const ByteSpan& srcChunk, FontHandler* fh)
        {
            auto sFactory = SVGFactory::getFactory();

            auto doc = std::make_shared<SVGDocument>(fh, 640, 480,96);
            if (!doc->loadFromChunk(srcChunk, fh))
                return {};

            return doc;
        }
        
        // A convenience to construct the document from a chunk, and return
        // a shared pointer to the document
        static std::shared_ptr<SVGDocument> createFromChunk(const ByteSpan& srcChunk, FontHandler* fh, const double w, const double h, const double ppi)
        {
            auto sFactory = SVGFactory::getFactory();
            
            auto doc = std::make_shared<SVGDocument>(fh, w, h, ppi);
            if (!doc->loadFromChunk(srcChunk, fh))
                return {};

            // BUGBUG - Maybe we should stop here, and use
            // a visitor to convert the raw DOM into a graphics tree
            // render into a blank context to get sizing
            IRenderSVG actx(fh);
            actx.setViewport(BLRect(0, 0, w, h));
            doc->draw(&actx, doc.get());

            return doc;
        }
    };
}
