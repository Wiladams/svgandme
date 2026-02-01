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
        SVGPathCommand fSegmentKind{ SVGPathCommand::M };
		size_t fIteration{ 0 };
		size_t fArgCount{ 0 };
		const char* fArgTypes{ nullptr };
		ByteSpan remains{};
		int fError{ 0 };

		SVGSegmentParseState() = default;

		SVGSegmentParseState(const ByteSpan& aSpan)
		{
			remains = aSpan;
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


namespace waavs 
{	
	//
	// readNextSegmentCommand
	// Given a current state of parsing, read the next segment command within
	// an SVG path.  The state is updated with the new command, and the numeric
	// arguments to go with it.
	//
	static bool readNextSegmentCommand(SVGSegmentIterator& iter, PathSegment& seg)
	{
		constexpr charset leadingChars("0123456789.+-");          // digits, symbols, and letters found at start of numbers
		constexpr  charset pathWsp = chrWspChars + ',';
		
		// always ignore leading whitespace
		iter.fState.remains.skipWhile(pathWsp);

		if (iter.fState.remains.empty())
			return false;
		
		// put in a progress guard to ensure we don't
		// cause an infinite loop
		//const unsigned char* before = iter.fState.remains.fStart;

		// if the next character is not numeric, then 
		// it must be a new command
		if (!leadingChars(*iter.fState.remains))
		{
			// If we're in here, there must be a command
			// if there isn't it's an error
			iter.fState.fArgTypes = getSegmentArgTypes(*iter.fState.remains);

			if (!iter.fState.fArgTypes) {
				iter.fState.fError = -1;	// Indicate parsing error
				return false;
			}

			iter.fState.fArgCount = strlen(iter.fState.fArgTypes);
			iter.fState.fSegmentKind = static_cast<SVGPathCommand>(*iter.fState.remains);
			iter.fState.fIteration = 0;	// New command, so reset iteration to zero

			iter.fState.remains++;
		}
		else {
            // Assume we've got some more arguments for the last command
			// so it's a repeated command
			if (iter.fState.fArgCount == 0)
			{
				// Strict: error out
				iter.fState.fError = -1;
				return false;
			}

			// If we are here, the next token is numeric
			// so, we assume we're in the next iteration of the same
			// command, so increment the iteration count
			iter.fState.fIteration++;
		}

		// Now, we need to read the numeric arguments
		if (iter.fState.fArgCount > 0)
		{
            float args[kMaxPathArgs]{ 0.0f };
			if (iter.fState.fArgCount != readFloatArguments(iter.fState.remains, iter.fState.fArgTypes, args))
			{
				iter.fState.fError = -1;	// Indicate parsing error
				return false;
			}
            seg.reset(args, iter.fState.fArgCount, iter.fState.fArgTypes, iter.fState.fSegmentKind, iter.fState.fIteration);
		} else {
			// no arguments
			seg.reset(nullptr, 0, nullptr, iter.fState.fSegmentKind, iter.fState.fIteration);
        }

		return true;
	}
}

