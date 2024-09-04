#pragma once

// References
// https://razrfalcon.github.io/notes-on-svg-parsing/index.html
//
// Notes
// Things not implemented
// <filter> - only stubs to deal with most common
// <flowroot> - a couple of stubs
// <clip-path> - nodes are thrown away
// <gradient> - only works for userSpaceOnUse, NOT objectSpace
// 
// Needs some work
// <pattern>
// <symbol> - needs to honor <use> node's width/height
// <style>  - css needs to support more complex selectors
// <g>      - should support local <defs>
// <text>   - fix alignment
// 
// URL lookups - include external files
// 
//

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <sstream>


#include "maths.h"

#include "svgcss.h"

#include "svgclip.h"
#include "svgconditional.h"
#include "svggradient.h"
#include "svgimage.h"
#include "svgmarker.h"
#include "svgmask.h"
#include "svgstructure.h"
#include "svgshapes.h"
#include "svgstyle.h"
#include "svgpattern.h"
#include "svgfont.h"
#include "svgfilter.h"
#include "svghyperlink.h"

#include "svgdrawingcontext.h"
//#include "svgwaavs.h"


namespace waavs {

    //
    struct SVGDocument : public  SVGGraphicsElement, public IAmGroot 
    {
        MemBuff fSourceMem{};
        
		FontHandler* fFontHandler = nullptr;
        
        // BUGBUG - this should go away
        // Although there can be multiple <svg> elements in a document
		// we track only the first one
        // We only have a single root 'SVGSVGElement' for the whole document
        std::shared_ptr<SVGSVGElement> fSVGNode = nullptr;

        
        // We need a style sheet for the entire document
		std::shared_ptr<CSSStyleSheet> fStyleSheet = nullptr;
        
        // IAmGroot
        // Information about the environment
        double fDpi = 96;
        double fCanvasWidth{};
		double fCanvasHeight{};
        
        // Information from the document
        double fDocumentWidth{};
		double fDocumentHeight{};
        
        
        
        //==========================================
        // Implementation
		//==========================================
        
        SVGDocument(FontHandler *fh, const double w, const double h, const double ppi = 96)
            :SVGGraphicsElement(this)
            , fDpi(ppi)
            , fFontHandler(fh)
            , fCanvasWidth(w)
            , fCanvasHeight(h)
        {
            // Create document style sheet that can be filled in
			fStyleSheet = std::make_shared<CSSStyleSheet>();
        }

		// Properties
		FontHandler* fontHandler() const override { return fFontHandler; }
        
        double dpi() const override { return fDpi; }
		void dpi(const double d) override { fDpi = d; }
        
		double canvasWidth() const override { return fCanvasWidth; }
		double canvasHeight() const override { return fCanvasHeight; }
        
        BLRect getBBox() const override 
        {
			return BLRect(0, 0, fCanvasWidth, fCanvasHeight);
        }
        
        std::shared_ptr<CSSStyleSheet> styleSheet() override { return fStyleSheet; }
        void styleSheet(std::shared_ptr<CSSStyleSheet> sheet) override { fStyleSheet = sheet; }
        

        // retrieve root svg node
		std::shared_ptr<SVGSVGElement> documentElement() const { return fSVGNode; }

        
        void draw(IRenderSVG * ctx, IAmGroot* groot) override
        {
            // Setup default values for a SVG context
            ctx->strokeJoin(BL_STROKE_JOIN_MITER_CLIP);
            ctx->strokeMiterLimit(4);
            ctx->setFillRule(BL_FILL_RULE_NON_ZERO);
            ctx->fill(BLRgba32(0, 0, 0));
            ctx->noStroke();
            ctx->strokeWidth(1.0);
            //ctx->textAlign(ALIGNMENT::LEFT, ALIGNMENT::BASELINE);
            //ctx->textFamily("Arial");
            //ctx->textSize(16);

            SVGGraphicsElement::draw(ctx, groot);
        }
        
        
        
        bool addNode(std::shared_ptr < SVGVisualNode > node, IAmGroot* groot) override
        {            
			if (!SVGGraphicsElement::addNode(node, groot))
                return false;

            
            if (node->name() == "svg")
            {

                if (nullptr == fSVGNode)
                {
                    fSVGNode = std::dynamic_pointer_cast<SVGSVGElement>(node);
                    
				}

            }
            
            return true;
        }
        
		//void loadSelfFromXmlIterator(XmlElementIterator& iter, IAmGroot* groot) override
		//{
            // do nothing here
		//}
        

        // loadFromXmlIterator
        // 
        // Load the document from an XML Iterator
        // Since this is the top level document, it will hold all the sub-elements.
        // This document also acts as the "groot" (graphics root).  That means you 
        // have to call bindToGroot() after successfully loading the document.
        void loadFromXmlIterator(XmlElementIterator& iter, IAmGroot* groot) override
        {

            // skip past any elements that come before the 'svg' element
            while (iter.next())
            {
                const XmlElement& elem = *iter;

                if (!elem)
                    break;

                //printXmlElement(elem);


                if (elem.isStart() && (elem.tagName() == "svg"))
                {
                    // There should be only one root node in a document, so we should 
                    // break here, but, curiosity...
                    auto node = std::make_shared<SVGSVGElement>(this);
                    
                    if (nullptr != node) {
                        node->loadFromXmlIterator(iter, groot);
                        addNode(node, groot);
                    }
                    else {
                        printf("SVGDocument.loadFromXmlIterator : ERROR - could not create SVG node\n");
                    }
				}
				else if (elem.isDoctype()) {

                    //printf(" ====  DOCTYPE ====\n");
                    // BUGBUG - We need more information on what kind
                    // of DOCTYPE it was.
                    // We only need to create an iterator if there is style information
                    // or a set of entities.
                    // 
                    // create an xmlelementiterator on the elem.data() 
                    // and iterate over the entities if they are there
                    // adding them to our entity collection
                    /*
                    XmlElementIterator entityIter(elem.data());
                    while (entityIter.next())
                    {
                        const XmlElement& entityElem = *entityIter;
						if (entityElem.isEntityDeclaration())
						{
                            ByteSpan name;
                            ByteSpan value;
                            ByteSpan content = chunk_ltrim(entityElem.data(), xmlwsp);


                            // Get the name of the entity
                            name = chunk_token(content, xmlwsp);

                            // Skip past the whitespace
                            content = chunk_ltrim(content, xmlwsp);

							// Get the value of the entity
                            readQuoted(content, value);


							if (name && value)
							{
								addEntity(name, value);
							}
						}
                    }
                    */
				}
                else {
                    //printf("SVGDocument.loadFromXmlIterator : ERROR - unexpected element: %s\n", toString(elem.tagName()).c_str());
                    //printChunk(elem.data());
                }
            }
        }

        
        
		// Assuming we've already got a file mapped into memory, load the document
        bool loadFromChunk(const ByteSpan &srcChunk)
        {
            // create a memBuff from srcChunk
            // since we use memory references, we need
            // to keep the memory around for the duration of the 
            // document's life
            fSourceMem.initFromSpan(srcChunk);
            
			// Create the XML Iterator we're going to use to parse the document
            XmlElementIterator iter(fSourceMem.span(), true);

            loadFromXmlIterator(iter, this);

            
            return true;
        }

        // A convenience to construct the document from a chunk, and return
        // a shared pointer to the document
        static std::shared_ptr<SVGDocument> createFromChunk(const ByteSpan& srcChunk, FontHandler* fh, const double w, const double h, const double ppi)
        {
            auto doc = std::make_shared<SVGDocument>(fh, w, h, ppi);
            if (!doc->loadFromChunk(srcChunk))
                return {};

            SVGDocument* groot = doc.get();
            doc->bindToGroot(groot, nullptr);
            
            return doc;
        }
        
    };
}

namespace waavs {
    struct SVGFactory
    {
        
        SVGFactory()
        {
            // Register attributes
            //SVGClipPathAttribute::registerFactory();
			SVGFillPaint::registerFactory();
            SVGFillOpacity::registerFactory();
            SVGOpacity::registerFactory();
            SVGStrokeLineCap::registerFactory();
            SVGStrokeLineJoin::registerFactory();
            SVGStrokeMiterLimit::registerFactory();
            SVGStrokeWidth::registerFactory();

            SVGStrokePaint::registerFactory();
            SVGStrokeOpacity::registerFactory();

            
            SVGFillRule::registerFactory();
            
            //SVGFontFamily::registerFactory();
            //SVGFontSize::registerFactory();
            //SVGTextAnchor::registerFactory();
            //             
			SVGMarkerAttribute::registerMarkerFactory();

            

            
            

            

            
            //SVGTransform::registerFactory();
            
            SVGVectorEffectAttribute::registerFactory();
            
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
            SVGImageElement::registerFactory();            // 'image'
            SVGSVGElement::registerFactory();           // 'svg'
            SVGStyleNode::registerFactory();            // 'style'
            SVGSwitchElement::registerFactory();        // 'switch'
            SVGTextNode::registerFactory();             // 'text'
            SVGTSpanNode::registerFactory();            // 'tspan'
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


            //DisplayCaptureElement::registerFactory();
        }

    };
}
