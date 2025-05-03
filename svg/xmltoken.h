#pragma once

#include "bspan.h"

namespace waavs {
    // Valid starting chars for xsd:Name / xsd:NCName (ASCII subset)
    static constexpr charset xmlNameStartChars = chrAlphaChars + "_";

    // Valid trailing chars for xsd:Name
    static constexpr charset xmlNameChars = chrAlphaChars + chrDecDigits + ".-_:";

    // Valid trailing chars for xsd:NCName
    static constexpr charset xmlNcnameChars = chrAlphaChars + chrDecDigits + ".-_";


    enum XmlTokenType {
        XML_TOKEN_INVALID = 0,
        XML_TOKEN_LT,       // <
        XML_TOKEN_GT,       // >
        XML_TOKEN_SLASH,    // /
        XML_TOKEN_EQ,       // =
        XML_TOKEN_QMARK,    // ?
        XML_TOKEN_BANG,      // !
        XML_TOKEN_NAME,     // NMTOKEN or element/attribute name
        XML_TOKEN_STRING,   // Quoted attribute value
        XML_TOKEN_TEXT,     // Raw character content
    };

    struct XmlToken {
        XmlTokenType type = XML_TOKEN_INVALID;
        ByteSpan value;

        void reset(XmlTokenType t = XML_TOKEN_INVALID, ByteSpan v = {}) noexcept {
            type = t;
            value = v;
        }
    };

    struct XmlTokenState {
        ByteSpan input;
        bool inTag = false;
    };

    static inline bool nextXmlToken(XmlTokenState& state, XmlToken& out)
    {
        out.reset();

        if (state.input.empty())
            return false;

        // === Outside of tag ===
        if (!state.inTag) {
            const unsigned char* start = state.input.fStart;
            const unsigned char* lt = static_cast<const unsigned char*>(memchr(start, '<', state.input.size()));
            if (!lt)
                lt = state.input.fEnd;

            if (lt != start) {
                out.reset(XML_TOKEN_TEXT, { start, lt });
                state.input.fStart = lt;
                return true;
            }

            // Found '<'
            ++state.input.fStart;
            state.inTag = true;
            out.reset(XML_TOKEN_LT);
            return true;
        }

        // === Inside tag ===
        state.input.skipWhile(chrWspChars);
        if (state.input.empty())
            return false;

        char ch = *state.input++;
        switch (ch) {
        case '>':
            state.inTag = false;
            out.reset(XML_TOKEN_GT);
            return true;
        case '/': out.reset(XML_TOKEN_SLASH); return true;
        case '=': out.reset(XML_TOKEN_EQ); return true;
        case '?': out.reset(XML_TOKEN_QMARK); return true;
        case '!': out.reset(XML_TOKEN_BANG); return true;

        case '"':
        case '\'':
        {
            const unsigned char* start = state.input.fStart;
            const unsigned char* end = start;
            while (end < state.input.fEnd && *end != ch)
                ++end;
            out.reset(XML_TOKEN_STRING, { start, end });
            state.input.fStart = (end < state.input.fEnd) ? end + 1 : end;
            return true;
        }

        default:
            if (xmlNameStartChars(ch)) {
                const unsigned char* start = state.input.fStart - 1;
                while (!state.input.empty() && xmlNameChars(*state.input))
                    ++state.input.fStart;
                out.reset(XML_TOKEN_NAME, { start, state.input.fStart });
                return true;
            }
            break;
        }

        out.reset(XML_TOKEN_INVALID, { state.input.fStart - 1, state.input.fStart });
        return true;
    }
}
