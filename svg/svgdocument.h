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
#include "svgsymbol.h"
#include "svgpattern.h"
#include "svgfont.h"
#include "svgfilter.h"
#include "svghyperlink.h"




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
        SVGDocument() = delete;
        
        SVGDocument(FontHandler *fh, const double w=0, const double h=0, const double ppi = 96)
            :SVGGraphicsElement()
            , fDpi(ppi)
            , fFontHandler(fh)
            , fCanvasWidth(w)
            , fCanvasHeight(h)
        {
            // Create document style sheet that can be filled in
			fStyleSheet = std::make_shared<CSSStyleSheet>();
        }

        SVGDocument(const ByteSpan& srcChunk, const double w = 64, const double h = 64, const double ppi = 96, FontHandler* fh=nullptr)
            :SVGGraphicsElement()
            , fDpi(ppi)
            , fFontHandler(fh)
            , fCanvasWidth(w)
            , fCanvasHeight(h)
        {
            resetFromSpan(srcChunk, fh, w, h, ppi);
        }
        
        
        void resetFromSpan(const ByteSpan& srcChunk, FontHandler* fh, const double w, const double h, const double ppi=96)
        {
            // clear out the old document
            //clear();
            fontHandler(fh);
            dpi(ppi);
            canvasSize(w, h);
            fStyleSheet = std::make_shared<CSSStyleSheet>();

            // load the new document
            loadFromChunk(srcChunk, fh);
        }
        
        
		// Properties
		FontHandler* fontHandler() const override { return fFontHandler; }
        void fontHandler(FontHandler* fh) { fFontHandler = fh; }
        
        double dpi() const override { return fDpi; }
		void dpi(const double d) override { fDpi = d; }
        
		double canvasWidth() const override { return fCanvasWidth; }
		double canvasHeight() const override { return fCanvasHeight; }
        void canvasSize(const double w, const double h) { fCanvasWidth = w; fCanvasHeight = h; }
        
		BLRect frame() const override { 
            return BLRect(0, 0, fCanvasWidth, fCanvasHeight); 
        }
        
        // This should be size of document elements
        BLRect getBBox() const override 
        {   
            BLRect extent{};
            bool firstOne = true;

            // traverse the graphics
            // expand bounding box to include their frames, without alternation
            for (auto& g : fNodes)
            {
                if (firstOne) {
                    extent = g->getBBox();
                    firstOne = false;
                }
                else {
					//expandRect(extent, g->getBBox());
                    //expandRect(extent, g->frame());
                }
            }

            return extent;
            
        }
        
        std::shared_ptr<CSSStyleSheet> styleSheet() override { return fStyleSheet; }
        void styleSheet(std::shared_ptr<CSSStyleSheet> sheet) override { fStyleSheet = sheet; }
        

        // retrieve root svg node
		std::shared_ptr<SVGSVGElement> documentElement() const { return fSVGNode; }


        
        
        
        bool addNode(std::shared_ptr < ISVGElement > node, IAmGroot* groot) override
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

            needsBinding(true);
        }

        
        
		// Assuming we've already got a file mapped into memory, load the document
        bool loadFromChunk(const ByteSpan &srcChunk, FontHandler* fh)
        {
            // create a memBuff from srcChunk
            // since we use memory references, we need
            // to keep the memory around for the duration of the 
            // document's life
            fSourceMem.initFromSpan(srcChunk);
            
			// Create the XML Iterator we're going to use to parse the document
            XmlElementIterator iter(fSourceMem.span(), true);

            // The first pass builds the DOM
            loadFromXmlIterator(iter, this);
            
            
            return true;
        }

        // For compound nodes (which have children) we want to 
        // do the base stuff (binding properties) then bind the children
        // If you sub-class this, you should call this first
        // then do your own thing.  We don't want to call a 'bindSelfToGroot'
        // here, because that complicates the interactions and sequences of things
        // so just override bindToGroot
        void bindToContext(IRenderSVG* ctx, IAmGroot* groot) noexcept override
        {
            this->fixupStyleAttributes(ctx, groot);
            convertAttributesToProperties(ctx, groot);

            this->bindSelfToContext(ctx, groot);

            needsBinding(false);
        }
        
        // Bind to a context of a given size
        // we are meant to do this once, per canvas size
        // that we're eventually going to draw into
        // This mimics what is done in drawing, but it's meant to 
        // allow those things that need to do binding to do what they need
        // and not actually do the drawing
        virtual void bindChildrenToContext(IRenderSVG* ctx, IAmGroot* groot)
        {
            for (auto& node : fNodes) 
            {
                node->bindToContext(ctx, groot);
            }
        }
        
        virtual void bindToContext(IRenderSVG* ctx, IAmGroot* groot, double cWidth, double cHeight) noexcept
        {
            // Start by setting the size of the canvas
			// that we intend to be drawing into
            ctx->setViewport(BLRect(0,0,cWidth,cHeight));

            ctx->push();
            
            this->fixupStyleAttributes(ctx, groot);
            convertAttributesToProperties(ctx, groot);

            this->bindSelfToContext(ctx, groot);
            this->bindChildrenToContext(ctx, groot);
            
            ctx->pop();

            needsBinding(false);
        }
        
        void drawSelf(IRenderSVG* ctx, IAmGroot*) override
        {
            // draw horizontal blue line at 10,10 for 300
            //ctx->strokeLine(10, 10, 300, 10, BLRgba32(0xff0000ff));
            //ctx->strokeLine(10, 10, 10, 300, BLRgba32(0xffff0000));
        }
        
        void draw(IRenderSVG* ctx, IAmGroot* groot) override
        {            
            ctx->push();

            //if (needsBinding())
            //    this->bindToContext(ctx, groot);
            
            // Should have valid bounding box by now
            // so set objectFrame on the context
            ctx->objectFrame(getBBox());

            this->applyProperties(ctx, groot);
            this->drawSelf(ctx, groot);

            this->drawChildren(ctx, groot);

            ctx->pop();
        }
        

        virtual void draw(IRenderSVG* ctx, IAmGroot* groot, double cWidth, double cHeight)
        {
            canvasSize(cWidth, cHeight);
            
            this->bindToContext(ctx, groot, cWidth, cHeight);

            ctx->push();
            ctx->objectFrame(getBBox());

            this->applyProperties(ctx, groot);
            this->drawSelf(ctx, groot);

            this->drawChildren(ctx, groot);

            ctx->pop();
        }

        
    };

    using SVGDocumentHandle = std::shared_ptr<SVGDocument>;

}


