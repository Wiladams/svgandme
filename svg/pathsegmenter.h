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
#include "pubsub.h"


namespace waavs {
	// Used for the iterator
	// Any  parameters we want to pass along to the
	// routines doing the segmentation
	// nothing interesting in here at the moment
	struct SVGSegmentParseParams {
		bool fFlattenCommands{ true };
	};


	struct SVGSegmentParseState {
		ByteSpan remains{};
		unsigned char fSegmentKind{ 0 };
		int iteration{ 0 };
		int fError{ 0 };
		const char* fArgTypes{};
		int fArgCount{ 0 };
		double args[8]{ 0 };		// The arguments for the command

		SVGSegmentParseState() = default;

		SVGSegmentParseState(const ByteSpan& aSpan)
		{
			remains = aSpan;
		}

		bool hasMore() const {
			return remains.size() > 0;
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
		static charset leadingChars("0123456789.+-");          // digits, symbols, and letters found at start of numbers
		static  charset pathWsp = chrWspChars + ',';
		
		//bool success = false;

		// always ignore leading whitespace
		cmdState.remains = chunk_ltrim(cmdState.remains, pathWsp);

		if (!cmdState.remains)
			return false;

		if (!leadingChars(*cmdState.remains)) {
			// If we're in here, there must be a command
			// if there isn't it's an error
			const char* argTypes = getSegmentArgTypes(*cmdState.remains);
			if (argTypes != nullptr)
			{
				cmdState.fSegmentKind = *cmdState.remains;
				cmdState.iteration = 0;
				cmdState.remains++;
				cmdState.fArgTypes = argTypes;
				cmdState.fArgCount = strlen(argTypes);

			}
			else
				return false;
		}

		if ((cmdState.fArgTypes != nullptr) && (cmdState.fArgCount > 0))
		{
			if (cmdState.fArgCount != readNumericArguments(cmdState.remains, cmdState.fArgTypes, cmdState.args))
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
}


namespace waavs {
	// SVGPathSegmentIterator
	// A convenience class.  Given a span that contains a path to 
	// be parsed, the iterator will setup the param, and initial state
	// then provide a 'nextSegment()', which will return 'true' as long
	// as there are valid segments to be returned.  False, upon error
	// or end of path. 

	struct SVGPathSegmentIterator
	{
	private:
		SVGSegmentParseParams fParams{};
		SVGSegmentParseState fCmdState;


	public:
		SVGPathSegmentIterator(const ByteSpan& pathSpan)
			: fCmdState(pathSpan)
		{}

		bool nextSegment(SVGSegmentParseState& cmdState)
		{
			auto success = readNextSegmentCommand(fParams, fCmdState);
			if (!success)
				return false;

			cmdState = fCmdState;

			return true;
		}
	};
}
