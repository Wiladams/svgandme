#pragma once

//
// pathsegment.h 
// 
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
// SVGSegmentIterator iter("M 10, 50Q 25, 25 40, 50t 30, 0 30, 0 30, 0 30, 0 30, 0");
//
// PathSegment seg;
// while (readNextSegmentCommand(iter, seg))
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




namespace waavs
{
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



    // SVGPathCommand
    // Represents the individual commands in an SVG path
    enum class SVGPathCommand : uint8_t
    {
        // Move to
        M = 'M',  // absolute moveto
        m = 'm',  // relative moveto

        // Line to
        L = 'L',  // absolute lineto
        l = 'l',  // relative lineto
        H = 'H',  // absolute horizontal lineto
        h = 'h',  // relative horizontal lineto
        V = 'V',  // absolute vertical lineto
        v = 'v',  // relative vertical lineto

        // Cubic Bezier
        C = 'C',  // absolute cubic Bezier
        c = 'c',  // relative cubic Bezier
        S = 'S',  // absolute smooth cubic Bezier
        s = 's',  // relative smooth cubic Bezier

        // Quadratic Bezier
        Q = 'Q',  // absolute quadratic Bezier
        q = 'q',  // relative quadratic Bezier
        T = 'T',  // absolute smooth quadratic Bezier
        t = 't',  // relative smooth quadratic Bezier

        // Elliptical arc
        A = 'A',  // absolute arc
        a = 'a',  // relative arc

        // Close path
        Z = 'Z',  // absolute closepath
        z = 'z'   // relative closepath (treated the same as Z in most renderers)
    };


    // struct PathSegment
    // 
    // This structure represents a single segment of an SVG path.
    // When parsing an SVG path, you get a series of segments,
    // Each segment has a command, and a set of arguments.
    //
    // Note:  By using the iteration field, we can do a rudimentary
    // run length encoding.  This would work well for relative path segments,
    // like a series of small line segments describing a circle.
    // 
    // BUGBUG - maybe need packing pragma to ensure tight packing
    static constexpr size_t kMaxPathArgs = 8;

    struct PathSegment final
    {

        float fArgs[kMaxPathArgs]{ 0.0 };   // 32 bytes
        char fArgTypes[kMaxPathArgs]{ 0 };  // 8 bytes
        uint8_t fArgCount{ 0 };             // 1 byte
        SVGPathCommand fSegmentKind;        // 1 byte
        uint16_t fIteration{ 0 };           // 2 byte
        uint32_t _reserved{ 0 };            // 4 bytes (rounds to 48 total)


        // Efficiently reset the contents of the struct

        void reset(const float* args, uint8_t argcount, const char* argtypes, SVGPathCommand kind, uint8_t iteration)
        {
            // we can only use memset as long as there's no virtual
            // function implemented.
            std::memset(this, 0, sizeof(PathSegment));
            fArgCount = argcount;
            fSegmentKind = kind;
            fIteration = iteration;

            // limit the number of args to copy
            size_t maxArgsToCopy = std::min(static_cast<size_t>(argcount), kMaxPathArgs);

            if (args != nullptr)
                std::copy_n(args, maxArgsToCopy, fArgs);
            if (argtypes != nullptr)
                std::copy_n(argtypes, maxArgsToCopy, fArgTypes);
        }

        const float* args() const { return fArgs; }
        void setArgs(const float* args, uint8_t argcount)
        {
            // limit the number of args to copy
            size_t maxArgsToCopy = std::min(static_cast<size_t>(argcount), kMaxPathArgs);

            if (args != nullptr)
            {
                std::copy_n(args, maxArgsToCopy, fArgs);
                fArgCount = maxArgsToCopy;
            }
            else {
                std::fill_n(fArgs, maxArgsToCopy, 0.0f);
                fArgCount = 0;
            }
        }

        constexpr uint16_t iteration() const { return fIteration; }
        constexpr SVGPathCommand command() const { return fSegmentKind; }

        bool isRelative() const {
            return (static_cast<unsigned char>(fSegmentKind) >= 'a' && static_cast<unsigned char>(fSegmentKind) <= 'z');
        }

        // Does the command use absolute coordinates?
        bool isAbsolute() const {
            return (static_cast<unsigned char>(fSegmentKind) >= 'A' && static_cast<unsigned char>(fSegmentKind) <= 'Z');
        }
    };

    ASSERT_MEMCPY_SAFE(PathSegment);
    ASSERT_STRUCT_SIZE(PathSegment, 48);
}


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

