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
	namespace svgsegmentconstruct
	{

		using SVGSegmentConstructorFunc = std::function<bool (unsigned char segCommand, const double *args, int iteration, BLPath*)>;


		struct SVGSegmentParams 
		{
			const char* fArgTypes{};
			SVGSegmentConstructorFunc fConstructor{};
		};
		
		// Used for the iterator
		struct SVGSegmentParseParams {
			bool fFlattenCommands{ true };
			BLPath *fPath{nullptr};
		};
		
		struct SVGSegmentParseState {
			ByteSpan remains;
			unsigned char fSegmentKind{ 0 };
			int iteration{ 0 };
			int fError{ 0 };
			SVGSegmentConstructorFunc fFunction{ nullptr };
			const char* fArgTypes{};
			double args[8];		// The arguments for the command

			
			SVGSegmentParseState(const ByteSpan& aSpan)
			{
				remains = aSpan;
			}

			bool hasMore() const {
				return remains.size() > 0;
			}
		};



		
		// Command - A
		static bool constructArcTo(unsigned char segCommand, const double* args, int iteration, BLPath *apath) noexcept
		{
			BLResult res = BL_SUCCESS;

			bool larc = args[3] > 0.5f;
			bool swp = args[4] > 0.5f;
			double xrot = radians(args[2]);

			return apath->ellipticArcTo(BLPoint(args[0], args[1]), xrot, larc, swp, BLPoint(args[5], args[6])) == BL_SUCCESS;
		}
		
		// Command - a
		static bool constructArcBy(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{

			BLPoint lastPos{};
			apath->getLastVertex(&lastPos);

			bool larc = args[3] > 0.5;
			bool swp = args[4] > 0.5;
			double xrot = radians(args[2]);


			return  apath->ellipticArcTo(BLPoint(args[0], args[1]), xrot, larc, swp, BLPoint(lastPos.x + args[5], lastPos.y + args[6])) == BL_SUCCESS;
		}
		
		
		// Command - C
		static bool constructCubicTo(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			return  apath->cubicTo(args[0], args[1], args[2], args[3], args[4], args[5]) == BL_SUCCESS;
		}
		
		// Command - c
		static bool constructCubicBy(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			BLPoint lastPos{};
			apath->getLastVertex(&lastPos);

			return apath->cubicTo(lastPos.x + args[0], lastPos.y + args[1], lastPos.x + args[2], lastPos.y + args[3], lastPos.x + args[4], lastPos.y + args[5]) == BL_SUCCESS;
		}

		
		// Command - H
		static bool constructHLineTo(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			BLPoint lastPos{};
			apath->getLastVertex(&lastPos);
			return apath->lineTo(args[0], lastPos.y) == BL_SUCCESS;
		}
		
		// Command - h
		static bool constructHLineBy(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			BLPoint lastPos{};
			apath->getLastVertex(&lastPos);
			return apath->lineTo(lastPos.x + args[0], lastPos.y) == BL_SUCCESS;
		}
		
		// Command 'L' - LineTo
		static bool constructLineTo(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			return apath->lineTo(args[0], args[1]) == BL_SUCCESS;
		}
		
		// Command 'l' - LineBy
		static bool constructLineBy(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			BLPoint lastPos{};
			apath->getLastVertex(&lastPos);

			return apath->lineTo(lastPos.x + args[0], lastPos.y + args[1]) == BL_SUCCESS;
		}
		
		// Command - M
		static bool constructMoveTo(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			BLResult res = BL_SUCCESS;

			if (iteration == 0) {
				res = apath->moveTo(args[0], args[1]);
			}
			else {
				res = apath->lineTo(args[0], args[1]);
			}

			return res == BL_SUCCESS;
		}
		
		// Command - m
		static bool constructMoveBy(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			BLResult res = BL_SUCCESS;

			BLPoint lastPos{};
			apath->getLastVertex(&lastPos);

			if (iteration == 0) {
				res = apath->moveTo(lastPos.x + args[0], lastPos.y + args[1]);
			}
			else {
				res = apath->lineTo(lastPos.x + args[0], lastPos.y + args[1]);
			}

			return res == BL_SUCCESS;
		}
		
		// Command - Q
		static bool constructQuadTo(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			return apath->quadTo(args[0], args[1], args[2], args[3]) == BL_SUCCESS;
		}
		
		// Command - q
		static bool constructQuadBy(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			BLPoint lastPos{};
			apath->getLastVertex(&lastPos);

			return apath->quadTo(lastPos.x + args[0], lastPos.y + args[1], lastPos.x + args[2], lastPos.y + args[3]) == BL_SUCCESS;
		}
		
		// Command - S
		static bool constructSmoothCubicTo(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			return apath->smoothCubicTo(args[0], args[1], args[2], args[3]) == BL_SUCCESS;
		}
		
		// Command - s
		static bool constructSmoothCubicBy(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			BLPoint lastPos{};
			apath->getLastVertex(&lastPos);

			return apath->smoothCubicTo(lastPos.x + args[0], lastPos.y + args[1], lastPos.x + args[2], lastPos.y + args[3]) == BL_SUCCESS;

		}
		
		// Command - T
		static bool constructSmoothQuadTo(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			return apath->smoothQuadTo(args[0], args[1]) == BL_SUCCESS;
		}
		
		// Command - t
		static bool constructSmoothQuadBy(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			BLPoint lastPos{};
			apath->getLastVertex(&lastPos);

			return apath->smoothQuadTo(lastPos.x + args[0], lastPos.y + args[1]) == BL_SUCCESS;
		}
		
		
		// Command - V
		static bool constructVLineTo(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			BLPoint lastPos{};
			apath->getLastVertex(&lastPos);
			return apath->lineTo(lastPos.x, args[0]) == BL_SUCCESS;
		}

		// Command - v
		static bool constructVLineBy(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			BLPoint lastPos{};
			apath->getLastVertex(&lastPos);
			return apath->lineTo(lastPos.x, lastPos.y + args[0]) == BL_SUCCESS;
		}

		
		// 
		// BUGBUG - there is a case where the Z is followed by 
		// a number, which is not a valid SVG path command
		// This number needs to be consumed somewhere
		// Command - Z
		//
		static bool constructClose(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
		{
			return apath->close() == BL_SUCCESS;
		}

		

		
		static bool getSegmentParser(uint8_t cmdIndex, SVGSegmentParams &cmd)
		{

			static std::unordered_map<unsigned char, SVGSegmentParams> SVGSegmentConstructs =
			{

				 {'A',	{"ccrffcc", constructArcTo}}			// ArcTo
				,{'C',	{"cccccc", constructCubicTo}}			// CubicTo
				,{'H',	{"c", constructHLineTo}}				// HLineTo
				,{'L',	{"cc", constructLineTo}}				// LineTo
				,{'M',	{"cc", constructMoveTo}}				// MoveTo
				,{'Q',	{"cccc", constructQuadTo}}				// QuadTo
				,{'S',	{"cccc", constructSmoothCubicTo}}		// SmoothCubicTo
				,{'T',	{"cc", constructSmoothQuadTo}}			// SmoothQuadTo
				,{'V',	{"c", constructVLineTo}}				// VLineTo
				,{'Z',  {nullptr, constructClose}}				// Close
				
				,{'a',	{"ccrffcc", constructArcBy}}			// ArcBy
				,{'c',	{"cccccc", constructCubicBy}}			// CubicBy
				,{'h',	{"c", constructHLineBy}}				// HLineBy
				,{'l',	{"cc", constructLineBy}}				// LineBy
				,{'m',	{"cc", constructMoveBy}}				// MoveBy
				,{'q',	{"cccc", constructQuadBy}}				// QuadBy
				,{'s',	{"cccc", constructSmoothCubicBy}}		// SmoothCubicBy
				,{'t',	{"cc", constructSmoothQuadBy}}			// SmoothQuadBy
				,{'v',	{"c", constructVLineBy}}				// VLineBy
				,{'z',  {nullptr, constructClose}}				// Close

			};

			// Try to find the command in the SVGSegmentConstruct table
			auto it = SVGSegmentConstructs.find(cmdIndex);
			if (it != SVGSegmentConstructs.end())
			{
				cmd = it->second;
				return true;
			}
			
			return false;
		}
		

		static bool readNextSegmentCommand(SVGSegmentParseParams& params, SVGSegmentParseState& cmdState)
		{
			static charset leadingChars("0123456789.+-");          // digits, symbols, and letters found at start of numbers
			static  charset pathWsp = chrWspChars + ',';
			bool success = false;

			// always ignore leading whitespace
			cmdState.remains = chunk_ltrim(cmdState.remains, pathWsp);

			if (!cmdState.remains)
				return false;

			SVGSegmentParams cmd;
			
			if (!leadingChars(*cmdState.remains)) {
				// If we're in here, there must be a command
				// if there isn't it's an error
				
				if (getSegmentParser(*cmdState.remains, cmd))
				{
					cmdState.fSegmentKind = *cmdState.remains;
					cmdState.fFunction = cmd.fConstructor;
					cmdState.iteration = 0;
					cmdState.remains++;
					cmdState.fArgTypes = cmd.fArgTypes;
				}
				else
					return false;
			}

			if (cmdState.fArgTypes != nullptr)
			{
				if (!readNumericArguments(cmdState.remains, cmdState.fArgTypes, cmdState.args))
				{
					cmdState.fError = -1;	// Indicate parsing error
					return false;
				}
				
				return true;
			}
			
			return true;

		}

		
		// parsePath()
		// parse a path string, filling in a BLPath object
		// along the way.
		// Return 'false' if there are any errors
		//
		// Note:  Most SVG objects of significance will have a lot
		// of paths, so we want to make this as fast as possible.
		static bool parsePathSegments(const waavs::ByteSpan& inSpan, BLPath& apath) noexcept
		{

			SVGSegmentParseParams params{};
			params.fPath = &apath;
			SVGSegmentParseState cmdState(inSpan);

			while (readNextSegmentCommand(params, cmdState))
			{
				switch (cmdState.fSegmentKind)
				{
					case 'Z':
					case 'z':
						// For the close commands, there can be a case where there is a number
						// after the 'z', which is not according to spec, but sometimes it's there
						// if that's the case, we want to skip past it, so we can continue
						if (cmdState.iteration > 0)
							cmdState.remains++;
						else
						{
							bool success = cmdState.fFunction(cmdState.fSegmentKind, cmdState.args, cmdState.iteration, params.fPath);
							cmdState.iteration++;

							if (!success)
								return false;
						}
					break;
					
					default: {
						bool success = cmdState.fFunction(cmdState.fSegmentKind, cmdState.args, cmdState.iteration, params.fPath);
						cmdState.iteration++;

						if (!success)
							return false;
					}
					break;
				}
			}

#ifdef PATH_COMMAND_DEBUG
			printf("// parsePath(), FINISHED\n");
#endif
			return true;
		}
	}
}