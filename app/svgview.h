#pragma once

#include "svgsurface.h"
#include "placeable.h"
#include "uievent.h"

//
// SVGView
// A surface that can be drawn anywhere.  Its content is
// an SVGDocument, which can be updated any time.
// The SVGView is a surface, so it has a backing store, which
// is a bitmap.  The encapsulated document is drawn into that bitmap
// When the SVGView is asked to draw, it simply blits the bitmap
// into the context.
//
// The SVGView also implements IViewable, which allows it to handle
// mouse and keyboard operations.
//
namespace waavs
{
	struct SVGView : public SVGSurface, public IViewable
	{
		BLRect fFrame;		// Where will we display our bitmap
		bool fNeedsRedraw{ true };
		
		// Mouse motion within the view
		bool fIsDragging{ false };
		vec2f fDragPos{ 0,0 };
		double fZoomFactor = 0.1;	// 0.1 normally
		
		// This is the thing that is drawn
		std::shared_ptr<ISVGDrawable> fScene = nullptr;

		// These can be called before and after the scene has been drawn
		std::function<void(IRenderSVG* ctx)> fPreRender = nullptr;
		std::function<void(IRenderSVG* ctx)> fPostRender = nullptr;

		
	public:

		//
		// We allow the setting
		//  of a thread count in case the caller wants  to get very specific
		// default to a reasonable number
		SVGView(int x, int y, int w, int h, FontHandler* fh, uint32_t threadCount=4)
			: SVGSurface(fh, w, h, threadCount)
			, fFrame( x,y,w,h )
			, fIsDragging(false)
		{
			// setup the viewport to match the frame initially.
			// This should result in a identity transform
			viewport().sceneFrame(BLRect(0,0,fFrame.w, fFrame.h));
			viewport().surfaceFrame(BLRect(0, 0, fFrame.w, fFrame.h));

		}


		// IPlaceable
		BLRect frame() const override { return fFrame; }
		void moveTo(double x, double y) override {fFrame.x = x;fFrame.y = y;}
		bool contains(double x, double y) override
		{
			return IPlaceable::contains(x, y);
		}
		
		// SVGView related
		// Set the scene to be drawn
		void scene(std::shared_ptr<ISVGDrawable> s) 
		{ 
			fScene = s;
			needsRedraw(true);
		}
		
		const BLRect &sceneFrame(){return viewport().sceneFrame();}
		void sceneFrame(const BLRect &sFrame)
		{
			viewport().sceneFrame(sFrame);
			needsRedraw(true);
		}
		
		// Reset the view to the identity transform
		void resetView()
		{
			viewport().sceneFrame(BLRect(0, 0, fFrame.w, fFrame.h));
			viewport().surfaceFrame(BLRect(0, 0, fFrame.w, fFrame.h));
		}
		

		void needsRedraw(bool needsIt) { fNeedsRedraw = needsIt; }
		bool needsRedraw() const { return fNeedsRedraw; }

		void preRender(std::function<void(IRenderSVG* ctx)> f)
		{
			fPreRender = f;
			needsRedraw(true);
		}

		void postRender(std::function<void(IRenderSVG* ctx)> f)
		{
			fPostRender = f;
			needsRedraw(true);
		}

		
		// snapshot
		// Force the scene to be drawn into our backing store
		virtual void snapshot()
		{
			if (!needsRedraw())
				return;

			// Do whatever before we render the scene
			if (fPreRender)
				fPreRender(this);

			push();

			// Apply viewport's transform before drawing the
			// primary content
			applyTransform(viewport().sceneToSurfaceTransform());

			// draw the scene into the surface
			if (fScene != nullptr)
				fScene->draw(this);

			pop();


			// Do whatever, after scene is rendered
			// This should have the same coordinate system as the scene
			// the preRender
			if (fPostRender)
				fPostRender(this);
			
			flush();
			
			needsRedraw(false);
		}

		// draw the scene
		virtual void drawSelf(IRenderSVG* ctx)
		{
			ctx->image(getImage(), 0, 0);
			ctx->flush();
		}
		
		void draw(IRenderSVG* ctx) override
		{
			snapshot();
			
			ctx->push();

			ctx->translate(fFrame.x, fFrame.y);
			
			drawSelf(ctx);

			ctx->pop();
		}
		


		// Viewport related
		// ICoordinate2DSpace
		// This is for the containing frame, NOT the sceneFrame
		void centerFrame(double cx, double cy)
		{
			double left = cx - (fFrame.w / 2.0);
			double top = cy - (fFrame.h / 2.0);

			//printf("centerFrame: %f %f\n", cx, cy);
			moveTo(left, top);
		}
		
		// Coordinates are in sceneFrame space
		// so calculate new sceneFrame
		virtual void lookAt(double cx, double cy)
		{
			//printf("SVGView::lookAt: %f,%f\n", cx, cy);
			
			BLRect oFrame = viewport().sceneFrame();
			auto w = oFrame.w;
			auto h = oFrame.h;
			oFrame.x = cx - (w / 2.0);
			oFrame.y = cy - (h / 2.0);

			viewport().sceneFrame(oFrame);
		}
		
		// Pan
		// This is a translation, so it will move the viewport in the opposite direction
		// of the provided values
		virtual void pan(double dx, double dy)
		{
			//printf("SVGView::pan(%3.2f, %3.2f)\n", dx, dy);

			viewport().translateBy(-dx, -dy);
			needsRedraw(true);
		}
		
		// zoomBy
		// Zoom in or out, by a specified amount (cumulative)  
		// z >  1.0 ==> zoom out, showing more of the scene
		// z <  1.0 ==> zoom 'in', focusing on a smaller portion of the scene.
		// The zoom can be centered around a specified point
		virtual void zoomBy(double z, double cx = 0, double cy = 0)
		{
			viewport().scaleBy(z, z, cx, cy);
			needsRedraw(true);
		}

		virtual void rotateBy(double r, double cx = 0, double cy = 0)
		{
			viewport().rotateBy(r, cx, cy);
			needsRedraw(true);
		}
		
		
		void mouseEvent(const MouseEvent& e) override
		{
			MouseEvent lev = e;
			lev.x = float((double)e.x - frame().x);
			lev.y = float((double)e.y - frame().y);
			
			//printf("SVGView::mouseEvent: %f,%f\n", lev.x, lev.y);
			
			switch (e.activity)
			{
				// When the mouse is pressed, get into the 'dragging' state
				case MOUSEPRESSED:
					fIsDragging = true;
					fDragPos = { e.x, e.y };
				break;
				
				// When the mouse is released, get out of the 'dragging' state
				case MOUSERELEASED:
					fIsDragging = false;
				break;
				
				case MOUSEMOVED: 
				{
					auto lastPos = viewport().surfaceToScene(fDragPos.x, fDragPos.y);
					auto currPos = viewport().surfaceToScene(e.x, e.y);
					
					//printf("SVGView::mouseEvent - currPos = %3.2f, %3.2f\n", currPos.x, currPos.y);
					
					if (fIsDragging)
					{
						double dx = currPos.x - lastPos.x;
						double dy = currPos.y - lastPos.y;
						
						// print mouse lastPos and currPos
						//printf("-----------------------------\n");
						//printf("lastPos = %3.2f, %3.2f\n", lastPos.x, lastPos.y);

						//printf("dx = %3.2f, dy = %3.2f\n", dx, dy);
						
						pan(dx, dy);
						fDragPos = { e.x, e.y };
					}
				}
				break;
				
				// We want to use the mouse wheel to 'zoom' in and out of the view
				// We will only zoom when the 'alt' key is pressed
				// Naked scroll might be used for something else
				case MOUSEWHEEL:
				{
					//printf("SVGView: MOUSEWHEEL\n");
					bool menuPressed =  (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

					
					if (!menuPressed)
						return;
					
					if (e.delta < 0)
						zoomBy(1.0 + fZoomFactor, lev.x, lev.y);
					else
						zoomBy(1.0 - fZoomFactor, lev.x, lev.y);
				}
				break;
				
				// Horizontal mouse wheel
				// Rotate around central point
				case MOUSEHWHEEL:
				{
					//printf("SVGView: MOUSEHWHEEL\n");
					if (e.delta < 0)
						rotateBy(waavs::radians(5.0f), lev.x, lev.y);
					else
						rotateBy(waavs::radians(-5.0f), lev.x, lev.y);
				}
				break;
			}
		}
		
	};
}
