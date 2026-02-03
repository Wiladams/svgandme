#pragma once

//
// Feature String: http://www.w3.org/TR/SVG11/feature#Marker
//
//#include <functional>
//#include <unordered_map>

#include "svgattributes.h"
#include "svgstructuretypes.h"
#include "svgportal.h"

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

		// Fields
		SVGPortal fPortal{};
		SVGVariableSize fDimRefX{};
		SVGVariableSize fDimRefY{};

		SpaceUnitsKind fMarkerUnits{ SpaceUnitsKind::SVG_SPACE_STROKEWIDTH };
		SVGOrient fOrientation{ nullptr };

		// Resolved field values
		double fRefX = 0;
		double fRefY = 0;

		
		BLPoint fMarkerContentScale{ 1,1 };
		BLPoint fMarkerTranslation{ 0,0 };
		BLRect fMarkerBoundingBox{ 0,0,3,3 };
		BLRect fViewbox{};
		
		
		SVGMarkerElement(IAmGroot* root)
			: SVGGraphicsElement()
		{
			setIsStructural(false);
		}

		BLRect viewPort() const override
		{
            BLRect vpFrame{};
			fPortal.getViewportFrame(vpFrame);

			return vpFrame;
		}
		
		BLRect getBBox() const override
		{
			return fPortal.getBBox();
		}
		

		const SVGOrient& orientation() const { return fOrientation; }


		//
		// createPortal
		// 
		// This code establishes the coordinate space for the element, and 
		// its child nodes.
		// 
		
		void createPortal(IRenderSVG *ctx, IAmGroot *groot)
		{
			// First, we want to know the size of the object we're going to be
			// rendered into
			// We also want to know the size of the container the object is in
			BLRect objectBoundingBox = ctx->getObjectFrame();
			BLRect containerBoundingBox = ctx->viewport();
			
			
			double dpi = 96;
			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}

			// Load parameters for the portal
			SVGDimension fDimWidth{};
			SVGDimension fDimHeight{};

			// If these are not specified, then we use default values of '3'
			fDimWidth.loadFromChunk(getAttributeByName("markerWidth"));
			fDimHeight.loadFromChunk(getAttributeByName("markerHeight"));
			bool haveViewbox = parseViewBox(getAttributeByName("viewBox"), fViewbox);
			fPortal.loadFromAttributes(fAttributes);
			
			double sWidth = ctx->getStrokeWidth();

			// First, we setup the marker bounding box
			// This is determined based on the markerWidth, and markerHeight
			// attributes.
			if (fMarkerUnits == SpaceUnitsKind::SVG_SPACE_STROKEWIDTH)
			{
				// We use the strokeWidth to scale the marker
				// If the dimension type is percentage, then calculate
				// based on a percentage of the stroke width
				// If the dimension is a number, then use that number, and 
				// multiply it by the strokeWidth
				if (fDimWidth.isSet())
				{
					if (fDimWidth.isPercentage())
					{
						fMarkerBoundingBox.w = fDimWidth.calculatePixels(sWidth);
					}
					else
					{
						fMarkerBoundingBox.w = fDimWidth.calculatePixels() * sWidth;
					}
				}
				
				if (fDimHeight.isSet())
				{
					if (fDimWidth.isPercentage())
					{
						fMarkerBoundingBox.h = fDimHeight.calculatePixels(sWidth);
					}
					else {
						fMarkerBoundingBox.h = fDimHeight.calculatePixels() * sWidth;
					}
				}
			}
			else if (fMarkerUnits == SpaceUnitsKind::SVG_SPACE_USER)
			{
				if (fDimWidth.isSet())
				{
					fMarkerBoundingBox.w = fDimWidth.calculatePixels(containerBoundingBox.w)*sWidth;
				}

				if (fDimHeight.isSet())
				{
					fMarkerBoundingBox.h = fDimHeight.calculatePixels(containerBoundingBox.h)*sWidth;
				}
			}
			
			// Now that we have the markerBoundingBox settled, we need to calculate an additional 
			// scaling for the content area if a viewBox is specified
			
			if (haveViewbox) {
				fMarkerContentScale.x = fMarkerBoundingBox.w / fViewbox.w;
				fMarkerContentScale.y = fMarkerBoundingBox.h / fViewbox.h;
			}
			
			fDimRefX.parseValue(fMarkerTranslation.x, ctx->getFont(), fMarkerBoundingBox.w, 0, dpi);
			fDimRefY.parseValue(fMarkerTranslation.y, ctx->getFont(), fMarkerBoundingBox.h, 0, dpi);

			fPortal.setViewportFrame(fMarkerBoundingBox);
		}
		
		void fixupSelfStyleAttributes(IRenderSVG*, IAmGroot*) override
		{
			// printf("fixupSelfStyleAttributes\n");
			getEnumValue(MarkerUnitEnum, getAttributeByName("markerUnits"), (uint32_t&)fMarkerUnits);
			//getEnumValue(SVGAspectRatioEnum, getAttribute("preserveAspectRatio"), (uint32_t&)fPreserveAspectRatio);

			fDimRefX.loadFromChunk(getAttributeByName("refX"));
			fDimRefY.loadFromChunk(getAttributeByName("refY"));
			fOrientation.loadFromChunk(getAttributeByName("orient"));

		}

		
		void bindSelfToContext(IRenderSVG *ctx, IAmGroot *groot) override 
		{
			createPortal(ctx, groot);

		}


		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			//createPortal(ctx, groot);
			
			ctx->scale(fMarkerContentScale.x, fMarkerContentScale.y);
			ctx->translate(-fMarkerTranslation.x, -fMarkerTranslation.y);

		}

		void drawChildren(IRenderSVG* ctx, IAmGroot* groot) override
		{
			ctx->push();

			// Setup drawing state
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
