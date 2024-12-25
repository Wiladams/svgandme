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
	enum class AspectMode {
		Preserve,
		Free
	};
	
	struct ViewPort final
	{
	private:
		BLMatrix2D fTransform = BLMatrix2D::makeIdentity();
		BLMatrix2D fInverseTransform = BLMatrix2D::makeIdentity();
		
		AspectMode fAspectMode = AspectMode::Preserve;
		
		// For rotation
		double fRotRad = 0.0;		// Number of radians rotated
		BLPoint fRotCenter{};		// Point around which we rotate

		BLRect fSurfaceFrame{ 0, 0, 1, 1 };		// Coordinate system we are projecting onto
		BLRect fSceneFrame{ 0, 0, 1, 1 };		// Coordinate system of scene we are projecting



	public:
		ViewPort() = default; 
		
		
		ViewPort(const ViewPort& other)
			: fTransform(other.fTransform)
			, fInverseTransform(other.fInverseTransform)
			, fAspectMode(other.fAspectMode)
			, fRotRad(other.fRotRad)
			, fRotCenter(other.fRotCenter)
			, fSurfaceFrame(other.fSurfaceFrame)
			, fSceneFrame(other.fSceneFrame)
		{
		}
		
		ViewPort& operator=(const ViewPort& other)
		{
			fTransform = other.fTransform;
			fInverseTransform = other.fInverseTransform;
			fAspectMode = other.fAspectMode;
			fRotRad = other.fRotRad;
			fRotCenter = other.fRotCenter;
			fSurfaceFrame = other.fSurfaceFrame;
			fSceneFrame = other.fSceneFrame;
				
			return *this;
		}
		
		//
		void clampFrames()
		{
			if (fSceneFrame.w < 0) fSceneFrame.w = 0;
			if (fSceneFrame.h < 0) fSceneFrame.h = 0;
			if (fSurfaceFrame.w < 0) fSurfaceFrame.w = 0;
			if (fSurfaceFrame.h < 0) fSurfaceFrame.h = 0;
		}
		
		ViewPort(const BLRect& aSurfaceFrame, const BLRect& aSceneFrame)
			:fSurfaceFrame(aSurfaceFrame)
			, fSceneFrame(aSceneFrame)
		{
			if (!isValid()) {
				clampFrames();
				// clamp frames
			}
			
			updateTransformMatrix();
		}
		
		ViewPort(double x, double y, double w, double h)
			:fSurfaceFrame{ x, y, w, h }
			, fSceneFrame{0,0,w,h}
		{
			updateTransformMatrix();
		}
		
		~ViewPort() = default;
		
		



		bool isValid() const
		{
			return ((fSceneFrame.w > 0) && (fSceneFrame.h > 0) && (fSurfaceFrame.w > 0) && (fSurfaceFrame.h > 0));
		}

		
		void reset() 
		{
			fRotRad = 0.0;
			fRotCenter = BLPoint{};
			fSurfaceFrame = BLRect(0, 0, 1, 1);
			fSceneFrame = BLRect(0, 0, 1, 1);

			updateTransformMatrix();
		}

		void aspectMode(AspectMode mode) {
			fAspectMode = mode;
		}
		
		AspectMode aspectMode() const {
			return fAspectMode;
		}
		
		// sceneToSurfaceTransform()
		// 
		// This is the transform that is applied to a drawing context when we're
		// drawing the scene onto the surface.
		
		const BLMatrix2D& sceneToSurfaceTransform() const {
			return fTransform; 
		}
		
		// surfaceToSceneTransform()
		//
		// Use this transform when you've got a point in the surfaceFrame and you 
		// want to know where in the scene it is.  This is typically used when 
		// you do a mouse click on the surface, and you want to know where in the
		// scene that click hits.
		const BLMatrix2D& surfaceToSceneTransform() const { 
			return fInverseTransform; 
		}
		
		// setting and getting surface frame
		bool surfaceFrame(const BLRect& fr) 
		{
			// return false if the frame is invalid
			if (fr.w <= 0 || fr.h <= 0) 
				return false;
			
			fSurfaceFrame = fr; 
			updateTransformMatrix(); 
			
			// return true if we actually set the frame
			return true;
		}
		
		const BLRect& surfaceFrame() const { return fSurfaceFrame; }

		// setting and getting scene frame
		bool sceneFrame(const BLRect& fr) 
		{ 
			// return false if the frame is invalid
			if (fr.w <= 0 || fr.h <= 0)
				return false;
			
			fSceneFrame = fr; 
			updateTransformMatrix(); 
		
			return true;
		}
		
		const BLRect& sceneFrame() const { return fSceneFrame; }

		
		// Convert a point from the scene to the surface
		BLPoint mapSceneToSurface(double x, double y) const
		{
			BLPoint pt = fTransform.mapPoint(x, y);
			return pt;
		}
		
		// Convert a point from the surface to the scene
		BLPoint mapSurfaceToScene(double x, double y) const
		{
			BLPoint pt = fInverseTransform.mapPoint(x, y );
			return pt;
		}

	private:

		bool scaleObjectFrameBy(double sx, double sy, double centerx, double centery)
		{
			if (!isValid())
				return false;

			fSceneFrame.x = float(centerx + ((double)fSceneFrame.x - centerx) * sx);
			fSceneFrame.y = float(centery + ((double)fSceneFrame.y - centery) * sy);
			fSceneFrame.w *= sx;
			fSceneFrame.h *= sy;

			return true;
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

		void applyScale()
		{
			BLPoint ascale(1, 1);

			// Scale
			if (fAspectMode == AspectMode::Free)
			{
				auto fScale = fullScale(ascale);
				fTransform.scale(fScale.x, fScale.y);
			}
			else if (fAspectMode == AspectMode::Preserve)
			{
				double tScale = trueScale(ascale);
				fTransform.scale(tScale, tScale);
			}
		}
		

		
		void updateTransformMatrix()
		{	
			fTransform = BLMatrix2D::makeIdentity();
			

			// Translate by surfaceFrame amount first
			fTransform.translate(fSurfaceFrame.x, fSurfaceFrame.y);
			
			// Rotate
			fTransform.rotate(fRotRad, fRotCenter);
			

			// Scale
			applyScale();



			// Translate scene frame amount
			double tranX = -fSceneFrame.x;
			double tranY = -fSceneFrame.y;
			fTransform.translate(tranX, tranY);
			
			// Calculate the inverse transform
			// so we can convert from world space to object space
			fInverseTransform = fTransform;
			fInverseTransform.invert();
		}
		
	public:
		// @brief Translates the scene frame to an absolute position in scene coordinates.
		//
		// Moves the origin of the current scene frame to (x, y) in the scene's coordinate
		// space. After updating the position, the internal transformation matrix is recalculated.
		//
		// @param[in] x  The new x-coordinate of the scene frame's origin in scene coordinates.
		// @param[in] y  The new y-coordinate of the scene frame's origin in scene coordinates.
		//
		// @return `true` if the viewport is valid and the translation was applied successfully;
		//         `false` otherwise.
	
		bool translateTo(double x, double y)
		{
			if (!isValid())
				return false;
			
			fSceneFrame.x = x;
			fSceneFrame.y = y;
			updateTransformMatrix();

			return true;
		}

		// @brief Translates the scene frame by a relative offset in scene coordinates.
		//
		// Moves the current scene frame by (dx, dy) from its existing position, then
		// recalculates the transformation matrix. Internally, this calls `translateTo()`
		// with the updated coordinates.
		//
		// @param[in] dx  The translation offset along the x-axis in scene coordinates.
		// @param[in] dy  The translation offset along the y-axis in scene coordinates.
		//
		// @return `true` if the translation was applied successfully (i.e., the viewport
		//         remains valid); `false` otherwise.
		bool translateBy(double dx, double dy)
		{
			return translateTo(fSceneFrame.x + dx, fSceneFrame.y + dy);
		}





		// @brief Scales the scene frame around a specified pivot in surface coordinates.
		//
		// Scales the current scene frame by the factors `sdx` and `sdy`, ensuring that the point 
		// (`cx`, `cy`) in surface coordinates remains visually consistent after the transformation. 
		// This function disallows zero or negative scaling; if `sdx` or `sdy` is non-positive, 
		// the method returns `false` and no scaling is applied.
		//
		// The pivot is converted from surface to scene coordinates using the current scaling 
		// before the final transformation is performed. If the scaling succeeds, the internal 
		// transformation matrix is updated to reflect the changes.
		//
		// @param[in] sdx  The horizontal scaling factor; must be greater than 0.
		// @param[in] sdy  The vertical scaling factor; must be greater than 0.
		// @param[in] cx   The x-coordinate of the scaling pivot in surface coordinates.
		// @param[in] cy   The y-coordinate of the scaling pivot in surface coordinates.
		//
		// @return `true` if the scale was successfully applied and the transform updated; 
		//         `false` if the viewport is invalid or if any scale factor is non-positive.
		//
		bool scaleBy(double sdx, double sdy, double cx, double cy)
		{
			// we don't allow for negative scaling
			if (sdx <= 0 || sdy <= 0)
				return false;
			
			BLPoint apoint{};
			double tScale = trueScale(apoint);
			double x = fSceneFrame.x + (cx - fSurfaceFrame.x) / tScale;
			double y = fSceneFrame.y + (cy - fSurfaceFrame.y) / tScale;
			double w = fSceneFrame.w / sdx;
			double h = fSceneFrame.h / sdy;

			if (!scaleObjectFrameBy(sdx, sdy, x, y))
				return false;
			
			updateTransformMatrix();

			return true;
		}

		// @brief Rotates the scene by a specified angle around a given pivot in surface coordinates.
		//
		// Rotates the current scene view by the amount specified in 'rad' radians, using (cx, cy)
		// as the rotation pivot in surface coordinates.  After updating the rotation angle,
		// the transfomration matrix is recalculated.  The total rotation is kept within the [0,2pi) range.
		//
		// @param[in] rad
		// @param[in] cx
		// @param[in] cy
		//
		// @return 'true' if the rotation wasa successfully applied (i.e., the viewport is still valid)
		//			'false' otherwise.
		//
		// @note The pivot is expected ini surface coordinates.  If your application needs to rotate
		//		around a point in the scene, convert that point to surface coordinates first using mapSceneToSurface()
		//
		bool rotateBy(double rad, double cx, double cy)
		{
			if (!isValid())
				return false;
			
			// Keep the rotation between 0 - 2pi
			fRotRad += rad;
			fRotRad = std::fmod(fRotRad, 2.0 * waavs::pi);
			if (fRotRad < 0.0)
				fRotRad += 2.0 * waavs::pi;

			fRotCenter = { cx, cy };
			updateTransformMatrix();

			return true;
		}

		// @brief Re-centers the view so that the specified scene coordinate appears at the surface’s center.
		//
		// Calculates the current scene coordinate mapped to the midpoint of the surface (the “viewport center”)
		// and shifts the scene frame such that `(cx, cy)` in scene coordinates becomes the new center of the view.
		// After adjusting the scene frame, the internal transformation matrix is updated.
		//
		// @param[in] cx  The x-coordinate in scene space that should become the center of the surface.
		// @param[in] cy  The y-coordinate in scene space that should become the center of the surface.
		//
		// @return `true` if the viewport is valid and the re-centering operation succeeded; 
		//         `false` otherwise.
		//
		bool lookAt(double cx, double cy)
		{
			if (!isValid())
				return false;
			
			double surfaceCenterX = surfaceFrame().x + surfaceFrame().w * 0.5;
			double surfaceCenterY = surfaceFrame().y + surfaceFrame().h * 0.5;

			BLPoint sceneCenter = mapSurfaceToScene(surfaceCenterX, surfaceCenterY);

			double dx = cx - sceneCenter.x;
			double dy = cy - sceneCenter.y;

			// Adjust the scene frame to apply this shift
			BLRect oFrame = sceneFrame();
			oFrame.x += dx;
			oFrame.y += dy;
			fSceneFrame = oFrame;

			updateTransformMatrix();
			
			return true;
		}

	};
}

