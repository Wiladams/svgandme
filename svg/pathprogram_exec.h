#pragma once

#include "pathprogram.h"


namespace waavs
{
    template <class Exec>
    static void runPathProgram(const PathProgram& p, Exec& exec) noexcept
    {
        size_t ip = 0;		// instruction pointer
        size_t ap = 0;		// argument pointer

        while (ip < p.ops.size()) {
            const uint8_t op = p.ops[ip++];
            if (op == OP_END)
                break;

            const uint8_t n = kPathOpArity[op];
            exec.execute(op, p.args.data() + ap);
            ap += n;
        }
    }
}

