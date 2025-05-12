#pragma once

#include <cstdint>
#include <cmath>

// Some data structures used for graphics
namespace waavs {
	// A simple structure to hold a color
	constexpr double absdiff(double a, double b) 
	{
		return (a > b ? a - b : b - a);
	}

	constexpr bool nearlyEqual(double a, double b, double epsilon = 1e-6) 
	{
		return absdiff(a,b) <= epsilon;
	}

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
	// A simple structure to hold a point
	// We're not making a mathematical distinction between a point and a vector
	// This structure will take on either meaning, depending on the context
	struct Point {
		double x = 0, y = 0;

		Point operator-(const Point& rhs) const { return { x - rhs.x, y - rhs.y }; }
		Point operator+(const Point& rhs) const { return { x + rhs.x, y + rhs.y }; }
		Point operator*(double s) const { return { x * s, y * s }; }

		Point midpoint(const Point& b) const { return (*this + b) * 0.5; }

		void normalize() {
			double len = std::sqrt(x * x + y * y);
			if (len > 1e-8) {
				x /= len;
				y /= len;
			}
		}

	};

	// A simple structure to hold a rectangle
//struct Rect
//{
//	double x, y, width, height;
//	Rect() : x(0), y(0), width(0), height(0) {}
//	Rect(double x, double y, double width, double height) : x(x), y(y), width(width), height(height) {}
//};

	static double distanceToLine(const Point& pt, const Point& a, const Point& b) {
		double dx = b.x - a.x;
		double dy = b.y - a.y;
		double num = std::abs(dy * pt.x - dx * pt.y + b.x * a.y - b.y * a.x);
		double den = std::sqrt(dx * dx + dy * dy);
		return den > 0 ? num / den : 0.0;
	}
}

namespace waavs
{
	// A path is composed of several segments
	// The PathSegment structure is a compact representation
	// of that segment.  You can reconstruct the segment in text
	// form, or pass this along to other functions for processing
	struct PathSegment 
	{
		double args[10]{ 0 };				// The arguments for the command
		const char* fArgTypes{};			// The types of the arguments	(BUGBUG - should be array of chars)
		int fArgCount{ 0 };					// how many args used
		unsigned char fSegmentKind{ 0 };	// Single segment command from SVG
	};
}
