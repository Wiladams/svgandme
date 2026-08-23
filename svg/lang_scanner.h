#pragma once

#include "lang_grammar.h"
#include "maths_transcend.h"

namespace waavs
{


    // Read an unsigned integer value from the ByteSpan
    // use the dual constraints of the limits of the ByteSpan
    // as well as the maximum number of digits to read
    // There MUST be the required number of digits available
    // or return false.
    // read_required_digits("234", &value, 2);
    // Will return '23' as a value
    //
    static INLINE bool fixed_digits_read(ByteSpan& s, uint64_t& v, size_t requiredDigits) noexcept
    {
        if (s.size() < requiredDigits)
            return false;

        v = 0;

        for (size_t i = 0; i < requiredDigits; ++i)
        {
            if (!is_digit(*s))
                return false;

            v = (v * 10) + uint64_t(*s - '0');
            ++s;
        }

        return true;
    }


    // u64_read
    //
    // Read a 64-bit unsigned integer from the input span
    // advance the span 
    static INLINE bool u64_read(ByteSpan& s, uint64_t& v, size_t& digitsRead) noexcept
    {
        digitsRead = 0;

        if (s.empty() || !is_digit(*s))
            return false;

        v = 0;

        while (s && is_digit(*s))
        {
            v = (v * 10) + uint64_t(*s - '0');
            ++s;
            ++digitsRead;
        }

        return true;
    }

    static INLINE bool integer_read(ByteSpan& s, int64_t& v) noexcept
    {
        if (s.empty())
            return false;

        int sign = 1;

        if (*s == '+' || *s == '-')
        {
            sign = (*s == '-') ? -1 : 1;
            ++s;

            if (s.empty())
                return false;
        }

        uint64_t uvalue = 0;
        size_t digitsRead = 0;

        if (!u64_read(s, uvalue, digitsRead))
            return false;

        v = (sign < 0) ? -int64_t(uvalue) : int64_t(uvalue);

        return true;
    }

    // 
    // number_read()
    //
    // Parse a number from the given ByteSpan, advancing the start
    // of the ByteSpan to beyond where we found the last character 
    // of the number.
    // 
    // Note: These numbers support a full mantissa, so the extended 'e' notation
    // is supported.  When reading numbers from a style sheet, readCSSNumber should
    // be used.
    // 
    // Construction:
    // number ::= integer ([Ee] integer)?
    //    | [+-] ? [0 - 9] * "."[0 - 9] + ([Ee] integer) ?

    // Assumption:  We're sitting at beginning of a number, all whitespace handling
    // has already occured.
    
    // Implementation note:
    // Just put this from_chars implementation here in case
    // we ever want to use it instead
    //double outNumber = 0;
    //auto res = std::from_chars((const char*)s.fStart, (const char*)s.fEnd, outNumber);
    //if (res.ec == std::errc::invalid_argument)
    //{
    //	printf("chunk_to_double: INVALID ARGUMENT: ");
    //	printChunk(s);
    //}
    //return outNumber;

    static INLINE bool number_read(ByteSpan& s, double& value) noexcept
    {
        if (!s)
            return false;

        bool isNegative = false;

        if (*s == '+' || *s == '-')
        {
            isNegative = (*s == '-');
            ++s;

            if (!s)
                return false;
        }

        uint64_t intPart = 0;
        bool hasDigits = false;

        // Integer part
        while (s && is_digit(*s))
        {
            intPart = (intPart * 10.0) + uint64_t(*s - '0');
            ++s;
            hasDigits = true;
        }

        // early return if it's a pure integer 
        // with no fractional or exponent part
        if (!s || (*s != '.' && *s != 'e' && *s != 'E'))
        {
            value = isNegative ? -double(intPart) : double(intPart);
            return hasDigits;
        }

        double result = double(intPart);

        // Fractional part
        if (s && *s == '.')
        {
            ++s;

            double scale = 0.1;

            while (s && is_digit(*s))
            {
                result += double(*s - '0') * scale;
                scale *= 0.1;
                ++s;
                hasDigits = true;
            }
        }

        if (!hasDigits)
            return false;

        // Optional exponent.
        // If no valid exponent digits follow, leave e/E unconsumed.
        // This preserves cases like "1em" / "1ex".
        if (s && (*s == 'e' || *s == 'E'))
        {
            ByteSpan exp = s;
            ++exp;

            bool expNegative = false;

            if (exp && (*exp == '+' || *exp == '-'))
            {
                expNegative = (*exp == '-');
                ++exp;
            }

            int expPart = 0;
            bool hasExpDigits = false;

            while (exp && is_digit(*exp))
            {
                if (expPart < 308)
                    expPart = (expPart * 10) + int(*exp - '0');

                ++exp;
                hasExpDigits = true;
            }

            if (hasExpDigits)
            {
                result *= POW(10.0, expNegative ? -double(expPart) : double(expPart));
                s = exp;
            }
        }

        value = isNegative ? -result : result;
        return true;
    }



    static INLINE bool number_list_read_next(ByteSpan& s, double& outNumber) noexcept
    {
        // typical whitespace found in lists of numbers, 
        // like on paths and polylines
        static const charset nextNumWsp = chrWspChars + ","; // (",+\t\n\r ");

        bspan_ltrim(s, nextNumWsp);

        if (s.empty())
            return false;

        return number_read(s, outNumber);
    }

}

namespace waavs
{


    // identifier_read()
    // 
    // An identifier typically begins with an allowed
    // set of characters as the first character, and continues
    // until you reach a character that is not in the set of
    // allowed body characters.
    static INLINE bool identifier_read(
        ByteSpan& src,
        ByteSpan& out,
        const charset& firstChars,
        const charset& bodyChars) noexcept
    {
        out.reset();

        if (!src || !firstChars(*src))
            return false;

        ByteSpan start = src;

        ++src;                      // consume valid first char
        bspan_read_while(src, bodyChars);

        out.resetPointers(start.begin(), src.begin());
        return true;
    }



    // parameter_read_next()
    // 
    // Read the next parameter from a source span, where parameters are
    // separated by a field delimiter (default ';'), and key-value pairs
    // are separated by a key-value separator (default '=').
    // The function trims whitespace around keys and values.
    //
    static INLINE bool parameter_read_next(
        ByteSpan& src,
        ByteSpan& key,
        ByteSpan& value,
        const uint8_t fieldDelimiter = ';',
        const uint8_t keyValueSeparator = '=') noexcept
    {
        key.reset();
        value.reset();

        bspan_ltrim_spaces(src);

        if (!src)
            return false;

        ByteSpan field = bspan_read_until(src, fieldDelimiter);
        bspan_trim_spaces(field);

        if (!field)
            return false;

        key = bspan_read_until(field, keyValueSeparator);
        value = field;

        bspan_trim_spaces(key);
        bspan_trim_spaces(value);

        return !!key;
    }
}