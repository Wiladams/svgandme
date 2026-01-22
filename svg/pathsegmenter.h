#pragma once

//
// pathsegmenter.h 
// The SVGPath element contains a fairly complex set of commands in the 'd' attribute
// The commands can define a composite shape, made up of several figures.
// Each of those figures is defined by a set of segments, beginning with a 'moveto'
// and ending with a close, or another moveto.
// 
// The code in this file provides a segment iterator.  Once setup, you can call:
//	readNextSegmentCommand()
// Several times, until it fails.  What you get on each call is a segment, which 
// is the name of the segment command, and the several arguments to that command.
//
//
// Usage: - using a convenience iterator
// SVGPathSegmentIterator iter("M 10, 50Q 25, 25 40, 50t 30, 0 30, 0 30, 0 30, 0 30, 0");
//
// SVGSegmentParseState seg;
// while (iter.nextSegment(seg))
// {
//	printf("CMD: %c\n", seg.fSegmentKind);
// }
//
// Noted:
// Tools: 
// https://svg-path-visualizer.netlify.app/
// References:
// https://svgwg.org/svg2-draft/paths.html#PathDataBNF
//


#include <array>

#include "bspan.h"
#include "converters.h"
#include "waavsgraph.h"
#include "pipeline.h"



namespace waavs 
{
	// Used for the iterator
	// Any  parameters we want to pass along to the
	// routines doing the segmentation
	// nothing interesting in here at the moment
	struct SVGSegmentParseParams {
		bool fFlattenCommands{ true };
	};

	struct SVGSegmentParseState
	{
        PathSegment seg;	// we need segment in here because we retain last command and iteration for implicit commands
							// maybe we could just store the command and iteration count separately?
		ByteSpan remains{};
		int fError{ 0 };

		SVGSegmentParseState() = default;

		SVGSegmentParseState(const ByteSpan& aSpan)
		{
			remains = aSpan;
		}

		unsigned char command() const {
			return static_cast<unsigned char>(seg.fSegmentKind);
		}

		bool hasMore() const {
			return !remains.empty();
		}
	};

	struct SVGSegmentIterator {
        SVGSegmentParseParams fParams{};
		SVGSegmentParseState fState{};

		SVGSegmentIterator(const ByteSpan& pathSpan)
			: fState(pathSpan)
		{
        }
	 };
}


namespace waavs {
	


	//
	// readNextSegmentCommand
	// Given a current state of parsing, read the next segment command within
	// an SVG path.  The state is updated with the new command, and the numeric
	// arguments to go with it.
	//
	static bool readNextSegmentCommand(SVGSegmentParseParams& params, SVGSegmentParseState& cmdState) //, PathSegment &seg)
	{
		constexpr charset leadingChars("0123456789.+-");          // digits, symbols, and letters found at start of numbers
		constexpr  charset pathWsp = chrWspChars + ',';
		
		// always ignore leading whitespace
		cmdState.remains.skipWhile(pathWsp);

		if (cmdState.remains.empty())
			return false;
		
		// put in a progress guard to ensure we don't
		// cause an infinite loop
		//const unsigned char* before = cmdState.remains.fStart;

		// if the next character is not numeric, then 
		// it must be a command
		if (!leadingChars(*cmdState.remains)) 
		{
			// If we're in here, there must be a command
			// if there isn't it's an error
			const char* argTypes = getSegmentArgTypes(*cmdState.remains);
		
			if (!argTypes) {
                cmdState.fError = -1;	// Indicate parsing error
				return false;
			}

			cmdState.seg.reset(nullptr, strlen(argTypes), argTypes, static_cast<SVGPathCommand>(*cmdState.remains), 0);
			cmdState.remains++;
		}
		else {
			// Do implicit repetition for those commands that expect
			// to have arguments
			if (cmdState.seg.fArgCount == 0)
			{
				// Strict: error out
				cmdState.fError = -1;
				return false;
			}

			// If we are here, the next token is numeric
			// so, we assume we're in the next iteration of the same
			// command, so increment the iteration count
			cmdState.seg.fIteration++;
		}

		// Now, we need to read the numeric arguments
		if (cmdState.seg.fArgCount > 0)
		{
			if (cmdState.seg.fArgCount != readFloatArguments(cmdState.remains, cmdState.seg.fArgTypes, cmdState.seg.fArgs))
			{
				cmdState.fError = -1;	// Indicate parsing error
				return false;
			}

		}

		return true;
	}
}


namespace waavs {
	struct SVGPathSegmentGenerator : public IProduce<PathSegment>
	{
		//SVGSegmentParseParams fParams{};
		//SVGSegmentParseState fCmdState;
        SVGSegmentIterator fIter;

		SVGPathSegmentGenerator(const ByteSpan& pathSpan)
			: fIter(pathSpan)
		{
		}

		bool next(PathSegment& seg) override
		{
			auto success = readNextSegmentCommand(fIter.fParams, fIter.fState);
			if (!success)
			{
				return false;
			}

			seg = fIter.fState.seg;

			return true;
		}

	};
}

