#pragma once

#include <list>
#include <memory>

#include "blend2d.h"
#include "svgattributes.h"
#include "svgpath.h"
#include "b2dpath.h"

namespace waavs {

	// The base class for something that can be drawn into
	// a BLContext
	struct AGraphic
	{
		SVGDrawingState fGraphState{};
		ViewportTransformer fPortal;
		BLRect fFrame{};
		BLSize fExtent{};

		
		AGraphic() = default;
		
		// Some conveniences
		virtual bool contains(BLPoint& pt)
		{
			// determine if the pt is within the bounds
			BLRect b = frame();
			return pt.x >= b.x && pt.x <= b.x + b.w &&
				pt.y >= b.y && pt.y <= b.y + b.h;
		}
		
		// The frame is expressed in the coordinate frame of the parent
		// graphic that contains this graphic.
		virtual BLRect frame() const noexcept
		{
			return fFrame;
		}

		void setFrame(const BLRect& fr) noexcept
		{
			fFrame = fr;
		}

		// The viewport represents the coordinate space within
		// which the graphic will be displayed.  The viewport
		// and viewbox combine to create the scaling matrix that
		// is applied before drawing occurs.
		// By default, the viewport would match the viewBox, creating
		// an identity matrix.
		// 
		virtual BLRect viewport() const noexcept { return fPortal.viewportFrame(); }
		void setViewport(const BLRect& fr)
		{
			fPortal.viewportFrame(fr);
		}
		
		// The bounds reports the portion of the graphic that
		// will be shown within the frame
		void setViewbox(const BLRect& b)
		{
			fPortal.viewBoxFrame(b);
		}
		virtual BLRect viewBox() { return fPortal.viewBoxFrame(); }
		

		// The extent represents the greatest boundary the drawing
		// will do.  This is different than the bounds, which 
		const BLSize& extent() const { return fExtent; }
		void setExtent(const BLSize& sz)
		{
			fExtent = sz;
		}

		// Setting Attributes
		const BLVar strokeStyle() const { return fGraphState.getStrokePaint(); }
		void setStrokeStyle(const BLVar& style) { fGraphState.setStrokePaint(style); }

		const BLVar& fillStyle() const { return fGraphState.getFillPaint(); }
		void setFillStyle(const BLVar& style) { fGraphState.setFillPaint(style); }

		void setPreserveAspectRatio(const char* par)
		{
			PreserveAspectRatio preserveAspectRatio(par);
			fPortal.preserveAspectRatio(preserveAspectRatio);
		}

		void setPreserveAspectRatio(const PreserveAspectRatio& par)
		{
			fPortal.preserveAspectRatio(par);
		}
		

		
		// Altering the graph state
		bool setAttribute(const ByteSpan& attName, const ByteSpan& attValue)
		{
			if (attName == "viewBox") {
				BLRect fr{};
				if (!parseViewBox(attValue, fr))
					return false;
				
				setViewbox(fr);
			}
			else if (attName == "portal") {
				BLRect fr{};
				if (!parseViewBox(attValue, fr))
					return false;

				setViewport(fr);
			}
			else if (attName == "preserveAspectRatio")
			{
				PreserveAspectRatio par(attValue);
				setPreserveAspectRatio(par);
			}
			if (attName == "fill")
			{
				SVGPaint paint(nullptr);
				paint.loadFromChunk(attValue);
				fGraphState.fillPaint(paint.getVariant(nullptr, nullptr));
			}
			else if (attName == "stroke")
			{
				SVGPaint apaint(nullptr);
				apaint.loadFromChunk(attValue);
				fGraphState.strokePaint(apaint.getVariant(nullptr, nullptr));
			}
			else {
				return fGraphState.setAttribute(attName, attValue);
			}

			return true;
		}

		// Set style attributes based on the collection
		// of attributes specified in the span.
		bool setStyle(const ByteSpan& attrs)
		{
			ByteSpan src = attrs;
			ByteSpan key;
			ByteSpan value;
			
			while (readNextCSSKeyValue(src, key, value))
			{
				setAttribute(key, value);
			}

			return true;
		}
		
		// Draw the graphic into the BLContext
		virtual void drawBackground(BLContext* ctx)
		{
			BLRect fr = frame();
			ctx->strokeGeometry(BLGeometryType::BL_GEOMETRY_TYPE_RECTD, &fr, BLRgba32(0xff000000));
		}
		
		virtual void drawSelf(BLContext* ctx) = 0;
		
		virtual void draw(BLContext* ctx)
		{
			ctx->save();
			
			// apply graphics state
			fGraphState.applyState(ctx);
			
			this->drawBackground(ctx);
			
			// Apply transform
			ctx->applyTransform(fPortal.viewBoxToViewportTransform());
			
			// Call drawSelf()
			this->drawSelf(ctx);
			ctx->restore();
		}


	};
	
	using AGraphicHandle = std::shared_ptr<AGraphic>;
	


	
	// A graphic that is based on a BLPath
	// This is a leaf node
	struct AGraphicShape : public AGraphic
	{

		BLPath fPath{};

		BLFillRule fFillRule{BLFillRule::BL_FILL_RULE_EVEN_ODD};
		uint32_t fPaintOrder{ SVG_PAINT_ORDER_NORMAL };

		AGraphicShape() = default;
		AGraphicShape(const ByteSpan& pathSpan)
		{
			initFromData(pathSpan);
		}
		

		
		void initFromData(const ByteSpan& inPath)
		{
			parsePath(inPath, fPath);
			setBounds(pathBounds(fPath));
		}
		


		
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
		
		
		void drawSelf(BLContext* ctx) override
		{
			uint32_t porder = fPaintOrder;
			
			for (int slot = 0; slot < 3; slot++)
			{
				uint32_t ins = porder & 0x03;	// get two lowest bits, which are a single instruction

				switch (ins)
				{
				case PaintOrderKind::SVG_PAINT_ORDER_FILL:
					ctx->fillPath(fPath, fillStyle());
					break;

				case PaintOrderKind::SVG_PAINT_ORDER_STROKE:
					ctx->strokePath(fPath, strokeStyle());
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