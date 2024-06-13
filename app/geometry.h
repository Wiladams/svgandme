#pragma once

#include "maths.h"


//=======================================
// DECLARATION RECTANGLE
//=======================================
namespace waavs
{
struct rectf
{
	float x;
	float y;
	float w;
	float h;
};

inline void moveTo(rectf& r, float x, float y);
inline void moveBy(rectf& r, float dx, float dy);

inline vec2f center(const rectf& r);
inline float width(const rectf& r);
inline float height(const rectf& r);
inline float right(const rectf& a);
inline float bottom(const rectf& a);
inline vec2f size(const rectf& r);
inline bool contains(const rectf& a, const vec2f& pt);

inline rectf merge(const rectf& a, const vec2f& b);
inline rectf merge(const rectf& a, const rectf& b);

inline void expand(rectf& a, const vec2f& b);
inline void expand(rectf& a, const rectf& b);

inline rectf intersection(const rectf& a, const rectf& b);
}



//=========================================
// DECLARATION AXIS ALIGNED BOUNDING BOX
//=========================================

namespace waavs
{


struct bbox2f 
{
	vec2f min = { flt_max, flt_max };
	vec2f max = { flt_min,flt_min };

	inline vec2f&		operator[](int i);
	inline const vec2f& operator[](int i) const;
};

struct bbox3f
{
	vec3f min = { flt_max, flt_max, flt_max };
	vec3f max = { flt_min, flt_min, flt_min };

	inline vec3f&		operator[](int i);
	inline const vec3f& operator[](int i) const;
};

// Empty box constants
constexpr bbox2f invalidb2f = bbox2f{};
constexpr bbox3f invalidb3f = bbox3f{};

//
// Bounding box properties
inline vec2f center(const bbox2f& a);
inline vec2f size(const bbox2f& a);
inline float width(const bbox2f& a);
inline float height(const bbox2f& a);
inline bool contains(const bbox2f& a, double x, double y);

// Bounding box comparisons
inline bool operator==(const bbox2f& a, const bbox2f& b);
inline bool operator!=(const bbox2f& a, const bbox2f& b);

// Bounding Box expansion
inline bbox2f merge(const bbox2f& a, const vec2f& b);
inline bbox2f merge(const bbox2f& a, const bbox2f& b);
inline void expand(bbox2f& a, const vec2f& b);
inline void expand(bbox2f& a, const bbox2f& b);

//
// Bounding box properties (vec3f)
inline vec3f center(const bbox3f& a);
inline vec3f size(const bbox3f& a);

// Bounding box comparisons (bbox3f)
inline bool operator==(const bbox3f& a, const bbox3f& b);
inline bool operator!=(const bbox3f& a, const bbox3f& b);

// Bounding Box expansion
inline bbox3f merge(const bbox3f& a, const vec3f& b);
inline bbox3f merge(const bbox3f& a, const bbox3f& b);
inline void expand(bbox3f& a, const vec3f& b);
inline void expand(bbox3f& a, const bbox3f& b);



}


//=========================================
// IMPLEMENTATION RECTANGLE
//=========================================
namespace waavs
{
	inline void moveTo(rectf& r, float x, float y) { r.x = x; r.y = y; }
	inline void moveTo(rectf& a, const vec2f& xy) { a.x = xy.x; a.y = xy.y; }
	inline void moveBy(rectf& r, float dx, float dy) { r.x += dx; r.y += dy; }
	inline void moveBy(rectf& a, vec2f dxy) { a.x += dxy.x; a.y += dxy.y; }

	inline vec2f center(const rectf& r) { return { r.x + r.w / 2,r.y + r.h / 2 }; }
	inline float width(const rectf& r) { return r.w; }
	inline float height(const rectf& r) { return r.h; }
	inline float left(const rectf& a) { return a.x; }
	inline float top(const rectf& a) { return a.y; }
	inline float right(const rectf& a) { return a.x + a.w; }
	inline float bottom(const rectf& a) { return a.y + a.h; }
	inline vec2f lefttop(const rectf& a) { return { a.x,a.y }; }
	inline vec2f rightbottom(const rectf& a) { return { a.x + a.w,a.y + a.h }; }

	inline vec2f size(const rectf& r) { return { r.w,r.h }; }
	inline bool contains(const rectf& a, const vec2f &pt) {
		return (pt.x >= a.x) && (pt.y >= a.y) &&
			(pt.x < a.x + a.w) && (pt.y < a.y + a.h);
	}

	inline rectf merge(const rectf& a, const vec2f& b) {
		return { min(a.x, b.x), min(a.y, b.y), max(a.x + a.w,b.x), max(a.y + a.h, b.y) };
	}

	inline rectf merge(const rectf& a, const rectf& b) {
		return { min(a.x, b.x), min(a.y, b.y), max(a.x + a.w, b.x + b.w), max(a.y + a.h,b.y + b.h) };
	}

	inline void expand(rectf& a, const vec2f& b) { a = merge(a, b); }
	inline void expand(rectf& a, const rectf& b) { a = merge(a, b); }

	inline rectf intersection(const rectf& ar, const rectf& br)
	{
		float x = ar.x > br.x ? ar.x : br.x;
		float y = ar.y > br.y ? ar.y : br.y;
		float r = (right(ar) < right(br) ? right(ar) : right(br));
		float b = (bottom(ar) < bottom(br) ? bottom(ar) : bottom(br));

		float w = (r - x) > 0 ? (r - x) : 0;
		float h = (b - y) > 0 ? (b - y) : 0;

		return rectf{ x,y,w,h };
	}


}

//=================================================
// IMPLEMENTATION	- AXIS aligned bounding boxes
//=================================================
namespace waavs
{
	inline vec2f& bbox2f::operator[](int i) { return (&min)[i]; }
	inline const vec2f& bbox2f::operator[](int i) const { return (&min)[i]; }

	inline vec3f& bbox3f::operator[](int i) { return (&min)[i]; }
	inline const vec3f& bbox3f::operator[](int i) const { return (&min)[i]; }

	inline vec2f center(const bbox2f& a) { return (a.min + a.max) / 2; }
	inline vec2f size(const bbox2f& a) { return a.max - a.min; }
	inline float width(const bbox2f &a) { return a.max.x - a.min.x; }
	inline float height(const bbox2f& a) { return a.max.y - a.min.y; }
	inline bool contains(const bbox2f& a, double x, double y) {
		return (x >= a.min.x) && (y >= a.min.y) &&
			(x <= a.max.x) && (y <= a.max.y);
	}
	
	// Comparison
	inline bool operator ==(const bbox2f& a, const bbox2f& b) { return a.min == b.min && a.max == b.max; }
	inline bool operator!=(const bbox2f& a, const bbox2f& b) {return a.min != b.min || a.max != b.max;}

	// Bounding box expansion
	inline bbox2f merge(const bbox2f& a, const vec2f& b) {
		return { min(a.min, b), max(a.max, b) };
	}
	inline bbox2f merge(const bbox2f& a, const bbox2f& b) {
		return { min(a.min, b.min), max(a.max, b.max) };
	}

	inline void expand(bbox2f& a, const vec2f& b) { a = merge(a, b); }
	inline void expand(bbox2f& a, const bbox2f& b) { a = merge(a, b); }


	// bbox2f
	inline vec3f center(const bbox3f& a) { return (a.min + a.max) / 2; }
	inline vec3f size(const bbox3f& a) { return a.max - a.min; }

	// Bounding box comparisons.
	inline bool operator==(const bbox3f& a, const bbox3f& b) {
		return a.min == b.min && a.max == b.max;
	}
	inline bool operator!=(const bbox3f& a, const bbox3f& b) {
		return a.min != b.min || a.max != b.max;
	}

	// Bounding box expansions with points and other boxes.
	inline bbox3f merge(const bbox3f& a, const vec3f& b) {
		return { min(a.min, b), max(a.max, b) };
	}
	inline bbox3f merge(const bbox3f& a, const bbox3f& b) {
		return { min(a.min, b.min), max(a.max, b.max) };
	}
	inline void expand(bbox3f& a, const vec3f& b) { a = merge(a, b); }
	inline void expand(bbox3f& a, const bbox3f& b) { a = merge(a, b); }
}


namespace waavs
{
	// Calculating normals
	// https://stackoverflow.com/questions/38499211/get-normal-of-bezier-curve-in-2d
	// 
	// Evaluate a cubic bezier curve along the specified
	// parameter 'u'
template <typename T>
inline T bezier_cubic_eval(const T& p0, const T& p1, const T& p2, const T& p3, float u)
{
	return p0 * (1.0f - u) * (1.0f - u) * (1.0f - u) + 
		p1 * 3 * u * (1.0f - u) * (1.0f - u) + 
		p2 * 3 * u * u * (1.0 - u) + 
		p3 * u * u * u;
}

template <typename T>
inline T interpolate_bezier_derivative(const T& p0, const T& p1, const T& p2, const T& p3, float u)
{
	return (p1 - p0) * 3 * (1.0f - u) * (1.0f - u) + 
		(p2 - p1) * 6 * u * (1.0f - u) +
		(p3 - p2) * 3 * u * u;
}

}