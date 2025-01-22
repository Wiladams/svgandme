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
// A convenient visualtion tool can be found here: https://svg-path-visualizer.netlify.app/
// References:
// https://svgwg.org/svg2-draft/paths.html#PathDataBNF
//

#include "bspan.h"


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
		double args[8];		// The arguments for the command

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
	///
	static const char * getSegmentArgTypes(unsigned char cmdIndex)
	{
		switch (cmdIndex) {
			case 'A':
			case 'a':
				return "ccrffcc";			// ArcTo
			case 'C':
			case 'c':
				return "cccccc";			// CubicTo
			case 'H':
			case 'h':
				return "c";				// HLineTo
			case 'L':
			case 'l':
				return "cc";				// LineTo
			case 'M':
			case 'm':
				return "cc";				// MoveTo
			case 'Q':
			case 'q':
				return "cccc";				// QuadTo
			case 'S':
			case 's':
				return "cccc";				// SmoothCubicTo
			case 'T':
			case 't':
				return "cc";				// SmoothQuadTo
			case 'V':
			case 'v':
				return "c";				// VLineTo
			case 'Z':
			case 'z':
				return "";					// Close

			default:
				return nullptr;
		}

		return nullptr;
	}


	static bool readNextSegmentCommand(SVGSegmentParseParams& params, SVGSegmentParseState& cmdState)
	{
		static charset leadingChars("0123456789.+-");          // digits, symbols, and letters found at start of numbers
		static  charset pathWsp = chrWspChars + ',';
		
		bool success = false;

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
			}
			else
				return false;
		}

		if (cmdState.fArgTypes != nullptr)
		{
			if (!readNumericArguments(cmdState.remains, cmdState.fArgTypes, cmdState.args))
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
