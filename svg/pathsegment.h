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


#include "lang_scanner.h"
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
    static INLINE void svgPath_wsp_skip(ByteSpan& s) noexcept
    {
        bspan_ltrim(s, chrWspChars);
    }

    // svgPathSep_skip
    // Skip over any whitespace or commas in the path data
    static INLINE void svgPath_sep_skip(ByteSpan& s) noexcept
    {
        static constexpr charset pathSep = chrWspChars + ',';
        bspan_ltrim(s, pathSep);
    }


    static INLINE bool svgPath_number_read(ByteSpan& s, float& out) noexcept
    {
        svgPath_sep_skip(s);

        double value = 0;
        if (!number_read(s, value))
            return false;

        out = static_cast<float>(value);
        return true;
    }

    static INLINE bool svgPath_arc_flag_read(ByteSpan& s, float& out) noexcept
    {
        svgPath_sep_skip(s);

        if (s.empty())
            return false;

        if (*s != '0' && *s != '1')
            return false;

        out = float(*s - '0');
        ++s;
        return true;
    }

    static INLINE int svgPath_args_read(
        ByteSpan& s,
        const char* argTypes,
        float* outArgs) noexcept
    {
        int i = 0;

        for (; argTypes[i]; ++i)
        {
            switch (argTypes[i])
            {
            case 'c':
            case 'r':
            {
                if (!svgPath_number_read(s, outArgs[i]))
                    return i;
            } break;

            case 'f':
            {
                if (!svgPath_arc_flag_read(s, outArgs[i]))
                    return i;
            } break;

            default:
                return 0;
            }
        }

        return i;
    }

    static INLINE bool svgPath_CommandArgs_read(
        ByteSpan& s,
        SVGPathCommand cmd,
        float* outArgs,
        uint8_t& outArgCount) noexcept
    {
        const char* argTypes = getSegmentArgTypes(static_cast<uint8_t>(cmd));
        if (!argTypes)
            return false;

        outArgCount = uint8_t(strlen(argTypes));

        return outArgCount == svgPath_args_read(s, argTypes, outArgs);
    }

    static INLINE bool svgPath_isCommandChar(uint8_t ch) noexcept
    {
        return getSegmentArgTypes(ch) != nullptr;
    }

    static INLINE bool svgPath_isNumberStart(uint8_t ch) noexcept
    {
        return is_digit(ch) || ch == '+' || ch == '-' || ch == '.';
    }

    static bool svgPathSegment_read(SVGSegmentIterator& iter, PathSegment& seg) noexcept
    {
        SVGSegmentParseState& st = iter.fState;

        // A segment may be preceded by whitespace, but not by a free-standing comma.
        svgPath_wsp_skip(st.remains);

        if (st.remains.empty())
            return false;

        const uint8_t ch = *st.remains;

        if (svgPath_isCommandChar(ch))
        {
            st.fSegmentKind = static_cast<SVGPathCommand>(ch);
            st.fArgTypes = getSegmentArgTypes(ch);
            st.fArgCount = st.fArgTypes ? strlen(st.fArgTypes) : 0;
            st.fIteration = 0;

            ++st.remains;

            // After an explicit command, path argument parsing may consume comma/wsp.
            svgPath_sep_skip(st.remains);
        }
        else if (svgPath_isNumberStart(ch))
        {
            // Repeated command: same command, next argument tuple.
            if (st.fArgTypes == nullptr || st.fArgCount == 0)
            {
                st.fError = -1;
                return false;
            }

            ++st.fIteration;
        }
        else
        {
            st.fError = -1;
            return false;
        }

        if (st.fArgCount == 0)
        {
            seg.reset(nullptr, 0, nullptr, st.fSegmentKind, static_cast<uint8_t>(st.fIteration));
            return true;
        }

        float args[kMaxPathArgs]{ 0.0f };
        uint8_t argCount = 0;

        if (!svgPath_CommandArgs_read(st.remains, st.fSegmentKind, args, argCount))
        {
            st.fError = -1;
            return false;
        }

        if (argCount != st.fArgCount)
        {
            st.fError = -1;
            return false;
        }

        seg.reset(
            args,
            argCount,
            st.fArgTypes,
            st.fSegmentKind,
            static_cast<uint8_t>(st.fIteration));

        return true;
    }

    //
    // readNextSegmentCommand
    // Given a current state of parsing, read the next segment command within
    // an SVG path.  The state is updated with the new command, and the numeric
    // arguments to go with it.
    //
    /*
    static bool readNextSegmentCommand(SVGSegmentIterator& iter, PathSegment& seg)
    {
        constexpr charset leadingChars("0123456789.+-");          // digits, symbols, and letters found at start of numbers
        constexpr  charset pathWsp = chrWspChars + ',';
        
        // always ignore leading whitespace
        bspan_ltrim(iter.fState.remains, pathWsp);

        if (iter.fState.remains.empty())
            return false;
        

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
    */
}

