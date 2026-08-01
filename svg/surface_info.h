// surface_info.h
#pragma once

#include "definitions.h"
#include "core_geometry.h"


namespace waavs
{

    // ----------------------------------------------------
    // Surface type + rows + basic fills/blends for ARGB32 
    // ----------------------------------------------------
    struct Surface_ARGB32 
    {
        uint8_t* data;          // base pointer
        int32_t  width;         // in pixels
        int32_t  height;        // in pixels
        ptrdiff_t stride;       // in bytes between rows
        bool     contiguous;    // whether the memory is contiguous (no gap between rows)
    } ;

    ASSERT_MEMCPY_SAFE(Surface_ARGB32);

    // Report on the validity of the surface structure.  
    static INLINE bool Surface_ARGB32_is_valid(const Surface_ARGB32 & s) noexcept
    {
        if (!s.data || s.width <= 0 || s.height <= 0 || s.stride <= 0)
            return false;

        const size_t minStride = (size_t)s.width * sizeof(uint32_t);

        return (size_t)s.stride >= minStride;
    }

    // rectangle convenience
    static INLINE WGRectI Surface_ARGB32_bounds(const Surface_ARGB32& s) noexcept
    {
        if (!Surface_ARGB32_is_valid(s))
            return WGRectI{ 0, 0, 0, 0 };

        return WGRectI{ 0, 0, s.width, s.height };
    }

    //
    // Surface_ARGB32_row_pointer_unchecked
    // 
    // Preconditions:
    //  - s is a valid pointer to a Surface_ARGB32
    //  - s->data is a valid pointer to the surface data, 
    //    and the memory layout matches the width, height, 
    //    and stride specified in the structure.
    //  - y is a valid row index (0 <= y < s->height)
    // 
    // Returns a pointer to the start of the specified row.  
    // The caller is responsible for ensuring that the preconditions are met.

    static  INLINE uint32_t* Surface_ARGB32_row_pointer_unchecked(const Surface_ARGB32& s, int y) 
    {
        return (uint32_t*)(s.data + ((size_t)y * (size_t)s.stride));
    }
    
    static INLINE uint32_t* Surface_ARGB32_row_pointer(const Surface_ARGB32& s, int y) noexcept
    {
        if (!Surface_ARGB32_is_valid(s))
            return nullptr;

        if (y < 0 || y >= s.height)
            return nullptr;

        return Surface_ARGB32_row_pointer_unchecked(s, y);
    }

    static INLINE const uint32_t* Surface_ARGB32_row_pointer_const(const Surface_ARGB32& s, int y) noexcept
    {
        return Surface_ARGB32_row_pointer(s, y);
    }

    // Surface_ARGB32_subview()
    //
    // Get a subarea view of the given source surface. 
    // The subarea shares memory with the original surface, 
    // so changes to the subarea will affect the original surface.
    // This is a 'view' onto the surface.  Efficient to create and 
    // use with 'whole surface' operations like fill_all() or clear_all(), 
    // or to read/write pixels within the subarea.
    //
    // This routine will perform boundary clipping and return a subarea
    // that actually fits within the source surface.
    //

    static INLINE Surface_ARGB32 Surface_ARGB32_subview(const Surface_ARGB32& src, const WGRectI& area) noexcept
    {
        Surface_ARGB32 subarea{};

        if (!Surface_ARGB32_is_valid(src))
            return subarea;

        WGRectI srcBounds = Surface_ARGB32_bounds(src);
        const WGRectI clippedArea = rectI_intersection(srcBounds, area);

        if (!rectI_is_valid(clippedArea))
            return subarea;

        subarea.data =
            src.data +
            (size_t)clippedArea.y * (size_t)src.stride +
            (size_t)clippedArea.x * sizeof(uint32_t);

        subarea.width = clippedArea.w;
        subarea.height = clippedArea.h;
        subarea.stride = src.stride;

        subarea.contiguous =
            src.contiguous &&
            clippedArea.x == 0 &&
            clippedArea.w == src.width;

        return subarea;
    }

}
