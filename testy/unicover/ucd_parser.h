// ucd_parser.h

#pragma once

#include <cstdint>

#include "lang_grammar.h"
#include "unicode_coverage_data.h"


namespace waavs
{
    // ========================================================================
    // UCDCodePointRange
    //
    // Inclusive Unicode code-point range.
    //
    // Represents both:
    //
    //      0041
    //
    // and:
    //
    //      0041..005A
    //
    // A single code point is represented by first == last.
    //
    // ========================================================================

    struct UCDCodePointRange
    {
        uint32_t first{ 0 };
        uint32_t last{ 0 };
    };


    // ========================================================================
    // UCDLine
    //
    // One meaningful data line from a Unicode Character Database text file.
    //
    // Blank lines and comment-only lines are skipped by UCDParser::next().
    //
    // For:
    //
    //      0000..007F; Basic Latin # ASCII
    //
    // data:
    //
    //      0000..007F; Basic Latin
    //
    // comment:
    //
    //      ASCII
    //
    // All spans refer directly into the original source buffer.
    //
    // ========================================================================

    struct UCDLine
    {
        ByteSpan data{};
        ByteSpan comment{};

        uint32_t lineNumber{ 0 };
    };


    // ========================================================================
    // ucdReadField
    //
    // Read one semicolon-separated field.
    //
    // The source span is advanced past the semicolon.
    //
    // If there is no semicolon, the remainder of the span becomes the final
    // field and source becomes empty.
    //
    // Leading and trailing whitespace are removed.
    //
    // ========================================================================

    static inline bool ucdReadField(ByteSpan& source, ByteSpan& outField) noexcept
    {
        outField.reset();

        bspan_trim_spaces(source);

        if (!source)
            return false;


        outField =
            bspan_read_until(source, ';');


        bspan_trim_spaces(outField);


        return true;
    }


    // ========================================================================
    // ucdParseCodePoint
    //
    // Parse one hexadecimal Unicode code point.
    //
    // Examples:
    //
    //      0041
    //      10FFFF
    //
    // ========================================================================

    static inline bool ucdParseCodePoint(const ByteSpan& source, uint32_t& outCodePoint) noexcept
    {
        ByteSpan value =
            source;


        bspan_trim_spaces(value);


        if (!value)
            return false;


        uint64_t parsed = 0;


        if (!parseHex64u(value, parsed))
            return false;


        if (parsed >= kUnicodeLimit)
            return false;


        outCodePoint =
            static_cast<uint32_t>(parsed);


        return true;
    }


    // ========================================================================
    // ucdParseCodePointRange
    //
    // Parse either:
    //
    //      XXXX
    //
    // or:
    //
    //      XXXX..YYYY
    //
    // Examples:
    //
    //      0041
    //      0041..005A
    //      1F300..1F5FF
    //
    // The range is inclusive.
    //
    // ========================================================================

    static inline bool ucdParseCodePointRange(const ByteSpan& source, UCDCodePointRange& outRange) noexcept
    {
        ByteSpan value = source;

        bspan_trim_spaces(value);

        if (!value)
            return false;

        // ---------------------------------------------------------------
        // Locate the ".." range separator.
        // ---------------------------------------------------------------

        const uint8_t* rangeSeparator = nullptr;
        const uint8_t* p = value.begin();
        const uint8_t* end = value.end();

        while (p + 1 < end)
        {
            if (p[0] == '.' &&
                p[1] == '.')
            {
                rangeSeparator = p;
                break;
            }

            ++p;
        }


        // ---------------------------------------------------------------
        // Single code point.
        // ---------------------------------------------------------------

        if (!rangeSeparator)
        {
            uint32_t cp = 0;


            if (!ucdParseCodePoint(value, cp))
                return false;


            outRange.first =
                cp;

            outRange.last =
                cp;


            return true;
        }


        // ---------------------------------------------------------------
        // Inclusive range.
        // ---------------------------------------------------------------

        ByteSpan firstText = ByteSpan::fromPointers(value.begin(), rangeSeparator);
        ByteSpan lastText = ByteSpan::fromPointers(rangeSeparator + 2, value.end());

        bspan_trim_spaces(firstText);
        bspan_trim_spaces(lastText);

        if (!firstText ||
            !lastText)
        {
            return false;
        }

        uint32_t first = 0;
        uint32_t last = 0;


        if (!ucdParseCodePoint(firstText, first))
            return false;

        if (!ucdParseCodePoint(lastText, last))
            return false;

        if (first > last)
            return false;

        outRange.first = first;
        outRange.last = last;

        return true;
    }


    // ========================================================================
    // ucdReadCodePointRange
    //
    // Convenience operation:
    //
    //      read next semicolon-separated field
    //      parse it as a Unicode code-point range
    //
    // ========================================================================

    static inline bool ucdReadCodePointRange(ByteSpan& source, UCDCodePointRange& outRange) noexcept
    {
        ByteSpan field;

        if (!ucdReadField(source, field))
            return false;

        return ucdParseCodePointRange(field, outRange);
    }


    // ========================================================================
    // UCDParser
    //
    // Generic line reader for Unicode Character Database text files.
    //
    // The parser:
    //
    //      - reads LF and CR/LF files
    //      - counts physical source lines
    //      - strips comments beginning with '#'
    //      - trims surrounding whitespace
    //      - skips blank lines
    //      - skips comment-only lines
    //
    // It deliberately does NOT interpret semicolon fields.
    //
    // File-specific parsers do that using ucdReadField() and the other
    // helpers above.
    //
    // ========================================================================

    class UCDParser
    {
    public:
        UCDParser() noexcept = default;


        explicit UCDParser(const ByteSpan& source) noexcept
            : mSource(source)
        {
        }


        // ====================================================================
        // reset
        // ====================================================================

        void reset(const ByteSpan& source) noexcept
        {
            mSource = source;
            mLineNumber = 0;
        }


        // ====================================================================
        // State
        // ====================================================================

        [[nodiscard]]
        bool empty() const noexcept
        {
            return !mSource;
        }


        [[nodiscard]]
        uint32_t lineNumber() const noexcept
        {
            return mLineNumber;
        }


        [[nodiscard]]
        const ByteSpan& remaining() const noexcept
        {
            return mSource;
        }


        // ====================================================================
        // next
        //
        // Return the next meaningful UCD data line.
        //
        // Blank and comment-only lines are consumed but never returned.
        //
        // Returns false only when there are no more data lines.
        //
        // ====================================================================

        bool next(UCDLine& outLine) noexcept
        {
            outLine = {};

            while (mSource)
            {
                // -----------------------------------------------------------
                // Read one physical line.
                //
                // bspan_read_until() also handles the final line when the
                // source does not end in '\n'.
                // -----------------------------------------------------------

                ByteSpan line = bspan_read_until(mSource, '\n');

                ++mLineNumber;

                // -----------------------------------------------------------
                // Split comment from data.
                //
                // UCD text formats do not use '#' quoting semantics, so the
                // first '#' always begins the comment portion.
                // -----------------------------------------------------------

                ByteSpan remainder = line;
                ByteSpan data = bspan_read_until(remainder, '#');
                ByteSpan comment = remainder;

                bspan_trim_spaces(data);
                bspan_trim_spaces(comment);


                // -----------------------------------------------------------
                // Empty line or comment-only line.
                // -----------------------------------------------------------

                if (!data)
                    continue;


                // -----------------------------------------------------------
                // Return meaningful record.
                // -----------------------------------------------------------

                outLine.data = data;
                outLine.comment = comment;
                outLine.lineNumber = mLineNumber;

                return true;
            }

            return false;
        }


    private:
        ByteSpan mSource{};
        uint32_t mLineNumber{ 0 };
    };

} // namespace waavs
