#pragma once

#include "wggeometry.h"
#include "blend2d_connect.h"
#include "svgenums.h"

// Common pieces used by filters, masks, and clip paths.  

namespace waavs
{
    // This is the state we need to capture in most cases
    struct EffectSurfaceState
    {
        // Filter-level coordinate systems
        SpaceUnitsKind regionUnits{ SpaceUnitsKind::SVG_SPACE_OBJECT };
        SpaceUnitsKind contentUnits{ SpaceUnitsKind::SVG_SPACE_USER };

        WGRectD objectBBoxUS{};     // Object bounding box in user space

        WGRectD effectRectUS{};     // Effect bounding box in user space
        WGRectI effectRectPX{};     // Effect bounding box in pixel space (integer)

        BLMatrix2D userToSurface{};
        BLMatrix2D surfaceToUser{};
    };

    static INLINE BLMatrix2D makeUserToSurfaceMatrix(const EffectSurfaceState& runState) noexcept
    {
        BLMatrix2D m = BLMatrix2D::makeIdentity();
        m.translate(-runState.objectBBox.x, -runState.objectBBox.y);

        return m;
    }

}