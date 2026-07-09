#pragma once

#include "lang_span.h"

namespace waavs {

    static INLINE void writeChunk(const ByteSpan& chunk) noexcept
    {
        ByteSpan s = chunk;

        while (s && *s) {
            printf("%c", *s);
            s++;
        }
    }

    static INLINE void writeChunkBordered(const ByteSpan& chunk) noexcept
    {
        ByteSpan s = chunk;

        printf("||");
        while (s && *s) {
            printf("%c", *s);
            s++;
        }
        printf("||");
    }

    static INLINE void printChunk(const ByteSpan& chunk) noexcept
    {
        if (chunk)
        {
            writeChunk(chunk);
            printf("\n");
        }
        else
            printf("BLANK==CHUNK\n");

    }
}
