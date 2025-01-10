#pragma once

#include <list>
#include <memory>

#include "blend2d.h"


namespace waavs {

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
		virtual bool contains(BLPoint pt)
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
		

		
		const BLVar& strokeStyle() const { return fStrokeStyle; }
		void setStrokeStyle(const BLVar& style){fStrokeStyle.assign(style);}

		const BLVar& fillStyle() const { return fFillStyle; }
		void setFillStyle(const BLVar& style){fFillStyle.assign(style);}
		
		// return path so it can be altered
		BLPath& path() { return fPath; }
		
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
			// BUGBUG - should use paint order
			ctx->strokePath(fPath, fStrokeStyle);
			ctx->fillPath(fPath, fFillStyle);
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