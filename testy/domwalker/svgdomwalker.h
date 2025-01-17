#pragma once

#include <deque>
#include <memory>

#include "svgdocument.h"
#include "svgdrawingstate.h"
#include "svgstructuretypes.h"
#include "svggraphic.h"

namespace waavs {

    // SVGStateManager
    // Manages the drawing state for those who are walking
    // A SVG DOM
    // Does what a IRenderSVG does, but doesn't actually  
    // do any drawing.
    // Once this object exists, the IRenderSVG should use
    // it to manage its state
    struct SVGStateManager
    {

        // Managing state
        std::deque<SVGDrawingState> fStateStack{};
        SVGDrawingState fCurrentState{};

        SVGStateManager()

        {

        }
        
        virtual void walk(SVGDocumentHandle doc)
        {
        }
    };


    
	struct SVGGraphicWalker : public SVGStateManager
	{
        int fCanvasWidth{ 800 };
        int fCanvasHeight{ 600 };
        int fDPI{ 96 };
        

		FontHandler *fFontHandler;
        
        SVGGraphicWalker(int w, int h, FontHandler *fh=nullptr, int dpi=96)
			:SVGDOMWalker(w, h, dpi)
		{
			fImage.create(fCanvasWidth, fCanvasHeight, BL_FORMAT_PRGB32);
			fContext.begin(fImage);
		}

        void setViewport(const BLRect& r) {
            fCurrentState.fViewport = r;
        }
        BLRect viewport() const { return fCurrentState.fViewport; }
        
        
        int canvasWidth() const { return fCanvasWidth; }
        int canvasHeight() const { return fCanvasHeight; }
        
		void walk(SVGDocumentHandle doc) override
		{
			fCurrentState.fClipRect = BLRect(0, 0, canvasWidth(), canvasHeight());
			fCurrentState.fViewport = BLRect(0, 0, canvasWidth(), canvasHeight());
			fCurrentState.fObjectFrame = BLRect(0, 0, canvasWidth(), canvasHeight());

			fStateStack.push_back(fCurrentState);

			//groot->accept(this);

			fStateStack.pop_back();
		}

        std::shared_ptr<AGraphicGroup> createSVGGraphic(SVGDocumentHandle doc, int cWidth, int cHeight)
        {
            ctx->setViewport(BLRect(0, 0, cWidth, cHeight));

            ctx->push();

            this->fixupStyleAttributes(ctx, groot);
            convertAttributesToProperties(ctx, groot);

            this->bindSelfToContext(ctx, groot);
            this->bindChildrenToContext(ctx, groot);

            ctx->pop();
        }
	};
}