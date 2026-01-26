#pragma once


#include "bspan.h"
#include "svgdocument.h"
#include "viewport.h"
#include "graphicview.h"


namespace waavs {

	
	struct SVGCachedDocument : public SVGCachedView
	{
		SVGDocumentHandle fDocument{nullptr};


		SVGCachedDocument(const BLRect& aframe)
			:SVGCachedView(aframe)
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
		
		virtual void resetFromDocument(SVGDocumentHandle doc) noexcept
		{
			// clear context
			fCacheContext.clear();

			fDocument = doc;

			auto sFrame = fDocument->topLevelViewPort();

			setBounds(sFrame);
			setNeedsRedraw(true);
			onDocumentLoad();
		}

		void drawSelf(IRenderSVG* ctx)
		{
			if (nullptr != fDocument) {
				fDocument->draw(ctx, fDocument.get());
			}
		}


	};

}