#pragma once

//
// svgpath.h 
// The SVGPath element contains a fairly complex set of commands in the 'd' attribute
// These commands are used to define the shape of the path, and can be fairly intricate.
// This file contains the code to parse the 'd' attribute and turn it into a BLPath object
//
// Usage:
// BLPath aPath{};
// parsePath("M 10 10 L 90 90", aPath);
//
// Noted:
// A convenient visualtion tool can be found here: https://svg-path-visualizer.netlify.app/
// References:
// https://svgwg.org/svg2-draft/paths.html#PathDataBNF



#include "pathsegmenter.h"
#include "blend2d.h"
	
		
namespace waavs {
	// Command - A
	static bool constructArcTo(unsigned char segCommand, const double* args, int iteration, BLPath* apath) noexcept
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

}




namespace waavs {
	// parsePath()
	// parse a path string, filling in a BLPath object
	// along the way.
	// Return 'false' if there are any errors
	//
	// Note:  Most SVG objects of significance will have a lot
	// of paths, so we want to make this as fast as possible.
	static INLINE bool parsePath(const waavs::ByteSpan& inSpan, BLPath& apath) noexcept
	{

		SVGSegmentParseParams params{};
		SVGSegmentParseState cmdState(inSpan);


		while (readNextSegmentCommand(params, cmdState))
		{
			bool success{ true };

			switch (cmdState.fSegmentKind)
			{
			case 'A':
				success = constructArcTo('A', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'a':
				success = constructArcBy('a', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'C':
				success = constructCubicTo('C', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'c':
				success = constructCubicBy('c', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'H':
				success = constructHLineTo('H', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'h':
				success = constructHLineBy('h', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'L':
				success = constructLineTo('L', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'l':
				success = constructLineBy('l', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'M':
				success = constructMoveTo('M', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'm':
				success = constructMoveBy('m', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'Q':
				success = constructQuadTo('Q', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'q':
				success = constructQuadBy('q', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'S':
				success = constructSmoothCubicTo('S', cmdState.args, cmdState.iteration, &apath);
				break;

			case 's':
				success = constructSmoothCubicBy('s', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'T':
				success = constructSmoothQuadTo('T', cmdState.args, cmdState.iteration, &apath);
				break;

			case 't':
				success = constructSmoothQuadBy('t', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'V':
				success = constructVLineTo('V', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'v':
				success = constructVLineBy('v', cmdState.args, cmdState.iteration, &apath);
				break;

			case 'Z':
			case 'z':
				// For the close commands, there can be a case where there is a number
				// after the 'z', which is not according to spec, but sometimes it's there
				// if that's the case, we want to skip past it, so we can continue
				if (cmdState.iteration > 0)
					cmdState.remains++;
				else
				{
					success = constructClose(cmdState.fSegmentKind, cmdState.args, cmdState.iteration, &apath);
					//cmdState.iteration++;

					//if (!success)
					//	return false;
				}
				break;

			default: {
				success = constructClose(cmdState.fSegmentKind, cmdState.args, cmdState.iteration, &apath);
				//cmdState.iteration++;

			}
				   break;
			}

			cmdState.iteration++;
			if (!success)
				return false;

		}


		return true;
	}

}

