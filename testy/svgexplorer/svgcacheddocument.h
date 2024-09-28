#pragma once

// SVGSprite
// A struct that takes an svg string and renders it to a bitmap

#include "bspan.h"
#include "svgdocument.h"
#include "viewport.h"


namespace waavs {

	struct SVGCachedDocument : public SVGViewable
	{
		BLImage fCachedImage{};
		BLRect fUIFrame{};
		BLMatrix2D fSceneToSurfaceTransform{};
		bool fNeedsRedraw{ true };
		
		SVGDocumentHandle fDocument{nullptr};

		
		
		SVGCachedDocument() :SVGViewable(nullptr) {}
		SVGCachedDocument(const BLRect& aframe)
			:SVGViewable(nullptr)
		{
			uiFrame(aframe);
		}
		
		void needsRedraw(const bool needsIt) { fNeedsRedraw = needsIt; }
		bool needsRedraw() const { return fNeedsRedraw; }
		
		void uiFrame(const BLRect& aframe) 
		{
			fUIFrame = aframe;
			fCachedImage.reset();
			fCachedImage.create(fUIFrame.w, fUIFrame.h, BL_FORMAT_PRGB32);
		}
		const BLRect & uiFrame() const { return fUIFrame; }
		

		virtual void onDocumentLoad()
		{

		}
		
		virtual void resetFromDocument(SVGDocumentHandle doc, FontHandler* fh = nullptr) noexcept
		{
			fDocument = doc;
			needsRedraw(true);
			onDocumentLoad();
			
			drawIntoCache();
		}
		
		// Transformation used to map from scene to our backing buffer surface
		const BLMatrix2D & sceneToSurfaceTransform() const { return fSceneToSurfaceTransform; }
		void sceneToSurfaceTransform(const BLMatrix2D& tform) { fSceneToSurfaceTransform = tform; }
		
		// drawIntoCache()
		// 
		// draw the document into the backing buffer
		//
		virtual void drawBackgroundIntoCache(IRenderSVG &ctx)
		{
			;
		}
		
		virtual void drawIntoCache() 
		{
			// Create a drawing context
			// And setup the transform
			IRenderSVG drawingContext(nullptr);
			drawingContext.begin(fCachedImage);
			drawingContext.renew();
			drawingContext.clear();
			
			drawBackgroundIntoCache(drawingContext);

			// Draw the document, after applying the transform
			drawingContext.setTransform(sceneToSurfaceTransform());

			if (nullptr != fDocument) {
				drawingContext.fontHandler(fDocument->fontHandler());
				fDocument->draw(&drawingContext, fDocument.get());
			}
			
			drawingContext.flush();

			needsRedraw(false);
		}
		
		void moveTo(const BLPoint& pt) noexcept
		{
			fUIFrame.x = pt.x;
			fUIFrame.y = pt.y;
		}
		
		void draw(IRenderSVG* ctx, IAmGroot* groot) override 
		{
			if (needsRedraw())
			{
				drawIntoCache();
				needsRedraw(false);
			}
			
			// just do a blt of the cached image
			ctx->image(fCachedImage, static_cast<int>(fUIFrame.x), static_cast<int>(fUIFrame.y));
			
			return; 
		}

		void draw(IRenderSVG* ctx)
		{
			draw(ctx, fDocument.get());
		}
	};

}