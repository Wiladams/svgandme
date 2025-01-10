#pragma once

#include "viewport.h"
#include "pubsub.h"

namespace waavs {
	// ViewNavigator
	//
	// Takes UI events, and turns them into document navigation
	// zoom, pan, scale, rotate, etc.
	// It is fed by a scene frame (the frame of the document)
	// and a 'surface' frame, the 'window' through which the document is viewed
	// It returns a transformation matrix that can be applied to the scene frame
	// so that it will fit within the window
	// BUGBUG - should be a topic that things can subscribe to when the transform changes
	// how to subscribe from within a class
	// 		subscribe([this](const T& e) {THandler(e); });
	//
	struct ViewNavigator: public Topic<bool>
	{
	private:
		ViewportTransformer fPortal{};

		bool fIsDragging = false;
		BLPoint fDragPos{ 0,0 };
		
		double fBaseAngle = 2.0;
		double fZoomFactor = 0.1;
		double fSpeedFactor = 1.0;
		
	public:
		void resetNavigator() {
			fPortal.reset();
			fIsDragging = false;
			fDragPos = { 0,0 };
			fZoomFactor = 0.1;
		}

		void setAspectAlign(const PreserveAspectRatio &preserve)
		{
			fPortal.preserveAspectRatio(preserve);
		}
		
		void speedFactor(double newFac)
		{
			fSpeedFactor = newFac;
		}
		double speedFactor() const { return fSpeedFactor; }


		
		// Adjusting zooming factor
		void zoomFactor(double newFac)
		{
			fZoomFactor = newFac;
		}
		double zoomFactor() const { return fZoomFactor; }
		
		// Setting scene and surface frames
		// setting and getting surface frame
		void setFrame(const BLRect& fr) { fPortal.viewportFrame(fr); }
		const BLRect& frame() const { return fPortal.viewportFrame(); }

		// setting and getting viewbox frame
		void setBounds(const BLRect& fr) { fPortal.viewBoxFrame(fr); }
		const BLRect& bounds() const { return fPortal.viewBoxFrame(); }


		BLPoint sceneToSurface(double x, double y) const
		{
			return fPortal.mapViewBoxToViewport(x, y);
		}

		BLPoint surfaceToScene(double x, double y) const
		{
			return fPortal.mapViewportToViewBox(x, y);
		}

		// Retrieving the transformations
		const BLMatrix2D & sceneToSurfaceTransform() const { return fPortal.viewBoxToViewportTransform(); }
		const BLMatrix2D & surfaceToSceneTransform() const { return fPortal.viewportToViewBoxTransform(); }


		void lookAt(double cx, double cy)
		{
			fPortal.lookAt(cx, cy);

			// notify subscribers that the transform changed
			notify(true);
		}
		

		// Actions that will change the transformations
		// Pan
		// This is a translation, so it will move the viewport in the opposite direction
		// of the provided values
		void panTo(double x, double y)
		{
			fPortal.translateTo(x, y);
			notify(true);
		}

		void panBy(double dx, double dy)
		{
			if ((fabs(dx) > dbl_eps) || (fabs(dy) > dbl_eps))
			{
				fPortal.translateBy(-dx, -dy);
				notify(true);
			}

		}

		// zoomBy
		// Zoom in or out, by a specified amount (cumulative)  
		// z >  1.0 ==> zoom out, showing more of the scene
		// z <  1.0 ==> zoom 'in', focusing on a smaller portion of the scene.
		// The zoom can be centered around a specified point
		void zoomBy(double z, double cx = 0, double cy = 0)
		{
			fPortal.scaleBy(z, z, cx, cy);
			notify(true);
		}

		//
		// rotateBy
		// Rotate the scene by a specified angle, around a specified point
		void rotateBy(double r, double cx = 0, double cy = 0)
		{
			fPortal.rotateBy(r, cx, cy);
			notify(true);
		}

		// onMouseEvent
		// 
		// Navigation based on mouse events
		//
		void mouseStartDrag(float x, float y)
		{
			fIsDragging = true;
			fDragPos = { x, y };
		}
		
		void mouseEndDrag(float x, float y)
		{
			fIsDragging = false;
		}
		
		void mouseUpdateDrag(float x, float y)
		{
			auto lastPos = fPortal.mapViewportToViewBox(fDragPos.x, fDragPos.y);
			auto currPos = fPortal.mapViewportToViewBox(x, y);

			double dx = currPos.x - lastPos.x;
			double dy = currPos.y - lastPos.y;

			panBy(dx, dy);
			fDragPos = { x, y };
		}

		// mouse scroll wheel, typically in the top of the mouse, between buttons
		// We want to use the mouse wheel to 'zoom' in and out of the view
		void mouseHandleWheel(float x, float y, float delta)
		{
			if (delta < 0)
				zoomBy(1.0 + (fZoomFactor*fSpeedFactor), x, y);
			else
				zoomBy(1.0 - (fZoomFactor*fSpeedFactor), x, y);
		}
		
		// Horizontal mouse wheel, typpically on the side of the mouse
		// Rotate around central point
		void mouseHandleHWheel(float x, float y, float delta)
		{
			if (delta < 0)
				rotateBy(waavs::radians(fBaseAngle*fSpeedFactor), x, y);
			else
				rotateBy(waavs::radians(-fBaseAngle * fSpeedFactor), x, y);
		}

		void onMouseEvent(const MouseEvent& e)
		{
			switch (e.activity)
			{
				// we can use the xbutton2 and  xbutton1 buttons to increment or decrement speed
				case MOUSEPRESSED:
					if (e.lbutton)
						mouseStartDrag(e.x, e.y);
					else if (e.xbutton2)	// faster
						speedFactor(speedFactor() * 1.2);
					else if (e.xbutton1)	// slower
						speedFactor(speedFactor() * 0.8);
				break;

				case MOUSERELEASED:
					mouseEndDrag(e.x, e.y);
				break;

				case MOUSEMOVED:
				{
					if (fIsDragging)
					{
						mouseUpdateDrag(e.x, e.y);
					}
				}
				break;

				case MOUSEWHEEL:
					mouseHandleWheel(e.x, e.y, e.delta);
				break;

				case MOUSEHWHEEL:
					mouseHandleHWheel(e.x, e.y, e.delta);
				break;

			}

		}

		// Any navigation by keyboard events
		static void onKeyboardEvent(const KeyboardEvent& ke)
		{

		}
	};
}

