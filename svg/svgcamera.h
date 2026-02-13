#pragma once

#include "blend2d.h"
#include "svgenums.h"
#include "charset.h"
#include "viewport.h"
#include "maths.h"

namespace waavs
{
	// PortalView
	// 
	// Constains the raw information that makes up a viewport
	// The viewport is the 'window' through which we are looking at a scene
	// Do, it contains the portion of the scene we are looking at, and the size
	// of the window.
	// This also contains the rotation and aspect ratio as well.
	// Note: We want to separate this from the transformation stuff
	// because we might want to use it in a context beyond that transformation.
	// If the values for the viewport or videwbox are <= 0, they are considered
	// to not be set.
	//
	struct PortalView
	{
	public:
		BLRect fViewportFrame{};
		BLRect fViewBoxFrame{};
		double fRotRad = 0.0;		// Number of radians rotated
		BLPoint fRotCenter{};		// Point around which we rotate
		PreserveAspectRatio fPreserveAspectRatio;

	public:
		static void clampFrame(BLRect& fr)
		{
			if (fr.w < 0) fr.w = 0;
			if (fr.h < 0) fr.h = 0;
		}

		void reset()
		{
			fViewportFrame = BLRect{};
			fViewBoxFrame = BLRect{};
			fRotRad = 0.0;
			fRotCenter = BLPoint{};
			fPreserveAspectRatio = PreserveAspectRatio{};
		}

		void resetView(const BLRect& viewBoxFr, const BLRect& viewportFr)
		{
			fViewportFrame = viewportFr;
			fViewBoxFrame = viewBoxFr;

			//clampFrame(fViewportFrame);
			//clampFrame(fViewBoxFrame);

			fRotRad = 0.0;
			fRotCenter = BLPoint{};
			fPreserveAspectRatio = PreserveAspectRatio{};
		}

		bool isValid() const
		{
			return ((fViewBoxFrame.w > 0) && (fViewBoxFrame.h > 0) && (fViewportFrame.w > 0) && (fViewportFrame.h > 0));
		}

		void setViewportFrame(const BLRect& fr) {
			fViewportFrame = fr;
			//clampFrame(fViewportFrame);
		}
		bool getViewportFrame(BLRect& outFr) const
		{
			if (fViewportFrame.w <= 0 || fViewportFrame.h <= 0)
				return false;
			outFr = fViewportFrame;
			return true;
		}

		void setViewBoxFrame(const BLRect& fr)
		{
			fViewBoxFrame = fr;
			//clampFrame(fViewBoxFrame);
		}
		bool getViewBoxFrame(BLRect& outFr) const
		{
			if (fViewBoxFrame.w <= 0 || fViewBoxFrame.h <= 0)
				return false;
			outFr = fViewBoxFrame;
			return true;
		}

		void setRotation(double rads, const BLPoint& center)
		{
			// Keep the rotation between 0 - 2pi
			fRotRad += rads;
			fRotRad = std::fmod(fRotRad, 2.0 * waavs::pi);
			if (fRotRad < 0.0)
				fRotRad += 2.0 * waavs::pi;

			fRotCenter = center;
		}

		bool getRotation(double& outRadians, BLPoint& outCenter) const
		{
			outRadians = fRotRad;
			outCenter = fRotCenter;
			return true;
		}

		void setPreserveAspectRatio(const PreserveAspectRatio& par) { fPreserveAspectRatio = par; }
		bool getPreserveAspectRatio(PreserveAspectRatio& outPar) const
		{
			outPar = fPreserveAspectRatio;
			return true;
		}
	};


	//
	// ViewportTransformer
	// 
	// The ViewportTransformer represents the mapping between a viewport and a viewbox.
	// 
	// The viewportFrame, is where the image is projected.  This is typically
	// the actual window on the screen the user is interacting with.
	// 
	// The viewboxFrame, is the content that is being looked at.  So, if you're looking
	// through a window as a painting outside, the viewboxFrame is the painting.
	// The 'viewbox' is assumed to be an infinite canvas.  The 'viewboxFrame' is the portion
	// of that infinite canvas you want to actually show up within the viewportFrame.
	// 
	// Interesting operations:
	// 1) If you have a bounded thing, like a bitmap, and you want it to fill the surface
	// you can specify the viewboxFrame(BLRect(0,0,img.width, img.height))
	// 
	// 2) If you want to do some 'panning', you send a viewboxFrame to equal the size
	// of the viewportFrame initially, then use the translateBy(), and translateTo() functions
	// to adjust the position of that boundary against the scene you're scrolling.
	// 
	// The viewport allows you to do typical 'camera' kinds of movements, like pan, zoom,
	// lookAt, and the like, and returns you the transformation matrix that you can 
	// apply to a drawing context.
	//

	struct ViewportTransformer
	{
	protected:
		BLMatrix2D fTransform = BLMatrix2D::makeIdentity();
		BLMatrix2D fInverseTransform = BLMatrix2D::makeIdentity();

		PortalView fPortalView{};



	public:
		ViewportTransformer() = default;


		ViewportTransformer(const ViewportTransformer& other)
			: fTransform(other.fTransform)
			, fInverseTransform(other.fInverseTransform)
			, fPortalView(other.fPortalView)

		{
		}

		ViewportTransformer& operator=(const ViewportTransformer& other)
		{
			fTransform = other.fTransform;
			fInverseTransform = other.fInverseTransform;
			fPortalView = other.fPortalView;

			return *this;
		}


		ViewportTransformer(const BLRect& aSurfaceFrame, const BLRect& aSceneFrame)

		{
			fPortalView.setViewportFrame(aSurfaceFrame);
			fPortalView.setViewBoxFrame(aSceneFrame);

			updateTransformMatrix();
		}

		ViewportTransformer(double x, double y, double w, double h)
		{
			fPortalView.setViewportFrame({ x,y,w,h });
			fPortalView.setViewBoxFrame({ 0,0,w,h });
			updateTransformMatrix();
		}

		~ViewportTransformer() = default;


		void reset()
		{
			fPortalView.reset();
			fPortalView.setViewportFrame({ 0,0,1,1 });
			fPortalView.setViewBoxFrame({ 0,0,1,1 });

			updateTransformMatrix();
		}

		void setPreserveAspectRatio(const PreserveAspectRatio& aPreserveAspectRatio)
		{
			fPortalView.setPreserveAspectRatio(aPreserveAspectRatio);
			updateTransformMatrix();
		}
		bool getPreserveAspectRatio(PreserveAspectRatio& aspect) const
		{
			return fPortalView.getPreserveAspectRatio(aspect);
		}


		// sceneToSurfaceTransform()
		// 
		// This is the transform that is applied to a drawing context when we're
		// drawing the scene onto the surface.

		const BLMatrix2D& viewBoxToViewportTransform() const {
			return fTransform;
		}

		// viewportToViewBoxTransform()
		//
		// Use this transform when you've got a point in the surfaceFrame and you 
		// want to know where in the scene it is.  This is typically used when 
		// you do a mouse click on the surface, and you want to know where in the
		// scene that click hits.
		const BLMatrix2D& viewportToViewBoxTransform() const {
			return fInverseTransform;
		}

		// setting and getting surface frame
		bool setViewportFrame(const BLRect& fr)
		{
			// return false if the frame is invalid
			if (fr.w <= 0 || fr.h <= 0)
				return false;

			fPortalView.setViewportFrame(fr);

			updateTransformMatrix();

			// return true if we actually set the frame
			return true;
		}

		bool getViewportFrame(BLRect& aFrame) const
		{
			return fPortalView.getViewportFrame(aFrame);
		}

		// setting and getting scene frame
		bool setViewBoxFrame(const BLRect& fr)
		{
			// return false if the frame is invalid
			if (fr.w <= 0 || fr.h <= 0)
				return false;

			fPortalView.setViewBoxFrame(fr);

			updateTransformMatrix();

			return true;
		}

		bool getViewBoxFrame(BLRect& aframe) const
		{
			return fPortalView.getViewBoxFrame(aframe);
		}


		// Convert a point from the scene to the surface
		BLPoint mapViewBoxToViewport(double x, double y) const
		{
			BLPoint pt = fTransform.mapPoint(x, y);
			return pt;
		}

		// Convert a point from the surface to the scene
		BLPoint mapViewportToViewBox(double x, double y) const
		{
			BLPoint pt = fInverseTransform.mapPoint(x, y);
			return pt;
		}

	protected:

		void getAspectScale(BLPoint& ascale)
		{
			if (fPortalView.fPreserveAspectRatio.align() == AspectRatioAlignKind::SVG_ASPECT_RATIO_NONE)
			{
				// Scale each dimension to fit the scene to the frame
				// do not preserve aspect ratio
				// ignore the meet/slice
				ascale.x = (fPortalView.fViewportFrame.w / fPortalView.fViewBoxFrame.w);
				ascale.y = (fPortalView.fViewportFrame.h / fPortalView.fViewBoxFrame.h);
			}
			else {
				double scaleX = (fPortalView.fViewportFrame.w / fPortalView.fViewBoxFrame.w);
				double scaleY = (fPortalView.fViewportFrame.h / fPortalView.fViewBoxFrame.h);

				// Check the meet/slice
				double uniformScale{};
				if (fPortalView.fPreserveAspectRatio.meetOrSlice() == AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_SLICE)
					uniformScale = std::max(scaleX, scaleY);
				else  // default to 'meet'
					uniformScale = std::min(scaleX, scaleY);

				ascale.x = uniformScale;
				ascale.y = uniformScale;
			}
		}

		void updateTransformMatrix()
		{
			fTransform = BLMatrix2D::makeIdentity();

			BLPoint ascale(1, 1);
			BLPoint atrans(0, 0);

			if (fPortalView.fPreserveAspectRatio.align() == AspectRatioAlignKind::SVG_ASPECT_RATIO_NONE)
			{
				// Scale each dimension to fit the scene to the frame
				// do not preserve aspect ratio
				// ignore the meet/slice
				ascale.x = (fPortalView.fViewportFrame.w / fPortalView.fViewBoxFrame.w);
				ascale.y = (fPortalView.fViewportFrame.h / fPortalView.fViewBoxFrame.h);

			}
			else {
				// calculate candidate scaling factors
				double scaleX = (fPortalView.fViewportFrame.w / fPortalView.fViewBoxFrame.w);
				double scaleY = (fPortalView.fViewportFrame.h / fPortalView.fViewBoxFrame.h);

				// Check the meet/slice
				double uniformScale{};
				if (fPortalView.fPreserveAspectRatio.meetOrSlice() == AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_SLICE)
					uniformScale = std::max(scaleX, scaleY);
				else  // default to 'meet'
					uniformScale = std::min(scaleX, scaleY);

				ascale.x = uniformScale;
				ascale.y = uniformScale;

				// Next, compute the alignment offset
				// we'll see how big the sceneFrame is 
				// with scaling applied
				double scaleW = fPortalView.fViewBoxFrame.w * uniformScale;
				double scaleH = fPortalView.fViewBoxFrame.h * uniformScale;

				// Decide alignment in X direction
				SVGAlignment xAlign{};
				SVGAlignment yAlign{};
				PreserveAspectRatio::splitAlignment(fPortalView.fPreserveAspectRatio.align(), xAlign, yAlign);
				//fPortalView.fPreserveAspectRatio.getAlignment(xAlign, yAlign);

				switch (xAlign)
				{
				case SVGAlignment::SVG_ALIGNMENT_START:
					atrans.x = 0;
					break;

				case SVGAlignment::SVG_ALIGNMENT_MIDDLE:
					atrans.x = (fPortalView.fViewportFrame.w - scaleW) / 2;
					break;

				case SVGAlignment::SVG_ALIGNMENT_END:
					atrans.x = fPortalView.fViewportFrame.w - scaleW;
					break;
				}

				// Decide alignment in Y direction
				switch (yAlign) {
				case SVGAlignment::SVG_ALIGNMENT_START:
					atrans.y = 0;
					break;

				case SVGAlignment::SVG_ALIGNMENT_MIDDLE:
					atrans.y = (fPortalView.fViewportFrame.h - scaleH) / 2;
					break;

				case SVGAlignment::SVG_ALIGNMENT_END:
					atrans.y = fPortalView.fViewportFrame.h - scaleH;
					break;
				}

			}


			// Translate by viewportFrame amount first
			// because we assume the context hasn't already done this
			fTransform.translate(fPortalView.fViewportFrame.x, fPortalView.fViewportFrame.y);


			// Apply the 'letter box' alignment offset
			fTransform.translate(atrans.x, atrans.y);





			// Scale
			// scale by the computed factors
			fTransform.scale(ascale);


			// Translate scene frame amount
			fTransform.translate(-fPortalView.fViewBoxFrame.x, -fPortalView.fViewBoxFrame.y);

			fTransform.rotate(fPortalView.fRotRad, fPortalView.fRotCenter);


			// Rotation; applied to the translated object space
			// space
			//BLPoint pivotLocal = {
			//	fPortalView.fRotCenter.x - (fPortalView.fViewBoxFrame.x),
			//	fPortalView.fRotCenter.y - (fPortalView.fViewBoxFrame.y)
			//};
			//fTransform.rotate(fPortalView.fRotRad, pivotLocal);


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
			if (!fPortalView.isValid())
				return false;


			fPortalView.fViewBoxFrame.x = x;
			fPortalView.fViewBoxFrame.y = y;
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
			return translateTo(fPortalView.fViewBoxFrame.x + dx, fPortalView.fViewBoxFrame.y + dy);
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
		bool scaleViewBoxBy(double sx, double sy, double centerx, double centery)
		{
			if (!fPortalView.isValid())
				return false;

			fPortalView.fViewBoxFrame.x = float(centerx + ((double)fPortalView.fViewBoxFrame.x - centerx) * sx);
			fPortalView.fViewBoxFrame.y = float(centery + ((double)fPortalView.fViewBoxFrame.y - centery) * sy);
			fPortalView.fViewBoxFrame.w *= sx;
			fPortalView.fViewBoxFrame.h *= sy;

			return true;
		}

		bool scaleBy(double sdx, double sdy, double cx, double cy)
		{
			// we don't allow for negative scaling
			if (sdx <= 0 || sdy <= 0)
				return false;

			BLPoint ascale{};
			getAspectScale(ascale);

			double x = fPortalView.fViewBoxFrame.x + (cx - fPortalView.fViewportFrame.x) / ascale.x;
			double y = fPortalView.fViewBoxFrame.y + (cy - fPortalView.fViewportFrame.y) / ascale.y;
			//double w = fViewBoxFrame.w / sdx;
			//double h = fViewBoxFrame.h / sdy;

			if (!scaleViewBoxBy(sdx, sdy, x, y))
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
		// @return 'true' if the rotation was successfully applied (i.e., the viewport is still valid)
		//			'false' otherwise.
		//
		// @note The pivot is expected in surface coordinates.  If your application needs to rotate
		//		around a point in the scene, convert that point to surface coordinates first using mapSceneToSurface()
		//
		bool rotateBy(double rad, double cx, double cy)
		{
			if (!fPortalView.isValid())
				return false;

			// Keep the rotation between 0 - 2pi
			fPortalView.fRotRad += rad;
			fPortalView.fRotRad = std::fmod(fPortalView.fRotRad, 2.0 * waavs::pi);
			if (fPortalView.fRotRad < 0.0)
				fPortalView.fRotRad += 2.0 * waavs::pi;

			fPortalView.fRotCenter = { cx, cy };
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
			if (!fPortalView.isValid())
				return false;

			BLRect vpFrame{};
			fPortalView.getViewBoxFrame(vpFrame);


			double surfaceCenterX = vpFrame.x + vpFrame.w * 0.5;
			double surfaceCenterY = vpFrame.y + vpFrame.h * 0.5;

			BLPoint sceneCenter = mapViewportToViewBox(surfaceCenterX, surfaceCenterY);

			double dx = cx - sceneCenter.x;
			double dy = cy - sceneCenter.y;

			// Adjust the scene frame to apply this shift
			BLRect oFrame{};
			fPortalView.getViewBoxFrame(oFrame);
			oFrame.x += dx;
			oFrame.y += dy;

			fPortalView.setViewBoxFrame(oFrame);

			updateTransformMatrix();

			return true;
		}

	};
}
