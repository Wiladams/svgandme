#pragma once

#include "blend2d/blend2d.h"

namespace waavs {
	//
	// Viewport
	// The Viewport represents the mapping between one 2D coordinate system and another.
	// 
	// The surfaceFrame, is where the image is projected.
	// The sceneFrame, is the content that is being looked at.
	// 
	// 
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
		
		ViewPort(double x, double y, double w, double h)
			:fSurfaceFrame{ (float)x, (float)y, (float)w, (float)h }
		{
			fSceneFrame = fSurfaceFrame;

			calcTransform();
		}
		
		// Start with these deleted until we know how
		// to use them.
		ViewPort(const ViewPort& other) = delete;
		ViewPort& operator=(const ViewPort& other) = delete;
		
		/*
		ViewPort(const Viewport& other)
			: fTransform(other.fTransform)
			, fInverseTransform(other.fInverseTransform)
			, fRotRad(other.fRotRad)
			, fRotCenter(other.fRotCenter)
			, fWorldFrame(other.fWorldFrame)
			, fObjectFrame(other.fObjectFrame)
		{}
		*/



		// retrieve the calculated transform
		const BLMatrix2D& sceneToSurfaceTransform() {
			// print the fTransform values
			//printf("ViewPort::sceneToSurfaceTransform: %f %f %f %f %f %f\n", fTransform.m[0], fTransform.m[1], fTransform.m[2], fTransform.m[3], fTransform.m[4], fTransform.m[5]);
			return fTransform; 
		}
		const BLMatrix2D& surfaceToSceneTransform() { return fInverseTransform; }
		
		// setting and getting surface frame
		void surfaceFrame(const BLRect& fr) { fSurfaceFrame = fr; calcTransform(); }
		const BLRect& surfaceFrame() const { return fSurfaceFrame; }

		// setting and getting scene frame
		void sceneFrame(const BLRect& fr) { fSceneFrame = fr; calcTransform(); }
		const BLRect& sceneFrame() const { return fSceneFrame; }


		// Convert a point from one frame to the other
		// this is a convenience, because you can just get the appropriate
		// transformation matrix and do it separately.
		// So, only use these for one offs, otherwise, get the transform
		// and use that if doing bulk conversions.
		BLPoint sceneToSurface(double x, double y) const
		{
			BLPoint pt = fTransform.mapPoint(x, y);
			return pt;
		}
		
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
		
		
		void calcTransform()
		{
			BLPoint ascale(1, 1);
			double tScale = trueScale(ascale);

			//printf("Viewport::calcTransform, object frame: %f %f %f %f\n", fObjectFrame.x, fObjectFrame.y, fObjectFrame.w, fObjectFrame.h);
			//printTransform();
			
			fTransform = BLMatrix2D::makeIdentity();
			fTransform.rotate(fRotRad, fRotCenter);
			fTransform.scale(tScale, tScale);
			fTransform.translate(-fSceneFrame.x, -fSceneFrame.y);

			// Calculate the inverse transform
			// so we can convert from world space to object space
			fInverseTransform = fTransform;
			fInverseTransform.invert();
		}


		// This will translate relative to the current x, y position
		bool translateBy(double dx, double dy)
		{
			// BUGBUG - take minimum frame into account
			fSceneFrame.x += (float)dx;
			fSceneFrame.y += (float)dy;
			calcTransform();

			return true;
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

	};
}

