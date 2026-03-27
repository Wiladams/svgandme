#pragma once


/* 

Design contract for GCodeProgram IR:
1. GCodeProgram is a machine - motion IR, not raw text G - code.
2. It stores canonical motion / state operations plus numeric arguments.
3. It is controller - neutral; dialect - specific formatting happens later.
4. Coordinates in the IR should generally be explicit numeric machine - space values.
5. Absolute - coordinate representation is preferred internally, even if text output later uses incremental forms.
6. PathProgram - to - GCodeProgram lowering should target this IR, not text.
7. Tool / process policy(pen, laser, router, mill) should lower into GCodeProgram rather than being baked into PathProgram conversion.
8. Text serialization is a separate final pass.
9. A first implementation only needs modal setup, feed, rapid, linear, and end - program support.
10. Direct arc support is optional and can be added after flatten - first lowering is working.

*/

#include "definitions.h"

#include <vector>
#include <cstddef>
#include <cstdint>

namespace waavs
{
    // GCodeProgram is a normalized machine-motion IR.
    //
    // It is not raw textual G-code.
    // It is a compact, controller-neutral instruction stream that can
    // later be serialized into one or more G-code dialects.
    //
    // Design goals:
    //  - canonical enough for comparison / caching
    //  - simple to build and dispatch
    //  - suitable as a target for PathProgram lowering
    //  - not tied to one machine or controller
    //
    // Notes:
    //  - coordinates are carried explicitly as numeric values
    //  - first implementation favors absolute coordinates
    //  - text formatting is a later serialization step

    enum GCodeOp : uint8_t
    {
        GOP_END = 0,

        // Modal setup
        GOP_UNITS_MM,           // G21
        GOP_UNITS_INCH,         // G20
        GOP_ABSOLUTE,           // G90
        GOP_INCREMENTAL,        // G91
        GOP_PLANE_XY,           // G17
        GOP_PLANE_ZX,           // G18
        GOP_PLANE_YZ,           // G19

        // Feed / spindle / coolant
        GOP_SET_FEED,           // F
        GOP_SET_SPINDLE_RPM,    // S
        GOP_SPINDLE_ON_CW,      // M3
        GOP_SPINDLE_ON_CCW,     // M4
        GOP_SPINDLE_OFF,        // M5
        GOP_COOLANT_ON,         // M8
        GOP_COOLANT_OFF,        // M9

        // Motion
        GOP_RAPID,              // G0 : x y z
        GOP_LINEAR,             // G1 : x y z
        GOP_ARC_CW,             // G2 : x y z i j k
        GOP_ARC_CCW,            // G3 : x y z i j k
        GOP_DWELL,              // G4 : seconds

        // Program control
        GOP_OPTIONAL_STOP,      // M1
        GOP_STOP,               // M0
        GOP_END_PROGRAM         // M2
    };

    static constexpr uint8_t kGCodeOpArity[] =
    {
        0,  // GOP_END

        0,  // GOP_UNITS_MM
        0,  // GOP_UNITS_INCH
        0,  // GOP_ABSOLUTE
        0,  // GOP_INCREMENTAL
        0,  // GOP_PLANE_XY
        0,  // GOP_PLANE_ZX
        0,  // GOP_PLANE_YZ

        1,  // GOP_SET_FEED         : feed
        1,  // GOP_SET_SPINDLE_RPM  : rpm
        0,  // GOP_SPINDLE_ON_CW
        0,  // GOP_SPINDLE_ON_CCW
        0,  // GOP_SPINDLE_OFF
        0,  // GOP_COOLANT_ON
        0,  // GOP_COOLANT_OFF

        3,  // GOP_RAPID            : x y z
        3,  // GOP_LINEAR           : x y z
        6,  // GOP_ARC_CW           : x y z i j k
        6,  // GOP_ARC_CCW          : x y z i j k
        1,  // GOP_DWELL            : seconds

        0,  // GOP_OPTIONAL_STOP
        0,  // GOP_STOP
        0   // GOP_END_PROGRAM
    };

    static_assert(GOP_END_PROGRAM + 1 == std::size(kGCodeOpArity),
        "GCodeOp arity table size mismatch");


    struct GCodeProgram
    {
        std::vector<uint8_t> ops;
        std::vector<double> args;

        void clear() noexcept
        {
            ops.clear();
            args.clear();
        }

        bool empty() const noexcept
        {
            return ops.empty();
        }
    };
}
