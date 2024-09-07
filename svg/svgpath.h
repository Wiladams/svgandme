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
// Noted:
// A convenient visualtion tool can be found here: https://svg-path-visualizer.netlify.app/
// References:
// https://svgwg.org/svg2-draft/paths.html#PathDataBNF



#include <functional>
#include <cstdint>

#include "blend2d.h"
#include "bspan.h"



// uncomment the following to print diagnostics
//#define PATH_COMMAND_DEBUG 1



namespace waavs {

    enum SegmentCommand : uint8_t
    {
        INVALID = 255
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

	/*
	// svgPathCommandHash
	// Turn a single character that represents one of the SVG path commands
	// into a hash value between 0 and 51 inclusive.
	// A value of 255 indicates "INVALID", meaning, the command is 
	// not one of the SVG Path commands.
	static constexpr INLINE int svgPathCommandHash(const uint8_t cmd) {
		if (cmd >= 'A' && cmd <= 'Z') {
			return cmd - 'A';
		}
		else if (cmd >= 'a' && cmd <= 'z') {
			return 26 + (cmd - 'a');
		}

		return 255; // Invalid command
	}
	*/
	
}



namespace waavs
{
	// Command parse function pointer
	using SVGPathCommandParseFunction = std::function<bool(ByteSpan&, BLPath&, int&)>;
	
	
	//
	// return true on success, false otherwise 
	// Aside from parsing SVG element structure, this is one 
	// of the most complex functions in the library
	//
	// This parser is fairly complete.  It can handle situations where
	// there are multiple coordinates for a command, in a chain
	// such as "M 10 10 20 20 30 30"
	//
	// Encoding command parameters
	// c - coordinate, floating point number
	// r - rotation, floating point number, in degrees
	// f - flag, a single character
	//
	
	namespace blpathparser {
		static charset pathCmdChars("mMlLhHvVcCqQsStTaAzZ");   // set of characters used for commands
		static charset numberChars("0123456789.+-eE");         // digits, symbols, and letters found in numbers
		static charset leadingChars("0123456789.+-");          // digits, symbols, and letters found at start of numbers


		static bool parseMoveTo(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			BLResult res = BL_SUCCESS;
			
			double args[2];
			if (!readNumericArguments(s, "cc", args))
				return false;
				
			if (iteration == 0) {
				res = apath.moveTo(args[0], args[1]);
			}
			else {
				res = apath.lineTo(args[0], args[1]);
			}

			iteration++;

			return res == BL_SUCCESS;
		}

		static bool parseMoveBy(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			BLResult res = BL_SUCCESS;
			
			
			double args[2];
			if (!readNumericArguments(s, "cc", args))
				return false;

			BLPoint lastPos{};
			apath.getLastVertex(&lastPos);
			
			if (iteration == 0) {
				res = apath.moveTo(lastPos.x + args[0], lastPos.y + args[1]);
			}
			else {
				res = apath.lineTo(lastPos.x + args[0], lastPos.y + args[1]);
			}

			iteration++;

			return res == BL_SUCCESS;
		}

		// Command 'L' - LineTo
		static bool parseLineTo(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			BLResult res = BL_SUCCESS;
			
			double args[2];
			if (!readNumericArguments(s, "cc", args))
				return false;

			res = apath.lineTo(args[0], args[1]);
			
			iteration++;

			return res == BL_SUCCESS;
		}

		static bool parseLineBy(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			BLResult res = BL_SUCCESS;
			
			double args[2];
			if (!readNumericArguments(s, "cc", args))
				return false;
			
			BLPoint lastPos{};
			apath.getLastVertex(&lastPos);

			res = apath.lineTo(lastPos.x + args[0], lastPos.y + args[1]);

			iteration++;

			return res == BL_SUCCESS;
		}

		// Command - H
		static bool parseHLineTo(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			double x{ 0 };
			BLResult res = BL_SUCCESS;
			
			if (!readNextNumber(s, x))
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
			
			if (!readNextNumber(s, x))
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
			
			if (!readNextNumber(s, y))
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
			
			if (!readNextNumber(s, y))
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
			
			if (!readNextNumber(s, x1))
				return false;
			if (!readNextNumber(s, y1))
				return false;
			if (!readNextNumber(s, x2))
				return false;
			if (!readNextNumber(s, y2))
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
			
			if (!readNextNumber(s, x1))
				return false;
			if (!readNextNumber(s, y1))
				return false;
			if (!readNextNumber(s, x2))
				return false;
			if (!readNextNumber(s, y2))
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
			
			if (!readNextNumber(s, x2))
				return false;
			if (!readNextNumber(s, y2))
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
			
			if (!readNextNumber(s, x2))
				return false;
			if (!readNextNumber(s, y2))
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
			BLResult res = BL_SUCCESS;

			// 'cccccc'
			double args[6];
			if (!readNumericArguments(s, "cccccc", args))
				return false;

			res = apath.cubicTo(args[0], args[1], args[2], args[3], args[4], args[5]);

			iteration++;

			return res == BL_SUCCESS;
		}

		// Command - c
		static bool parseCubicBy(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			BLResult res = BL_SUCCESS;

			BLPoint lastPos{};
			apath.getLastVertex(&lastPos);

			// 'cccccc'
			double args[6];
			if (!readNumericArguments(s, "cccccc", args))
				return false;
			
			res = apath.cubicTo(lastPos.x + args[0], lastPos.y + args[1], lastPos.x + args[2], lastPos.y + args[3], lastPos.x + args[4], lastPos.y + args[5]);
			
			iteration++;

			return res == BL_SUCCESS;
		}

		// Command - S
		static bool parseSmoothCubicTo(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			BLResult res = BL_SUCCESS;
			
			double x2{ 0 };
			double y2{ 0 };
			double x3{ 0 };
			double y3{ 0 };

			
			if (!readNextNumber(s, x2))
				return false;
			if (!readNextNumber(s, y2))
				return false;
			if (!readNextNumber(s, x3))
				return false;
			if (!readNextNumber(s, y3))
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
			BLResult res = BL_SUCCESS;
			
			double x2{ 0 };
			double y2{ 0 };
			double x3{ 0 };
			double y3{ 0 };

			
			if (!readNextNumber(s, x2))
				return false;
			if (!readNextNumber(s, y2))
				return false;
			if (!readNextNumber(s, x3))
				return false;
			if (!readNextNumber(s, y3))
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
			BLResult res = BL_SUCCESS;
			
			double args[7]{ 0 };
			


			if (!readNumericArguments(s, "ccrffcc", args))
				return false;
			
			bool larc = args[3] > 0.5f;
			bool swp = args[4] > 0.5f;
			double xrot = radians(args[2]);

			
			res = apath.ellipticArcTo(BLPoint(args[0], args[1]), xrot, larc, swp, BLPoint(args[5], args[6]));

			iteration++;

			return res == BL_SUCCESS;
		}

		// Command - a
		static bool parseArcBy(ByteSpan& s, BLPath& apath, int& iteration) noexcept
		{
			BLResult res = BL_SUCCESS;
			
			BLPoint lastPos{};
			apath.getLastVertex(&lastPos);
			double args[7]{ 0 };



			if (!readNumericArguments(s, "ccrffcc", args))
				return false;

			bool larc = args[3] > 0.5;
			bool swp = args[4] > 0.5;
			double xrot = radians(args[2]);

			
			res = apath.ellipticArcTo(BLPoint(args[0], args[1]), xrot, larc, swp, BLPoint(lastPos.x + args[5], lastPos.y + args[6]));

			iteration++;

			return res == BL_SUCCESS;
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
			
			return res == BL_SUCCESS;
		}



		
		// Static array to store function pointers
		// This should be a very fast way to connect a single character command
		// to the function pointer to parse it.  
		// In addition to being relatively fast, it also does not required the use
		// of any std library collections.
		// An alternative is to use a simple switch statement to quickly
		// do the same.
		// I like this lookup method, although it requires some extra memory
		// and a hash function (svgPathCommandHash) to generate the index.
		//

		
		static SVGPathCommandParseFunction PathCommandParseFunctions[] = {
			parseArcTo,				// A	'rrffcc'
			/* B */ nullptr,
			parseCubicTo,			// C	'cccccc'
			/* D */ nullptr,
			/* E */ nullptr,
			/* F */ nullptr,
			/* G */ nullptr,
			parseHLineTo,			// H	'c'
			/* I */ nullptr,
			/* J */ nullptr,
			/* K */ nullptr,
			parseLineTo,			// L	'cc'
			parseMoveTo,			// M	'cc'
			/* N */ nullptr,
			/* O */ nullptr,
			/* P */ nullptr,
			parseQuadTo,			// Q	'cccc'
			nullptr,				// R 
			parseSmoothCubicTo,		// S	'cccc'
			parseSmoothQuadTo,		// T	'cc'
			nullptr,				// U
			parseVLineTo,			// V	'c'
			/* W */ nullptr,
			/* X */ nullptr,
			/* Y */ nullptr,
			parseClose,				// Z	''
			parseArcBy,				// a	'rrffcc'
			nullptr,				// b
			parseCubicBy,			// c	'cccccc'
			/* d */ nullptr,
			/* e */ nullptr,
			/* f */ nullptr,
			/* g */ nullptr,
			parseHLineBy,			// h	'c'
			nullptr,				// i
			nullptr,				// j
			nullptr,				// k
			parseLineBy,			// l	'cc'
			parseMoveBy,			// m	'cc'
			nullptr,				// n
			nullptr,				
			nullptr,				// p
			parseQuadBy,			// q	'cccc'
			nullptr,				// r
			parseSmoothCubicBy,		// s	'cccc'
			parseSmoothQuadBy,		// t	'cc'
			nullptr,				// u
			parseVLineBy,			// v  'c'
			nullptr,				// w
			nullptr,				// x
			nullptr,				// y
			parseClose				// z  ''
		};
		
			
		// A sparse table of segmet command function pointers
		// this provides the fastest lookup for a command
		static SVGPathCommandParseFunction SVGPathCommandParseFunctions[] = {
		// nul, soh, stx, etx, eot, enq, ack, bel,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		//  bs,  ht,  nl,  vt,  np,  cr,  so,  si,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		// dle, dc1, dc2, dc3, dc4, nak, syn, etb,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		// can,  em, sub, esc,  fs,  gs,  rs,  us,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		//  sp, '!', '"', '#', '$', '%', '&', ''',
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		// '(', ')', '*', '+', ',', '-', '.', '/',
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		// '0', '1', '2', '3', '4', '5', '6', '7',
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		// '8', '9', ':', ';', '<', '=', '>', '?',
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		// '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
		nullptr, parseArcTo, nullptr, parseCubicTo,  nullptr, nullptr, nullptr,    nullptr,
		// 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
		parseHLineTo,   nullptr,   nullptr,  nullptr,  parseLineTo,  parseMoveTo,  nullptr,  nullptr,
		// 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
		nullptr,  parseQuadTo,  nullptr,  parseSmoothCubicTo,  parseSmoothQuadTo,  nullptr,  parseVLineTo,  nullptr,
		// 'X', 'Y', 'Z', '[', '\', ']', '^', '_',
		nullptr, nullptr, parseClose, nullptr, nullptr, nullptr, nullptr, nullptr,
		// '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
		nullptr,  parseArcBy,  nullptr,  parseCubicBy,  nullptr,  nullptr,  nullptr,  nullptr,
		// 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
		 parseHLineBy,  nullptr,  nullptr,  nullptr,  parseLineBy,  parseMoveBy,  nullptr,  nullptr,
		// 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
		 nullptr,  parseQuadBy,  nullptr,  parseSmoothCubicBy,  parseSmoothQuadBy,  nullptr,  parseVLineBy,  nullptr,
		// 'x', 'y', 'z', '{', '|', '}', '~', del,
		 nullptr,  nullptr,  parseClose, nullptr, nullptr, nullptr, nullptr, nullptr
	};
		
		// parsePath()
		// parse a path string, filling in a BLPath object
		// along the way.
		// Return 'false' if there are any errors
		//
		// Note:  Most SVG objects of significance will have a lot
		// of paths, so we want to make this as fast as possible.
		static bool parsePath(const waavs::ByteSpan& inSpan, BLPath& apath) noexcept
		{
			charset pathWsp = chrWspChars + ',';
				
			// Use a ByteSpan as a cursor on the input
			ByteSpan s = inSpan;
			int iteration = 0;
			SVGPathCommandParseFunction pFunc{ nullptr };
			bool success = false;
			
			while (s)
			{

				// always ignore leading whitespace
				s = chunk_ltrim(s, pathWsp);

				// If we've gotten to the end, we're done
				// so just return
				if (!s)
					break;

				// Check if the character is the beginning of a number
				// if it's not, then try to look up a function pointer
				// It could go the other way, we could check if it's
				// one of the segment commands
				if (!leadingChars(*s)) {
					// Check to see if it's one of our path commands
					// We can do this by performing the hash function
					// and looking up a function pointer
					//int cmdIndex = svgPathCommandHash(*s);
					int cmdIndex = *s;
					if (cmdIndex >= 0 && cmdIndex < 128)
					{
						// We have a potential command index
						// see if it matches a function.
						pFunc = SVGPathCommandParseFunctions[cmdIndex];
						if (pFunc != nullptr) {
							iteration = 0;
							s++;
						}
					}
				}
				
				if (pFunc != nullptr)
				{
					success = pFunc(s, apath, iteration);
				} 
				else {
					success = false;
					printf("FUNCTION POINTER IS NULL: %c\n", *s);
				}
				
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