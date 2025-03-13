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
		uint8_t lastCmd{ 0 };	// The last command we processed
		BLPath& fPath;			// Reference to the path we are building

		// Initialize a path builder with a reference to a path
		B2DPathBuilder(BLPath& apath) :fPath(apath) {}

		using CommandFunc = int (B2DPathBuilder::*)(const double*, int);
		static inline const std::array<CommandFunc, 128>& getCommandTable()
		{
			static std::array<CommandFunc, 128> cmdTbl = [] {
				std::array<CommandFunc, 128> table{};  // Zero-initializes all elements to nullptr
				table['A'] = &B2DPathBuilder::arcTo;   table['a'] = &B2DPathBuilder::arcBy;
				table['C'] = &B2DPathBuilder::cubicTo; table['c'] = &B2DPathBuilder::cubicBy;
				table['L'] = &B2DPathBuilder::lineTo;  table['l'] = &B2DPathBuilder::lineBy;
				table['M'] = &B2DPathBuilder::moveTo;  table['m'] = &B2DPathBuilder::moveBy;
				table['Q'] = &B2DPathBuilder::quadTo;  table['q'] = &B2DPathBuilder::quadBy;
				table['S'] = &B2DPathBuilder::smoothCubicTo; table['s'] = &B2DPathBuilder::smoothCubicBy;
				table['T'] = &B2DPathBuilder::smoothQuadTo;  table['t'] = &B2DPathBuilder::smoothQuadBy;
				table['H'] = &B2DPathBuilder::hLineTo; table['h'] = &B2DPathBuilder::hLineBy;
				table['V'] = &B2DPathBuilder::vLineTo; table['v'] = &B2DPathBuilder::vLineBy;
				table['Z'] = &B2DPathBuilder::close;   table['z'] = &B2DPathBuilder::close;
				return table;
				}();

			return cmdTbl;
		}

		// Overload operator() to handle the events we are subscribed to
		// This allows the class to be a subscriber, and all the events
		// will be handled by this function
		int operator()(const SVGSegmentParseState& cmdState)
		{
			// if the last command was a 'Z' or 'z', then, if the next
			// command is not a 'M' or 'm', we need to first insert a moveTo
			// for the last location, then perform the next command
			if (((lastCmd == 'Z') || (lastCmd == 'z')) && (cmdState.fSegmentKind != 'M' || cmdState.fSegmentKind != 'm'))
			{
				BLPoint lastPos{};
				fPath.getLastVertex(&lastPos);

				fPath.moveTo(lastPos.x, lastPos.y);
			}

			const auto& commandTable = getCommandTable();
			if (cmdState.fSegmentKind < 128 && commandTable[cmdState.fSegmentKind])
			{
				int err = (this->*(commandTable[cmdState.fSegmentKind]))(cmdState.args, cmdState.iteration);
				if (err != BL_SUCCESS)
					return err;	// indicating an error
			}

			lastCmd = cmdState.fSegmentKind;

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

			return res;
		}

		// Command - m
		int moveBy(const double* args, int iteration) noexcept
		{
			BLResult res = BL_SUCCESS;

			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			if (iteration == 0) {
				res = fPath.moveTo(lastPos.x + args[0], lastPos.y + args[1]);
			}
			else {
				res = fPath.lineTo(lastPos.x + args[0], lastPos.y + args[1]);
			}

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
			return fPath.close();
		}

	};


	// parsePath()
	// parse a path string, filling in a BLPath object
	// along the way.
	// Return 'false' if there are any errors
	static INLINE bool parsePath(const waavs::ByteSpan& inSpan, BLPath& apath) noexcept
	{
		PathCommandDispatch dispatch;
		B2DPathBuilder builder(apath);

		dispatch.addSubscriber(builder);
		dispatch.parse(inSpan);
	
		return true;
	}
}


