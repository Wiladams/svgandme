#pragma once

#include "bspan.h"


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

    XML_TOKEN_SELF_CLOSE,   // />
};





namespace waavs {
    // Valid starting chars for xsd:Name / xsd:NCName (ASCII subset)
    static constexpr charset xmlNameStartChars = chrAlphaChars + "_";

    // Valid trailing chars for xsd:Name
    static constexpr charset xmlNameChars = chrAlphaChars + chrDecDigits + ".-_:";

    // Valid trailing chars for xsd:NCName
    static constexpr charset xmlNcnameChars = chrAlphaChars + chrDecDigits + ".-_";

    // The retained state of the nextXmlToken function
    struct XmlTokenState {
        ByteSpan input;
        bool inTag = false;

        bool empty() const noexcept { return input.empty(); }   
    };


    // 32-bytes
    struct XmlToken final
    {
        XmlTokenType type = XML_TOKEN_INVALID;
        ByteSpan value;
        bool inTag = false;

        void reset() noexcept
        {
            type = XML_TOKEN_INVALID;
            value = {};
            inTag = false;
        }

        void reset(XmlTokenType t, const ByteSpan v, bool inTagFlag) noexcept 
        {
            type = t;
            value = v;
			inTag = inTagFlag;
        }
    };



    // === Outside of tag ===
    static inline bool readText(XmlTokenState& state, XmlToken& out)
    {
        const unsigned char* start = state.input.fStart;
        const unsigned char* lt = static_cast<const unsigned char*>(memchr(start, '<', state.input.size()));
        
        if (!lt)
            lt = state.input.fEnd;

        if (lt != start) {
            out.reset(XML_TOKEN_TEXT, { start, lt }, false);
            state.input.fStart = lt;
            return true;
        }

        // Found '<'
        ++state.input.fStart;
        state.inTag = true;
        out.reset(XML_TOKEN_LT, {}, true);

        return true;
    }

    // === Inside tag ===
    static inline bool readTagToken(XmlTokenState& state, XmlToken& out)
    {
        state.input.skipWhile(chrWspChars);
        if (state.input.empty())
            return false;

        char ch = *state.input++;
        switch (ch) {
        case '>':
            state.inTag = false;
            out.reset(XML_TOKEN_GT, {}, true);
            return true;
        case '/': out.reset(XML_TOKEN_SLASH, {}, true); return true;
        case '=': out.reset(XML_TOKEN_EQ, {}, true); return true;
        case '?': out.reset(XML_TOKEN_QMARK, {}, true); return true;
        case '!': out.reset(XML_TOKEN_BANG, {}, true); return true;

        case '"':
        case '\'':
        {
            const unsigned char* start = state.input.fStart;
            const unsigned char* end = start;
            while (end < state.input.fEnd && *end != ch)
                ++end;
            out.reset(XML_TOKEN_STRING, { start, end }, true);
            state.input.fStart = (end < state.input.fEnd) ? end + 1 : end;
            return true;
        }

        default:
            if (xmlNameStartChars(ch)) {
                const unsigned char* start = state.input.fStart - 1;
                while (!state.input.empty() && xmlNameChars(*state.input))
                    ++state.input.fStart;
                out.reset(XML_TOKEN_NAME, { start, state.input.fStart }, true);
                return true;
            }
            break;
        }

        out.reset(XML_TOKEN_INVALID, { state.input.fStart - 1, state.input.fStart }, state.inTag);
        return true;
    }

    // generator of xml tokens
    // If the return value is false, then the state of the token 
    // is not valid.
    static inline bool nextXmlToken(XmlTokenState& state, XmlToken& out)
    {
        if (state.input.empty())
        {
			out.reset();
            return false;
        }

        return state.inTag ? readTagToken(state, out)
            : readText(state, out);

    }
}

