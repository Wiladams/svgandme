#pragma once

#include "definitions.h"
#include "core_geometry.h"
#include "maths_bezier.h"

//
// Bounding box calculations for quadratic and cubic Bezier curves.
// So, what are these doing in here, rather than located with the 
// bezier math routines in maths_bezier.h?  Because, these routines 
// have knowledge of a bounding box as a data structure, which 
// is not really a part of maths.
//
namespace waavs
{
    static INLINE void bez_quad_bounds(
        double x0, double y0,
        double x1, double y1,
        double x2, double y2,
        double& minX, double& minY,
        double& maxX, double& maxY)
    {
        bboxExpand(minX, minY, maxX, maxY, x2, y2);

        double t = 0.0;

        if (bez_quad_solve_extrema(x0, x1, x2, t)) {
            double x = 0.0;
            double y = 0.0;
            bez_quad_eval_point(x0, y0, x1, y1, x2, y2, t, x, y);
            bboxExpand(minX, minY, maxX, maxY, x, y);
        }

        if (bez_quad_solve_extrema(y0, y1, y2, t)) {
            double x = 0.0;
            double y = 0.0;
            bez_quad_eval_point(x0, y0, x1, y1, x2, y2, t, x, y);
            bboxExpand(minX, minY, maxX, maxY, x, y);
        }
    }


    static INLINE void bez_cubic_bounds(
        double x0, double y0,
        double x1, double y1,
        double x2, double y2,
        double x3, double y3,
        double& minX, double& minY,
        double& maxX, double& maxY)
    {
        bboxExpand(minX, minY, maxX, maxY, x3, y3);

        double tValues[2];

        int xCount = bez_cubic_solve_extrema(x0, x1, x2, x3, tValues);
        for (int i = 0; i < xCount; ++i) {
            double x = 0.0;
            double y = 0.0;
            bez_cubic_eval_point(x0, y0, x1, y1, x2, y2, x3, y3, tValues[i], x, y);
            bboxExpand(minX, minY, maxX, maxY, x, y);
        }

        int yCount = bez_cubic_solve_extrema(y0, y1, y2, y3, tValues);
        for (int i = 0; i < yCount; ++i) {
            double x = 0.0;
            double y = 0.0;
            bez_cubic_eval_point(x0, y0, x1, y1, x2, y2, x3, y3, tValues[i], x, y);
            bboxExpand(minX, minY, maxX, maxY, x, y);
        }
    }
}
