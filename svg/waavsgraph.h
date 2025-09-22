#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <type_traits>

#include "definitions.h"

// Some data structures used for graphics
namespace waavs {

	// absolute value of the difference between two doubles
	constexpr double absdiff(double a, double b) 
	{
		return (a > b ? a - b : b - a);
	}

	// compare two double values.  They are equal within a tolerance
	constexpr bool nearlyEqual(double a, double b, double epsilon = 1e-6) 
	{
		return absdiff(a,b) <= epsilon;
	}


	// struct Color
	// A simple structure to hold a color
	// constexpr is used everywhere so the Color structure can 
	// be used as a literal value.
	// The arithmetic operators are implemented so interpolation can be done
	//
	struct Color {
		double r, g, b, a;

		constexpr Color() : r(0), g(0), b(0), a(0) {}
		constexpr Color(double r, double g, double b, double a)
			: r(r), g(g), b(b), a(a) {
		}

		// Perceptual luminance (W3C)
		constexpr double luminance() const {
			return 0.2126 * r + 0.7152 * g + 0.0722 * b;
		}

		// Euclidean RGB distance
		constexpr double colorDistance(const Color& other) const {
			double dr = r - other.r;
			double dg = g - other.g;
			double db = b - other.b;
			return dr * dr + dg * dg + db * db;
		}

		// Perceptual match: considers luminance and RGB delta
		constexpr bool perceptualMatch(const Color& other,
			double maxLumaDiff = 0.01,
			double maxColorDist = 0.001,
			double maxAlphaDiff = 0.01) const
		{
			return absdiff(luminance(), other.luminance()) <= maxLumaDiff &&
				colorDistance(other) <= maxColorDist &&
				absdiff(a, other.a) <= maxAlphaDiff;
		}

		constexpr Color operator+(const Color& other) const {
			return Color(r + other.r, g + other.g, b + other.b, a + other.a);
		}

		constexpr Color operator-(const Color& other) const {
			return Color(r - other.r, g - other.g, b - other.b, a - other.a);
		}

		constexpr Color operator*(double scalar) const {
			return Color(r * scalar, g * scalar, b * scalar, a * scalar);
		}

		// Approximate equality
		constexpr bool equals(const Color& other, double epsilon = 1e-6) const {
			return nearlyEqual(r, other.r, epsilon) &&
				nearlyEqual(g, other.g, epsilon) &&
				nearlyEqual(b, other.b, epsilon) &&
				nearlyEqual(a, other.a, epsilon);
		}
	};
	

}

namespace waavs
{
	// struct Point
	// 
	// A simple structure to hold a point
	// We're not making a mathematical distinction between a point and a vector
	// This structure will take on either meaning, depending on the context
	//
	struct Point2d final 
	{
		double x{ 0.0 };
		double y{ 0.0 };

		Point2d operator-(const Point2d& rhs) const { return { x - rhs.x, y - rhs.y }; }
		Point2d operator+(const Point2d& rhs) const { return { x + rhs.x, y + rhs.y }; }
		Point2d operator*(double s) const { return { x * s, y * s }; }

		Point2d midpoint(const Point2d& b) const { return (*this + b) * 0.5; }

		void normalize() {
			double len = std::sqrt(x * x + y * y);
			if (len > 1e-8) {
				x /= len;
				y /= len;
			}
		}

	};

	ASSERT_POD_TYPE(Point2d);

	// distanceToLine()
	// 
	// Calculate the distance from a point to a line segment
	//
	static double distanceToLine(const Point2d& pt, const Point2d& a, const Point2d& b) 
	{
		double dx = b.x - a.x;
		double dy = b.y - a.y;
		double num = std::abs(dy * pt.x - dx * pt.y + b.x * a.y - b.y * a.x);
		double den = std::sqrt(dx * dx + dy * dy);
		return den > 0 ? num / den : 0.0;
	}
}

namespace waavs
{
	// SVGPathCommand
	// Represents the individual commands in an SVG path
	enum class SVGPathCommand : uint8_t
	{
		// Move to
		M = 'M',  // absolute moveto
		m = 'm',  // relative moveto

		// Line to
		L = 'L',  // absolute lineto
		l = 'l',  // relative lineto
		H = 'H',  // absolute horizontal lineto
		h = 'h',  // relative horizontal lineto
		V = 'V',  // absolute vertical lineto
		v = 'v',  // relative vertical lineto

		// Cubic Bézier
		C = 'C',  // absolute cubic Bézier
		c = 'c',  // relative cubic Bézier
		S = 'S',  // absolute smooth cubic Bézier
		s = 's',  // relative smooth cubic Bézier

		// Quadratic Bézier
		Q = 'Q',  // absolute quadratic Bézier
		q = 'q',  // relative quadratic Bézier
		T = 'T',  // absolute smooth quadratic Bézier
		t = 't',  // relative smooth quadratic Bézier

		// Elliptical arc
		A = 'A',  // absolute arc
		a = 'a',  // relative arc

		// Close path
		Z = 'Z',  // absolute closepath
		z = 'z'   // relative closepath (treated the same as Z in most renderers)
	};


	// struct PathSegment
	// 
	// A path is composed of several segments
	// The PathSegment structure is a compact representation
	// of that segment.  You can reconstruct the segment in text
	// form, or pass this along to other functions for processing
	//
	// BUGBUG - maybe need packing pragma to ensure tight packing
	struct PathSegment final
	{
		float fArgs[8]{ 0.0 };             // 32 bytes
		char fArgTypes[8]{ 0 };             // 8 bytes
		uint8_t fArgCount{ 0 };             // 1 byte
		SVGPathCommand fSegmentKind;      // 1 byte
		uint16_t fIteration{ 0 };            // 2 byte
		uint32_t _reserved{ 0 };            // 4 bytes (rounds to 48 total)


		// Efficiently reset the contents of the struct

		void reset(const float* args, uint8_t argcount, const char* argtypes, SVGPathCommand kind, uint8_t iteration)
		{
			// we can only use memset as long as there's no virtual
			// function implemented.
			std::memset(this, 0, sizeof(PathSegment));
			fArgCount = argcount;
			fSegmentKind = kind;
			fIteration = iteration;

			if (args != nullptr)
				std::copy_n(args, argcount, fArgs);
			if (argtypes != nullptr)
				std::copy_n(argtypes, argcount, fArgTypes);
		}

		const float* args() const { return fArgs; }
		void setArgs(const float* args, uint8_t argcount)
		{
			if (args != nullptr)
			{
				std::copy_n(args, argcount, fArgs);
				fArgCount = argcount;
			}
			else {
				std::fill_n(fArgs, 8, 0.0f);
				fArgCount = 0;
			}
		}

		constexpr uint16_t iteration() const { return fIteration; }
		constexpr SVGPathCommand command() const { return fSegmentKind; }

		bool isRelative() const {
			return (static_cast<unsigned char>(fSegmentKind) >= 'a' && static_cast<unsigned char>(fSegmentKind) <= 'z');
		}

		// Does the command use absolute coordinates?
		bool isAbsolute() const {
			return (static_cast<unsigned char>(fSegmentKind) >= 'A' && static_cast<unsigned char>(fSegmentKind) <= 'Z');
		}
	};

	ASSERT_MEMCPY_SAFE(PathSegment);
	ASSERT_STRUCT_SIZE(PathSegment, 48);
}
