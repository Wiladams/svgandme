#pragma once

#include "blend2d.h"
#include "svgenums.h"
#include "charset.h"


namespace waavs {
	
	// Parse the preserveAspectRatio attribute, returning the alignment and meetOrSlice values.
	static bool parsePreserveAspectRatio(const ByteSpan& inChunk, AspectRatioAlignKind& alignment, AspectRatioMeetOrSliceKind& meetOrSlice)
	{
		ByteSpan s = chunk_trim(inChunk, chrWspChars);
		if (s.empty())
			return false;


		// Get first token, which should be alignment
		ByteSpan align = chunk_token(s, chrWspChars);

		if (!align)
			return false;

		// We have an alignment token, convert to numeric value
		getEnumValue(SVGAspectRatioAlignEnum, align, (uint32_t&)alignment);

		// Now, see if there is a slice value
		chunk_ltrim(s, chrWspChars);

		if (s.empty())
			return false;

		getEnumValue(SVGAspectRatioMeetOrSliceEnum, s, (uint32_t&)meetOrSlice);

		return true;
	}
}

namespace waavs {
	
	struct PreserveAspectRatio final
	{
	private:
		AspectRatioAlignKind fAlignment = AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMID;
		AspectRatioMeetOrSliceKind fMeetOrSlice = AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_MEET;

	public:
		PreserveAspectRatio() = default;
		
		PreserveAspectRatio(const char* cstr)
		{
			loadFromChunk(cstr);
		}
		
		PreserveAspectRatio(const ByteSpan& inChunk)
		{
			loadFromChunk(inChunk);
		}
		
		AspectRatioAlignKind align() const { return fAlignment; }
		void setAlign(AspectRatioAlignKind a) { fAlignment = a; }

		AspectRatioMeetOrSliceKind meetOrSlice() const { return fMeetOrSlice; }
		void setMeetOrSlice(AspectRatioMeetOrSliceKind m) { fMeetOrSlice = m; }

		// Return the alignment in the x axis
		SVGAlignment xAlignment() const {
			switch (fAlignment)
			{
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMINYMIN:
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMINYMID:
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMINYMAX:
				return SVGAlignment::SVG_ALIGNMENT_START;
				break;
				
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMIN:
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMID:
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMAX:
				return SVGAlignment::SVG_ALIGNMENT_MIDDLE;
				break;

			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMAXYMIN:
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMAXYMID:
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMAXYMAX:
				return SVGAlignment::SVG_ALIGNMENT_END;
				break;
			
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_NONE:
			default:
				return SVGAlignment::SVG_ALIGNMENT_NONE;
				break;
			}
		}
		
		SVGAlignment yAlignment() const {
			switch (fAlignment) {
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMINYMIN:
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMIN:
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMAXYMIN:
				return SVGAlignment::SVG_ALIGNMENT_START;
				break;

			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMINYMID:
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMID:
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMAXYMID:
				return SVGAlignment::SVG_ALIGNMENT_MIDDLE;
				break;

			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMINYMAX:
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMAXYMAX:
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMAX:
				return SVGAlignment::SVG_ALIGNMENT_END;
				break;

			case AspectRatioAlignKind::SVG_ASPECT_RATIO_NONE:
			default:
				return SVGAlignment::SVG_ALIGNMENT_NONE;
				break;

			}
		}
		
		// Load the data type from a single ByteSpan
		bool loadFromChunk(const ByteSpan& inChunk)
		{
			return parsePreserveAspectRatio(inChunk, fAlignment, fMeetOrSlice);
		}


	};
	
	//
	// ViewportTransformer
	// 
	// The ViewportTransformer represents the mapping between a viewport and a viewbox.
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

	struct ViewportTransformer
	{
	protected:
		BLMatrix2D fTransform = BLMatrix2D::makeIdentity();
		BLMatrix2D fInverseTransform = BLMatrix2D::makeIdentity();
		
		PreserveAspectRatio fPreserveAspectRatio;

		
		// For rotation
		double fRotRad = 0.0;		// Number of radians rotated
		BLPoint fRotCenter{};		// Point around which we rotate

		BLRect fViewportFrame{ 0, 0, 1, 1 };		// Coordinate system we are projecting onto
		BLRect fViewBoxFrame{ 0, 0, 1, 1 };		// Coordinate system of scene we are projecting



	public:
		ViewportTransformer() = default;
		
		
		ViewportTransformer(const ViewportTransformer& other)
			: fTransform(other.fTransform)
			, fInverseTransform(other.fInverseTransform)
			, fPreserveAspectRatio(other.fPreserveAspectRatio)
			, fRotRad(other.fRotRad)
			, fRotCenter(other.fRotCenter)
			, fViewportFrame(other.fViewportFrame)
			, fViewBoxFrame(other.fViewBoxFrame)
		{
		}
		
		ViewportTransformer& operator=(const ViewportTransformer& other)
		{
			fTransform = other.fTransform;
			fInverseTransform = other.fInverseTransform;
			fPreserveAspectRatio = other.fPreserveAspectRatio;
			fRotRad = other.fRotRad;
			fRotCenter = other.fRotCenter;
			fViewportFrame = other.fViewportFrame;
			fViewBoxFrame = other.fViewBoxFrame;
				
			return *this;
		}
		
		//
		static void clampFrame(BLRect &fr)
		{
			if (fr.w < 0) fr.w = 0;
			if (fr.h < 0) fr.h = 0;
		}
		
		ViewportTransformer(const BLRect& aSurfaceFrame, const BLRect& aSceneFrame)
			:fViewportFrame(aSurfaceFrame)
			, fViewBoxFrame(aSceneFrame)
		{
			if (!isValid()) {
				clampFrame(fViewBoxFrame);
				clampFrame(fViewportFrame);
			}
			
			updateTransformMatrix();
		}
		
		ViewportTransformer(double x, double y, double w, double h)
			:fViewportFrame{ x, y, w, h }
			, fViewBoxFrame{0,0,w,h}
		{
			updateTransformMatrix();
		}
		
		~ViewportTransformer() = default;
		
		



		bool isValid() const
		{
			return ((fViewBoxFrame.w > 0) && (fViewBoxFrame.h > 0) && (fViewportFrame.w > 0) && (fViewportFrame.h > 0));
		}

		
		void reset() 
		{
			fRotRad = 0.0;
			fRotCenter = BLPoint{};
			fViewportFrame = BLRect(0, 0, 1, 1);
			fViewBoxFrame = BLRect(0, 0, 1, 1);

			updateTransformMatrix();
		}

		void preserveAspectRatio(const PreserveAspectRatio& aPreserveAspectRatio)
		{
			fPreserveAspectRatio = aPreserveAspectRatio;
			updateTransformMatrix();
		}
		PreserveAspectRatio preserveAspectRatio() const
		{
			return fPreserveAspectRatio;
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
		bool viewportFrame(const BLRect& fr) 
		{
			// return false if the frame is invalid
			if (fr.w <= 0 || fr.h <= 0) 
				return false;
			
			fViewportFrame = fr; 
			updateTransformMatrix(); 
			
			// return true if we actually set the frame
			return true;
		}
		
		const BLRect& viewportFrame() const { return fViewportFrame; }

		// setting and getting scene frame
		bool viewBoxFrame(const BLRect& fr) 
		{ 
			// return false if the frame is invalid
			if (fr.w <= 0 || fr.h <= 0)
				return false;
			
			fViewBoxFrame = fr; 
			updateTransformMatrix(); 
		
			return true;
		}
		
		const BLRect& viewBoxFrame() const { return fViewBoxFrame; }

		
		// Convert a point from the scene to the surface
		BLPoint mapViewBoxToViewport(double x, double y) const
		{
			BLPoint pt = fTransform.mapPoint(x, y);
			return pt;
		}
		
		// Convert a point from the surface to the scene
		BLPoint mapViewportToViewBox(double x, double y) const
		{
			BLPoint pt = fInverseTransform.mapPoint(x, y );
			return pt;
		}

	protected:



		void getAspectScale(BLPoint& ascale)
		{
			if (fPreserveAspectRatio.align() == AspectRatioAlignKind::SVG_ASPECT_RATIO_NONE)
			{
				// Scale each dimension to fit the scene to the frame
				// do not preserve aspect ratio
				// ignore the meet/slice
				ascale.x = (fViewportFrame.w / fViewBoxFrame.w);
				ascale.y = (fViewportFrame.h / fViewBoxFrame.h);
			}
			else {
				double scaleX = (fViewportFrame.w / fViewBoxFrame.w);
				double scaleY = (fViewportFrame.h / fViewBoxFrame.h);
				
				// Check the meet/slice
				double uniformScale{};
				if (fPreserveAspectRatio.meetOrSlice() == AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_SLICE)
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
			
			if (fPreserveAspectRatio.align() == AspectRatioAlignKind::SVG_ASPECT_RATIO_NONE)
			{
				// Scale each dimension to fit the scene to the frame
				// do not preserve aspect ratio
				// ignore the meet/slice
				ascale.x = (fViewportFrame.w / fViewBoxFrame.w);
				ascale.y = (fViewportFrame.h / fViewBoxFrame.h);

			} else {
				// calculate candidate scaling factors
				double scaleX = (fViewportFrame.w / fViewBoxFrame.w);
				double scaleY = (fViewportFrame.h / fViewBoxFrame.h);

				// Check the meet/slice
				double uniformScale{};
				if (fPreserveAspectRatio.meetOrSlice() == AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_SLICE)
					uniformScale = std::max(scaleX, scaleY);
				else  // default to 'meet'
					uniformScale = std::min(scaleX, scaleY);
					
				ascale.x = uniformScale;
				ascale.y = uniformScale;
					
				// Next, compute the alignment offset
				// we'll see how big the sceneFrame is 
				// with scaling applied
				double scaleW = fViewBoxFrame.w * uniformScale;
				double scaleH = fViewBoxFrame.h * uniformScale;
					
				// Decide alignment in X direction
				SVGAlignment xAlign = fPreserveAspectRatio.xAlignment();

				switch (xAlign)
				{
					case SVGAlignment::SVG_ALIGNMENT_START:
						atrans.x = 0;
						break;
						
					case SVGAlignment::SVG_ALIGNMENT_MIDDLE:
						atrans.x = (fViewportFrame.w - scaleW) / 2;
						break;
						
					case SVGAlignment::SVG_ALIGNMENT_END:
						atrans.x = fViewportFrame.w - scaleW;
						break;
				}
					
				// Decide alignment in Y direction
				SVGAlignment yAlign = fPreserveAspectRatio.yAlignment();

				switch (yAlign) {
					case SVGAlignment::SVG_ALIGNMENT_START:
						atrans.y = 0;
						break;

					case SVGAlignment::SVG_ALIGNMENT_MIDDLE:
						atrans.y = (fViewportFrame.h - scaleH) / 2;
						break;
						
					case SVGAlignment::SVG_ALIGNMENT_END:
						atrans.y = fViewportFrame.h - scaleH;
						break;
				}

			}

			
			// Translate by viewportFrame amount first
			// because we assume the context hasn't already done this
			fTransform.translate(fViewportFrame.x, fViewportFrame.y);
			
			// Now apply transformations in order
			// Rotate
			fTransform.rotate(fRotRad, fRotCenter);
			

			// Scale
			// scale by the computed factors
			fTransform.scale(ascale);


			// Apply the alignment offset
			fTransform.translate(atrans.x, atrans.y);

			// Translate scene frame amount
			fTransform.translate(-fViewBoxFrame.x, -fViewBoxFrame.y);

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
			
			fViewBoxFrame.x = x;
			fViewBoxFrame.y = y;
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
			return translateTo(fViewBoxFrame.x + dx, fViewBoxFrame.y + dy);
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
			if (!isValid())
				return false;

			fViewBoxFrame.x = float(centerx + ((double)fViewBoxFrame.x - centerx) * sx);
			fViewBoxFrame.y = float(centery + ((double)fViewBoxFrame.y - centery) * sy);
			fViewBoxFrame.w *= sx;
			fViewBoxFrame.h *= sy;

			return true;
		}
		
		bool scaleBy(double sdx, double sdy, double cx, double cy)
		{
			// we don't allow for negative scaling
			if (sdx <= 0 || sdy <= 0)
				return false;
			
			BLPoint ascale{};
			getAspectScale(ascale);

			double x = fViewBoxFrame.x + (cx - fViewportFrame.x) / ascale.x;
			double y = fViewBoxFrame.y + (cy - fViewportFrame.y) / ascale.y;
			double w = fViewBoxFrame.w / sdx;
			double h = fViewBoxFrame.h / sdy;

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
			
			double surfaceCenterX = viewportFrame().x + viewportFrame().w * 0.5;
			double surfaceCenterY = viewportFrame().y + viewportFrame().h * 0.5;

			BLPoint sceneCenter = mapViewportToViewBox(surfaceCenterX, surfaceCenterY);

			double dx = cx - sceneCenter.x;
			double dy = cy - sceneCenter.y;

			// Adjust the scene frame to apply this shift
			BLRect oFrame = viewBoxFrame();
			oFrame.x += dx;
			oFrame.y += dy;
			fViewBoxFrame = oFrame;

			updateTransformMatrix();
			
			return true;
		}

	};
}

