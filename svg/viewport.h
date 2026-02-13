#pragma once


#include "blend2d.h"

#include "charset.h"
#include "maths.h"
#include "svgenums.h"

#include "svgdatatypes.h"


namespace waavs {
	
	// parsePreserveAspectRatio
	// 
	// Parse the preserveAspectRatio attribute, returning the alignment and meetOrSlice values.
	static bool parsePreserveAspectRatio(const ByteSpan& inChunk, AspectRatioAlignKind& alignment, AspectRatioMeetOrSliceKind& meetOrSlice)
	{
		alignment = AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMID;
		meetOrSlice = AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_MEET;

		ByteSpan s = chunk_trim(inChunk, chrWspChars);
		if (s.empty())
			return false;


		// Get first token, which should be alignment
		ByteSpan align = chunk_token(s, chrWspChars);

		if (align.empty())
			return false;

		// We have an alignment token, convert to numeric value
		if (!getEnumValue(SVGAspectRatioAlignEnum, align, (uint32_t&)alignment))
			return false;

		// Now, see if there is a slice value
		s = chunk_trim(s, chrWspChars);

        // if there is no meetOrSlice token, we should set the 
		// meetOrSlice to 'meet' by default
		if (!s.empty()) {
			ByteSpan mos = chunk_token(s, chrWspChars);
			if (!mos.empty() && !getEnumValue(SVGAspectRatioMeetOrSliceEnum, mos, (uint32_t&)meetOrSlice))
				return false;
		}

		return true;
	}
}

namespace waavs {

	struct PreserveAspectRatio final
	{
	private:
		static constexpr AspectRatioAlignKind DEFAULT_ALIGNMENT = AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMID;
		static constexpr AspectRatioMeetOrSliceKind DEFAULT_MEET_OR_SLICE = AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_MEET;

		AspectRatioAlignKind fAlignment = DEFAULT_ALIGNMENT;
		AspectRatioMeetOrSliceKind fMeetOrSlice = DEFAULT_MEET_OR_SLICE;
        bool fIsSet{ false };

	public:
		PreserveAspectRatio() = default;

		PreserveAspectRatio(const char* cstr) noexcept
		{
			ByteSpan aspan(cstr);
			loadFromChunk(aspan);
		}

		PreserveAspectRatio(const ByteSpan& inChunk) noexcept
		{
			loadFromChunk(inChunk);
		}

		constexpr AspectRatioMeetOrSliceKind meetOrSlice() const noexcept { return fMeetOrSlice; }
		void setMeetOrSlice(AspectRatioMeetOrSliceKind m) noexcept { fMeetOrSlice = m; }

		constexpr AspectRatioAlignKind align() const noexcept { return fAlignment; }
		void setAlign(AspectRatioAlignKind a) noexcept { fAlignment = a; }

		static void splitAlignment(AspectRatioAlignKind aligned, SVGAlignment& xAlign, SVGAlignment& yAlign) noexcept
		{
			switch (aligned)
			{
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMINYMIN:
				xAlign = SVGAlignment::SVG_ALIGNMENT_START;
				yAlign = SVGAlignment::SVG_ALIGNMENT_START;
				return;

			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMINYMID:
				xAlign = SVGAlignment::SVG_ALIGNMENT_START;
				yAlign = SVGAlignment::SVG_ALIGNMENT_MIDDLE;
				return;


			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMINYMAX:
				xAlign = SVGAlignment::SVG_ALIGNMENT_START;
				yAlign = SVGAlignment::SVG_ALIGNMENT_END;
				return;


				// X Middle
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMIN:
				xAlign = SVGAlignment::SVG_ALIGNMENT_MIDDLE;
				yAlign = SVGAlignment::SVG_ALIGNMENT_START;
				return;

			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMID:
				xAlign = SVGAlignment::SVG_ALIGNMENT_MIDDLE;
				yAlign = SVGAlignment::SVG_ALIGNMENT_MIDDLE;
				return;

			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMAX:
				xAlign = SVGAlignment::SVG_ALIGNMENT_MIDDLE;
				yAlign = SVGAlignment::SVG_ALIGNMENT_END;
				return;


				// X End
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMAXYMIN:
				xAlign = SVGAlignment::SVG_ALIGNMENT_END;
				yAlign = SVGAlignment::SVG_ALIGNMENT_START;
				return;

			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMAXYMID:
				xAlign = SVGAlignment::SVG_ALIGNMENT_END;
				yAlign = SVGAlignment::SVG_ALIGNMENT_MIDDLE;
				return;

			case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMAXYMAX:
				xAlign = SVGAlignment::SVG_ALIGNMENT_END;
				yAlign = SVGAlignment::SVG_ALIGNMENT_END;
				return;

				// NONE
			case AspectRatioAlignKind::SVG_ASPECT_RATIO_NONE:
			default:
				xAlign = SVGAlignment::SVG_ALIGNMENT_NONE;
				yAlign = SVGAlignment::SVG_ALIGNMENT_NONE;
				return;

			}
		}


		// Load the data type from a single ByteSpan
		bool loadFromChunk(const ByteSpan& inChunk) noexcept
		{
			fIsSet = parsePreserveAspectRatio(inChunk, fAlignment, fMeetOrSlice);
			return fIsSet;
		}


	};

	static bool parseViewBox(const ByteSpan& inChunk, BLRect& r) noexcept
	{
		if (!inChunk)
			return false;

		SVGTokenListView view(inChunk);
		double x = 0.0, y = 0.0, w = 0.0, h = 0.0;

		if (!view.readANumber(x)) return false;
		if (!view.readANumber(y)) return false;
		if (!view.readANumber(w) || (w <= 0.0)) return false;
		if (!view.readANumber(h) || (h <= 0.0)) return false;

		r.reset(x, y, w, h);

		return true;
	}

    // ViewStateAuthoring
	//
    // Represents the as-authored state of the viewport and viewbox settings for an element.
    // This is the state that is loaded from the XML attributes, and it preserves the original units and values.
    // This is separate from the resolved state (SVGViewportState) because we may need to re-resolve the viewport 
	// settings if the context changes (e.g. parent element's viewport changes, or the canvas size changes).
	struct DocViewportState
	{
		// as-authored values (after style merge)
		SVGLengthValue x{};
		SVGLengthValue y{};
		SVGLengthValue width{};
		SVGLengthValue height{};

		PreserveAspectRatio par{};

        bool hasViewBox{ false };
		BLRect viewBox{};
	};

	struct SVGViewportState
	{
		BLRect fViewport;   // The viewport rectangle

		bool fHasViewBox{ false };
		BLRect fViewBox;    // The viewBox rectangle

		PreserveAspectRatio fPreserveAspect{};

		// The transformation from viewBox to viewport,
		// Calculated based on the viewport, viewbox, and preserveAspectRatio settings
		BLMatrix2D viewBoxToViewportXform{};

		// Whether the viewBoxToViewportXform has been resolved based on the current settings
		bool fResolved{ false };
	};

	// onLoadedFromXmlPull
	//
	// This is called after we've loaded the element and all its children from the XML
	// In the case of a top level svg element, the entirety of the document has been loaded
	// so it's safe to resolve style attributes now.
	//
	// For non-top level svg elements, it's not guaranteed that the 
	// style information has been fully loaded yet, so we just
	// resolve what we can.
	//
	static void loadDocViewportState(DocViewportState& vps, const XmlAttributeCollection& attrs)
	{

		// Load the non-bound attribute values here, for processing later
		// when we bind.
		// x, y, width, height
		ByteSpan xAttr{}, yAttr{}, wAttr{}, hAttr{};
		attrs.getValue(svgattr::x(), xAttr);
		attrs.getValue(svgattr::y(), yAttr);
		attrs.getValue(svgattr::width(), wAttr);
		attrs.getValue(svgattr::height(), hAttr);

		vps.x = parseLengthAttr(xAttr);
		vps.y = parseLengthAttr(yAttr);
		vps.width = parseLengthAttr(wAttr);
		vps.height = parseLengthAttr(hAttr);

		// preserAspectRatio
		ByteSpan parAttr{};
		if (attrs.getValue(svgattr::preserveAspectRatio(), parAttr))
		{
			vps.par.loadFromChunk(parAttr);
		}

		// viewBox (structural)
		ByteSpan vbAttr{};
		BLRect vbFrame{};
		if (attrs.getValue(svgattr::viewBox(), vbAttr) && parseViewBox(vbAttr, vbFrame))
		{
			vps.hasViewBox = true;
			vps.viewBox = vbFrame;
		}


	}
}



namespace waavs
{

	static bool computeViewBoxToViewport(
		const BLRect& viewport,
		const BLRect& viewBox,
		const PreserveAspectRatio& par,
		BLMatrix2D& out)
	{
		if (viewport.w <= 0 || viewport.h <= 0) return false;
		if (viewBox.w <= 0 || viewBox.h <= 0) return false;

		const double sx0 = viewport.w / viewBox.w;
		const double sy0 = viewport.h / viewBox.h;

		double sx = sx0, sy = sy0;
		double ax = 0.0, ay = 0.0;

		if (par.align() != AspectRatioAlignKind::SVG_ASPECT_RATIO_NONE) {
			double s = sx0;
			if (par.meetOrSlice() == AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_SLICE)
				s = waavs::max(sx0, sy0);
			else
				s = waavs::min(sx0, sy0);

			sx = sy = s;

			const double fitW = viewBox.w * s;
			const double fitH = viewBox.h * s;

			SVGAlignment xA{}, yA{};
			PreserveAspectRatio::splitAlignment(par.align(), xA, yA);

			if (xA == SVGAlignment::SVG_ALIGNMENT_MIDDLE) ax = (viewport.w - fitW) * 0.5;
			else if (xA == SVGAlignment::SVG_ALIGNMENT_END) ax = (viewport.w - fitW);

			if (yA == SVGAlignment::SVG_ALIGNMENT_MIDDLE) ay = (viewport.h - fitH) * 0.5;
			else if (yA == SVGAlignment::SVG_ALIGNMENT_END) ay = (viewport.h - fitH);
		}

		out = BLMatrix2D::makeIdentity();
		out.translate(viewport.x, viewport.y);
		out.translate(ax, ay);
		out.scale(sx, sy);
		out.translate(-viewBox.x, -viewBox.y);

		return true;
	}


	// Minimal resolve:
// - containingVP: parent viewport rect (user units)
// - authored: parsed attribute state (lengths + viewBox + par)
// - isTopLevel: top-level <svg> ignores x/y; nested honors x/y
// - dpi/font: only needed for em/ex; pass nullptr font if you want
	static bool resolveViewState(
		const BLRect& containingVP,
		const DocViewportState& authored,
		bool isTopLevel,
		double dpi,
		const BLFont* fontOpt,
		SVGViewportState& out) noexcept
	{
		out = SVGViewportState{}; // reset

		// If containingVP is not meaningful, fall back to SVG defaults.
		const double containW = (containingVP.w > 0.0) ? containingVP.w : 300.0;
		const double containH = (containingVP.h > 0.0) ? containingVP.h : 150.0;

		// Defaults:
		// - nested <svg>: width/height default 100% of containingVP
		// - top-level <svg>: if containingVP is meaningful, use it; else 300x150
		const double defaultW = isTopLevel ? containW : containW;
		const double defaultH = isTopLevel ? containH : containH;

		// Percent references for x/y/w/h are the containing viewport width/height.
		const LengthResolveCtx ctxX = makeLengthCtxUser(containW, /*origin*/0.0, dpi, fontOpt);
		const LengthResolveCtx ctxY = makeLengthCtxUser(containH, /*origin*/0.0, dpi, fontOpt);
		const LengthResolveCtx ctxW = makeLengthCtxUser(containW, /*origin*/0.0, dpi, fontOpt);
		const LengthResolveCtx ctxH = makeLengthCtxUser(containH, /*origin*/0.0, dpi, fontOpt);

		// x/y:
		// Your rule: top-level ignores x/y; nested honors them.
		double x = isTopLevel ? containingVP.x : (resolveLengthOr(authored.x, ctxX, 0.0) + containingVP.x);
		double y = isTopLevel ? containingVP.y : (resolveLengthOr(authored.y, ctxY, 0.0) + containingVP.y);

		// width/height:
		double w = resolveLengthOr(authored.width, ctxW, defaultW);
		double h = resolveLengthOr(authored.height, ctxH, defaultH);

		// Clamp negatives => non-renderable viewport (minimal policy)
		if (w < 0.0) w = 0.0;
		if (h < 0.0) h = 0.0;

		out.fViewport = BLRect{ x, y, w, h };

		if (out.fViewport.w <= 0.0 || out.fViewport.h <= 0.0) {
			out.fResolved = false;
			return false;
		}

		// viewBox:
		out.fPreserveAspect = authored.par;

		if (authored.hasViewBox) {
			out.fHasViewBox = true;
			out.fViewBox = authored.viewBox;
		}
		else {
			out.fHasViewBox = false;
			out.fViewBox = BLRect{ 0.0, 0.0, out.fViewport.w, out.fViewport.h };
		}

		if (out.fViewBox.w <= 0.0 || out.fViewBox.h <= 0.0) {
			out.fResolved = false;
			return false;
		}

		// viewBox -> viewport mapping
		if (!computeViewBoxToViewport(out.fViewport, out.fViewBox, out.fPreserveAspect, out.viewBoxToViewportXform)) {
			out.fResolved = false;
			return false;
		}

		out.fResolved = true;
		return true;
	}

}


