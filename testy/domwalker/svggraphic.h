#pragma once

#include <list>
#include <memory>

#include "blend2d.h"
#include "svgenums.h"

namespace waavs {
	
	// AGraphState encapsulates the set of attributes that should be
	// applied when drawing a shape.  These attributes are stored together
	// so they can be applied to a BLContext in a single operation.
	// Or can be referenced by multiple graphic objects
	struct AGraphState final
	{
		BLVar fStrokeStyle{};
		BLVar fFillStyle{};
		BLFillRule fFillRule{ BLFillRule::BL_FILL_RULE_EVEN_ODD };
		uint32_t fPaintOrder{ SVG_PAINT_ORDER_NORMAL };
		BLStrokeCap fStrokeCap{ BLStrokeCap::BL_STROKE_CAP_ROUND };
		//BLStrokeCapPosition fCapPosition{BLStrokeCapPosition::BL_STROKE_CAP_POSITION_END}
	};
	
	// The base class for something that can be drawn into
	// a BLContext
	struct AGraphic
	{
		// The bounds reports the size and location of the graphic
		// relative to the container within which it is drawn
		virtual BLRect bounds() = 0;

		// Draw the graphic into the BLContext
		virtual void draw(BLContext* ctx) = 0;

		// Some conveniences
		virtual bool contains(BLPoint &pt)
		{
			// determine if the pt is within the bounds
			BLRect b = bounds();
			return pt.x >= b.x && pt.x <= b.x + b.w &&
				pt.y >= b.y && pt.y <= b.y + b.h;
		}
	};
	
	using AGraphicHandle = std::shared_ptr<AGraphic>;
	


	
	// A graphic that is based on a BLPath
	// This is a leaf node
	struct AGraphicShape : public AGraphic
	{
		BLPath fPath{};
		BLVar fStrokeStyle{};
		BLVar fFillStyle{};
		BLFillRule fFillRule{BLFillRule::BL_FILL_RULE_EVEN_ODD};
		uint32_t fPaintOrder{ SVG_PAINT_ORDER_NORMAL };

		
		const BLVar& strokeStyle() const { return fStrokeStyle; }
		void setStrokeStyle(const BLVar& style){fStrokeStyle.assign(style);}

		const BLVar& fillStyle() const { return fFillStyle; }
		void setFillStyle(const BLVar& style){fFillStyle.assign(style);}
		
		void paintOrder(uint32_t po) { fPaintOrder = po; }
		
		// return path so it can be altered
		BLPath& path() { return fPath; }
		
		// Determine whether the specified point is within the fill portion of the shape
		bool contains(BLPoint &pt) override
		{

			// BUGBUG - do we need to check for a transform, and transform the point?
			// check to see if we have a transform property
			// if we do, transform the points through that transform
			// then check to see if the point is inside the transformed path
			//if (fHasTransform)
			//{
				// get the inverse transform
			//	auto inverse = fTransform;
			//	inverse.invert();
			//	localPoint = inverse.mapPoint(localPoint);
			//}


			// BUGBUG - should use actual fill rule
			BLHitTest ahit = fPath.hitTest(pt, BLFillRule::BL_FILL_RULE_EVEN_ODD);

			return (ahit == BLHitTest::BL_HIT_TEST_IN);
		}
		
		// fulfilling the AGraphic interface
		// Get the bounds of the shape.  This does not take into 
		// account the stroke paint, and can be quite liberal
		BLRect bounds()
		{
			// get the bounds of the path
			BLBox bbox{};
			fPath.getBoundingBox(&bbox);

			return BLRect(bbox.x0, bbox.y0, bbox.x1 - bbox.x0, bbox.y1 - bbox.y0);

		}
		
		void draw(BLContext* ctx) override
		{
			uint32_t porder = fPaintOrder;
			
			for (int slot = 0; slot < 3; slot++)
			{
				uint32_t ins = porder & 0x03;	// get two lowest bits, which are a single instruction

				switch (ins)
				{
				case PaintOrderKind::SVG_PAINT_ORDER_FILL:
					ctx->fillPath(fPath, fFillStyle);
					break;

				case PaintOrderKind::SVG_PAINT_ORDER_STROKE:
					ctx->strokePath(fPath, fStrokeStyle);
					break;

				case PaintOrderKind::SVG_PAINT_ORDER_MARKERS:
				{
					//drawMarkers(ctx, groot);
				}
				break;
				}

				// discard instruction, shift down to get the next one ready
				porder = porder >> 2;
			}


		}
	};

	struct AGraphicGroup : public AGraphic
	{
		std::list<AGraphicHandle> fChildren{};
		
		void addChild(AGraphicHandle  child)
		{
			fChildren.push_back(child);
		}
		
		void draw(BLContext* ctx) override
		{
			for (auto& child : fChildren)
			{
				child->draw(ctx);
			}
		}
	};
}