#pragma once

//
// svgpath.h 
// The SVGPath element contains a fairly complex set of commands in the 'd' attribute
// These commands are used to define the shape of the path, and can be fairly intricate.
// This file contains the code to parse the 'd' attribute and turn it into a BLPath object
//
// Usage:
//	BLPath path;
//	std::string pathString = "M 10 10 L 20 20";
//	waavs::ByteSpan span(pathString);
//	waavs::blpathparser::parsePath(span, path);
//

#include <map>
#include <functional>
#include <cstdint>

#include "blend2d.h"


#include "bspan.h"
#include "maths.h"

// uncomment the following to print diagnostics
//#define PATH_COMMAND_DEBUG 1

/*
// Old reference
// A dispatch std::map that matches the command character to the
		// appropriate parse function
		static std::map<SegmentCommand, std::function<bool(ByteSpan&, BLPath&, int&)>> parseMap = {
			{SegmentCommand::MoveTo, parseMoveTo},				// M
			{SegmentCommand::MoveBy, parseMoveBy},				// m
			{SegmentCommand::LineTo, parseLineTo},				// L
			{SegmentCommand::LineBy, parseLineBy},				// l
			{SegmentCommand::HLineTo, parseHLineTo},			// H
			{SegmentCommand::HLineBy, parseHLineBy},			// h
			{SegmentCommand::VLineTo, parseVLineTo},			// V
			{SegmentCommand::VLineBy, parseVLineBy},			// v
			{SegmentCommand::CubicTo, parseCubicTo},			// C
			{SegmentCommand::CubicBy, parseCubicBy},			// c
			{SegmentCommand::SCubicTo, parseSmoothCubicTo},		// S
			{SegmentCommand::SCubicBy, parseSmoothCubicBy},		// s
			{SegmentCommand::QuadTo, parseQuadTo},				// Q
			{SegmentCommand::QuadBy, parseQuadBy},				// q
			{SegmentCommand::SQuadTo, parseSmoothQuadTo},		// T
			{SegmentCommand::SQuadBy, parseSmoothQuadBy},		// t
			{SegmentCommand::ArcTo, parseArcTo},				// A
			{SegmentCommand::ArcBy, parseArcBy},				// a
			{SegmentCommand::CloseTo, parseClose},				// Z
			{SegmentCommand::CloseBy, parseClose}				// z
		};
*/


namespace waavs {

    // SVG path commands
    // M - move       (M, m)
    // L - line       (L, l, H, h, V, v)
    // C - cubic      (C, c, S, s)
    // Q - quad       (Q, q, T, t)
    // A - ellipticArc  (A, a)
    // Z - close        (Z, z)
    enum class SegmentCommand : uint8_t
    {
        INVALID = 0
        , MoveTo = 'M'
        , MoveBy = 'm'
        , LineTo = 'L'
        , LineBy = 'l'
        , HLineTo = 'H'
        , HLineBy = 'h'
        , VLineTo = 'V'
        , VLineBy = 'v'
        , CubicTo = 'C'
        , CubicBy = 'c'
        , SCubicTo = 'S'
        , SCubicBy = 's'
        , QuadTo = 'Q'
        , QuadBy = 'q'
        , SQuadTo = 'T'
        , SQuadBy = 't'
        , ArcTo = 'A'
        , ArcBy = 'a'
        , CloseTo = 'Z'
        , CloseBy = 'z'
    };
}





namespace waavs
{
	//
	// return true on success, false otherwise 
	// Aside from parsing SVG element structure, this is one 
	// of the most complex functions in the library
	//
	// This parser is fairly complete.  It can handle situations where
	// there are multiple coordinates for a command, in a chain
	// such as "M 10 10 20 20 30 30"
	//
	namespace blpathparser {
		static charset pathCmdChars("mMlLhHvVcCqQsStTaAzZ");   // set of characters used for commands
		static charset numberChars("0123456789.+-eE");         // digits, symbols, and letters found in numbers
		static charset leadingChars("0123456789.+-");          // digits, symbols, and letters found at start of numbers


		
		static bool parseMoveTo(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double x{ 0 };
			double y{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, x))
				return false;
			if (!parseNextNumber(s, y))
				return false;

			if (iteration == 0) {
				res = apath.moveTo(x, y);
#ifdef PATH_COMMAND_DEBUG
				printf("apath.moveTo(%f, %f);\n", x, y);
#endif
			}
			else {
				res = apath.lineTo(x, y);
#ifdef PATH_COMMAND_DEBUG
				printf("// moveTo, iteration: %d\n", iteration);
				printf("apath.lineTo(%f, %f);\n", x, y);
#endif
			}
			
			iteration++;

			return true;
		}

		static bool parseMoveBy(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double x{ 0 };
			double y{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, x))
				return false;
			if (!parseNextNumber(s, y))
				return false;

			BLPoint lastPos{};
			apath.getLastVertex(&lastPos);

			if (iteration == 0) {
				res = apath.moveTo(lastPos.x + x, lastPos.y + y);

#ifdef PATH_COMMAND_DEBUG
				printf("// moveBy(%f, %f), iteration: %d\n", x, y, iteration);
				printf("apath.moveTo(%f, %f);\n", lastPos.x + x, lastPos.y + y);
#endif
			}
			else {
				res = apath.lineTo(lastPos.x + x, lastPos.y + y);
#ifdef PATH_COMMAND_DEBUG
				printf("// moveBy(%f, %f), iteration: %d\n", x, y, iteration);
				printf("apath.lineTo(%f, %f);\n", lastPos.x + x, lastPos.y + y);
#endif
			}
			iteration++;

			return true;
		}

		// Command 'L' - LineTo
		static bool parseLineTo(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double x{ 0 };
			double y{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, x))
				return false;
			if (!parseNextNumber(s, y))
				return false;

			res = apath.lineTo(x, y);

#ifdef PATH_COMMAND_DEBUG
			printf("apath.lineTo(%f, %f);\n", x, y);
#endif
			
			iteration++;

			return true;
		}

		static bool parseLineBy(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double x{ 0 };
			double y{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, x))
				return false;
			if (!parseNextNumber(s, y))
				return false;

			BLPoint lastPos{};
			apath.getLastVertex(&lastPos);

			res = apath.lineTo(lastPos.x + x, lastPos.y + y);

#ifdef PATH_COMMAND_DEBUG
			printf("// lineBy: (%f, %f), %d\n", x, y, iteration);
			printf("apath.lineTo( %f, %f);\n", lastPos.x + x, lastPos.y + y);
#endif
			
			iteration++;

			return true;
		}

		// Command - H
		static bool parseHLineTo(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double x{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, x))
				return false;

			BLPoint lastPos{};
			apath.getLastVertex(&lastPos);
			res = apath.lineTo(x, lastPos.y);

#ifdef PATH_COMMAND_DEBUG
			printf("apath.lineTo(%f, %f);\n", x, lastPos.y);
#endif
			
			iteration++;

			return true;
		}

		// Command - h
		static bool parseHLineBy(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double x{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, x))
				return false;

			BLPoint lastPos{};
			apath.getLastVertex(&lastPos);
			res = apath.lineTo(lastPos.x + x, lastPos.y);

#ifdef PATH_COMMAND_DEBUG
			//printf("// hLineBy\n");
			printf("apath.lineTo(%f, %f);\n", lastPos.x + x, lastPos.y);
#endif
			
			iteration++;

			return true;
		}

		// Command - V
		static bool parseVLineTo(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double y{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, y))
				return false;

			BLPoint lastPos{};
			apath.getLastVertex(&lastPos);
			res = apath.lineTo(lastPos.x, y);

#ifdef PATH_COMMAND_DEBUG
			//printf("// VLineTo, iteration: %d\n", iteration);
			printf("apath.lineTo(%f, %f);\n", lastPos.x, y);
#endif
			
			iteration++;

			return true;
		}

		// Command - v
		static bool parseVLineBy(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double y{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, y))
				return false;

			BLPoint lastPos{};
			apath.getLastVertex(&lastPos);
			res = apath.lineTo(lastPos.x, lastPos.y + y);

#ifdef PATH_COMMAND_DEBUG
			//printf("// vLineBy, iteration: %d\n", iteration);
			printf("apath.lineTo(%f,%f);\n", lastPos.x, lastPos.y + y);
#endif
			iteration++;

			return true;
		}

		// Command - Q
		static bool parseQuadTo(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double x1{ 0 };
			double y1{ 0 };
			double x2{ 0 };
			double y2{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, x1))
				return false;
			if (!parseNextNumber(s, y1))
				return false;
			if (!parseNextNumber(s, x2))
				return false;
			if (!parseNextNumber(s, y2))
				return false;

			res = apath.quadTo(x1, y1, x2, y2);

#ifdef PATH_COMMAND_DEBUG
			//printf("// quadTo, iteration: %d\n", iteration);
			printf("apath.quadTo(%f,%f, %f, %f);\n", x1, y1, x2, y2);
#endif

			
			iteration++;

			return true;
		}

		// Command - q
		static bool parseQuadBy(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double x1{ 0 };
			double y1{ 0 };
			double x2{ 0 };
			double y2{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, x1))
				return false;
			if (!parseNextNumber(s, y1))
				return false;
			if (!parseNextNumber(s, x2))
				return false;
			if (!parseNextNumber(s, y2))
				return false;

			BLPoint lastPos{};
			apath.getLastVertex(&lastPos);

			res = apath.quadTo(lastPos.x + x1, lastPos.y + y1, lastPos.x + x2, lastPos.y + y2);

#ifdef PATH_COMMAND_DEBUG
			//printf("// quadTo, iteration: %d\n", iteration);
			printf("apath.quadTo(%f,%f, %f, %f);\n", lastPos.x + x1, lastPos.y + y1, lastPos.x + x2, lastPos.y + y2);
#endif
			
			iteration++;

			return true;
		}

		// Command - T
		static bool parseSmoothQuadTo(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double x2{ 0 };
			double y2{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, x2))
				return false;
			if (!parseNextNumber(s, y2))
				return false;

			res = apath.smoothQuadTo(x2, y2);

#ifdef PATH_COMMAND_DEBUG
			//printf("// quadTo, iteration: %d\n", iteration);
			printf("apath.smoothQuadTo(%f,%f);\n", x2, y2);
#endif
			
			iteration++;

			return true;
		}

		// Command - t
		static bool parseSmoothQuadBy(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double x2{ 0 };
			double y2{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, x2))
				return false;
			if (!parseNextNumber(s, y2))
				return false;

			BLPoint lastPos{};
			apath.getLastVertex(&lastPos);

			res = apath.smoothQuadTo(lastPos.x + x2, lastPos.y + y2);

#ifdef PATH_COMMAND_DEBUG
			//printf("// quadTo, iteration: %d\n", iteration);
			printf("apath.smoothQuadTo(%f,%f);\n", lastPos.x + x2, lastPos.y + y2);
#endif
			
			iteration++;

			return true;
		}

		// Command - C
		static bool parseCubicTo(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double x1{ 0 };
			double y1{ 0 };
			double x2{ 0 };
			double y2{ 0 };
			double x3{ 0 };
			double y3{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, x1))
				return false;
			if (!parseNextNumber(s, y1))
				return false;
			if (!parseNextNumber(s, x2))
				return false;
			if (!parseNextNumber(s, y2))
				return false;
			if (!parseNextNumber(s, x3))
				return false;
			if (!parseNextNumber(s, y3))
				return false;

			res = apath.cubicTo(x1, y1, x2, y2, x3, y3);

#ifdef PATH_COMMAND_DEBUG
			printf("apath.cubicTo(%f,%f,%f,%f,%f,%f);\n", x1, y1, x2, y2, x3, y3);
#endif
			
			iteration++;

			return true;
		}

		// Command - c
		static bool parseCubicBy(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double x1{ 0 };
			double y1{ 0 };
			double x2{ 0 };
			double y2{ 0 };
			double x3{ 0 };
			double y3{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, x1))
				return false;
			if (!parseNextNumber(s, y1))
				return false;
			if (!parseNextNumber(s, x2))
				return false;
			if (!parseNextNumber(s, y2))
				return false;
			if (!parseNextNumber(s, x3))
				return false;
			if (!parseNextNumber(s, y3))
				return false;

			BLPoint lastPos{};
			apath.getLastVertex(&lastPos);

			res = apath.cubicTo(lastPos.x + x1, lastPos.y + y1, lastPos.x + x2, lastPos.y + y2, lastPos.x + x3, lastPos.y + y3);

#ifdef PATH_COMMAND_DEBUG
			//printf("apath.cubicBy(%f,%f,%f,%f,%f,%f);\n", x1, y1, x2, y2, x3, y3);

			printf("apath.cubicTo(%f,%f,%f,%f,%f,%f);\n", lastPos.x + x1, lastPos.y + y1, lastPos.x + x2, lastPos.y + y2, lastPos.x + x3, lastPos.y + y3);
#endif
			
			iteration++;

			return true;
		}

		// Command - S
		static bool parseSmoothCubicTo(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double x2{ 0 };
			double y2{ 0 };
			double x3{ 0 };
			double y3{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, x2))
				return false;
			if (!parseNextNumber(s, y2))
				return false;
			if (!parseNextNumber(s, x3))
				return false;
			if (!parseNextNumber(s, y3))
				return false;

			res = apath.smoothCubicTo(x2, y2, x3, y3);

#ifdef PATH_COMMAND_DEBUG
			printf("apath.smoothCubicTo(%f,%f,%f,%f);\n", x2, y2, x3, y3);
#endif
			
			iteration++;

			return true;
		}

		// Command - s
		static bool parseSmoothCubicBy(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double x2{ 0 };
			double y2{ 0 };
			double x3{ 0 };
			double y3{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, x2))
				return false;
			if (!parseNextNumber(s, y2))
				return false;
			if (!parseNextNumber(s, x3))
				return false;
			if (!parseNextNumber(s, y3))
				return false;

			BLPoint lastPos{};
			apath.getLastVertex(&lastPos);

			res = apath.smoothCubicTo(lastPos.x + x2, lastPos.y + y2, lastPos.x + x3, lastPos.y + y3);

#ifdef PATH_COMMAND_DEBUG
			printf("apath.smoothCubicTo(%f,%f,%f,%f);\n", lastPos.x + x2, lastPos.y + y2, lastPos.x + x3, lastPos.y + y3);
#endif
			iteration++;

			return true;
		}


		// Command - A
		static bool parseArcTo(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double rx{ 0 };
			double ry{ 0 };
			double xAxisRotation{ 0 };
			double largeArcFlag{ 0 };
			double sweepFlag{ 0 };
			double x{ 0 };
			double y{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, rx))
				return false;
			if (!parseNextNumber(s, ry))
				return false;
			if (!parseNextNumber(s, xAxisRotation))
				return false;
			if (!parseNextNumber(s, largeArcFlag))
				return false;
			if (!parseNextNumber(s, sweepFlag))
				return false;
			if (!parseNextNumber(s, x))
				return false;
			if (!parseNextNumber(s, y))
				return false;

			bool larc = largeArcFlag > 0.5f;
			bool swp = sweepFlag > 0.5f;
			double xrot = radians(xAxisRotation);

			res = apath.ellipticArcTo(BLPoint(rx, ry), xrot, larc, swp, BLPoint(x, y));

#ifdef PATH_COMMAND_DEBUG
			printf("apath.ellipticArcTo(BLPoint(%f,%f) ,%f,%d, %d, BLPoint(%f, %f);\n",rx, ry, xrot, larc, swp, x, y);
#endif
			
			iteration++;

			return true;
		}

		// Command - a
		static bool parseArcBy(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double rx{ 0 };
			double ry{ 0 };
			double xAxisRotation{ 0 };
			double largeArcFlag{ 0 };
			double sweepFlag{ 0 };
			double x{ 0 };
			double y{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!parseNextNumber(s, rx))
				return false;
			if (!parseNextNumber(s, ry))
				return false;
			if (!parseNextNumber(s, xAxisRotation))
				return false;
			if (!parseNextNumber(s, largeArcFlag))
				return false;
			if (!parseNextNumber(s, sweepFlag))
				return false;
			if (!parseNextNumber(s, x))
				return false;
			if (!parseNextNumber(s, y))
				return false;

			bool larc = largeArcFlag > 0.5f;
			bool swp = sweepFlag > 0.5f;
			double xrot = radians(xAxisRotation);

			BLPoint lastPos{};
			apath.getLastVertex(&lastPos);

			res = apath.ellipticArcTo(BLPoint(rx, ry), xrot, larc, swp, BLPoint(lastPos.x + x, lastPos.y + y));

#ifdef PATH_COMMAND_DEBUG
			printf("apath.ellipticArcTo(BLPoint(%f,%f) ,%f,%d, %d, BLPoint(%f, %f);\n", rx, ry, xrot, larc, swp, lastPos.x + x, lastPos.y + y);
#endif
			
			iteration++;

			return true;
		}

		// Command - Z, z
		static bool parseClose(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			BLResult res = BL_SUCCESS;
			
			if (iteration > 0)
			{
				// consume the character and return
				// This deals with the odd case where there is a number
				// typically '0' after a 'z' close

#ifdef PATH_COMMAND_DEBUG
				printf("PARSECLOSE(), unknown character after first command (%c), ignoring\n", *s);
#endif
				s++;
				iteration++;


				return false;
			}
			
			res = apath.close();
			
#ifdef PATH_COMMAND_DEBUG
			printf("// z\n");
			printf("apath.close();\n");
#endif
			// No parameters expected to follow
			// so we increment the iteration and 
			// if we enter here with it other than '0'
			// we know we have an error
			iteration++;
			
			return true;
		}




		static bool parsePath(const waavs::ByteSpan& inSpan, BLPath& apath) noexcept
		{
			// Use a ByteSpan as a cursor on the input
			ByteSpan s = inSpan;
			SegmentCommand currentCommand = SegmentCommand::INVALID;
			int iteration = 0;
			std::function<bool(ByteSpan&, BLPath&, int&)> pFunc{ nullptr };
			bool success = false;
			
			while (s)
			{

				
				// always ignore leading whitespace
				s = chunk_ltrim(s, chrWspChars);

				// If we've gotten to the end, we're done
				// so just return
				if (!s)
					break;

				if (pathCmdChars[*s])
				{
					// we have a command
					currentCommand = SegmentCommand(*s);

					// reset iteration counter
					iteration = 0;

					// move past the command character
					s++;
				}
				else {
					// The tricky part here is, the character we're sitting
					// on is not a valid command character, so we're either
					// sitting at the beginning of a number, or we're sitting
					// at an invalid command.
					// We'll just assume we're sitting at a number, and 
					// process it as if it's a continuation of the last command
			        // to be more robust, we can check whether it's the beginning
					// of a number or not.  If not, then return false
					//if (!leadingChars[*s])
					// {
					//	 return false;
					// }
				}
				
#ifdef PATH_COMMAND_DEBUG
				//printf("// Command: %c [%d]\n", currentCommand, iteration);
#endif
				
				switch (currentCommand)
				{
					case SegmentCommand::MoveTo: pFunc = parseMoveTo; break;
					case SegmentCommand::MoveBy: pFunc = parseMoveBy; break;
					case SegmentCommand::LineTo: pFunc = parseLineTo; break;
					case SegmentCommand::LineBy: pFunc = parseLineBy; break;
					case SegmentCommand::HLineTo: pFunc = parseHLineTo; break;
					case SegmentCommand::HLineBy: pFunc = parseHLineBy; break;
					case SegmentCommand::VLineTo: pFunc = parseVLineTo; break;
					case SegmentCommand::VLineBy: pFunc = parseVLineBy; break;
					case SegmentCommand::CubicTo: pFunc = parseCubicTo; break;
					case SegmentCommand::CubicBy: pFunc = parseCubicBy; break;
					case SegmentCommand::SCubicTo: pFunc = parseSmoothCubicTo; break;
					case SegmentCommand::SCubicBy: pFunc = parseSmoothCubicBy; break;
					case SegmentCommand::QuadTo: pFunc = parseQuadTo; break;
					case SegmentCommand::QuadBy: pFunc = parseQuadBy; break;
					case SegmentCommand::SQuadTo: pFunc = parseSmoothQuadTo; break;
					case SegmentCommand::SQuadBy: pFunc = parseSmoothQuadBy; break;
					case SegmentCommand::ArcTo: pFunc = parseArcTo; break;
					case SegmentCommand::ArcBy: pFunc = parseArcBy; break;
					case SegmentCommand::CloseTo: pFunc = parseClose; break;
					case SegmentCommand::CloseBy: pFunc = parseClose; break;
						
					default:
						printf("parsePath: INVALID COMMAND: %c\n", *s);
						return false;
				}
				
				if (pFunc != nullptr)
					success = pFunc(s, apath, iteration);
				
				if (!success)
				{
					printf("parsePath: failed to parse command: %c\n", *s);
					return false;
				}
			}

#ifdef PATH_COMMAND_DEBUG
			printf("// parsePath(), FINISHED\n");
#endif
			
			return success;
		}
	}
}