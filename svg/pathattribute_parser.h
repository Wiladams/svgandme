#pragma once

#include "pathattribute_parser.h"
#include "pathprogram_builder.h"
#include "pathsegment_normalizer.h"

namespace waavs
{
    // ------------------------------------------------------------
    // parsePathProgram()
    // 
    // build a PathProgram from path data, 
    // represented by SVG <path> 'd' attribute.
    // The program is a canonicalized, normalized representation of the path data,
    // so, there are no relative commands, no implicit lineto after moveto, 
    // arcs are in endpoint form, etc.
    // ------------------------------------------------------------

    static INLINE bool parsePathProgram(const ByteSpan& inSpan, PathProgram& outProg) noexcept
    {
        PathProgramBuilder builder;
        PathSegmentNormalizer normalizer(builder);

        SVGSegmentIterator iter(inSpan);
        PathSegment seg{};

        while (readNextSegmentCommand(iter, seg)) {
            normalizer.consume(seg);
        }

        builder.end();
        outProg = std::move(builder.prog);
        return true;
    }
}
