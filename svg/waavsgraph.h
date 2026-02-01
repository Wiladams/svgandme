#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <type_traits>

#include "definitions.h"

#include "coloring.h"

/*
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
}
*/


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
	ASSERT_STRUCT_SIZE(Point2d, 16);

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
	/// <summary>
	/// Return the type of arguments that are associated with a given segment command
	/// </summary>
	/// <param name="cmdIndex"></param>
	/// <returns>a null terminated string of the argument types, or nullptr on invalid command</returns>
	/// c - number
	/// f - flag
	/// r - radius
	///

	static const char* getSegmentArgTypes(unsigned char cmdIndex) noexcept {
		static std::array<const char*, 128> lookupTable = [] {
			std::array<const char*, 128> table{}; // Default initializes all to nullptr
			table['A'] = table['a'] = "ccrffcc";  // ArcTo
			table['C'] = table['c'] = "cccccc";   // CubicTo
			table['H'] = table['h'] = "c";        // HLineTo
			table['L'] = table['l'] = "cc";       // LineTo
			table['M'] = table['m'] = "cc";       // MoveTo
			table['Q'] = table['q'] = "cccc";     // QuadTo
			table['S'] = table['s'] = "cccc";     // SmoothCubicTo
			table['T'] = table['t'] = "cc";       // SmoothQuadTo
			table['V'] = table['v'] = "c";        // VLineTo
			table['Z'] = table['z'] = "";         // Close
			return table;
			}();

		return cmdIndex < 128 ? lookupTable[cmdIndex] : nullptr;
	}



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
	// This structure represents a single segment of an SVG path.
	// When parsing an SVG path, you get a series of segments,
	// Each segment has a command, and a set of arguments.
	//
	// Note:  By using the iteration field, we can do a rudimentary
	// run length encoding.  This would work well for relative path segments,
	// like a series of small line segments describing a circle.
	// 
	// BUGBUG - maybe need packing pragma to ensure tight packing
	static constexpr size_t kMaxPathArgs = 8;

	struct PathSegment final
	{
		
		float fArgs[kMaxPathArgs]{ 0.0 };             // 32 bytes
		char fArgTypes[kMaxPathArgs]{ 0 };             // 8 bytes
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

			// limit the number of args to copy
			size_t maxArgsToCopy = std::min(static_cast<size_t>(argcount), kMaxPathArgs);

			if (args != nullptr)
				std::copy_n(args, maxArgsToCopy, fArgs);
			if (argtypes != nullptr)
				std::copy_n(argtypes, maxArgsToCopy, fArgTypes);
		}

		const float* args() const { return fArgs; }
		void setArgs(const float* args, uint8_t argcount)
		{
			// limit the number of args to copy
			size_t maxArgsToCopy = std::min(static_cast<size_t>(argcount), kMaxPathArgs);

			if (args != nullptr)
			{
				std::copy_n(args, maxArgsToCopy, fArgs);
				fArgCount = maxArgsToCopy;
			}
			else {
				std::fill_n(fArgs, maxArgsToCopy, 0.0f);
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
