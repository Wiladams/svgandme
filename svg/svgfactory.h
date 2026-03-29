#pragma once

#include "svgdocument.h"
#include "nametable.h"

namespace waavs {
    struct SVGFactory
    {
        PSNameTable fFeatureNames{};

        // Disallow copying and assignment
        SVGFactory(const SVGFactory&) = delete;
        SVGFactory& operator=(const SVGFactory&) = delete;

        SVGFactory()
        {
            registerFeatures();
            registerNodeTypes();
        }



        // Query whether the runtime supports a w3c feature.
        // Controlled by "Feature String"s defined in https://www.w3.org/TR/SVG11/feature.html
        void addFeature(const char* feature)
        {
            fFeatureNames.intern(feature);
        }


        virtual bool hasFeature(const char* feature) const
        {
            return fFeatureNames.hasName(feature);
        }

        void registerFeatures()
        {
            // Register supported features here
            addFeature("http://www.w3.org/TR/SVG11/feature#BasicStructure");
            addFeature("http://www.w3.org/TR/SVG11/feature#Clip");
            addFeature("http://www.w3.org/TR/SVG11/feature#ConditionalProcessing");
            addFeature("http://www.w3.org/TR/SVG11/feature#Filter");
            addFeature("http://www.w3.org/TR/SVG11/feature#Font");
            addFeature("http://www.w3.org/TR/SVG11/feature#Gradient");
            addFeature("http://www.w3.org/TR/SVG11/feature#Image");
            addFeature("http://www.w3.org/TR/SVG11/feature#Marker");
            addFeature("http://www.w3.org/TR/SVG11/feature#Mask");
            addFeature("http://www.w3.org/TR/SVG11/feature#OpacityAttribute");
            addFeature("http://www.w3.org/TR/SVG11/feature#PaintAttribute");
            addFeature("http://www.w3.org/TR/SVG11/feature#Pattern");
            addFeature("http://www.w3.org/TR/SVG11/feature#Shape");
            addFeature("http://www.w3.org/TR/SVG11/feature#Style");
            addFeature("http://www.w3.org/TR/SVG11/feature#Text");
            addFeature("http://www.w3.org/TR/SVG11/feature#ViewportAttribute");
            addFeature("http://www.w3.org/TR/SVG11/feature#XlinkAttribute");
            addFeature("http://www.w3.org/TR/SVG11/feature#SVG-static");

        }


        void registerNodeTypes()
        {
            //=============================================
            // Register attributes
            //=============================================

            // Global paint attributes
            SVGPaintOrderAttribute::registerFactory();
            SVGOpacity::registerFactory();
            SVGColorPaint::registerFactory();

            // Fill attributes
            SVGFillPaint::registerFactory();
            SVGFillOpacity::registerFactory();
            SVGFillRuleAttribute::registerFactory();

            // Stroke attributes
            SVGStrokePaint::registerFactory();
            SVGStrokeOpacity::registerFactory();

            SVGStrokeLineCap::registerFactory();
            SVGStrokeLineJoin::registerFactory();
            SVGStrokeMiterLimit::registerFactory();
            SVGStrokeWidth::registerFactory();
            SVGVectorEffectAttribute::registerFactory();



            // Typography Attributes
            SVGTextAnchorAttribute::registerFactory();

            // Fontography Attributes
            SVGFontFamily::registerFactory();
            SVGFontSize::registerFactory();
            SVGFontStyleAttribute::registerFactory();
            SVGFontWeightAttribute::registerFactory();
            SVGFontStretchAttribute::registerFactory();

            SVGTransform::registerFactory();

            // stroke dash attributes
            SVGStrokeDashArray::registerFactory();
            SVGStrokeDashOffset::registerFactory();

            // Clipping
            //SVGClipPathAttribute::registerFactory();        // 'clip-path' attribute
            //SVGClipPathElement::registerFactory();        // 'clipPath' element

            //=============================================
            // Register SVG Node Types
            //=============================================
            // Shape nodes
            SVGCircleElement::registerFactory();               // 'circle'
            SVGEllipseElement::registerFactory();              // 'ellipse'
            SVGLineElement::registerFactory();                 // 'line'
            SVGPolygonElement::registerFactory();              // 'polygon'
            SVGPolylineElement::registerFactory();             // 'polyline'
            SVGPathElement::registerFactory();                 // 'path'
            SVGRectElement::registerFactory();                 // 'rect'

            SVGMarkerAttribute::registerMarkerFactory();
            SVGSymbolNode::registerFactory();           // 'symbol'


            // Register Structural nodes
            SVGAElement::registerFactory();             // 'a'
            SVGGElement::registerFactory();             // 'g'
            SVGImageElement::registerFactory();         // 'image'
            SVGSVGElement::registerFactory();           // 'svg'
            SVGStyleNode::registerFactory();            // 'style'
            SVGSwitchElement::registerFactory();        // 'switch'
            SVGUseElement::registerFactory();           // 'use'

            // Text nodes
            SVGTextNode::registerFactory();             // 'text'
            SVGTSpanNode::registerFactory();            // 'tspan'
            SVGFlowRoot::registerFactory();             // 'flowRoot'
            

            // Non-Structural Nodes
            SVGSolidColorElement::registerFactory();    // 'solidColor'
            SVGDefsNode::registerFactory();             // 'defs'

            // Gradient nodes
            SVGConicGradient::registerFactory();        // 'conicGradient'
            SVGLinearGradient::registerFactory();       // 'linearGradient'
            SVGRadialGradient::registerFactory();       // 'radialGradient'

            SVGMarkerElement::registerFactory();           // 'marker'
            SVGMaskElement::registerFactory();             // 'mask'
            SVGPatternElement::registerFactory();          // 'pattern'


            // Filter node registrations
            // Do this registration at the application level
            // If you want to turn the whole thing on or off
            //SVGFilterElement::registerFactory();                // 'filter'

            SVGFeBlendElement::registerFactory();               // 'feBlend'
            SVGFeColorMatrixElement::registerFactory();         // 'feColorMatrix'
            SVGFeComponentTransferElement::registerFactory();   // 'feComponentTransfer'
            SVGFeCompositeElement::registerFactory();           // 'feComposite'
            SVGFeConvolveMatrixElement::registerFactory();      // 'feConvolveMatrix'
            SVGFeDiffuseLightingElement::registerFactory();     // 'feDiffuseLighting'
            SVGFeDisplacementMapElement::registerFactory();     // 'feDisplacementMap'
            SVGFeDropShadowElement::registerFactory();          // 'feDropShadow'
            SVGFeFloodElement::registerFactory();               // 'feFlood'
            SVGFeGaussianBlurElement::registerFactory();        // 'feGaussianBlur'
            SVGFeImageElement::registerFactory();               // 'feImage'
            SVGFeMergeElement::registerFactory();               // 'feMerge'
            SVGFeMorphologyElement::registerFactory();          // 'feMorphology'
            SVGFeOffsetElement::registerFactory();              // 'feOffset'
            SVGFeTileElement::registerFactory();                // 'feTile'
            SVGFeTurbulenceElement::registerFactory();          // 'feTurbulence'
            SVGFeSpecularLightingElement::registerFactory();    // 'feSpecularLighting'

            // sub-elements
            SVGFeMergeNodeElement::registerFactory();           // 'feMergeNode'
            SVGFeDistantLightElement::registerFactory();        // 'feDistantLightMap'
            SVGFePointLightElement::registerFactory();          // 'fePointLight'
            SVGFeSpotLightElement::registerFactory();           // 'feSpotLight'

            // Filter sub-node registrations
            SVGFeFuncElement::registerSingularNode();          // 'feFuncR', 'feFuncG', 'feFuncB', 'feFuncA'

            // Font node registrations
            // These are obsolete and deprecated
            //SVGFontNode::registerFactory();             // 'font'
            //SVGFontFaceNode::registerFactory();         // 'font-face'
            //SVGGlyphNode::registerFactory();            // 'glyph'
            //SVGMissingGlyphNode::registerFactory();     // 'missing-glyph'
            //SVGFontFaceNameNode::registerFactory();     // 'font-face-name'
            //SVGFontFaceSrcNode::registerFactory();      // 'font-face-src'

            // Miscellaneous
            //SVGDescNode::registerFactory();             // 'desc'
            //SVGTitleNode::registerFactory();            // 'title'


        }

    private:


        // A convenience to construct the document from a chunk, and return
        // a shared pointer to the document
        static std::shared_ptr<SVGDocument> createFromChunkInternal(const ByteSpan& srcChunk, const double w, const double h, const double ppi)
        {
            // this MUST be done, or node registrations will not happen
            auto doc = SVGDocument::createFromChunk(srcChunk, w, h, ppi);

            if (doc == nullptr)
            {
                printf("SVGFactory::CreateFromChunk() failed to load\n");
                return nullptr;
            }



            return doc;
        }

    public:
        // We have a singleton factory object for the entire application
        static SVGFactory* getSingleton()
        {
            static std::unique_ptr<SVGFactory> sFactory = std::make_unique<SVGFactory>();

            return sFactory.get();
        }

        // A convenience to construct the document from a chunk, and return
        // a shared pointer to the document
        static std::shared_ptr<SVGDocument> createFromChunk(const ByteSpan& srcChunk, double w, double h, double ppi)
        {
            auto doc = getSingleton()->createFromChunkInternal(srcChunk, w, h, ppi);

            if (!doc)
                return nullptr;

            return doc;
        }
    };
}
