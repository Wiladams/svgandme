#pragma once

#include "surface_info.h"
#include "pixeling.h"
#include "surface_draw.h"

namespace waavs
{
    enum WGScaleFilter : uint32_t
    {
        WG_BLIT_SCALE_Nearest = 0,
        WG_BLIT_SCALE_Bilinear = 1
    };

    enum WGImageEdgeMode : uint32_t
    {
        WG_IMAGE_EDGE_Clear = 0,
        WG_IMAGE_EDGE_Clamp = 1
    };

    static INLINE uint32_t wg_interp_u8_4(
        uint32_t v00,
        uint32_t v10,
        uint32_t v01,
        uint32_t v11,
        double w00,
        double w10,
        double w01,
        double w11) noexcept
    {
        const double v =
            double(v00) * w00 +
            double(v10) * w10 +
            double(v01) * w01 +
            double(v11) * w11;

        return quantize0_255(v / 255.0);
    }

    // -------------------------------------------
    // wg_sample_nearest_PRGB32()
    //

    static INLINE Pixel_ARGB32 wg_sample_nearest_PRGB32(
        const Surface_ARGB32& src,
        double sx,
        double sy,
        const WGRectI& sampleBounds) noexcept
    {
        const int minX = sampleBounds.x;
        const int minY = sampleBounds.y;
        const int maxX = wg_rectI_max_x(sampleBounds);
        const int maxY = wg_rectI_max_y(sampleBounds);

        int ix = int(std::floor(sx + 0.5));
        int iy = int(std::floor(sy + 0.5));

        ix = clamp(ix, minX, maxX);
        iy = clamp(iy, minY, maxY);

        const Pixel_ARGB32* row = Surface_ARGB32_row_pointer_const(&src, iy);
        return row[ix];
    }

    static INLINE Pixel_ARGB32 wg_sample_bilinear_PRGB32(
        const Surface_ARGB32& src,
        double sx,
        double sy,
        const WGRectI& sampleBounds) noexcept
    {
        const int minX = sampleBounds.x;
        const int minY = sampleBounds.y;
        const int maxX = wg_rectI_max_x(sampleBounds);
        const int maxY = wg_rectI_max_y(sampleBounds);

        int x0 = int(std::floor(sx));
        int y0 = int(std::floor(sy));

        const double tx = sx - double(x0);
        const double ty = sy - double(y0);

        int x1 = x0 + 1;
        int y1 = y0 + 1;

        x0 = clamp(x0, minX, maxX);
        x1 = clamp(x1, minX, maxX);
        y0 = clamp(y0, minY, maxY);
        y1 = clamp(y1, minY, maxY);

        const Pixel_ARGB32* row0 = Surface_ARGB32_row_pointer_const(&src, y0);
        const Pixel_ARGB32* row1 = Surface_ARGB32_row_pointer_const(&src, y1);

        const Pixel_ARGB32 p00 = row0[x0];
        const Pixel_ARGB32 p10 = row0[x1];
        const Pixel_ARGB32 p01 = row1[x0];
        const Pixel_ARGB32 p11 = row1[x1];

        uint32_t a00, r00, g00, b00;
        uint32_t a10, r10, g10, b10;
        uint32_t a01, r01, g01, b01;
        uint32_t a11, r11, g11, b11;

        argb32_unpack_u32(p00, a00, r00, g00, b00);
        argb32_unpack_u32(p10, a10, r10, g10, b10);
        argb32_unpack_u32(p01, a01, r01, g01, b01);
        argb32_unpack_u32(p11, a11, r11, g11, b11);

        const double wx0 = 1.0 - tx;
        const double wx1 = tx;
        const double wy0 = 1.0 - ty;
        const double wy1 = ty;

        const double w00 = wx0 * wy0;
        const double w10 = wx1 * wy0;
        const double w01 = wx0 * wy1;
        const double w11 = wx1 * wy1;

        const uint32_t a = wg_interp_u8_4(a00, a10, a01, a11, w00, w10, w01, w11);
        const uint32_t r = wg_interp_u8_4(r00, r10, r01, r11, w00, w10, w01, w11);
        const uint32_t g = wg_interp_u8_4(g00, g10, g01, g11, w00, w10, w01, w11);
        const uint32_t b = wg_interp_u8_4(b00, b10, b01, b11, w00, w10, w01, w11);

        return argb32_pack_u32(a, r, g, b);
    }

    static INLINE Pixel_ARGB32 wg_sample_PRGB32(
        const Surface_ARGB32& src,
        double sx,
        double sy,
        const WGRectI& sampleBounds,
        WGScaleFilter filter) noexcept
    {
        if (filter == WG_BLIT_SCALE_Nearest)
            return wg_sample_nearest_PRGB32(src, sx, sy, sampleBounds);

        return wg_sample_bilinear_PRGB32(src, sx, sy, sampleBounds);
    }

    static INLINE Pixel_ARGB32 wg_sample_PRGB32_edge(
        const Surface_ARGB32& src,
        double sx,
        double sy,
        const WGRectI& sampleBounds,
        WGScaleFilter filter,
        WGImageEdgeMode edgeMode) noexcept
    {
        if (edgeMode == WG_IMAGE_EDGE_Clear)
        {
            if (!wg_rectI_contains_sample(sampleBounds, sx, sy))
                return Pixel_ARGB32(0);
        }

        return wg_sample_PRGB32(src, sx, sy, sampleBounds, filter);
    }

    static INLINE uint32_t wg_transform_blit_PRGB32(
        Surface_ARGB32& dst,
        const WGRectI& dstRectIn,
        const Surface_ARGB32& src,
        const WGRectI& srcRectIn,
        const WGMatrix3x3& srcToDst,
        WGScaleFilter filter = WG_BLIT_SCALE_Bilinear,
        WGImageEdgeMode edgeMode = WG_IMAGE_EDGE_Clear) noexcept
    {
        if (!dst.data || dst.width <= 0 || dst.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (!src.data || src.width <= 0 || src.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (!wg_rectI_is_valid(dstRectIn) || !wg_rectI_is_valid(srcRectIn))
            return WG_ERROR_Invalid_Argument;

        const WGRectI srcBounds{ 0, 0, int(src.width), int(src.height) };
        const WGRectI dstBounds{ 0, 0, int(dst.width), int(dst.height) };

        if (!wg_rectI_contains(srcRectIn, srcBounds))
            return WG_ERROR_Invalid_Argument;

        const WGRectI dstRect = intersection(dstRectIn, dstBounds);

        if (!wg_rectI_is_valid(dstRect))
            return WG_SUCCESS;

        WGMatrix3x3 dstToSrc{};
        if (!WGMatrix3x3::invert(dstToSrc, srcToDst))
            return WG_ERROR_Invalid_Argument;

        const bool affine = dstToSrc.isAffine2D();

        if (affine)
        {
            WGPointD rowStart = dstToSrc.mapPoint(
                double(dstRect.x) + 0.5,
                double(dstRect.y) + 0.5);

            const WGPointD stepX = dstToSrc.mapVector(1.0, 0.0);
            const WGPointD stepY = dstToSrc.mapVector(0.0, 1.0);

            rowStart.x -= 0.5;
            rowStart.y -= 0.5;

            for (int dy = 0; dy < dstRect.h; ++dy)
            {
                Pixel_ARGB32* dstRow =
                    Surface_ARGB32_row_pointer(&dst, dstRect.y + dy);

                WGPointD srcPt = rowStart;

                for (int dx = 0; dx < dstRect.w; ++dx)
                {
                    dstRow[dstRect.x + dx] =
                        wg_sample_PRGB32_edge(
                            src,
                            srcPt.x,
                            srcPt.y,
                            srcRectIn,
                            filter,
                            edgeMode);

                    srcPt.x += stepX.x;
                    srcPt.y += stepX.y;
                }

                rowStart.x += stepY.x;
                rowStart.y += stepY.y;
            }

            return WG_SUCCESS;
        }

        // Not affine, something more interesting happening
        for (int dy = 0; dy < dstRect.h; ++dy)
        {
            Pixel_ARGB32* dstRow =
                Surface_ARGB32_row_pointer(&dst, dstRect.y + dy);

            const double dstY = double(dstRect.y + dy) + 0.5;

            for (int dx = 0; dx < dstRect.w; ++dx)
            {
                const double dstX = double(dstRect.x + dx) + 0.5;

                WGPointD srcPt = dstToSrc.mapPoint(dstX, dstY);

                dstRow[dstRect.x + dx] =
                    wg_sample_PRGB32_edge(
                        src,
                        srcPt.x - 0.5,
                        srcPt.y - 0.5,
                        srcRectIn,
                        filter,
                        edgeMode);
            }
        }

        return WG_SUCCESS;
    }

        static INLINE uint32_t wg_scale_blit(
            Surface_ARGB32& dst,
            const WGRectI& dstRect,
            const Surface_ARGB32& src,
            const WGRectI& srcRect,
            WGScaleFilter filter = WG_BLIT_SCALE_Bilinear) noexcept
        {
            if (!wg_rectI_is_valid(dstRect) || !wg_rectI_is_valid(srcRect))
                return WG_ERROR_Invalid_Argument;

            const double sx = double(dstRect.w) / double(srcRect.w);
            const double sy = double(dstRect.h) / double(srcRect.h);

            WGMatrix3x3 srcToDst =
                WGMatrix3x3::makeTranslation(-double(srcRect.x), -double(srcRect.y));

            srcToDst.postScale(sx, sy);
            srcToDst.postTranslate(double(dstRect.x), double(dstRect.y));

            return wg_transform_blit_PRGB32(
                dst,
                dstRect,
                src,
                srcRect,
                srcToDst,
                filter,
                WG_IMAGE_EDGE_Clamp);
        }

        /*
    static INLINE uint32_t wg_scale_blit_PRGB32(
        Surface_ARGB32& dst,
        const WGRectI& dstRectIn,
        const Surface_ARGB32& src,
        const WGRectI& srcRectIn,
        WGScaleFilter filter = WG_BLIT_SCALE_Bilinear) noexcept
    {
        if (!dst.data || dst.width <= 0 || dst.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (!src.data || src.width <= 0 || src.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (!wg_rectI_is_valid(dstRectIn) || !wg_rectI_is_valid(srcRectIn))
            return WG_ERROR_Invalid_Argument;

        const WGRectI srcBounds{ 0, 0, int(src.width), int(src.height) };
        const WGRectI dstBounds{ 0, 0, int(dst.width), int(dst.height) };

        if (!wg_rectI_contains(srcRectIn, srcBounds))
            return WG_ERROR_Invalid_Argument;

        const WGRectI dstRect = intersection(dstRectIn, dstBounds);

        if (!wg_rectI_is_valid(dstRect))
            return WG_SUCCESS;

        if (srcRectIn.w == dstRectIn.w && srcRectIn.h == dstRectIn.h)
        {
            const WGRectI srcClip{
                srcRectIn.x + (dstRect.x - dstRectIn.x),
                srcRectIn.y + (dstRect.y - dstRectIn.y),
                dstRect.w,
                dstRect.h
            };

            Surface_ARGB32 srcView{};
            Surface_ARGB32 dstView{};

            if (Surface_ARGB32_get_subarea(src, srcClip, srcView) != WG_SUCCESS)
                return WG_ERROR_Invalid_Argument;

            if (Surface_ARGB32_get_subarea(dst, dstRect, dstView) != WG_SUCCESS)
                return WG_ERROR_Invalid_Argument;

            return wg_blit_copy_unchecked(dstView, srcView);
        }

        const double scaleX = double(srcRectIn.w) / double(dstRectIn.w);
        const double scaleY = double(srcRectIn.h) / double(dstRectIn.h);

        const int clippedOffsetX = dstRect.x - dstRectIn.x;
        const int clippedOffsetY = dstRect.y - dstRectIn.y;

        const double startX =
            double(srcRectIn.x) +
            (double(clippedOffsetX) + 0.5) * scaleX -
            0.5;

        const double startY =
            double(srcRectIn.y) +
            (double(clippedOffsetY) + 0.5) * scaleY -
            0.5;

        double sy = startY;

        for (int dy = 0; dy < dstRect.h; ++dy)
        {
            Pixel_ARGB32* dstRow = Surface_ARGB32_row_pointer(&dst, dstRect.y + dy);
            double sx = startX;

            for (int dx = 0; dx < dstRect.w; ++dx)
            {
                dstRow[dstRect.x + dx] =
                    wg_sample_PRGB32(src, sx, sy, srcRectIn, filter);

                sx += scaleX;
            }

            sy += scaleY;
        }

        return WG_SUCCESS;
    }
    */


    // wg_scale_blit_full_PRGB32
    // 
    // scale the entirety of the src into the box indicated
    // by the dstRect
    //
    static INLINE uint32_t wg_scale_blit_full_PRGB32(
        Surface_ARGB32& dst,
        const WGRectI& dstRect,
        const Surface_ARGB32& src,
        WGScaleFilter filter = WG_BLIT_SCALE_Bilinear) noexcept
    {
        const WGRectI srcRect{ 0, 0, int(src.width), int(src.height) };
        return wg_scale_blit(dst, dstRect, src, srcRect, filter);
    }



}
