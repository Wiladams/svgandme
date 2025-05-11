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

	//
	// Note:  Most SVG objects of significance will have a lot
	// of paths, so we want to make this as fast as possible.
	struct B2DPathBuilder
	{
		//uint8_t lastCmd{ 0 };	// The last command we processed
		BLPath& fPath;			// Reference to the path we are building
		bool pathJustClosed = false;	// has the path seen a 'Z' or 'z' yet


		// Initialize a path builder with a reference to a path
		B2DPathBuilder(BLPath& apath) :fPath(apath) {}

		inline void setPathClosed() noexcept { pathJustClosed = true; }
		inline void clearPathClosed() noexcept { pathJustClosed = false; }


		//
		// addSegment()
		// Similar to addGeometry on the BLPath object
		// whereas addGeometry is very low level, and not an exact match to the SVG
		// segments, addSegment is higher level, dealing with segment commands, which 
		// in the end might result in multiple 'geometries' being added to the BLPath
		// as certain curves are broken down into multiple simpler curves
		int addSegment(unsigned char cmd, const double *args, size_t iteration=0)
		{
			// if the last command was a 'Z' or 'z', then, if the next
			// command is not a 'M' or 'm', we need to first insert a moveTo
			// for the last location, then perform the next command
			// Only inject moveTo if last operation closed the path
			if (pathJustClosed && (cmd != 'M' && cmd != 'm')) 
			{
				BLPoint lastPos{};
				fPath.getLastVertex(&lastPos);
				fPath.moveTo(lastPos.x, lastPos.y);
			}

			// Fast path: inline the hottest commands
			if (cmd == 'M') return moveTo(args, iteration);
			if (cmd == 'L') return lineTo(args, iteration);
			if (cmd == 'C') return cubicTo(args, iteration);
			if (cmd == 'Z' || cmd == 'z') return close(args, iteration);

			// Fallback switch
			switch (cmd) {
			case 'm': return moveBy(args, iteration);
			case 'l': return lineBy(args, iteration);
			case 'c': return cubicBy(args, iteration);
			case 'H': return hLineTo(args, iteration);
			case 'h': return hLineBy(args, iteration);
			case 'V': return vLineTo(args, iteration);
			case 'v': return vLineBy(args, iteration);
			case 'Q': return quadTo(args, iteration);
			case 'q': return quadBy(args, iteration);
			case 'S': return smoothCubicTo(args, iteration);
			case 's': return smoothCubicBy(args, iteration);
			case 'T': return smoothQuadTo(args, iteration);
			case 't': return smoothQuadBy(args, iteration);
			case 'A': return arcTo(args, iteration);
			case 'a': return arcBy(args, iteration);
			default:
				printf("Unknown command: %c\n", cmd);
				return -1;
			}

		}

		// Overload operator() to handle the events we are subscribed to
		// This allows the class to be a subscriber, and all the events
		// will be handled by this function
		int operator()(const SVGSegmentParseState& cmdState)
		{
			return addSegment(cmdState.fSegmentKind, cmdState.args, cmdState.iteration);
		}

		// All the routines that do the actual work of building the path
		// Command - A
		int arcTo(const double* args, int iteration) noexcept
		{
			bool larc = args[3] > 0.5f;
			bool swp = args[4] > 0.5f;
			double xrot = radians(args[2]);

			return fPath.ellipticArcTo(BLPoint(args[0], args[1]), xrot, larc, swp, BLPoint(args[5], args[6]));
		}

		// Command - a
		int arcBy(const double* args, int iteration) noexcept
		{

			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			bool larc = args[3] > 0.5;
			bool swp = args[4] > 0.5;
			double xrot = radians(args[2]);


			return  fPath.ellipticArcTo(BLPoint(args[0], args[1]), xrot, larc, swp, BLPoint(lastPos.x + args[5], lastPos.y + args[6]));
		}


		// Command - C
		int cubicTo(const double* args, int iteration) noexcept
		{
			return  fPath.cubicTo(args[0], args[1], args[2], args[3], args[4], args[5]);
		}

		// Command - c
		int cubicBy(const double* args, int iteration) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.cubicTo(lastPos.x + args[0], lastPos.y + args[1], lastPos.x + args[2], lastPos.y + args[3], lastPos.x + args[4], lastPos.y + args[5]);
		}

		// Command - H
		int hLineTo(const double* args, int iteration) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.lineTo(args[0], lastPos.y);
		}

		// Command - h
		int hLineBy(const double* args, int iteration) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			auto res = fPath.lineTo(lastPos.x + args[0], lastPos.y);
			return res;
		}

		// Command 'L' - LineTo
		int lineTo(const double* args, int iteration) noexcept
		{
			return fPath.lineTo(args[0], args[1]);
		}

		// Command 'l' - LineBy
		int lineBy(const double* args, int iteration) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.lineTo(lastPos.x + args[0], lastPos.y + args[1]);
		}

		// Command - M
		int moveTo(const double* args, int iteration) noexcept
		{
			BLResult res = BL_SUCCESS;

			if (iteration == 0) {
				res = fPath.moveTo(args[0], args[1]);
			}
			else {
				res = fPath.lineTo(args[0], args[1]);
			}
			clearPathClosed();

			return res;
		}

		// Command - m
		int moveBy(const double* args, int iteration) noexcept
		{
			BLResult res = BL_SUCCESS;

			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			double dx = args[0];
			double dy = args[1];

			if (iteration == 0) {
				res = fPath.moveTo(lastPos.x + dx, lastPos.y + dy);
			}
			else {
				res = fPath.lineTo(lastPos.x + dx, lastPos.y + dy);
			}
			clearPathClosed();

			return res;
		}

		// Command - Q
		int quadTo(const double* args, int iteration) noexcept
		{
			return fPath.quadTo(args[0], args[1], args[2], args[3]);
		}

		// Command - q
		int quadBy(const double* args, int iteration) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.quadTo(lastPos.x + args[0], lastPos.y + args[1], lastPos.x + args[2], lastPos.y + args[3]);
		}

		// Command - S
		int smoothCubicTo(const double* args, int iteration) noexcept
		{
			return fPath.smoothCubicTo(args[0], args[1], args[2], args[3]);
		}

		// Command - s
		int smoothCubicBy(const double* args, int iteration) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.smoothCubicTo(lastPos.x + args[0], lastPos.y + args[1], lastPos.x + args[2], lastPos.y + args[3]);
		}

		// Command - T
		int smoothQuadTo(const double* args, int iteration) noexcept
		{
			return fPath.smoothQuadTo(args[0], args[1]);
		}

		// Command - t
		int smoothQuadBy(const double* args, int iteration) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.smoothQuadTo(lastPos.x + args[0], lastPos.y + args[1]);
		}

		// Command - V
		int vLineTo(const double* args, int iteration) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.lineTo(lastPos.x, args[0]);
		}

		// Command - v
		int vLineBy(const double* args, int iteration) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.lineTo(lastPos.x, lastPos.y + args[0]);
		}

		// Command - Z, z
		//
		int close(const double* args, int iteration) noexcept
		{
			int err = fPath.close();
			setPathClosed();

			return err;
		}

	};


	// parsePath()
	// parse a path string, filling in a BLPath object according
	// to the individual segment commands.
	// 
	// Return 'false' if there are any errors
	//
	static INLINE bool parsePath(const waavs::ByteSpan& inSpan, BLPath& apath) noexcept
	{
		B2DPathBuilder builder(apath);
	

		SVGSegmentParseParams params{};
		SVGSegmentParseState cmdState(inSpan);

		while (readNextSegmentCommand(params, cmdState))
		{
			builder(cmdState);
		}

		return true;
	}
}


