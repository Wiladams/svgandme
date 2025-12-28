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

#include "xmlutil.h"

#include "fonthandler.h"
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




namespace waavs 
{

    //
    struct SVGDocument : public  SVGGraphicsElement, public IAmGroot 
    {
        MemBuff fSourceMem{};
        
		FontHandler* fFontHandler = nullptr;
        
        // BUGBUG - this should go away
        // Although there can be multiple <svg> elements in a document
		// we track only the first one
        // We only have a single root 'SVGSVGElement' for the whole document
        std::shared_ptr<SVGSVGElement> fTopLevelNode = nullptr;

        
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
        
        ~SVGDocument() = default;


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
		std::shared_ptr<SVGSVGElement> documentElement() const { return fTopLevelNode; }


        bool addNode(std::shared_ptr < ISVGElement > node, IAmGroot* groot) override
        {            
			if (!SVGGraphicsElement::addNode(node, groot))
                return false;

            
            if (node->name() == "svg")
            {
                fTopLevelNode = std::dynamic_pointer_cast<SVGSVGElement>(node);

                if (fTopLevelNode != nullptr)
                {
					fTopLevelNode->setTopLevel(true);
                }

            }
            
            return true;
        }
        
        // We override this here, because we don't want to do anything with the information
        // in any of the top level xml elements
        // Maybe we should hold onto the XMLDECL if it's seen, 
        // so we can know some version information
        void loadFromXmlElement(const XmlElement& elem, IAmGroot* groot) override
        {
        }

        void loadFromXmlIterator(XmlElementIterator& iter, IAmGroot* groot) override
        {
            // By the time we get here, the iterator is already positioned on a ready
            // to use element
            // Do something with that information if we need to
            // before continuing onto other nodes
            do
            {
                // BUGBUG - debug
                //printXmlElement(*iter);

                const XmlElement& elem = *iter;

                switch (elem.kind())
                {
                case XML_ELEMENT_TYPE_START_TAG:                    // <tag>
                    this->loadStartTag(iter, groot);
                    break;

                case XML_ELEMENT_TYPE_END_TAG:                      // </tag>
                    this->loadEndTag(elem, groot);
                    return;

                case XML_ELEMENT_TYPE_SELF_CLOSING:                 // <tag/>
                    this->loadSelfClosingNode(elem, groot);
                    break;

                case XML_ELEMENT_TYPE_CONTENT:                      // <tag>content</tag>
                    this->loadContentNode(elem, groot);
                    break;

                case XML_ELEMENT_TYPE_COMMENT:                      // <!-- comment -->
                    this->loadComment(elem, groot);
                    break;

                case XML_ELEMENT_TYPE_CDATA:                        // <![CDATA[<greeting>Hello, world!</greeting>]]>
                    this->loadCDataNode(elem, groot);
                    break;

                case XML_ELEMENT_TYPE_DOCTYPE:                      // <!DOCTYPE greeting SYSTEM "hello.dtd">
                case XML_ELEMENT_TYPE_ENTITY:                       // <!ENTITY hello "Hello">
                case XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION:       // <?target data?>
                case XML_ELEMENT_TYPE_XMLDECL:                      // <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
                case XML_ELEMENT_TYPE_EMPTY_TAG:                    // <br>
                default:
                {
                    // Ignore anything else
                    //printf("SVGGraphicsElement::loadFromXmlIterator ==> IGNORING kind(%d) name:", elem.kind());
                    //printChunk(elem.nameSpan());
                    //printChunk(elem.data());

                    //printf("SVGGraphicsElement::loadFromXmlIterator ==> IGNORING: %s\n", elem.name().c_str());
                    //printXmlElement(elem);
                }
                break;
                }


            } while (iter.next());

            needsBinding(true);
        }

 
        
        
		// Assuming we've already got a file mapped into memory, load the document
        bool loadFromChunk(const ByteSpan &srcChunk, FontHandler* fh)
        {
            // create a memBuff from srcChunk
            // since we use memory references, we need
            // to keep the memory around for the duration of the 
            // document's life
            // BUGBUG - ideally, we should take this opportunity to expand
            // basic entities, and eliminate whitespace, if that's what we're doing
            // It will cause some performance slow down here, but simplify later
            // processing.

            fSourceMem.initFromSpan(srcChunk);
            //size_t sz = expandXmlEntities(srcChunk, fSourceMem.span());

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
            ctx->setObjectFrame(getBBox());

            this->applyProperties(ctx, groot);
            this->drawSelf(ctx, groot);

            this->drawChildren(ctx, groot);

            ctx->pop();
        }
        

        virtual void draw(IRenderSVG* ctx, IAmGroot* groot, double cWidth, double cHeight)
        {
            canvasSize(cWidth, cHeight);
            
            if (needsBinding())
                this->bindToContext(ctx, groot, cWidth, cHeight);

            ctx->push();
            ctx->setObjectFrame(getBBox());

            this->applyProperties(ctx, groot);
            this->drawSelf(ctx, groot);

            this->drawChildren(ctx, groot);

            ctx->pop();
        }

        static std::shared_ptr<SVGDocument> createFromChunk(const ByteSpan& srcChunk, FontHandler* fh, const double w = 64, const double h = 64, const double ppi = 96)
        {
            // this MUST be done, or node registrations will not happen
            //auto sFactory = getFactory();
            auto doc = std::make_shared<SVGDocument>(fh, w, h, ppi);
            if (!doc->loadFromChunk(srcChunk, fh))
            {
                printf("SVGFactory::CreateFromChunk() failed to load\n");
                return nullptr;
            }
            return doc;
        }

    };

    using SVGDocumentHandle = std::shared_ptr<SVGDocument>;

}


