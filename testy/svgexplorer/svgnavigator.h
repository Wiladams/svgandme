#pragma once

#include "viewport.h"
#include "pubsub.h"

namespace waavs {
	// SVGNavigator
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
	struct SVGNavigator: public Topic<bool>
	{
		ViewPort fPortal{};
		bool fIsDragging = false;
		BLPoint fDragPos{ 0,0 };
		double fZoomFactor = 0.1;

		void resetNavigator() {
			fPortal.reset();
			fIsDragging = false;
			fDragPos = { 0,0 };
			fZoomFactor = 0.1;
		}

		// Setting scene and surface frames
		// setting and getting surface frame
		void surfaceFrame(const BLRect& fr) { fPortal.surfaceFrame(fr); }
		const BLRect& surfaceFrame() const { return fPortal.surfaceFrame(); }

		// setting and getting scene frame
		void sceneFrame(const BLRect& fr) { fPortal.sceneFrame(fr); }
		const BLRect& sceneFrame() const { return fPortal.sceneFrame(); }


		// Retrieving the transformations
		BLMatrix2D sceneToSurfaceTransform() { return fPortal.sceneToSurfaceTransform(); }
		BLMatrix2D surfaceToSceneTransform() { return fPortal.surfaceToSceneTransform(); }


		// Actions that will change the transformations
		// Pan
		// This is a translation, so it will move the viewport in the opposite direction
		// of the provided values
		void pan(double dx, double dy)
		{
			fPortal.translateBy(-dx, -dy);
			notify(true);
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
		void onMouseEvent(const MouseEvent& e)
		{
			MouseEvent lev = e;
			lev.x = float((double)e.x);
			lev.y = float((double)e.y);

			//printf("SVGViewer::mouseEvent: %f,%f\n", lev.x, lev.y);

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
				auto lastPos = fPortal.surfaceToScene(fDragPos.x, fDragPos.y);
				auto currPos = fPortal.surfaceToScene(e.x, e.y);

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

		// Any navigation by keyboard events
		static void onKeyboardEvent(const KeyboardEvent& ke)
		{

		}
	};
}

