#pragma once

#pragma comment(lib, "blend2d.lib")

#include "blend2d.h"

#include "surface.h"

#include "pathprogramexec.h"



namespace waavs
{
    // surfaceFromBLImage
    // 
    // Create a Surface wrapper of a BLImage.  Using this method
    // the Surface will point at the BLImage data, but it will NOT
    // own the memory for it.  The BLImage is still reponsible for 
    // the lifetime of the pixel data, and knows nothing about 
    // the existence of the Surface wrapper.
    // This is ok for temporary usage, where the wrapper is created
    // and rapidly discarded.
    // It's also useful in cases where the Surface and BLImage are
    // in the same lifetime scope.

    static INLINE Surface surfaceFromBLImage(const BLImage& img) noexcept
    {
        BLImageData imgData{};
        img.getData(&imgData);

        Surface surf{};
        surf.createFromData((size_t)img.width(), (size_t)img.height(), imgData.stride, (uint8_t*)imgData.pixelData);

        return surf;
    }

    // blImageFromSurface
    //
    // When you've got a Surface, and you need to pass it along to 
    // a blend2d interface.
    static INLINE BLImage blImageFromSurface(const Surface& surf) noexcept
    {
        BLImage img;
        img.createFromData((int)surf.width(), (int)surf.height(), BL_FORMAT_PRGB32, (uint8_t*)surf.data(), surf.stride());
        return img;
    }

}

namespace waavs
{

    // ------------------------------------------------------------
    // Executor: PathProgram -> BLPath
    // 
    // (retains current "after close inject moveTo" behavior)
    // ------------------------------------------------------------

    struct BLPathProgramExec
    {
        BLPath& path;

        // Track current/subpath start for close injection semantics.
        double cx{ 0.0 }, cy{ 0.0 };
        double sx{ 0.0 }, sy{ 0.0 };
        bool   hasCP{ false };

        bool   pathJustClosed{ false };

        explicit BLPathProgramExec(BLPath& p) noexcept : path(p) {}

        inline void maybeInjectMoveAfterClose_(uint8_t op) noexcept
        {
            if (!pathJustClosed) return;
            if (op == OP_MOVETO) return;

            // mimic your current workaround
            path.moveTo(cx, cy);
            pathJustClosed = false;
        }

        void execute(uint8_t op, const float* a) noexcept
        {
            maybeInjectMoveAfterClose_(op);

            switch (op) {
            case OP_MOVETO: {
                cx = a[0]; cy = a[1];
                sx = cx;   sy = cy;
                hasCP = true;
                path.moveTo(cx, cy);
                pathJustClosed = false;
            } break;

            case OP_LINETO: {
                cx = a[0]; cy = a[1];
                hasCP = true;
                path.lineTo(cx, cy);
                pathJustClosed = false;
            } break;

            case OP_QUADTO: {
                // x1 y1 x y
                cx = a[2]; cy = a[3];
                hasCP = true;
                path.quadTo(a[0], a[1], a[2], a[3]);
                pathJustClosed = false;
            } break;

            case OP_CUBICTO: {
                // x1 y1 x2 y2 x y
                cx = a[4]; cy = a[5];
                hasCP = true;
                path.cubicTo(a[0], a[1], a[2], a[3], a[4], a[5]);
                pathJustClosed = false;
            } break;

            case OP_ARCTO: {
                // rx ry xrot large sweep x y
                const bool larc = a[3] > 0.5f;
                const bool swp = a[4] > 0.5f;

                // Your current code converts degrees->radians here
                const double xrot = radians(a[2]);

                cx = a[5]; cy = a[6];
                hasCP = true;

                path.ellipticArcTo(BLPoint(a[0], a[1]), xrot, larc, swp, BLPoint(a[5], a[6]));
                pathJustClosed = false;
            } break;

            case OP_CLOSE: {
                path.close();
                // after close, SVG current point becomes subpath start
                cx = sx; cy = sy;
                pathJustClosed = true;
            } break;

            default:
                // OP_END should never be executed by runPathProgram()
                break;
            }
        }
    };




    // blPath_from_PathProgram
    //
    // Filling a BLPath from a PathProgram.
    //
    static INLINE bool blPath_from_PathProgram(const PathProgram& prog, BLPath& path) noexcept
    {
        BLPathProgramExec exec(path);
        runPathProgram(prog, exec);
        path.shrink();

        return true;
    }

    // ------------------------------------------------------------
    // parsePath()
    // 
    // Parse a path, and create a BLPath directly
    // ------------------------------------------------------------
    /*
    static INLINE bool parsePath(const waavs::ByteSpan& inSpan, BLPath& apath) noexcept
    {
        PathProgram prog;
        if (!parsePathProgram(inSpan, prog))
            return false;

        BLPathProgramExec exec(apath);
        runPathProgram(prog, exec);
        return true;
    }
    */
}
