#pragma once

#include "irendersvg.h"
#include "viewport.h"
#include "uievent.h"

namespace waavs {
	//
	// GraphicView
	//
	// The primary purpose of this class is to act as a basis for any visuals
	// the user can interact with.  There are a few primary things it provides
	// interfaces for
	// 
	// 1) Input device handling - Mouse, Keyboard, Joystick, Touch
	//		onMouseEvent()
	//		onKeyboardEvent()
	// 2) Drawing to a context - an SVG aware context
	//		draw()
	// 3) Mapping between its position in the world space, and local coordinate space
	//		frame()
	//		bounds()
	//
	struct GraphicView
	{
	private:
		BLMatrix2D fSceneToSurfaceTransform{};
		BLMatrix2D fSurfaceToSceneTransform{};

		BLRect fFrame{};
		BLRect fBounds{};

	public:
		GraphicView()
		{
		}

		GraphicView(const BLRect& aframe)
			: fFrame(aframe)
			, fBounds({ 0,0,aframe.w, aframe.h })
		{
			fSceneToSurfaceTransform.reset();
			fSurfaceToSceneTransform.reset();
		}

		// Transformation used to map from scene to our backing buffer surface
		const BLMatrix2D& sceneToSurfaceTransform() const { return fSceneToSurfaceTransform; }
		void setSceneToSurfaceTransform(const BLMatrix2D& tform) { fSceneToSurfaceTransform = tform; }


		const BLRect & frame() const { return fFrame; }
		virtual void setFrame(const BLRect& arect) { fFrame = arect;  }


		const BLRect & bounds() const { return fBounds; }
		virtual void setBounds(const BLRect& arect) { fBounds = arect;  }

		virtual bool contains(double x, double y) {
			const auto & fr = frame();
			return (x >= fr.x && x < fr.x + fr.w && y >= fr.y && y < fr.y + fr.h);
		}

		virtual void onMouseEvent(const MouseEvent& e){}

		virtual void onKeyboardEvent(const KeyboardEvent& ke){}

		// For animation
		void onFrameEvent(const FrameCountEvent& fe) {}

		// This is meant to be rendered without any transformation applied
		// except translation based on our frame offset
		virtual void drawBackground(IRenderSVG* ctx){}

		// This is where the content should be drawn
		virtual void drawSelf(IRenderSVG* ctx){}

		virtual void drawForeground(IRenderSVG* ctx){}

		virtual void draw(IRenderSVG* ctx)
		{
			BLRect fr = frame();


			ctx->push();
			ctx->translate(fr.x, fr.y);
			ctx->clipToRect(BLRect(0,0,fr.w, fr.h));
			drawBackground(ctx);
			ctx->noClip();
			ctx->pop();

			// Apply viewport transformation
			ctx->push();
			ctx->translate(fr.x, fr.y);
			ctx->clipToRect(BLRect(0, 0, fr.w, fr.h));
			//ctx->setTransform(sceneToSurfaceTransform());
			drawSelf(ctx);
			ctx->noClip();
			ctx->pop();

			ctx->push();
			ctx->translate(frame().x, frame().y);
			ctx->clipToRect(BLRect(0, 0, fr.w, fr.h));
			drawForeground(ctx);
			ctx->noClip();
			ctx->pop();

		}
	};
}

namespace waavs {
	struct SVGCachedView : public GraphicView
	{
		BLImage fCachedImage{};
		IRenderSVG fCacheContext{ nullptr };

		bool fNeedsRedraw{ true };

		SVGCachedView(const BLRect& aframe, FontHandler *fh)
			:GraphicView(aframe)
		{
			fCacheContext.fontHandler(fh);

			setFrame(aframe);
		}

		virtual ~SVGCachedView() = default;

		void setNeedsRedraw(const bool needsIt) { fNeedsRedraw = needsIt; }
		bool needsRedraw() const { return fNeedsRedraw; }

		void setFrame(const BLRect& arect) override
		{
			GraphicView::setFrame(arect);
			fCachedImage.reset();
			fCachedImage.create(static_cast<int>(arect.w), static_cast<int>(arect.h), BL_FORMAT_PRGB32);
			fCacheContext.begin(fCachedImage);
			fCacheContext.fontFamily("Arial");
			fCacheContext.setViewport(arect);
			
			setNeedsRedraw(true);
		}


		void draw(IRenderSVG* ctx) override
		{
			if (needsRedraw())
			{
				fCacheContext.renew();
				fCacheContext.clear();

				BLRect fr = frame();
				
				//ctx->clipToRect(fr);

				//ctx->push();
				//ctx->translate(-frame().x, -frame().y);
				drawBackground(&fCacheContext);
				//ctx->pop();

				// Apply viewport transformation
				fCacheContext.push();
				//ctx->translate(frame().x, frame().y);
				fCacheContext.setTransform(sceneToSurfaceTransform());
				drawSelf(&fCacheContext);
				fCacheContext.pop();

				//ctx->push();
				//ctx->translate(frame().x, frame().y);
				drawForeground(&fCacheContext);
				//ctx->pop();

				//ctx->noClip();



				setNeedsRedraw(false);
			}

			// just do a blt of the cached image
			BLRect fr = frame();
			ctx->image(fCachedImage, static_cast<int>(fr.x), static_cast<int>(fr.y));
		}
	};
}