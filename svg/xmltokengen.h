#ifndef XMLTOKENGEN_H_INCLUDED
#define XMLTOKENGEN_H_INCLUDED

#include "xmltoken.h"
#include "pipeline.h"


namespace waavs
{
    // Objectified XML token generator
    // Put a IProduce interface on it, and maintain
    // the state associated with the tokenizer
    //
    struct XmlTokenGenerator : IProduce<XmlToken>
    {
        XmlTokenState fState{};

        XmlTokenGenerator(const ByteSpan& src)
            :fState{ src, false }
        {
        }

        bool next(XmlToken& tok) override
        {
            return nextXmlToken(fState, tok);
        }
    };
}

#endif // XMLTOKENGEN_H_INCLUDED
