#pragma once

#include "svg/irendersvg.h"
#include "svg/svgsurface.h"
#include "core/viewport.h"


#include "core/fonthandler.h"
#include "user32pixelmap.h"

#include <functional>
#include <memory>

namespace waavs
{

	// 
	// A camera renders a scene.
	// The camera also controls the window through which you see the scene
	// so you can zoom, pan, and rotate the view.
	//
	struct Camera2D : public ISVGDrawable
	{
		User32PixelMap fPixelMap{};
		SVGSurface fSurface;
		bool fNeedsRedraw{ true };
		
		// This is the thing that will be drawn
		std::shared_ptr<ISVGDrawable> fScene = nullptr;
		std::function<void(IRenderSVG* ctx)> fPreRender = nullptr;
		std::function<void(IRenderSVG* ctx)> fPostRender = nullptr;
		
		
		// The initial viewport
		SVGViewPort fViewport;
		
		Camera2D(FontHandler *fh)
			: fSurface(fh)
		{
		}
		
		Camera2D(double w, double h, FontHandler* fh)
			:fSurface(fh)
			,fViewport(0,0,w,h)
		{
			fPixelMap.init((int)w, (int)h);
			fSurface.attachPixelArray(fPixelMap,8);
			fSurface.textFont("Arial");
		}

		void init(int w, int h)
		{
			fPixelMap.init(w, h);
			fSurface.attachPixelArray(fPixelMap);
			needsRedraw(true);
		}
		
		void worldFrame(const rectf& fr) { fViewport.worldFrame(fr); needsRedraw(true); }
		const rectf& worldFrame() const { return fViewport.worldFrame(); }

		void objectFrame(const rectf& fr) { fViewport.objectFrame(fr); needsRedraw(true);}
		const rectf& objectFrame() const { return fViewport.objectFrame(); }
		
		
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
		
		// Set the scene that we will be drawing
		// Allow for adjusting the viewport as well
		void scene(std::shared_ptr<ISVGDrawable> s, const rectf& objFrame)
		{
			fViewport.objectFrame(objFrame);
			fScene = s;
			needsRedraw(true);

			snapshot();
		}

		std::shared_ptr<ISVGDrawable> scene() { return fScene; }

		// Take a snapshot of the scene into the camera's pixelmap
		void snapshot()
		{
			if (!needsRedraw())
				return;
			
			// print objectFrame()
			//printf("Camera2D snapshot:  objectFrame = %f, %f, %f, %f\n", objectFrame().x, objectFrame().y, objectFrame().w, objectFrame().h);
			
			fSurface.push();

			// Do whatever before we render the scene
			if (fPreRender)
				fPreRender(&fSurface);

			fSurface.push();
			// Allow the viewport to alter the surface before drawing
			// anything else
			fViewport.draw(&fSurface);
			//fSurface.exec(fViewport);

			// draw the scene into the surface
			if (fScene != nullptr)
				fScene->draw(&fSurface);

			fSurface.flush();
			
			fSurface.pop();


			// Do whatever, after scene is rendered
			// This should have the same coordinate system as the scene
			// the preRender
			if (fPostRender)
				fPostRender(&fSurface);
			
			fSurface.flush();
			
			fSurface.pop();

			needsRedraw(false);
		}

		
		virtual void drawSelf(IRenderSVG* ctx)
		{
			// draw the scene
			auto fr = fViewport.worldFrame();
			ctx->image(fSurface.getImage(), (int)fr.x, (int)fr.y);
		}


		virtual vec2f worldToObject(float x, float y)
		{
			return fViewport.worldToObject(x, y);
		}
		
		// Coordinates are in object space
		// so calculate new objectFrame
		virtual void lookAt(float cx, float cy)
		{
			waavs::rectf oFrame = fViewport.objectFrame();
			auto w = oFrame.w;
			auto h = oFrame.h;
			oFrame.x = cx - w / 2;
			oFrame.y = cy - h / 2;
			
			objectFrame(oFrame);
		}
		
		// zoomBy
		// Zoom in or out, by a specified amount (cumulative)  
		// z >  1.0 ==> zoom out, showing more of the scene
		// z <  1.0 ==> zoom 'in', focusing on a smaller portion of the scene.
		// The zoom can be centered around a specified point
		virtual void zoomBy(float z, float cx = 0, float cy = 0)
		{
			fViewport.scaleBy(z, z, cx, cy);
			needsRedraw(true);
		}
		
		virtual void rotateBy(double r, double cx = 0, double cy = 0)
		{
			fViewport.rotateBy(r, cx, cy);
			needsRedraw(true);
		}
		
		// Pan
		// This is a translation, so it will move the viewport in the opposite direction
		// of the provided values
		virtual void pan(double dx, double dy)
		{
			//printf("Camera2D::pan(%3.2f, %3.2f)\n", dx, dy);
			
			fViewport.translateBy(-dx, -dy);
			needsRedraw(true);
		}
		
	};


}