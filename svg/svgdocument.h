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


#include "xmlscan.h"
#include "xmlentity.h"

#include "fonthandler.h"
#include "svgcss.h"

#include "svgclip.h"
#include "svgconditional.h"
#include "svgdefs.h"
#include "svggradient.h"
#include "svgimage.h"
#include "svgmarker.h"
#include "svgmask.h"
#include "svgstructure.h"
#include "svgshapes.h"
#include "svgsolidcolor.h"
#include "svgstyle.h"
#include "svgsymbol.h"
#include "svgpattern.h"
#include "svgfont.h"
#include "svgfilter.h"
#include "svghyperlink.h"
#include "svgflowroot.h"
#include "svguse.h"



namespace waavs 
{
    // SVGDocument
    // 
    // This is the structure that represents an entire SVG file/document
    // Each document may contain several top level <svg> elements
    // But, you typically have only one root <svg> element.
    // 
    // This is not a generic DOM structure, but rather a structure
    // meant for rendering SVG content.  A separate DOM structure
    // can be created if what you want is to just traverse the XML tree.
    // An SVGDocument is meant to be rendered to a canvas, so it is constructed
    // with items such as the canvas size, dpi, etc.
    // 
    // The primary method to create an SVGDocument is to use the static
    // createFromChunk(const ByteSpan& srcChunk, const double w, const double h, const double ppi)
    // 
    // Which will return a shared_ptr to an SVGDocument, or nullptr if there was
    // any error.
    // 
    // As the core document makes use of ByteSpan references into memory,
    // the initial source memory is copied into a MemBuff that is held
    // onto for the life of the document.
    //
    struct SVGDocument : public  SVGGraphicsElement, public IAmGroot 
    {
        MemBuff fSourceMem{};
                
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
        // Construction / Destruction
		//==========================================
        SVGDocument() = default;
        
        SVGDocument(const double w, const double h, const double ppi)
            :SVGGraphicsElement()
            , fDpi(ppi)
            , fCanvasWidth(w)
            , fCanvasHeight(h)
        {
            // Create document style sheet that can be filled in
			fStyleSheet = std::make_shared<CSSStyleSheet>();
        }

        ~SVGDocument() = default;

        
        void resetFromSpan(const ByteSpan& srcChunk, const double w, const double h, const double ppi=96)
        {
            // clear out the old document
            //clear();

            setDpi(ppi);
            canvasSize(w, h);
            fStyleSheet = std::make_shared<CSSStyleSheet>();

            // load the new document
            loadFromChunk(srcChunk);
        }
        
        
        double dpi() const override { return fDpi; }
		void setDpi(const double d) override { fDpi = d; }
        
		double canvasWidth() const override { return fCanvasWidth; }
		double canvasHeight() const override { return fCanvasHeight; }
        void canvasSize(const double w, const double h) { fCanvasWidth = w; fCanvasHeight = h; }
        
        BLRect viewPort() const override
        {
            if (fTopLevelNode != nullptr)
            {
                return fTopLevelNode->viewPort();
            }

            if (fCanvasWidth <= 0 || fCanvasHeight <= 0)
                return BLRect{};

            BLRect outFr(0, 0, fCanvasWidth, fCanvasHeight);
            return outFr;
        }


        std::shared_ptr<CSSStyleSheet> styleSheet() override { return fStyleSheet; }
        void styleSheet(std::shared_ptr<CSSStyleSheet> sheet) override { fStyleSheet = sheet; }
        

        // retrieve root svg node
		std::shared_ptr<SVGSVGElement> documentElement() const { return fTopLevelNode; }


        bool addNode(std::shared_ptr < IViewable > node, IAmGroot* groot) override
        {            
			if (!SVGGraphicsElement::addNode(node, groot))
                return false;

            if (!fTopLevelNode && node->name() == "svg")
            {
                fTopLevelNode = std::dynamic_pointer_cast<SVGSVGElement>(node);
                
                if (fTopLevelNode != nullptr)
                {
                    fTopLevelNode->setTopLevel(true);
                }
            }
            
            return true;
        }
        
        void onDocumentLoaded(IAmGroot* groot)
        {
            if (!groot)
                return;

            // 1) Normal DOM pass to resolve styles
            this->resolveStyleSubtree(groot);

            //primeResourceNodes(groot);
        }

        void primeResources(IAmGroot* groot)
        {
            for (auto& node : fNodes)
            {
                //node->primeResources(groot);
            }
        }

        // We override this here, because we don't want to do anything with the information
        // in any of the top level xml elements
        // Maybe we should hold onto the XMLDECL if it's seen, 
        // so we can know some version and encoding info?
        void loadFromXmlElement(const XmlElement& elem, IAmGroot* groot) override
        {
        }

        void loadFromXmlPull(XmlPull& iter, IAmGroot* groot) override
        {
            while (iter.next())
            {
                const XmlElement& elem = iter.fCurrentElement;

                switch (elem.kind())
                {
                case XML_ELEMENT_TYPE_START_TAG:                    // <tag>
                    this->loadStartTag(iter, groot);
                    break;
                case XML_ELEMENT_TYPE_END_TAG:                      // </tag>
                    this->loadEndTag(elem, groot);
                    onEndTag(groot);
                    break;
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
                }
                break;
                }
            }

            setNeedsBinding(true);
            onDocumentLoaded(groot);
        }
 
        
        // loadFromChunk
        // 
		// Assuming we've already got a document mapped into memory
        // construct the SVGDocument from the given memory chunk
        bool loadFromChunk(const ByteSpan &srcChunk)
        {
            // create a memBuff from srcChunk
            // since we use memory references, we need
            // to keep the memory around for the duration of the 
            // document's life
            

            fSourceMem.resetFromSpan(srcChunk);
            
            // BUGBUG - It would be nice to take this opportunity to convert basic XML
            // entities such as &amp; &lt; &gt; &quot; &#39; into their
            // byte values.  It could simplify later processing.
            // But... When to convert the entities is dependent on context
            // So, it might be ok to do it early here, except, when doing XML 
            // processing, having the entities converted will cause problems for the scanner.
            // Although it seems like this would be a great place to do this, it's probably 
            // better to either do it only when it matters, or at the scanner level.
            //fSourceMem.resetFromSize(srcChunk.size());
            //ByteSpan dstSpan = fSourceMem.span();
            //size_t sz = expandXmlEntities(srcChunk, dstSpan);

			// Create the XML Iterator we're going to use to parse the document
            XmlPull iter(fSourceMem.span(), true);
            loadFromXmlPull(iter, this);
            
            return true;
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
        
        // Binding to a context gives the document tree a chance to fixup
        // relative sizing, as well as object references
        // 
        void bindToContext(IRenderSVG* ctx, IAmGroot* groot) noexcept override
        {
            BLRect vpFrame(0, 0, fCanvasWidth, fCanvasHeight);

            ctx->setViewport(vpFrame);
            ctx->setObjectFrame(vpFrame);

            this->bindSelfToContext(ctx, groot);
            this->bindChildrenToContext(ctx, groot);

            setNeedsBinding(false);
        }


        void drawSelf(IRenderSVG* ctx, IAmGroot*) override
        {
            // draw horizontal blue line at 10,10 for 300
            //ctx->strokeLine(10, 10, 300, 10, BLRgba32(0xff0000ff));
            //ctx->strokeLine(10, 10, 10, 300, BLRgba32(0xffff0000));
        }
        
        void draw(IRenderSVG* ctx, IAmGroot* groot) override
        {        
            if (needsBinding())
                this->bindToContext(ctx, groot);

            BLRect vpFrame = viewPort();
            //BLRect bbox = getBBox();

            ctx->push();

            // To start, we need to set the viewport and object frame
            // on the context, so binding can get the right sizes to start

            ctx->setViewport(vpFrame);
            ctx->setObjectFrame(vpFrame);

            this->drawSelf(ctx, groot);
            this->drawChildren(ctx, groot);

            ctx->pop();
        }


        static std::shared_ptr<SVGDocument> createFromChunk(const ByteSpan& srcChunk, const double w, const double h, const double ppi)
        {
            auto doc = std::make_shared<SVGDocument>(w, h, ppi);
            if (!doc->loadFromChunk(srcChunk))
            {
                printf("SVGFactory::CreateFromChunk() failed to load\n");
                return nullptr;
            }
            return doc;
        }

    };

    using SVGDocumentHandle = std::shared_ptr<SVGDocument>;

}


