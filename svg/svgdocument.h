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

#include "svgshapes.h"
#include "svgfont.h"
#include "svgfilter.h"
#include "svgcss.h"
#include "svgdrawingcontext.h"



namespace waavs {

    //
    struct SVGDocument : public  SVGGraphicsElement, public IAmGroot
    {
		FontHandler* fFontHandler = nullptr;
        
        // We only have a single root 'SVGSVGElement' for the whole document
        std::shared_ptr<SVGSVGElement> fSVGNode = nullptr;
        //std::shared_ptr<SVGVisualNode> fSVGNode = nullptr;
        
        // We need a style sheet for the entire document
		std::shared_ptr<CSSStyleSheet> fStyleSheet = nullptr;
        
        // IAmGroot
        double fDpi = 96;
        double fCanvasWidth{};
		double fCanvasHeight{};
        
        std::unordered_map<std::string, std::shared_ptr<SVGViewable>> fDefinitions{};
        std::unordered_map<std::string, ByteSpan> fEntities{};
        
        
        //==========================================
        // Implementation
		//==========================================
        
        SVGDocument(FontHandler *fh)
            :SVGGraphicsElement(this)
            , fFontHandler(fh)
        {
            //fDpi = systemPpi;


			fStyleSheet = std::make_shared<CSSStyleSheet>();
        }

		// Properties
		FontHandler* fontHandler() const override { return fFontHandler; }
        
        double dpi() const override { return fDpi; }
		double canvasWidth() const override { return fCanvasWidth; }
		double canvasHeight() const override { return fCanvasHeight; }
        
        
        std::shared_ptr<CSSStyleSheet> styleSheet() override { return fStyleSheet; }
        void styleSheet(std::shared_ptr<CSSStyleSheet> sheet) override { fStyleSheet = sheet; }
        

        // retrieve root svg node
		std::shared_ptr<SVGSVGElement> documentElement() const { return fSVGNode; }
        

        //=================================================================
		// IAmGroot
		//=================================================================
        // Expand text that may have entities in it
        // return true if there were any entities
        bool expandText(const ByteSpan& txt, std::ostringstream& oss)
        {
            return false;
        }
        
        std::shared_ptr<SVGViewable> getElementById(const std::string& name) override
        {
			if (fDefinitions.find(name) != fDefinitions.end())
                return fDefinitions[name];

		    printf("SVGDocument::getElementById, FAIL: %s\n", name.c_str());
            
            return nullptr;
        }

        // Load a URL Reference
        std::shared_ptr<SVGViewable> findNodeByHref(const ByteSpan& inChunk) override
        {
            ByteSpan str = inChunk;

            auto id = chunk_trim(str, xmlwsp);

            // The first character could be '.' or '#'
            // so we need to skip past that
            if (*id == '.' || *id == '#')
                id++;

            if (!id)
                return nullptr;

            // lookup the thing we're referencing
            std::string idStr = toString(id);

            return getElementById(idStr);
        }

        // Load a URL reference, including the 'url(' function indicator
        std::shared_ptr<SVGViewable> findNodeByUrl(const ByteSpan& inChunk)
        {
            ByteSpan str = inChunk;

            // the id we want should look like this
            // url(#id)
            // so we need to skip past the 'url(#'
            // and then find the closing ')'
            // and then we have the id
            auto url = chunk_token(str, "(");
            auto id = chunk_trim(chunk_token(str, ")"), xmlwsp);

			// sometimes the id is quoted
            // so trim that as well
			id = chunk_trim(id, "\"");
			id = chunk_trim(id, "'");
            
            return findNodeByHref(id);
        }
        
        ByteSpan findEntity(const std::string& name) override
        {
			if (fEntities.find(name) != fEntities.end())
				return fEntities[name];

			return ByteSpan{};
        }
        
        void addDefinition(const std::string& name, std::shared_ptr<SVGViewable> obj)
        {
            fDefinitions[name] = obj;
        }
        
        virtual void addEntity(const std::string& name, ByteSpan expansion)
        {
            fEntities[name] = expansion;
        }

        void update() override
        {
			if (fSVGNode)
				fSVGNode->update();
        }
        
        void draw(IRenderSVG * ctx) override
        {
            if (nullptr == fSVGNode)
                return;

            //double startTime = seconds();
            fSVGNode->draw(ctx);
			//double endTime = seconds();

			//printf("Drawing Duration: %f\n", endTime - startTime);
        }
        
        BLRect sceneFrame() const
        {
            if (nullptr == fSVGNode)
                return BLRect();
            
            return fSVGNode->viewport();
        }

        
        bool addNode(std::shared_ptr < SVGVisualNode > node) override
        {            
			if (!SVGGraphicsElement::addNode(node))
                return false;

            if (node->name() == "svg")
            {

                if (nullptr == fSVGNode)
                {
                    fSVGNode = std::dynamic_pointer_cast<SVGSVGElement>(node);
                    //fSVGNode = node;
                    
                    BLRect vport = fSVGNode->viewport();
                    fCanvasWidth = vport.w;
                    fCanvasHeight = vport.h;

                    fSVGNode->bindToGroot(this);
                }
            }

            return true;
        }
        
        
        // Load the document from an XML Iterator
        // Since this is the top level document, we just want to kick
        // off loading the root node 'svg', and we're done 
        void loadFromXmlIterator(XmlElementIterator& iter) override
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
                        node->loadFromXmlIterator(iter);
                        addNode(node);
                    }
                    else {
                        printf("SVGDocument.loadFromXmlIterator : ERROR - could not create SVG node\n");
                    }
                }
            }
        }
        
        
		// Assuming we've already got a file mapped into memory, load the document
        bool loadFromChunk(const ByteSpan &srcChunk)
        {
            XmlElementIterator iter(srcChunk, true);

            loadFromXmlIterator(iter);

            return true;
        }

        // A convenience to construct the document from a chunk, and return
        // a shared pointer to the document
        static std::shared_ptr<SVGDocument> createFromChunk(const ByteSpan& srcChunk, FontHandler* fh)
        {
            auto doc = std::make_shared<SVGDocument>(fh);
            doc->loadFromChunk(srcChunk);

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
            
            SVGFillRule::registerFactory();
            
            SVGFontFamily::registerFactory();
            SVGFontSize::registerFactory();
            
			SVGMarkerAttribute::registerMarkerFactory();

            
            SVGOpacity::registerFactory();
            
            SVGRawAttribute::registerFactory();
            
            SVGStrokeLineCap::registerFactory();
            SVGStrokeLineJoin::registerFactory();
            SVGStrokeMiterLimit::registerFactory();
            SVGStrokeWidth::registerFactory();
            
            SVGStrokePaint::registerFactory();
			SVGStrokeOpacity::registerFactory();
            
			SVGTextAnchor::registerFactory();
            
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
            SVGImageNode::registerFactory();            // 'image'
            SVGSVGElement::registerFactory();           // 'svg'
            SVGStyleNode::registerFactory();            // 'style'
            SVGSwitchElement::registerFactory();        // 'switch'
            SVGTextNode::registerFactory();             // 'text'
            SVGTSpanNode::registerFactory();            // 'tspan'
            SVGUseElement::registerFactory();           // 'use'
            
            // Non-Structural Nodes
			SVGSolidColorElement::registerFactory();    // 'solidColor'
            SVGClipPath::registerFactory();             // 'clipPath'
            SVGDefsNode::registerFactory();             // 'defs'
            SVGConicGradient::registerFactory();        // 'conicGradient'
            SVGLinearGradient::registerFactory();       // 'linearGradient'
            SVGMarkerNode::registerFactory();           // 'marker'
            SVGMaskNode::registerFactory();             // 'mask'
            SVGPatternNode::registerFactory();          // 'pattern'
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
