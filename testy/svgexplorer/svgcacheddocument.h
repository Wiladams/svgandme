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
			// Draw a background into cache
			ctx->background(BLRgba32(0xffffff00));
			ctx->stroke(BLRgba32(0xffff0000));
			auto fr = frame();
			ctx->strokeLine(0, 0, fr.w, fr.h);
			ctx->strokeLine(0, fr.h, fr.w, 0);
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