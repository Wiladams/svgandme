#pragma once

//#include <unordered_map>
#include <array>

#include "pathsegmenter.h"
#include "pubsub.h"
#include "blend2d.h"

using namespace waavs;


namespace waavs {
	struct PathPoint {
		double x;
		double y;
	};

	// PathCommandDispatch
	// 
	// A topic which will generate SVGSegmentParseState events
	// An interested party can subscribe to this topic
	// and handle the incoming events in whatever way they want
	// By having this as a topic, we get a loose coupling, which does
	// not require complex inheritance chains to deal with the events
	struct PathCommandDispatch : public Topic<SVGSegmentParseState>
	{
		bool parse(const waavs::ByteSpan& inSpan) noexcept
		{
			SVGSegmentParseParams params{};
			SVGSegmentParseState cmdState(inSpan);

			while (readNextSegmentCommand(params, cmdState))
			{
				notify(cmdState);
			}

			return true;
		}
	};

	// Take path segment commands, and turn them into print statements
	// that show how the commands apply to a BLPath object
	struct PathCmdPrinter {
		// A tiny bit of state we maintain
		// primarily to support relative position operations 'by'
		PathPoint fLastMoveTo{};
		PathPoint fLastPoint{};

		// Return the array that maps the single letter commands to the functions
		// that handle their arguments
		using CommandFunc = void (PathCmdPrinter::*)(const double*, int);
		static inline const std::array<CommandFunc, 128>& getCommandTable()
		{
			static std::array<CommandFunc, 128> cmdTbl = [] {
				std::array<CommandFunc, 128> table{};  // Zero-initializes all elements to nullptr
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
		static inline void logCommand(const char* cmd, const double* args, int n) noexcept 
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
		inline PathPoint relativePoint(const PathPoint& ref, const double* args, int offset = 0) noexcept 
		{
			return { ref.x + args[offset], ref.y + args[offset + 1] };
		}

		// Overload operator() to handle the events we are subscribed to
		void operator()(const SVGSegmentParseState& cmdState) {
			const auto& commandTable = getCommandTable();
			if (cmdState.fSegmentKind < 128 && commandTable[cmdState.fSegmentKind]) {
				(this->*(commandTable[cmdState.fSegmentKind]))(cmdState.args, cmdState.iteration);
			}

			if (cmdState.fSegmentKind == 'M' || cmdState.fSegmentKind == 'm') {
				fLastMoveTo = fLastPoint;
			}

			//if (cmdState.fArgCount >= 2)
			//{
			//	fLastPoint = { cmdState.args[cmdState.fArgCount - 2], cmdState.args[cmdState.fArgCount - 1] };
			//}
		}



		// Command - A
		void arcTo(const double* args, int iteration) noexcept
		{
			bool larc = args[3] > 0.5f;
			bool swp = args[4] > 0.5f;
			double xrot = radians(args[2]);

			// need to convert to radians before sending along
			//logCommand("ellipticArcTo", args, 7);
			printf("apath.ellipticArcTo(BLPoint(%f, %f), %f, %d, %d, BLPoint(%f, %f));\n",
				args[0], args[1], xrot, larc, swp, args[5], args[6]);

			fLastPoint = { args[5], args[6] };
		}

		// Command - a
		void arcBy(const double* args, int iteration) noexcept
		{
			bool larc = args[3] > 0.5;
			bool swp = args[4] > 0.5;
			double xrot = radians(args[2]);
			PathPoint lastPos = relativePoint(fLastPoint, args, 5);

			printf("apath.ellipticArcTo(BLPoint(%f, %f), %f, %d, %d, BLPoint(%f, %f));\n",
				args[0], args[1], xrot, larc, swp, lastPos.x, lastPos.y);

			fLastPoint = lastPos;
		}


		// Command - C
		void cubicTo(const double* args, int iteration) noexcept
		{
			logCommand("cubicTo", args, 6);
			fLastPoint = { args[4], args[5] };
		}

		// Command - c
		void cubicBy(const double* args, int iteration) noexcept
		{
			PathPoint lastPos = relativePoint(fLastPoint, args, 4);

			printf("apath.cubicTo(%f, %f, %f, %f, %f, %f);\n", fLastPoint.x + args[0], fLastPoint.y + args[1], fLastPoint.x + args[2], fLastPoint.y + args[3], lastPos.x, lastPos.y);
			fLastPoint = lastPos;
		}


		// Command - H
		void hLineTo(const double* args, int iteration) noexcept
		{
			printf("apath.lineTo(%f, %f);\n", args[0], fLastPoint.y);
			fLastPoint = { args[0], fLastPoint.y };
		}

		// Command - h
		void hLineBy(const double* args, int iteration) noexcept
		{
			printf("apath.lineTo(%f, %f);\n", fLastPoint.x + args[0], fLastPoint.y);
			fLastPoint = { fLastPoint.x + args[0], fLastPoint.y };
		}

		// Command 'L' - LineTo
		void lineTo(const double* args, int iteration) noexcept
		{

			logCommand("lineTo", args, 2);
			//printf("apath.lineTo(%f, %f);\n", args[0], args[1]);
			fLastPoint = { args[0], args[1] };

		}

		// Command 'l' - LineBy
		void lineBy(const double* args, int iteration) noexcept
		{
			PathPoint lastPos = relativePoint(fLastPoint, args, 0);

			printf("apath.lineTo(%f, %f);\n", lastPos.x, lastPos.y);
			fLastPoint = lastPos;
		}

		// Command - M
		void moveTo(const double* args, int iteration) noexcept
		{
			if (iteration == 0) {
				fLastMoveTo = { args[0], args[1] };
				fLastPoint = fLastMoveTo;

				logCommand("moveTo", args, 2);
				//printf("apath.moveTo(%f, %f);\n", fLastMoveTo.x, fLastMoveTo.y);
			}
			else {
				lineTo(args, iteration);
			}

		}

		// Command - m
		void moveBy(const double* args, int iteration) noexcept
		{

			if (iteration == 0) {
				PathPoint lastPos = relativePoint(fLastPoint, args, 0);

				printf("apath.moveTo(%f, %f);\n", lastPos.x, lastPos.y);
				fLastMoveTo = lastPos;
				fLastPoint = lastPos;
			}
			else {
				lineBy(args, iteration);
			}
		}

		// Command - Q
		void quadTo(const double* args, int iteration) noexcept
		{
			logCommand("quadTo", args, 4);
			//printf("apath.quadTo(%f, %f, %f, %f);\n", args[0], args[1], args[2], args[3]);
			fLastPoint = { args[2], args[3] };
		}

		// Command - q
		void quadBy(const double* args, int iteration) noexcept
		{
			PathPoint lastPos = relativePoint(fLastPoint, args, 2);

			printf("apath.quadTo(%f, %f, %f, %f);\n", fLastPoint.x + args[0], fLastPoint.y + args[1], lastPos.x, lastPos.y);
			fLastPoint = lastPos;
		}

		// Command - S
		void smoothCubicTo(const double* args, int iteration) noexcept
		{
			printf("apath.smoothCubicTo(%f, %f, %f, %f);\n", args[0], args[1], args[2], args[3]);
			fLastPoint = { args[2], args[3] };
		}

		// Command - s
		void smoothCubicBy(const double* args, int iteration) noexcept
		{
			PathPoint lastPos = relativePoint(fLastPoint, args, 2);

			printf("apath.smoothCubicTo(%f, %f, %f, %f);\n", fLastPoint.x + args[0], fLastPoint.y + args[1], fLastPoint.x + args[2], fLastPoint.y + args[3]);
			fLastPoint = lastPos;
		}

		// Command - T
		void smoothQuadTo(const double* args, int iteration) noexcept
		{
			printf("apath.smoothQuadTo(%f, %f);\n", args[0], args[1]);
			fLastPoint = { args[0], args[1] };
		}

		// Command - t
		void smoothQuadBy(const double* args, int iteration) noexcept
		{
			PathPoint lastPos = relativePoint(fLastPoint, args, 0);

			printf("apath.smoothQuadTo(%f, %f);\n", fLastPoint.x + args[0], fLastPoint.y + args[1]);
			fLastPoint = lastPos;
		}


		// Command - V
		void vLineTo(const double* args, int iteration) noexcept
		{
			printf("apath.lineTo(%f, %f);\n", fLastPoint.x, args[0]);
			fLastPoint = { fLastPoint.x, args[0] };
		}

		// Command - v
		void vLineBy(const double* args, int iteration) noexcept
		{
			printf("apath.lineTo(%f, %f);\n", fLastPoint.x, fLastPoint.y + args[0]);
			fLastPoint = { fLastPoint.x, fLastPoint.y + args[0] };
		}


		// 
		// BUGBUG - there is a case where the Z is followed by 
		// a number, which is not a valid SVG path command
		// This number needs to be consumed somewhere
		// Command - Z
		//
		void close(const double* args, int iteration) noexcept
		{
			printf("apath.close();\n");
			fLastPoint = fLastMoveTo;
		}

	};
}

static void testPathPrinter()
{
	ByteSpan multiCircle("M 448.5,337 C 277.56787,337 139,475.56787 139,646.5 139,817.43213 277.56787,956 448.5,956 619.43213,956 758,817.43213 758,646.5 758,475.56787 619.43213,337 448.5,337 Z m 0,31 C 602.3113,368 727,492.6887 727,646.5 727,800.3113 602.3113,925 448.5,925 294.6887,925 170,800.3113 170,646.5 170,492.6887 294.6887,368 448.5,368 Z");
	ByteSpan doubleCircle("M105.75 517.27c-90.1-52-121-167.3-69-257.4s167.3-121 257.5-69c90.1 52 121 167.3 69 257.4-52.1 90.1-167.3 121-257.5 69zm239-79.7c46.2-80 18.8-182.2-61.2-228.3-80-46.2-182.2-18.8-228.3 61.2-46.2 79.9-18.8 182.2 61.2 228.3 79.9 46.2 182.1 18.8 228.3-61.2zm0 0c46.2-80 18.8-182.2-61.2-228.3-80-46.2-182.2-18.8-228.3 61.2-46.2 79.9-18.8 182.2 61.2 228.3 79.9 46.2 182.1 18.8 228.3-61.2z");
	ByteSpan wavy("M 10, 50Q 25, 25 40, 50t 30, 0 30, 0 30, 0 30, 0 30, 0");

	PathCommandDispatch dispatch;
	PathCmdPrinter printer;

	dispatch.subscribe(printer);

	dispatch.parse(multiCircle);
}


void testPathSegmenter()
{
	SVGPathSegmentIterator iter("M 448.5,337 C 277.56787,337 139,475.56787 139,646.5 139,817.43213 277.56787,956 448.5,956 619.43213,956 758,817.43213 758,646.5 758,475.56787 619.43213,337 448.5,337 Z m 0,31 C 602.3113,368 727,492.6887 727,646.5 727,800.3113 602.3113,925 448.5,925 294.6887,925 170,800.3113 170,646.5 170,492.6887 294.6887,368 448.5,368 Z");
    //SVGPathSegmentIterator iter("M 10, 50Q 25, 25 40, 50t 30, 0 30, 0 30, 0 30, 0 30, 0");
	//SVGPathSegmentIterator iter("M105.75 517.27c-90.1-52-121-167.3-69-257.4s167.3-121 257.5-69c90.1 52 121 167.3 69 257.4-52.1 90.1-167.3 121-257.5 69zm239-79.7c46.2-80 18.8-182.2-61.2-228.3-80-46.2-182.2-18.8-228.3 61.2-46.2 79.9-18.8 182.2 61.2 228.3 79.9 46.2 182.1 18.8 228.3-61.2zm0 0c46.2-80 18.8-182.2-61.2-228.3-80-46.2-182.2-18.8-228.3 61.2-46.2 79.9-18.8 182.2 61.2 228.3 79.9 46.2 182.1 18.8 228.3-61.2z");
    
	SVGSegmentParseState seg;
    while (iter.nextSegment(seg))
    {
        printf("%c ", seg.fSegmentKind);
        int n = strlen(seg.fArgTypes);
        switch (n) {
        case 7:
            printf("%3.2f ", seg.args[n - 7]);
        case 6:
            printf("%3.2f ", seg.args[n - 6]);
        case 5:
            printf("%3.2f ", seg.args[n - 5]);
        case 4:
            printf("%3.2f ", seg.args[n - 4]);
        case 3:
            printf("%3.2f ", seg.args[n - 3]);
        case 2:
            printf("%3.2f ", seg.args[n - 2]);
        case 1:
            printf("%3.2f ", seg.args[n - 1]);
        default:
            printf("\n");
        }
    }
}


int main(int argc, char** argv)
{
	//testPathSegmenter();
	testPathPrinter();

	return 0;
}
