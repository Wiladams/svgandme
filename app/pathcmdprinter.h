#pragma once

#include <array>
#include "pathsegmenter.h"
#include "pipeline.h"

namespace waavs 
{
	// ConsumerFn interface for printing path segments
	static void printCompactPathSegment(const PathSegment& seg)
	{
		printf("%c ", static_cast<char>(seg.fSegmentKind));
		int n = seg.fArgCount;
		switch (n) 
		{
		case 7:
			printf("%3.2f ", seg.args()[n - 7]);
		case 6:
			printf("%3.2f ", seg.args()[n - 6]);
		case 5:
			printf("%3.2f ", seg.args()[n - 5]);
		case 4:
			printf("%3.2f ", seg.args()[n - 4]);
		case 3:
			printf("%3.2f ", seg.args()[n - 3]);
		case 2:
			printf("%3.2f ", seg.args()[n - 2]);
		case 1:
			printf("%3.2f ", seg.args()[n - 1]);
		default:
			printf("\n");
		}

	}
	
	static void printCompactSegments(ProducerFn<PathSegment> src)
	{
		PathSegment seg{};
		while (src(seg))
		{
			printCompactPathSegment(seg);
		}
	}

	// Take path segment commands, and turn them into print statements
	// that show how the commands apply to a BLPath object
	struct PathCmdPrinter : public IConsume<PathSegment>
	{
		// A tiny bit of state we maintain
		// primarily to support relative position operations 'by'
		Point2d fLastMoveTo{};
		Point2d fLastPoint{};

		// Return the array that maps the single letter commands to the functions
		// that handle their arguments
		using SegFunc = void (PathCmdPrinter::*)(const PathSegment &);
		//using SegFunc = ConsumerFn<const PathSegment&>;
		static inline const std::array<SegFunc, 128>& getCommandTable()
		{
			static std::array<SegFunc, 128> cmdTbl = [] {
				std::array<SegFunc, 128> table{};  // Zero-initializes all elements to nullptr
				table['A'] = &PathCmdPrinter::arcTo;   table['a'] = &PathCmdPrinter::arcBy;
				table['C'] = &PathCmdPrinter::cubicTo; table['c'] = &PathCmdPrinter::cubicBy;
				table['L'] = &PathCmdPrinter::lineTo;  table['l'] = &PathCmdPrinter::lineBy;
				table['M'] = &PathCmdPrinter::moveTo;  table['m'] = &PathCmdPrinter::moveBy;
				table['Q'] = &PathCmdPrinter::quadTo;  table['q'] = &PathCmdPrinter::quadBy;
				table['S'] = &PathCmdPrinter::smoothCubicTo; table['s'] = &PathCmdPrinter::smoothCubicBy;
				table['T'] = &PathCmdPrinter::smoothQuadTo;  table['t'] = &PathCmdPrinter::smoothQuadBy;
				table['H'] = &PathCmdPrinter::hLineTo; table['h'] = &PathCmdPrinter::hLineBy;
				table['V'] = &PathCmdPrinter::vLineTo; table['v'] = &PathCmdPrinter::vLineBy;
				table['Z'] = &PathCmdPrinter::close;   table['z'] = &PathCmdPrinter::close;
				return table;
				}();

			return cmdTbl;
		}

		// generic printf to log a single command
		// This requires all the arguments to be in the args array
		// so, it doesn't work when that's not the case.
		// The precision of the prints needs to be changed here
		static inline void logCommand(const char *cmd, const float* args, int n) noexcept
		{
			printf("apath.%s(", cmd);

			switch (n) {
			case 7:
				printf("%3.2f, ", args[n - 7]);
			case 6:
				printf("%3.2f, ", args[n - 6]);
			case 5:
				printf("%3.2f, ", args[n - 5]);
			case 4:
				printf("%3.2f, ", args[n - 4]);
			case 3:
				printf("%3.2f, ", args[n - 3]);
			case 2:
				printf("%3.2f, ", args[n - 2]);
			case 1:
				printf("%3.2f ", args[n - 1]);
			default:
				printf(");\n");
			}

		}




		PathCmdPrinter() = default;


		void reset()
		{
			fLastMoveTo = {};
			fLastPoint = {};
		}

		// Given a reference point, and an array of doubles, return a new point
		// which is the reference point plus the x and y values in the array
		inline Point2d relativePoint(const Point2d& ref, const float* args, int offset = 0) noexcept
		{
			return { ref.x + args[offset], ref.y + args[offset + 1] };
		}

		void consume(const PathSegment &seg) override
		{
			const auto& commandTable = getCommandTable();
			// make sure the segment kind is within the range we can
			// handle, and ensure the segment kind has a function to
			// handle it.
			// Call the appropriate function to handle the segment kind
			if (static_cast<char>(seg.command()) < 128 && commandTable[static_cast<char>(seg.command())])
			{
				(this->*(commandTable[static_cast<char>(seg.command())]))(seg);
			}

			// Make sure we capture the last moveTo as 
			// the lastPoint
			if (seg.command() == SVGPathCommand::M || seg.command() == SVGPathCommand::m)
			{
				fLastMoveTo = fLastPoint;
			}
		}

		// Overload operator() to handle the events we are subscribed to
		// This allows the class to be a subscriber, and all the events
		// will be handled by this function
		//void operator()(const PathSegment& seg)
		//{
		//	execCommand(seg);
		//}



		// Command - A
		void arcTo(const PathSegment& seg)
		{
			bool larc = seg.args()[3] > 0.5f;
			bool swp = seg.args()[4] > 0.5f;
			double xrot = radians(seg.args()[2]);

			// need to convert to radians before sending along
			//logCommand("ellipticArcTo", args, 7);
			printf("apath.ellipticArcTo(BLPoint(%f, %f), %f, %d, %d, BLPoint(%f, %f));\n",
				seg.args()[0], seg.args()[1], xrot, larc, swp, seg.args()[5], seg.args()[6]);

			fLastPoint = { seg.args()[5], seg.args()[6] };

		}

		// Command - a
		void arcBy(const PathSegment& seg)
		{
			bool larc = seg.args()[3] > 0.5;
			bool swp = seg.args()[4] > 0.5;
			double xrot = radians(seg.args()[2]);
			Point2d lastPos = relativePoint(fLastPoint, seg.args(), 5);

			printf("apath.ellipticArcTo(BLPoint(%f, %f), %f, %d, %d, BLPoint(%f, %f));\n",
				seg.args()[0], seg.args()[1], xrot, larc, swp, lastPos.x, lastPos.y);

			fLastPoint = lastPos;

		}


		// Command - C
		void cubicTo(const PathSegment& seg) noexcept
		{
			logCommand("cubicTo", seg.args(), 6);
			fLastPoint = { seg.args()[4], seg.args()[5] };
		}

		// Command - c
		void cubicBy(const PathSegment& seg) noexcept
		{
			Point2d lastPos = relativePoint(fLastPoint, seg.args(), 4);

			printf("apath.cubicTo(%f, %f, %f, %f, %f, %f);\n", fLastPoint.x + seg.args()[0], fLastPoint.y + seg.args()[1], fLastPoint.x + seg.args()[2], fLastPoint.y + seg.args()[3], lastPos.x, lastPos.y);
			fLastPoint = lastPos;
		}


		// Command - H
		void hLineTo(const PathSegment& seg) noexcept
		{
			printf("apath.lineTo(%f, %f);\n", seg.args()[0], fLastPoint.y);
			fLastPoint = { seg.args()[0], fLastPoint.y };
		}

		// Command - h
		void hLineBy(const PathSegment& seg) noexcept
		{
			printf("apath.lineTo(%f, %f);\n", fLastPoint.x + seg.args()[0], fLastPoint.y);
			fLastPoint = { fLastPoint.x + seg.args()[0], fLastPoint.y };
		}

		// Command 'L' - LineTo
		void lineTo(const PathSegment& seg) noexcept
		{

			logCommand("lineTo", seg.args(), 2);
			//printf("apath.lineTo(%f, %f);\n", args[0], args[1]);
			fLastPoint = { seg.args()[0], seg.args()[1] };

		}

		// Command 'l' - LineBy
		void lineBy(const PathSegment& seg) noexcept
		{
			Point2d lastPos = relativePoint(fLastPoint, seg.args(), 0);

			printf("apath.lineTo(%f, %f);\n", lastPos.x, lastPos.y);
			fLastPoint = lastPos;
		}

		// Command - M
		void moveTo(const PathSegment& seg) noexcept
		{
			if (seg.iteration() == 0) {
				fLastMoveTo = { seg.args()[0], seg.args()[1] };
				fLastPoint = fLastMoveTo;

				logCommand("moveTo", seg.args(), 2);
				//printf("apath.moveTo(%f, %f);\n", fLastMoveTo.x, fLastMoveTo.y);
			}
			else {
				lineTo(seg);
			}

		}

		// Command - m
		void moveBy(const PathSegment& seg) noexcept
		{

			if (seg.iteration() == 0) {
				Point2d lastPos = relativePoint(fLastPoint, seg.args(), 0);

				printf("apath.moveTo(%f, %f);\n", lastPos.x, lastPos.y);
				fLastMoveTo = lastPos;
				fLastPoint = lastPos;
			}
			else {
				lineBy(seg);
			}
		}

		// Command - Q
		void quadTo(const PathSegment& seg) noexcept
		{
			logCommand("quadTo", seg.args(), 4);
			//printf("apath.quadTo(%f, %f, %f, %f);\n", args[0], args[1], args[2], args[3]);
			fLastPoint = { seg.args()[2], seg.args()[3] };
		}

		// Command - q
		void quadBy(const PathSegment& seg) noexcept
		{
			Point2d lastPos = relativePoint(fLastPoint, seg.args(), 2);

			printf("apath.quadTo(%f, %f, %f, %f);\n", fLastPoint.x + seg.args()[0], fLastPoint.y + seg.args()[1], lastPos.x, lastPos.y);
			fLastPoint = lastPos;
		}

		// Command - S
		void smoothCubicTo(const PathSegment& seg) noexcept
		{
			printf("apath.smoothCubicTo(%f, %f, %f, %f);\n", seg.args()[0], seg.args()[1], seg.args()[2], seg.args()[3]);
			fLastPoint = { seg.args()[2], seg.args()[3] };
		}

		// Command - s
		void smoothCubicBy(const PathSegment& seg) noexcept
		{
			Point2d lastPos = relativePoint(fLastPoint, seg.args(), 2);

			printf("apath.smoothCubicTo(%f, %f, %f, %f);\n", fLastPoint.x + seg.args()[0], fLastPoint.y + seg.args()[1], fLastPoint.x + seg.args()[2], fLastPoint.y + seg.args()[3]);
			fLastPoint = lastPos;
		}

		// Command - T
		void smoothQuadTo(const PathSegment& seg) noexcept
		{
			printf("apath.smoothQuadTo(%f, %f);\n", seg.args()[0], seg.args()[1]);
			fLastPoint = { seg.args()[0], seg.args()[1] };
		}

		// Command - t
		void smoothQuadBy(const PathSegment& seg) noexcept
		{
			Point2d lastPos = relativePoint(fLastPoint, seg.args(), 0);

			printf("apath.smoothQuadTo(%f, %f);\n", fLastPoint.x + seg.args()[0], fLastPoint.y + seg.args()[1]);
			fLastPoint = lastPos;
		}


		// Command - V
		void vLineTo(const PathSegment& seg) noexcept
		{
			printf("apath.lineTo(%f, %f);\n", fLastPoint.x, seg.args()[0]);
			fLastPoint = { fLastPoint.x, seg.args()[0] };
		}

		// Command - v
		void vLineBy(const PathSegment& seg) noexcept
		{
			printf("apath.lineTo(%f, %f);\n", fLastPoint.x, fLastPoint.y + seg.args()[0]);
			fLastPoint = { fLastPoint.x, fLastPoint.y + seg.args()[0] };
		}


		// 
		// BUGBUG - there is a case where the Z is followed by 
		// a number, which is not a valid SVG path command
		// This number needs to be consumed somewhere
		// Command - Z
		//
		void close(const PathSegment& seg) noexcept
		{
			printf("apath.close();\n");
			fLastPoint = fLastMoveTo;
		}

	};
}
