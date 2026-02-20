#pragma once

//
// Feature String: http://www.w3.org/TR/SVG11/feature#Marker
//

#include "svgstructuretypes.h"
#include "svgattributes.h"
#include "viewport.h"

namespace waavs
{
	// -----------------------------
	// Authored marker state (Doc)
	// -----------------------------
	struct DocMarkerState
	{
		// markerUnits default per SVG is strokeWidth
		SpaceUnitsKind markerUnits{ SpaceUnitsKind::SVG_SPACE_STROKEWIDTH };

		// markerWidth/markerHeight default to 3 (SVG spec default)
		// Store as <length>/<percentage> like everything else.
		SVGLengthValue markerWidth{ 3.0, SVG_LENGTHTYPE_NUMBER, true };
		SVGLengthValue markerHeight{ 3.0, SVG_LENGTHTYPE_NUMBER, true };

		// refX/refY default to 0
		SVGLengthValue refX{ 0.0, SVG_LENGTHTYPE_NUMBER, true };
		SVGLengthValue refY{ 0.0, SVG_LENGTHTYPE_NUMBER, true };

		// orient
		SVGOrient orient{ nullptr };

		// viewBox + preserveAspectRatio live in the same doc viewport structure you already have
		DocViewportState vp{};
		bool hasVP{ false };
	};


	// -----------------------------
	// Resolved marker state
	// -----------------------------
	struct ResolvedMarkerState
	{
		// Marker viewport in the *parent user space of the marker instance*
		// We keep it local at origin because MarkerProgramExec already translates to the vertex.
		BLRect viewport{ 0,0,0,0 };

		// The viewBox in marker content user space (children see this as nearest viewport)
		BLRect viewBox{ 0,0,0,0 };
		bool hasViewBox{ false };

		// Final mapping to apply before drawing marker children:
		// maps marker content user space -> marker instance space (at origin)
		BLMatrix2D contentToInstance{ BLMatrix2D::makeIdentity() };

		bool resolved{ false };
	};

	// Helpers: treat markerWidth/Height differently depending on markerUnits.
// Policy used here (consistent + simple):
// - First resolve length to "user units" using percentage ref.
// - If markerUnits == strokeWidth: scale the result by strokeWidth (so 3 -> 3*strokeWidth).
	static INLINE double resolveMarkerLen(
		const SVGLengthValue& L,
		double percentRef,      // usually nearest viewport w/h for userSpaceOnUse, or strokeWidth for strokeWidth units
		double strokeWidth,
		double dpi,
		const BLFont* fontOpt,
		SpaceUnitsKind markerUnits) noexcept
	{
		LengthResolveCtx ctx = makeLengthCtxUser(percentRef, /*origin*/0.0, dpi, fontOpt);
		double v = resolveLengthOr(L, ctx, /*fallback*/0.0);

		if (markerUnits == SpaceUnitsKind::SVG_SPACE_STROKEWIDTH)
			v *= strokeWidth;

		return v;
	}

	// Resolve marker state for a specific instance.
//
// Inputs you already have at draw time:
// - nearestVP: ctx->viewport() (nearest viewport for percentages in markerWidth/Height when userSpaceOnUse)
// - strokeWidth: ctx->getStrokeWidth()
//
// Output:
// - out.viewport: {0,0,w,h}
// - out.viewBox: authored viewBox or default {0,0,w,h}
// - out.contentToInstance: viewBox->viewport + refX/refY shift baked in
//
	
	static INLINE bool resolveMarkerState(
		const DocMarkerState& doc,
		const BLRect& nearestVP,
		double strokeWidth,
		double dpi,
		const BLFont* fontOpt,
		ResolvedMarkerState& out) noexcept
	{
		out = ResolvedMarkerState{};

		// ----------------------------
		// 1) Resolve markerWidth/Height in TWO spaces:
		//    - "units"  : marker content units (strokeWidth units if markerUnits=strokeWidth)
		//    - "user"   : parent user space units (actual instance size)
		//
		// Key fix:
		//   When markerUnits=strokeWidth and there is NO viewBox specified,
		//   the default content coordinate system must be in *strokeWidth units*
		//   (i.e. w_units/h_units), while the viewport is w_user/h_user.
		// ----------------------------

		// Percentage reference policy (keep your original intent):
		// - userSpaceOnUse: % uses nearest viewport w/h
		// - strokeWidth  : % uses strokeWidth
		const double percentRefW =
			(doc.markerUnits == SpaceUnitsKind::SVG_SPACE_USER) ? nearestVP.w : strokeWidth;
		const double percentRefH =
			(doc.markerUnits == SpaceUnitsKind::SVG_SPACE_USER) ? nearestVP.h : strokeWidth;

		// Resolve markerWidth/Height to "base" scalar (pre-markerUnits scaling).
		// We do this explicitly so we can preserve "units space" even when we later
		// multiply by strokeWidth for the actual instance size.
		//
		// NOTE: resolveLengthOr expects a context where percentRef makes sense.
		// For SVG_SPACE_STROKEWIDTH, our percentRef is strokeWidth (policy).
		LengthResolveCtx mwCtx = makeLengthCtxUser(percentRefW, 0.0, dpi, fontOpt);
		LengthResolveCtx mhCtx = makeLengthCtxUser(percentRefH, 0.0, dpi, fontOpt);

		double w_units = resolveLengthOr(doc.markerWidth, mwCtx, 0.0);
		double h_units = resolveLengthOr(doc.markerHeight, mhCtx, 0.0);

		// Clamp invalid sizes
		if (w_units < 0.0) w_units = 0.0;
		if (h_units < 0.0) h_units = 0.0;

		// Convert to user space instance size
		double w_user = w_units;
		double h_user = h_units;
		if (doc.markerUnits == SpaceUnitsKind::SVG_SPACE_STROKEWIDTH) {
			w_user *= strokeWidth;
			h_user *= strokeWidth;
		}

		out.viewport = BLRect{ 0.0, 0.0, w_user, h_user };
		if (out.viewport.w <= 0.0 || out.viewport.h <= 0.0)
			return false;

		// ----------------------------
		// 2) Establish the marker content viewBox (content user space)
		// ----------------------------
		out.hasViewBox = doc.vp.hasViewBox;
		if (doc.vp.hasViewBox) {
			out.viewBox = doc.vp.viewBox;
		}
		else {
			// KEY FIX:
			// If no authored viewBox:
			// - markerUnits=userSpaceOnUse: content space == viewport in user units
			// - markerUnits=strokeWidth  : content space == marker units (unscaled)
			out.viewBox = (doc.markerUnits == SpaceUnitsKind::SVG_SPACE_STROKEWIDTH)
				? BLRect{ 0.0, 0.0, w_units, h_units }
			: BLRect{ 0.0, 0.0, w_user,  h_user };
		}

		if (out.viewBox.w <= 0.0 || out.viewBox.h <= 0.0)
			return false;

		// ----------------------------
		// 3) Resolve refX/refY in marker content space (viewBox space)
		// ----------------------------
		{
			const double refRefW = out.viewBox.w;
			const double refRefH = out.viewBox.h;

			LengthResolveCtx rxCtx = makeLengthCtxUser(refRefW, 0.0, dpi, fontOpt);
			LengthResolveCtx ryCtx = makeLengthCtxUser(refRefH, 0.0, dpi, fontOpt);

			const double refX = resolveLengthOr(doc.refX, rxCtx, 0.0);
			const double refY = resolveLengthOr(doc.refY, ryCtx, 0.0);

			// ----------------------------
			// 4) Build content->instance mapping:
			//    (viewBox -> viewport) + alignment + ref shift
			// ----------------------------
			const BLRect& viewport = out.viewport; // in USER space
			const BLRect& viewBox = out.viewBox;  // in CONTENT space
			const PreserveAspectRatio& par = doc.vp.par;

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

			BLMatrix2D m = BLMatrix2D::makeIdentity();
			m.translate(viewport.x, viewport.y);
			m.translate(ax, ay);
			m.scale(sx, sy);
			m.translate(-(viewBox.x + refX), -(viewBox.y + refY)); // ref baked here

			out.contentToInstance = m;
		}

		out.resolved = true;
		return true;
	}
	

	/*
	static INLINE bool resolveMarkerState(
		const DocMarkerState& doc,
		const BLRect& nearestVP,
		double strokeWidth,
		double dpi,
		const BLFont* fontOpt,
		ResolvedMarkerState& out) noexcept
	{
		out = ResolvedMarkerState{};

		// Defaults: if doc markerWidth/Height not set, SVG default is 3
		// (your DocMarkerState constructor already sets them as set=true).

		// Percentage refs:
		// - userSpaceOnUse: markerWidth/Height % use nearest viewport w/h
		// - strokeWidth: % treated as % of strokeWidth (simple + matches your previous intent)
		const double refW = (doc.markerUnits == SpaceUnitsKind::SVG_SPACE_USER) ? nearestVP.w : strokeWidth;
		const double refH = (doc.markerUnits == SpaceUnitsKind::SVG_SPACE_USER) ? nearestVP.h : strokeWidth;

		double w = resolveMarkerLen(doc.markerWidth, refW, strokeWidth, dpi, fontOpt, doc.markerUnits);
		double h = resolveMarkerLen(doc.markerHeight, refH, strokeWidth, dpi, fontOpt, doc.markerUnits);

		// Clamp invalid sizes
		if (w < 0.0) w = 0.0;
		if (h < 0.0) h = 0.0;

		out.viewport = BLRect{ 0.0, 0.0, w, h };

		if (out.viewport.w <= 0.0 || out.viewport.h <= 0.0)
			return false;

		// viewBox/par
		out.hasViewBox = doc.vp.hasViewBox;
		out.viewBox = doc.vp.hasViewBox
			? doc.vp.viewBox
			: BLRect{ 0.0, 0.0, out.viewport.w, out.viewport.h };

		if (out.viewBox.w <= 0.0 || out.viewBox.h <= 0.0)
			return false;

		// Resolve refX/refY:
		// Spec-wise these are in the marker’s own coordinate system (i.e. viewBox space when viewBox is present).
		// So % references should use viewBox w/h (not viewport w/h).
		const double refRefW = out.viewBox.w;
		const double refRefH = out.viewBox.h;

		LengthResolveCtx rxCtx = makeLengthCtxUser(refRefW, 0.0, dpi, fontOpt);
		LengthResolveCtx ryCtx = makeLengthCtxUser(refRefH, 0.0, dpi, fontOpt);

		const double refX = resolveLengthOr(doc.refX, rxCtx, 0.0);
		const double refY = resolveLengthOr(doc.refY, ryCtx, 0.0);

		// Build content->instance matrix:
		// This is computeViewBoxToViewport(...) PLUS translating so that (refX,refY) becomes the origin.
		//
		// We can bake refX/refY into the final translate that also accounts for viewBox.x/y:
		//   ... scale(sx,sy); translate(-(viewBox.x + refX), -(viewBox.y + refY))
		//
		// We reuse your alignment logic.
		{
			const BLRect& viewport = out.viewport;
			const BLRect& viewBox = out.viewBox;
			const PreserveAspectRatio& par = doc.vp.par;

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

			BLMatrix2D m = BLMatrix2D::makeIdentity();
			m.translate(viewport.x, viewport.y);
			m.translate(ax, ay);
			m.scale(sx, sy);
			m.translate(-(viewBox.x + refX), -(viewBox.y + refY)); // ref baked here
			out.contentToInstance = m;
		}

		out.resolved = true;
		return true;
	}
	*/

		// Load DocMarkerState from styled attributes (no binding).
		static INLINE void loadDocMarkerState(DocMarkerState& d, const XmlAttributeCollection& attrs) noexcept
		{
			d = DocMarkerState{}; // reset defaults

			// markerUnits
			ByteSpan mu{};
			if (attrs.getValue(svgattr::markerUnits(), mu))
				getEnumValue(MarkerUnitEnum, mu, (uint32_t&)d.markerUnits);

			// markerWidth/markerHeight (defaults already in d)
			ByteSpan mw{}, mh{};
			if (attrs.getValue(svgattr::markerWidth(), mw))  d.markerWidth = parseLengthAttr(mw);
			if (attrs.getValue(svgattr::markerHeight(), mh)) d.markerHeight = parseLengthAttr(mh);

			// refX/refY (defaults already in d)
			ByteSpan rx{}, ry{};
			if (attrs.getValue(svgattr::refX(), rx)) d.refX = parseLengthAttr(rx);
			if (attrs.getValue(svgattr::refY(), ry)) d.refY = parseLengthAttr(ry);

			// orient
			ByteSpan o{};
			if (attrs.getValue(svgattr::orient(), o))
				d.orient.loadFromChunk(o);

			// viewBox/par
			loadDocViewportState(d.vp, attrs);
			d.hasVP = true;
		}


}

namespace waavs {
	//=================================================
	// SVGMarkerNode

	// Reference: https://svg-art.ru/?page_id=855
	//=================================================

	struct SVGMarkerElement : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			registerContainerNodeByName("marker",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGMarkerElement>(groot);
					node->loadFromXmlPull(iter, groot);

					return node;
				});
			
		}

		

		DocMarkerState fDoc;
		bool fHasDoc{ false };

		// Cached resolved (optional, but pragmatic)
		ResolvedMarkerState fRes{};
        double fResStrokeWidth{ -1.0 }; // track strokeWidth used for resolution so we know when to re-resolve
        BLRect fResNearestVP{}; // track nearest viewport used for resolution so we know when to re-resolve
        double fResDpi{ -1.0 }; // track dpi used for resolution so we know when to re-resolve
		bool fHasResolved{ false };

		SVGMarkerElement(IAmGroot *) : SVGGraphicsElement()
		{
			setIsStructural(true);
			setIsVisible(false);
		}


		const SVGOrient& orientation() const { return fDoc.orient; }
	
		// For markers, the viewport is the marker tile size
		BLRect objectBoundingBox() const override
		{
			return fHasResolved ? fRes.viewport : BLRect{};
		}
		

		void fixupSelfStyleAttributes(IAmGroot*) override
		{
			loadDocMarkerState(fDoc, fAttributes);
			fHasDoc = true;

			// clear cache
			fHasResolved = false;
			fResStrokeWidth = -1.0;
			fResDpi = -1.0;
			fResNearestVP.reset();
		}

		
		void bindSelfToContext(IRenderSVG *ctx, IAmGroot *groot) override 
		{
			// do nothing here as there's nothing persistent to bind
			// markers depend on current strokeWidth and nearest viewport
            // so we resolve them at draw time, and cache the result, 
			// but we don't have anything to bind here.
		}

		// Resolve (with a tiny cache) using the current context.
		INLINE bool ensureResolved(IRenderSVG* ctx, IAmGroot* groot) noexcept
		{
			if (!fHasDoc || !ctx) return false;

			const double dpi = groot ? groot->dpi() : 96.0;
			const BLFont* fontOpt = &ctx->getFont();

			const double sw = ctx->getStrokeWidth();
			const BLRect nearestVP = ctx->viewport();

			// Tiny cache: reuse if same parameters
			if (fHasResolved &&
				fResStrokeWidth == sw &&
				fResDpi == dpi &&
				fResNearestVP.x == nearestVP.x && fResNearestVP.y == nearestVP.y &&
				fResNearestVP.w == nearestVP.w && fResNearestVP.h == nearestVP.h)
			{
				return true;
			}

			ResolvedMarkerState tmp{};
			if (!resolveMarkerState(fDoc, nearestVP, sw, dpi, fontOpt, tmp))
				return false;

			fRes = tmp;
			fResStrokeWidth = sw;
			fResNearestVP = nearestVP;
			fResDpi = dpi;
			fHasResolved = true;
			return true;
		}


		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			if (!ensureResolved(ctx, groot))
				return;

			// MarkerProgramExec already did:
			//   ctx->translate(vertex);
			//   ctx->rotate(angle);
			// So here we only establish marker content coordinates.

			// Map marker content user space -> marker instance user space
			ctx->applyTransform(fRes.contentToInstance);

			// Establish nearest viewport for marker children (percent lengths inside marker)
			// Use the marker's content user space viewport (viewBox).
			ctx->setViewport(fRes.viewBox);
            //ctx->setViewport(fRes.viewport);
		}

		void drawChildren(IRenderSVG* ctx, IAmGroot* groot) override
		{
			// Keep your current marker child paint defaults
			ctx->push();

			ctx->blendMode(BL_COMP_OP_SRC_OVER);
			ctx->fill(BLRgba32(0, 0, 0));
			ctx->noStroke();
			ctx->strokeWidth(1.0);
			ctx->lineJoin(BLStrokeJoin::BL_STROKE_JOIN_MITER_BEVEL);

			SVGGraphicsElement::drawChildren(ctx, groot);

			ctx->pop();
		}


	};
}

namespace waavs 
{

	struct ResolvedMarkers {
		std::shared_ptr<SVGMarkerElement> start;
		std::shared_ptr<SVGMarkerElement> mid;
		std::shared_ptr<SVGMarkerElement> end;
		std::shared_ptr<SVGMarkerElement> any; // 'marker' fallback

		INLINE bool hasAny() const noexcept { return (start || mid || end || any); }
	};

    // Pick the marker for the given position from a resolved set of markers
    // default is the 'any' marker
	INLINE SVGMarkerElement* pickMarker(const ResolvedMarkers& rm, MarkerPosition pos) noexcept
	{
		switch (pos) {
		case MarkerPosition::MARKER_POSITION_START:  return rm.start ? rm.start.get() : rm.any.get();
		case MarkerPosition::MARKER_POSITION_MIDDLE: return rm.mid ? rm.mid.get() : rm.any.get();
		case MarkerPosition::MARKER_POSITION_END:    return rm.end ? rm.end.get() : rm.any.get();
		}
		return rm.any.get();
	}


	INLINE std::shared_ptr<SVGMarkerElement> resolveMarkerNode(ISVGElement* self, IRenderSVG* ctx, IAmGroot* groot, InternedKey key)
	{
		auto prop = self->getVisualProperty(key);
		if (!prop) return nullptr;

		auto attr = std::dynamic_pointer_cast<SVGMarkerAttribute>(prop);
		if (!attr) return nullptr;

		auto node = attr->markerNode(ctx, groot);
		if (!node) return nullptr;

		return std::dynamic_pointer_cast<SVGMarkerElement>(node);
	}
}


// Marker resolution helpers
namespace waavs
{

	struct MarkerProgramExec
	{
		IRenderSVG* ctx{};
		IAmGroot* groot{};
		ISVGElement* owner{};
		ResolvedMarkers rm{};

		// --- path state ---
		BLPoint subStart{};
		BLPoint cur{};
		bool hasCP{ false };

		bool subpathOpen{ false };
		bool pendingStart{ false };     // saw moveto, not yet emitted start marker (need first segment tangent)

		bool havePrevEndTan{ false };
		BLPoint prevEndTan{};         // tangent at end of previous segment (incoming to current vertex)

		// last segment info, used for end marker when a subpath ends without CLOSE
		bool haveLastSeg{ false };
		BLPoint lastEndTan{};

		explicit MarkerProgramExec(IRenderSVG* c, IAmGroot* g, ISVGElement* o)
			: ctx(c), groot(g), owner(o)
		{
		}

		// Call once before running program
		bool initResolvedMarkers() noexcept
		{
			rm.start = resolveMarkerNode(owner, ctx, groot, svgattr::marker_start());
			rm.mid = resolveMarkerNode(owner, ctx, groot, svgattr::marker_mid());
			rm.end = resolveMarkerNode(owner, ctx, groot, svgattr::marker_end());
			rm.any = resolveMarkerNode(owner, ctx, groot, svgattr::marker());
			return rm.hasAny();
		}

		// --- small vector helpers ---
		static INLINE BLPoint sub(const BLPoint& a, const BLPoint& b) noexcept { return { a.x - b.x, a.y - b.y }; }

		static INLINE bool isZero(const BLPoint& v) noexcept { return v.x == 0.0 && v.y == 0.0; }

		// Manufacture 3 points from tangents for your existing orientation API:
		// p1 = pt - inTan, p2 = pt, p3 = pt + outTan
		static INLINE void makeTriplet(const BLPoint& pt, const BLPoint& inTan, const BLPoint& outTan,
			BLPoint& p1, BLPoint& p2, BLPoint& p3) noexcept
		{
			p1 = { pt.x - inTan.x,  pt.y - inTan.y };
			p2 = pt;
			p3 = { pt.x + outTan.x, pt.y + outTan.y };
		}

		INLINE void drawMarkerAt(MarkerPosition pos, const BLPoint& pt, const BLPoint& inTan, const BLPoint& outTan) noexcept
		{
			SVGMarkerElement* m = pickMarker(rm, pos);
			if (!m) return;

			// Avoid degenerate atan2(0,0) situations by falling back.
			BLPoint inV = inTan;
			BLPoint outV = outTan;
			if (isZero(inV) && !isZero(outV)) inV = outV;
			if (isZero(outV) && !isZero(inV)) outV = inV;
			if (isZero(inV) && isZero(outV)) { outV = { 1.0, 0.0 }; inV = outV; }

			BLPoint p1{}, p2{}, p3{};
			makeTriplet(pt, inV, outV, p1, p2, p3);

			const double rads = m->orientation().calcRadians(pos, p1, p2, p3);

			ctx->push();
			ctx->translate(pt);
			ctx->rotate(rads);
			m->draw(ctx, groot);
			ctx->pop();
		}

		// Called at the *start* of a new segment, with the segment's start tangent.
		INLINE void atSegmentStart(const BLPoint& segStartTan) noexcept
		{
			if (!subpathOpen) return;

			if (pendingStart) {
				// start marker at subStart, oriented by first segment's start tangent
				drawMarkerAt(MarkerPosition::MARKER_POSITION_START, subStart, segStartTan, segStartTan);
				pendingStart = false;
				// At the first segment start, there is no "incoming" tangent for a mid marker.
				havePrevEndTan = false;
				return;
			}

			if (havePrevEndTan) {
				// mid marker at current vertex (cur), oriented by incoming and outgoing tangents
				drawMarkerAt(MarkerPosition::MARKER_POSITION_MIDDLE, cur, prevEndTan, segStartTan);
			}
		}

		// Called at the end of a segment, with the segment's end tangent.
		INLINE void atSegmentEnd(const BLPoint& segEndTan) noexcept
		{
			havePrevEndTan = true;
			prevEndTan = segEndTan;

			haveLastSeg = true;
			lastEndTan = segEndTan;
		}

		// Finish a subpath that ended without OP_CLOSE (either OP_MOVETO starts a new subpath, or OP_END).
		INLINE void finishOpenSubpath() noexcept
		{
			if (!subpathOpen) return;

			if (pendingStart) {
				// degenerate subpath: moveTo only, no segments
				// pragmatic: if markers exist, draw start & end at that point
				drawMarkerAt(MarkerPosition::MARKER_POSITION_START, subStart, { 1,0 }, { 1,0 });
				drawMarkerAt(MarkerPosition::MARKER_POSITION_END, subStart, { 1,0 }, { 1,0 });
			}
			else if (haveLastSeg) {
				drawMarkerAt(MarkerPosition::MARKER_POSITION_END, cur, lastEndTan, lastEndTan);
			}

			// reset per-subpath
			subpathOpen = false;
			pendingStart = false;
			havePrevEndTan = false;
			haveLastSeg = false;
		}

		// --- executor entry point ---
		void execute(uint8_t op, const float* a) noexcept
		{
			switch ((PathOp)op) {
			case OP_MOVETO: {
				// Starting a new subpath implicitly ends the previous one (if any)
				finishOpenSubpath();

				cur = { a[0], a[1] };
				subStart = cur;

				hasCP = true;
				subpathOpen = true;
				pendingStart = true;

				havePrevEndTan = false;
				haveLastSeg = false;
			} break;

			case OP_LINETO: {
				if (!hasCP) break;
				const BLPoint p0 = cur;
				const BLPoint p1{ a[0], a[1] };

				const BLPoint t = sub(p1, p0);          // start and end tangents are same for line
				atSegmentStart(t);

				cur = p1;
				atSegmentEnd(t);
			} break;

			case OP_QUADTO: {
				if (!hasCP) break;
				const BLPoint p0 = cur;
				const BLPoint c{ a[0], a[1] };
				const BLPoint p1{ a[2], a[3] };

				// derivatives at ends (scaled, scale doesn't matter for angle)
				const BLPoint t0 = sub(c, p0);          // proportional to 2*(c-p0)
				const BLPoint t1 = sub(p1, c);          // proportional to 2*(p1-c)

				atSegmentStart(t0);

				cur = p1;
				atSegmentEnd(t1);
			} break;

			case OP_CUBICTO: {
				if (!hasCP) break;
				const BLPoint p0 = cur;
				const BLPoint c1{ a[0], a[1] };
				const BLPoint c2{ a[2], a[3] };
				const BLPoint p1{ a[4], a[5] };

				const BLPoint t0 = sub(c1, p0);         // proportional to 3*(c1-p0)
				const BLPoint t1 = sub(p1, c2);         // proportional to 3*(p1-c2)

				atSegmentStart(t0);

				cur = p1;
				atSegmentEnd(t1);
			} break;

			case OP_ARCTO: {
				if (!hasCP) break;
				// args: rx ry xrot large sweep x y
				const BLPoint p0 = cur;
				const BLPoint p1{ a[5], a[6] };

				// Pragmatic tangent: chord direction (good enough for arrows in many cases)
				const BLPoint t = sub(p1, p0);

				atSegmentStart(t);

				cur = p1;
				atSegmentEnd(t);
			} break;

			case OP_CLOSE: {
				if (!hasCP) break;
				// CLOSE adds a segment back to subStart and sets current point = subStart
				const BLPoint p0 = cur;
				const BLPoint p1 = subStart;
				const BLPoint t = sub(p1, p0);

				atSegmentStart(t);

				cur = p1;
				// End marker on close is at the close point (subStart / now cur)
				drawMarkerAt(MarkerPosition::MARKER_POSITION_END, cur, t, t);

				// reset per-subpath
				subpathOpen = false;
				pendingStart = false;
				havePrevEndTan = false;
				haveLastSeg = false;
			} break;

			case OP_END:
			default:
				// OP_END is handled outside by caller (or you can handle here too)
				break;
			}
		}
	};

}

namespace waavs
{
    // Most useful utility:
    // Called by an element (owner) to draw markers along a given path program
	//
	INLINE bool drawMarkersForPathProgram(ISVGElement* owner, IRenderSVG* ctx, IAmGroot* groot, const PathProgram& prog) noexcept
	{
		if (!owner || !ctx || !groot) return false;

		MarkerProgramExec exec(ctx, groot, owner);
		if (!exec.initResolvedMarkers())
			return false;

		runPathProgram(prog, exec);
		exec.finishOpenSubpath();
		return true;
	}
}
