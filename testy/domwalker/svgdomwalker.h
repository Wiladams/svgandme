#pragma once

#include <deque>
#include <memory>

#include "svgdocument.h"
#include "svgdrawingstate.h"
#include "svgstructuretypes.h"

namespace waavs {
    // A general purpose DOM walker
    // it maintains state information that facilitates sub-classes
    // who are performing specific actions while they are walking
    // Need to maintain state similar to what IRenderSVG does
    struct SVGDOMWalker
    {
        int fCanvasWidth{ 800 };
        int fCanvasHeight{ 600 };
        int fDPI{ 96 };
        
        // Managing state
        std::deque<SVGDrawingState> fStateStack{};
        SVGDrawingState fCurrentState{};

        SVGDOMWalker(int w, int h, int dpi)
            :fCanvasWidth(w)
            ,fCanvasHeight(h)
            ,fDPI(dpi)
        {

        }
        
        virtual void walk(IAmGroot* groot)
        {
        }
    };


    
	struct SVGRenderWalker : public SVGDOMWalker
	{
		BLImage fImage;
		BLContext fContext;

		SVGRenderWalker(int w, int h, int dpi)
			:SVGDOMWalker(w, h, dpi)
		{
			fImage.create(fCanvasWidth, fCanvasHeight, BL_FORMAT_PRGB32);
			fContext.begin(fImage);
		}

		void walk(IAmGroot* groot) override
		{
			fCurrentState.fClipRect = BLRect(0, 0, fCanvasWidth, fCanvasHeight);
			fCurrentState.fViewport = BLRect(0, 0, fCanvasWidth, fCanvasHeight);
			fCurrentState.fObjectFrame = BLRect(0, 0, fCanvasWidth, fCanvasHeight);

			fStateStack.push_back(fCurrentState);

			//groot->accept(this);

			fStateStack.pop_back();
		}

        std::shared_ptr<SVGDocument> createSVGGraphic() 
        {

        }
	};
}