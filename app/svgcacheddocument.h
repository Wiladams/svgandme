#pragma once

// SVGSprite
// A struct that takes an svg string and renders it to a bitmap

#include "bspan.h"
#include "svgdocument.h"
#include "viewport.h"
#include "graphicview.h"


namespace waavs {

	
	struct SVGCachedDocument : public SVGCachedView
	{
		SVGDocumentHandle fDocument{nullptr};


		SVGCachedDocument(const BLRect& aframe, FontHandler *fh=nullptr)
			:SVGCachedView(aframe, fh)
		{
		}
		
		virtual ~SVGCachedDocument() = default;

		void moveTo(const BLPoint& pt) noexcept
		{
			auto fr = frame();
			fr.x = pt.x;
			fr.y = pt.y;
			setFrame(fr);
		}


		virtual void onFrameEvent(const FrameCountEvent& fe)
		{
			if (fDocument != nullptr)
			{
				setNeedsRedraw(true);
				fDocument->update(fDocument.get());
			}
		}
		
		virtual void onDocumentLoad()
		{
			// clear background
		}
		
		virtual void resetFromDocument(SVGDocumentHandle doc, FontHandler* fh = nullptr) noexcept
		{
			// clear context
			fCacheContext.clear();

			fDocument = doc;

			auto sFrame = fDocument->frame();
			//auto sFrame = fDocument->getBBox();
			setBounds(sFrame);
			setNeedsRedraw(true);
			onDocumentLoad();
		}
		
		/*
		void drawBackground(IRenderSVG *ctx) override
		{	
		}
		*/

		void drawSelf(IRenderSVG* ctx)
		{
			if (nullptr != fDocument) {
				ctx->fontHandler(fDocument->fontHandler());
				fDocument->draw(ctx, fDocument.get());
			}
		}


	};

}