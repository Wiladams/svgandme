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



namespace waavs {
	// Used for the iterator
	// Any  parameters we want to pass along to the
	// routines doing the segmentation
	// nothing interesting in here at the moment
	struct SVGSegmentParseParams {
		bool fFlattenCommands{ true };
	};

	struct SVGSegmentParseState
	{
		PathSegment seg;
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

		// Does the command use relative coordinates?
		bool isRelative() const {
			return (static_cast<unsigned char>(seg.fSegmentKind) >= 'a' && static_cast<unsigned char>(seg.fSegmentKind) <= 'z');
		}

		// Does the command use absolute coordinates?
		bool isAbsolute() const {
			return (static_cast<unsigned char>(seg.fSegmentKind) >= 'A' && static_cast<unsigned char>(seg.fSegmentKind) <= 'Z');
		}

		bool hasMore() const {
			return !remains.empty();
		}
	};
}


namespace waavs {
	
	/// <summary>
	/// Return the type of arguments that are associated with a given segment command
	/// </summary>
	/// <param name="cmdIndex"></param>
	/// <returns>a null terminated string of the argument types, or nullptr on invalid command</returns>
	/// c - number
	/// f - flag
	/// r - radius
	///

	static const char* getSegmentArgTypes(unsigned char cmdIndex) noexcept {
		static std::array<const char*, 128> lookupTable = [] {
			std::array<const char*, 128> table{}; // Default initializes all to nullptr
			table['A'] = table['a'] = "ccrffcc";  // ArcTo
			table['C'] = table['c'] = "cccccc";   // CubicTo
			table['H'] = table['h'] = "c";        // HLineTo
			table['L'] = table['l'] = "cc";       // LineTo
			table['M'] = table['m'] = "cc";       // MoveTo
			table['Q'] = table['q'] = "cccc";     // QuadTo
			table['S'] = table['s'] = "cccc";     // SmoothCubicTo
			table['T'] = table['t'] = "cc";       // SmoothQuadTo
			table['V'] = table['v'] = "c";        // VLineTo
			table['Z'] = table['z'] = "";         // Close
			return table;
			}();

		return cmdIndex < 128 ? lookupTable[cmdIndex] : nullptr;
	}

	//
	// readNextSegmentCommand
	// Given a current state of parsing, read the next segment command within
	// an SVG path.  The state is updated with the new command, and the numeric
	// arguments to go with it.
	//
	static bool readNextSegmentCommand(SVGSegmentParseParams& params, SVGSegmentParseState& cmdState)
	{
		constexpr charset leadingChars("0123456789.+-");          // digits, symbols, and letters found at start of numbers
		constexpr  charset pathWsp = chrWspChars + ',';
		
		// always ignore leading whitespace
		cmdState.remains.skipWhile(pathWsp);

		if (cmdState.remains.empty())
			return false;

		// if the next character is not numeric, then 
		// it must be a command
		if (!leadingChars(*cmdState.remains)) {
			// If we're in here, there must be a command
			// if there isn't it's an error
			const char* argTypes = getSegmentArgTypes(*cmdState.remains);
			if (argTypes != nullptr)
			{
				// start with iteration == 0 to indicate this is
				// the first instance of the segment
				//cmdState.fSegmentKind = *cmdState.remains;
				//cmdState.iteration = 0;
				//cmdState.fArgTypes = argTypes;
				//cmdState.fArgCount = strlen(argTypes);
				cmdState.seg.reset(nullptr, strlen(argTypes), argTypes, 
					static_cast<SVGPathCommand>(*cmdState.remains),0);
				cmdState.remains++;

			}
			else {
				// Invalid command
				cmdState.fError = -1;	// Indicate parsing error
				return false;
			}
		}
		else {
			// If we are here, the next token is numeric
			// so, we assume we're in the next iteration of the same
			// command, so increment the iteration count
			cmdState.seg.fIteration++;
		}

		// Now, we need to read the numeric arguments
		//if ((cmdState.fArgTypes != nullptr) && (cmdState.seg.fArgCount > 0))
		if (cmdState.seg.fArgCount > 0)
		{
			if (cmdState.seg.fArgCount != readFloatArguments(cmdState.remains, cmdState.seg.fArgTypes, cmdState.seg.fArgs))
			{
				cmdState.fError = -1;	// Indicate parsing error
				return false;
			}

			return true;
		}

		return true;
	}
}


namespace waavs {
	struct SVGPathSegmentGenerator : public IProduce<PathSegment>
	{
		SVGSegmentParseParams fParams{};
		SVGSegmentParseState fCmdState;

		SVGPathSegmentGenerator(const ByteSpan& pathSpan)
			: fCmdState(pathSpan)
		{
		}

		bool next(PathSegment& seg) override
		{
			auto success = readNextSegmentCommand(fParams, fCmdState);
			if (!success)
				return false;
			seg = fCmdState.seg;

			return true;
		}

	};
}

