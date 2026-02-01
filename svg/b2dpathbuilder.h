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
#include "pipeline.h"
#include "blend2d.h"
#include "pathpu.h"


namespace waavs 
{
	// ------------------------------------------------------------
// Executor: PathProgram -> BLPath
// (keeps your current "after close inject moveTo" behavior)
// ------------------------------------------------------------

	struct BLPathProgramExec
	{
		BLPath& path;

		// Track current/subpath start for close injection semantics.
		double cx{ 0.0 }, cy{ 0.0 };
		double sx{ 0.0 }, sy{ 0.0 };
		bool   hasCP{ false };

		bool   pathJustClosed{ false };

		explicit BLPathProgramExec(BLPath& p) noexcept : path(p) {}

		inline void maybeInjectMoveAfterClose_(uint8_t op) noexcept
		{
			if (!pathJustClosed) return;
			if (op == OP_MOVETO) return;

			// mimic your current workaround
			path.moveTo(cx, cy);
			pathJustClosed = false;
		}

		void execute(uint8_t op, const float* a) noexcept
		{
			maybeInjectMoveAfterClose_(op);

			switch (op) {
			case OP_MOVETO: {
				cx = a[0]; cy = a[1];
				sx = cx;   sy = cy;
				hasCP = true;
				path.moveTo(cx, cy);
				pathJustClosed = false;
			} break;

			case OP_LINETO: {
				cx = a[0]; cy = a[1];
				hasCP = true;
				path.lineTo(cx, cy);
				pathJustClosed = false;
			} break;

			case OP_QUADTO: {
				// x1 y1 x y
				cx = a[2]; cy = a[3];
				hasCP = true;
				path.quadTo(a[0], a[1], a[2], a[3]);
				pathJustClosed = false;
			} break;

			case OP_CUBICTO: {
				// x1 y1 x2 y2 x y
				cx = a[4]; cy = a[5];
				hasCP = true;
				path.cubicTo(a[0], a[1], a[2], a[3], a[4], a[5]);
				pathJustClosed = false;
			} break;

			case OP_ARCTO: {
				// rx ry xrot large sweep x y
				const bool larc = a[3] > 0.5f;
				const bool swp = a[4] > 0.5f;

				// Your current code converts degrees->radians here
				const double xrot = radians(a[2]);

				cx = a[5]; cy = a[6];
				hasCP = true;

				path.ellipticArcTo(BLPoint(a[0], a[1]), xrot, larc, swp, BLPoint(a[5], a[6]));
				pathJustClosed = false;
			} break;

			case OP_CLOSE: {
				path.close();
				// after close, SVG current point becomes subpath start
				cx = sx; cy = sy;
				pathJustClosed = true;
			} break;

			default:
				// OP_END should never be executed by runPathProgram()
				break;
			}
		}
	};


	// ------------------------------------------------------------
	// parsePathProgram(): build program from path data
	// ------------------------------------------------------------

	static INLINE bool parsePathProgram(const waavs::ByteSpan& inSpan, PathProgram& outProg) noexcept
	{
		PathProgramBuilder builder;
		PathProgramFromSegments normalizer(builder);

		SVGSegmentIterator iter(inSpan);
		PathSegment seg{};

		while (readNextSegmentCommand(iter, seg)) {
			normalizer.consume(seg);
		}

		builder.end();
		outProg = std::move(builder.prog);
		return true;
	}

	// ------------------------------------------------------------
	// parsePath(): legacy convenience, now via PathProgram
	// ------------------------------------------------------------

	static INLINE bool parsePath(const waavs::ByteSpan& inSpan, BLPath& apath) noexcept
	{
		PathProgram prog;
		if (!parsePathProgram(inSpan, prog))
			return false;

		BLPathProgramExec exec(apath);
		runPathProgram(prog, exec);
		return true;
	}
}

namespace waavs {
	/*
	//
	// Note:  Most SVG objects of significance will have a lot
	// of paths, so we want to make this as fast as possible.
	struct B2DPathBuilder : public IConsume<PathSegment>
	{
		BLPath& fPath;			// Reference to the path we are building
		bool pathJustClosed = false;	// has the path seen a 'Z' or 'z' yet


		// Initialize a path builder with a reference to a path
		B2DPathBuilder(BLPath& apath) :fPath(apath) {}

		inline void setPathClosed() noexcept { pathJustClosed = true; }
		inline void clearPathClosed() noexcept { pathJustClosed = false; }


		//
		// addSegment()
		// 
		
		// consume()
		// 
		// Similar to addGeometry on the BLPath object
		// whereas addGeometry is very low level, and not an exact match to the SVG
		// segments, addSegment is higher level, dealing with segment commands, which 
		// in the end might result in multiple 'geometries' being added to the BLPath
		// as certain curves are broken down into multiple simpler curves
		void consume(const PathSegment& seg)
		{
			// if the last command was a 'Z' or 'z', then, if the next
			// command is not a 'M' or 'm', we need to first insert a moveTo
			// for the last location, then perform the next command
			// Only inject moveTo if last operation closed the path
			if (pathJustClosed && (seg.fSegmentKind != SVGPathCommand::M && seg.fSegmentKind != SVGPathCommand::m))
			{
				BLPoint lastPos{};
				fPath.getLastVertex(&lastPos);
				fPath.moveTo(lastPos.x, lastPos.y);
			}

			// Fast path: inline the hottest commands
			if (seg.fSegmentKind == SVGPathCommand::M) { moveTo({ seg }); return; }
			if (seg.fSegmentKind == SVGPathCommand::L) { lineTo(seg); return; }
			if (seg.fSegmentKind == SVGPathCommand::C) { cubicTo(seg); return; }
			if (seg.fSegmentKind == SVGPathCommand::Z || seg.fSegmentKind == SVGPathCommand::z) { close(seg); return; }

			// Fallback switch
			switch (seg.fSegmentKind) {
			case SVGPathCommand::m: { moveBy(seg); return; }
			case SVGPathCommand::l: { lineBy(seg); return; }
			case SVGPathCommand::c: { cubicBy(seg); return; }
			case SVGPathCommand::H: { hLineTo(seg); return; }
			case SVGPathCommand::h: { hLineBy(seg); return; }
			case SVGPathCommand::V: { vLineTo(seg); return; }
			case SVGPathCommand::v: { vLineBy(seg); return; }
			case SVGPathCommand::Q: { quadTo(seg); return; }
			case SVGPathCommand::q: { quadBy(seg); return; }
			case SVGPathCommand::S: { smoothCubicTo(seg); return; }
			case SVGPathCommand::s: { smoothCubicBy(seg); return; }
			case SVGPathCommand::T: { smoothQuadTo(seg); return; }
			case SVGPathCommand::t: { smoothQuadBy(seg); return; }
			case SVGPathCommand::A: { arcTo(seg); return; }
			case SVGPathCommand::a: { arcBy(seg); return; }
			default:
				printf("Unknown command: %c\n", static_cast<char>(seg.fSegmentKind));
				return ;
			}

		}


		// All the routines that do the actual work of building the path
		// Command - A
		int arcTo(const PathSegment &seg) noexcept
		{
			bool larc = seg.args()[3] > 0.5f;
			bool swp = seg.args()[4] > 0.5f;
			double xrot = radians(seg.fArgs[2]);

			return fPath.ellipticArcTo(BLPoint(seg.args()[0] , seg.args()[1]), xrot, larc, swp, BLPoint(seg.args()[5], seg.args()[6]));
		}

		// Command - a
		int arcBy(const PathSegment& seg) noexcept
		{

			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			bool larc = seg.args()[3] > 0.5;
			bool swp = seg.args()[4] > 0.5;
			double xrot = radians(seg.args()[2]);


			return  fPath.ellipticArcTo(BLPoint(seg.args()[0], seg.args()[1]), xrot, larc, swp, BLPoint(lastPos.x + seg.args()[5], lastPos.y + seg.args()[6]));
		}


		// Command - C
		int cubicTo(const PathSegment& seg) noexcept
		{
			return  fPath.cubicTo(seg.args()[0], seg.args()[1], seg.args()[2], seg.args()[3], seg.args()[4], seg.args()[5]);
		}

		// Command - c
		int cubicBy(const PathSegment& seg) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.cubicTo(lastPos.x + seg.args()[0], lastPos.y + seg.args()[1], lastPos.x + seg.args()[2], lastPos.y + seg.args()[3], lastPos.x + seg.args()[4], lastPos.y + seg.args()[5]);
		}

		// Command - H
		int hLineTo(const PathSegment& seg) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.lineTo(seg.args()[0], lastPos.y);
		}

		// Command - h
		int hLineBy(const PathSegment& seg) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			auto res = fPath.lineTo(lastPos.x + seg.args()[0], lastPos.y);
			return res;
		}

		// Command 'L' - LineTo
		int lineTo(const PathSegment& seg) noexcept
		{
			return fPath.lineTo(seg.args()[0], seg.args()[1]);
		}

		// Command 'l' - LineBy
		int lineBy(const PathSegment& seg) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.lineTo(lastPos.x + seg.args()[0], lastPos.y + seg.args()[1]);
		}

		// Command - M
		int moveTo(const PathSegment& seg) noexcept
		{
			BLResult res = BL_SUCCESS;

			if (seg.iteration() == 0) {
				res = fPath.moveTo(seg.args()[0], seg.args()[1]);
			}
			else {
				res = fPath.lineTo(seg.args()[0], seg.args()[1]);
			}
			clearPathClosed();

			return res;
		}

		// Command - m
		int moveBy(const PathSegment& seg) noexcept
		{
			BLResult res = BL_SUCCESS;

			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			double dx = seg.args()[0];
			double dy = seg.args()[1];

			if (seg.iteration() == 0) {
				res = fPath.moveTo(lastPos.x + dx, lastPos.y + dy);
			}
			else {
				res = fPath.lineTo(lastPos.x + dx, lastPos.y + dy);
			}
			clearPathClosed();

			return res;
		}

		// Command - Q
		int quadTo(const PathSegment& seg) noexcept
		{
			return fPath.quadTo(seg.args()[0], seg.args()[1], seg.args()[2], seg.args()[3]);
		}

		// Command - q
		int quadBy(const PathSegment& seg) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.quadTo(lastPos.x + seg.args()[0], lastPos.y + seg.args()[1], lastPos.x + seg.args()[2], lastPos.y + seg.args()[3]);
		}

		// Command - S
		int smoothCubicTo(const PathSegment& seg) noexcept
		{
			return fPath.smoothCubicTo(seg.args()[0], seg.args()[1], seg.args()[2], seg.args()[3]);
		}

		// Command - s
		int smoothCubicBy(const PathSegment& seg) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.smoothCubicTo(lastPos.x + seg.args()[0], lastPos.y + seg.args()[1], lastPos.x + seg.args()[2], lastPos.y + seg.args()[3]);
		}

		// Command - T
		int smoothQuadTo(const PathSegment& seg) noexcept
		{
			return fPath.smoothQuadTo(seg.args()[0], seg.args()[1]);
		}

		// Command - t
		int smoothQuadBy(const PathSegment& seg) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.smoothQuadTo(lastPos.x + seg.args()[0], lastPos.y + seg.args()[1]);
		}

		// Command - V
		int vLineTo(const PathSegment& seg) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.lineTo(lastPos.x, seg.args()[0]);
		}

		// Command - v
		int vLineBy(const PathSegment& seg) noexcept
		{
			BLPoint lastPos{};
			fPath.getLastVertex(&lastPos);

			return fPath.lineTo(lastPos.x, lastPos.y + seg.args()[0]);
		}

		// Command - Z, z
		//
		int close(const PathSegment& seg) noexcept
		{
			int err = fPath.close();
			setPathClosed();

			return err;
		}

	};
	*/
	/*
	// parsePath()
	// parse a path string, filling in a BLPath object according
	// to the individual segment commands.
	// 
	// Return 'false' if there are any errors
	//
	static INLINE bool parsePath(const waavs::ByteSpan& inSpan, BLPath& apath) noexcept
	{
		B2DPathBuilder builder(apath);
		SVGSegmentIterator iter(inSpan);

		PathSegment seg{};
		while (readNextSegmentCommand(iter, seg))
		{
			builder.consume(seg);
		}

		return true;
	}
	*/
}


