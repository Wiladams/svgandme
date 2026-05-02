// filter_fevoronoi.h

#pragma once

#include "filter_types.h"
#include "svgdatatypes.h"
#include "filter_primitive_element.h"


namespace waavs {
    // feVoronoi is a procedural generator, so it doesn't have any inputs.  
    // It just generates a pattern based on the filter region and some parameters.
    // So, we can treat it as a generator, and just write to the output tile 
    // directly, without reading from any input tiles.
    //
    // The actual Voronoi generation code is not implemented here, but this is where it would go.  You would need to implement the Voronoi algorithm, which typically involves:
    // 1. Generating a set of random seed points within the filter region.
    // 2. For each pixel in the output tile, determining which seed point is closest, and coloring the pixel based on that seed point (e.g., using the seed point's index to determine color).
    //
    // Typical XML Shape
    // <waavs:feVoronoi
    //    result = "voronoi"
    //    seed = "0"
    //    cellCount = "64"
    //    jitter = "1"
    //    metric = "euclidean"
    //    output = "edgeDistance"
    //    edgeWidth = "0.02"
    //    colorMode = "grayscale" / >
    //

    enum class VoronoiMetric : uint32_t {
        Euclidean           = 0,
        EuclideanSquared    = 1,
        Manhattan           = 2,
        Chebyshev           = 3
    };

    enum class VoronoiOutput : uint32_t {
        Distance1       = 0,    // nearest seed distance (d1)
        Distance2       = 1,    // second nearest seed distance (d2)
        EdgeDistance    = 2,    // d2 - d1
        EdgeMask        = 3,    // thresholded d2 - d1
        CellId          = 4,    // hashed nearest-cell id
        CellColor       = 5,    // random stable color per nearest cell
    };

    enum class VoronoiNormalize : uint8_t
    {
        UnitCell = 0,        // values normalized in grid/cell space
        EdgeWidth = 1,      // mainly useful for EdgeDistance
        None = 2
    };

    enum VoronoiFlags : uint8_t
    {
        VORONOI_FLAG_STITCH_TILES = 0x01,   // optional later
        VORONOI_FLAG_INVERT = 0x02,         // useful for direct mask output
        VORONOI_FLAG_ALPHA_ONLY = 0x04      // RGB white, alpha carries value
    };


    struct FilterVoronoiParams
    {
        SVGNumberOrPercent baseFrequencyX;
        SVGNumberOrPercent baseFrequencyY;

        float jitter;             // 0..1 usually, but clamp at execution
        float edgeWidth;          // in Voronoi grid units
        uint32_t seed;

        VoronoiMetric metric;
        VoronoiOutput output;
        VoronoiNormalize normalize;

        uint8_t flags;
    };




    /*
    static INLINE void feVoronoiPRGB32_row(
        uint32_t* dst,
        int x,
        int y,
        int count,
        const FilterRunState& runState) noexcept
    {

    }
    */
}