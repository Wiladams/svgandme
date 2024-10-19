#pragma once

#include "blend2d.h"

namespace waavs {
	//
	// Viewport
	// The Viewport represents the mapping between one 2D coordinate system and another.
	// 
	// The surfaceFrame, is where the image is projected.  This is typically
	// the actual window on the screen the user is interacting with.
	// 
	// The sceneFrame, is the content that is being looked at.  So, if you're looking
	// through a window as a painting outside, the sceneFrame is the painting.
	// The 'scene' is assumed to be an infinite canvas.  The 'sceneFrame' is the portion
	// of that infinite canvas you want to actually show up within the surfaceFrame.
	// 
	// Interesting operations:
	// 1) If you have a bounded thing, like a bitmap, and you want it to fill the surface
	// you can specify the sceneFrame(BLRect(0,0,,img.width, img.height))
	// 2) If you want to do some 'scolling', you send a sceneFrame to equal the size
	// of the surfaceFrame initially, then use the translateBy(), and translateTo() functions
	// to adjust the position of that boundary against the scene you're scrolling.
	// 
	// The viewport allows you to do typical 'camera' kinds of movements, like pan, zoom,
	// lookAt, and the like, and returns you the transformation matrix that will
	// be applied to a drawing context.
	//
	struct ViewPort
	{
	private:
		BLMatrix2D fTransform = BLMatrix2D::makeIdentity();
		BLMatrix2D fInverseTransform = BLMatrix2D::makeIdentity();
		
		// For rotation
		double fRotRad = 0.0;		// Number of radians rotated
		BLPoint fRotCenter{};		// Point around which we rotate

		BLRect fSurfaceFrame{};		// Coordinate system we are projecting onto
		BLRect fSceneFrame{};		// Coordinate system of scene we are projecting



	public:
		ViewPort() {
			// Default to a 1:1 mapping
			fSceneFrame = BLRect(0, 0, 1, 1); 
			fSurfaceFrame = BLRect(0, 0, 1, 1);
		}
		
		
		// 		ViewPort(const ViewPort& other) = delete;
		
		ViewPort(const ViewPort& other)
			: fTransform(other.fTransform)
			, fInverseTransform(other.fInverseTransform)
			, fRotRad(other.fRotRad)
			, fRotCenter(other.fRotCenter)
			, fSurfaceFrame(other.fSurfaceFrame)
			, fSceneFrame(other.fSceneFrame)
		{
		}



		ViewPort(const BLRect& aSurfaceFrame, const BLRect& aSceneFrame)
			:fSurfaceFrame(aSurfaceFrame)
			, fSceneFrame(aSceneFrame)
		{
			calcTransform();
		}
		
		ViewPort(double x, double y, double w, double h)
			:fSurfaceFrame{ (float)x, (float)y, (float)w, (float)h }
			, fSceneFrame{0,0,w,h}
		{
			calcTransform();
		}
		
		// Start with these deleted until we know how
		// to use them.

		ViewPort& operator=(const ViewPort& other) = delete;
		
		virtual ~ViewPort() = default;



		virtual void reset() {
			fTransform.reset();
			fInverseTransform.reset();
			fRotRad = 0.0;
			fRotCenter = BLPoint{};
			fSurfaceFrame = BLRect(0, 0, 1, 1);
			fSceneFrame = BLRect(0, 0, 1, 1);
		}

		// retrieve the calculated transform
		// Usage:
		// Use this transformation when you need to draw the scene into a context
		const BLMatrix2D& sceneToSurfaceTransform() const {
			// print the fTransform values
			//printf("ViewPort::sceneToSurfaceTransform: %f %f %f %f %f %f\n", fTransform.m[0], fTransform.m[1], fTransform.m[2], fTransform.m[3], fTransform.m[4], fTransform.m[5]);
			return fTransform; 
		}
		const BLMatrix2D& surfaceToSceneTransform() const { return fInverseTransform; }
		
		// setting and getting surface frame
		void surfaceFrame(const BLRect& fr) { fSurfaceFrame = fr; calcTransform(); }
		const BLRect& surfaceFrame() const { return fSurfaceFrame; }

		// setting and getting scene frame
		void sceneFrame(const BLRect& fr) { fSceneFrame = fr; calcTransform(); }
		const BLRect& sceneFrame() const { return fSceneFrame; }

		
		// Convert a point from the scene to the surface
		BLPoint sceneToSurface(double x, double y) const
		{
			BLPoint pt = fTransform.mapPoint(x, y);
			return pt;
		}
		
		// Convert a point from the surface to the scene
		BLPoint surfaceToScene(double x, double y) const
		{
			BLPoint pt = fInverseTransform.mapPoint(x, y );
			return pt;
		}

		// An internal scale calculator
		// Creates scale from scene to surface
		// Calculate a scale, returning the minimal
		// value that preserves aspect ratio.
		double trueScale(BLPoint &ascale) const
		{
			double tScale = 1.0;

			ascale.x = (fSurfaceFrame.w / fSceneFrame.w);
			ascale.y = (fSurfaceFrame.h / fSceneFrame.h);
			tScale = std::abs(std::min(ascale.x, ascale.y));
			
			return tScale;
		}

		BLPoint fullScale(BLPoint& ascale) const
		{
			double tScale = 1.0;

			ascale.x = (fSurfaceFrame.w / fSceneFrame.w);
			ascale.y = (fSurfaceFrame.h / fSceneFrame.h);

			return ascale;
		}

		
		void calcTransform(bool freeAspect = false)
		{
			BLPoint ascale(1, 1);
			double tScale = trueScale(ascale);
			auto fScale = fullScale(ascale);

			
			fTransform = BLMatrix2D::makeIdentity();
			

			// Translate by surfaceFrame amount first
			fTransform.translate(fSurfaceFrame.x, fSurfaceFrame.y);
			
			// Rotate
			fTransform.rotate(fRotRad, fRotCenter);
			

			// Scale
			if (freeAspect)
				fTransform.scale(fScale.x, fScale.y);
			else
				fTransform.scale(tScale, tScale);


			// Translate scene frame amount
			double tranX = -fSceneFrame.x;
			double tranY = -fSceneFrame.y;
			fTransform.translate(tranX, tranY);
			
			// Calculate the inverse transform
			// so we can convert from world space to object space
			fInverseTransform = fTransform;
			fInverseTransform.invert();
		}
		
		
		// This will translate relative to the current x, y position
		bool translateTo(double x, double y)
		{
			fSceneFrame.x = x;
			fSceneFrame.y = y;
			calcTransform();

			return true;
		}

		bool translateBy(double dx, double dy)
		{
			return translateTo(fSceneFrame.x + dx, fSceneFrame.y + dy);
		}

		bool scaleObjectFrameBy(double sx, double sy, double centerx, double centery)
		{
			fSceneFrame.x = float(centerx + ((double)fSceneFrame.x - centerx) * sx);
			fSceneFrame.y = float(centery + ((double)fSceneFrame.y - centery) * sy);
			fSceneFrame.w *= (float)sx;
			fSceneFrame.h *= (float)sy;

			return true;
		}

		// This does a scale relative to a given point
		// You must provide a center point, which indicates
		// where the scaling is relative to.
		// The routine will do the necessary translations, so the center
		// point remains in the center.
		bool scaleBy(double sdx, double sdy, double cx, double cy)
		{
			//printf("ViewRect::scaleBy: %3.3f, %3.3f, %3.3f, %3.3f\n", sx, sy, centerx, centery);
			BLPoint apoint{};
			double tScale = trueScale(apoint);
			double x = fSceneFrame.x + (cx - fSurfaceFrame.x) / tScale;
			double y = fSceneFrame.y + (cy - fSurfaceFrame.y) / tScale;
			double w = fSceneFrame.w / sdx;
			double h = fSceneFrame.h / sdy;

			scaleObjectFrameBy(sdx, sdy, x, y);
			calcTransform();

			//printf("SVGViewBox::scaleBy(2): %3.3f, %3.3f, %3.3f, %3.3f\n", fRect.x, fRect.y, fRect.w, fRect.h);
			return true;
		}

		bool rotateBy(double rad, double cx, double cy)
		{
			fRotRad += rad;
			fRotCenter = { cx, cy };
			calcTransform();

			return true;
		}

		// Calculate the transforms, based on the currently set 
		// values for the two frames, and the rotation
		void printTransform()
		{
			BLPoint ascale(1, 1);

			printf("ViewPort::print\n");
			printf("   Rotation: %f (%f,%f)\n", fRotRad, fRotCenter.x, fRotCenter.y);
			printf("      Scale: %3.4f\n", trueScale(ascale));
			printf("  Translate: %3.4f, %3.4f\n", fSceneFrame.x, fSceneFrame.y);
		}
	};
}

