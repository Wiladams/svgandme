#pragma once

#include "blend2d/blend2d.h"
#include "irendersvg.h"

//
// These are various routines that help manipulate the BLRect
// structure.  Finding corners, moving, query containment
// scaling, merging, expanding, and the like

namespace waavs {
	inline double right(const BLRect& r) { return r.x + r.w; }
	inline double left(const BLRect& r) { return r.x; }
	inline double top(const BLRect& r) { return r.y; }
	inline double bottom(const BLRect& r) { return r.y + r.h; }
	inline BLPoint center(const BLRect& r) { return { r.x + (r.w / 2),r.y + (r.h / 2) }; }
	
	inline void moveBy(BLRect& r, double dx, double dy) { r.x += dx; r.y += dy; }
	inline void moveBy(BLRect& r, const BLPoint& dxy) { r.x += dxy.x; r.y += dxy.y; }
	
	
	inline bool containsRect(const BLRect& a, double x, double y)
	{
		return (x >= a.x && x < a.x + a.w && y >= a.y && y < a.y + a.h);
	}
	
	inline bool containsRect(const BLRect& a, const BLPoint& pt)
	{
		return containsRect(a, pt.x, pt.y);
	}
	
	inline BLRect mergeRect(const BLRect& a, const BLPoint& b) {
		return { min(a.x, b.x), min(a.y, b.y), max(a.x + a.w,b.x), max(a.y + a.h, b.y) };
	}
	
	inline BLRect mergeRect(const BLRect& a, const BLRect& b) {
		return { min(a.x, b.x), min(a.y, b.y), max(a.x + a.w, b.x + b.w), max(a.y + a.h,b.y + b.h) };
	}
	
	inline void expandRect(BLRect& a, const BLPoint& b) { a = mergeRect(a, b); }
	inline void expandRect(BLRect& a, const BLRect& b) { a = mergeRect(a, b); }
}

namespace waavs {
// Experimental
	struct IPlaceable
	{
		bool fAutoMoveToFront{ false };

		
		void autoMoveToFront(bool b) { fAutoMoveToFront = b; }
		bool autoMoveToFront() const { return fAutoMoveToFront; }
		
		virtual BLRect frame() const = 0;

		virtual bool contains(double x, double y)
		{
			return containsRect(frame(), x, y);
		}

		virtual void gainFocus() { ; }
		virtual void loseFocus(double x, double y) { ; }
		
		virtual void moveTo(double x, double y) = 0;
		virtual void moveBy(double dx, double dy) {
			moveTo(frame().x + dx, frame().y + dy);
		}

		//virtual void mouseEvent(const MouseEvent& e) { return; }
		//virtual void keyEvent(const KeyboardEvent& e) { return; }
	};


}

namespace waavs {
	// Something that can be drawn in the user interface
	struct IViewable : public IPlaceable, public ISVGDrawable
	{
		std::string fName{};
		
		void name(const ByteSpan& aname) { if (!aname) return;  fName = toString(aname); }
		void name(const char* aname) { fName = aname; }
		void name(const std::string& aname) { fName = aname; }
		const std::string & name() const { return fName; }
	};
}