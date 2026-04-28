// surface_info.h
#pragma once

#include "definitions.h"
#include "wggeometry.h"


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
        ptrdiff_t stride;        // in bytes between rows
        bool     contiguous;    // whether the memory is contiguous (no gap between rows)
    } ;


    // rectangle convenience
    static INLINE WGRectI Surface_ARGB32_bounds(const Surface_ARGB32* s) noexcept
    {
        return WGRectI{ 0, 0, s->width, s->height };
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

    static  INLINE uint32_t* Surface_ARGB32_row_pointer_unchecked(const Surface_ARGB32* s, int y) 
    {
        return (uint32_t*)(s->data + ((size_t)y * (size_t)s->stride));
    }
    
    static  INLINE uint32_t* Surface_ARGB32_row_pointer(const Surface_ARGB32* s, int y)
    {
        return (uint32_t*)(s->data + ((size_t)y * (size_t)s->stride));
    }

    static  INLINE const uint32_t* Surface_ARGB32_row_pointer_const(const Surface_ARGB32* s, int y) {
        return (const uint32_t*)(s->data + (size_t)y * (size_t)s->stride);
    }

    // wg_get_subarea()
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
    static  WGResult Surface_ARGB32_get_subarea(const Surface_ARGB32& src, const WGRectI& area, Surface_ARGB32& subarea) noexcept
    {
        // If there's not associated data in the source
        // then return an early error
        if (!src.data || (src.width <= 0) || (src.height <= 0) || (src.stride < src.width*4))
            return WG_ERROR_Invalid_Argument;

        // Get the intersection of the requested area with 
        // the source bounds, and use that as the actual area 
        // for the subarea.
        WGRectI srcBounds = Surface_ARGB32_bounds(&src);
        WGRectI clippedArea = intersection(srcBounds, area);

        // early return if no intersection
        if (clippedArea.w <= 0 || clippedArea.h <= 0)
            return WG_ERROR_Invalid_Argument;

        subarea.data = src.data + (size_t)clippedArea.y * (size_t)src.stride + (size_t)clippedArea.x * 4;
        subarea.width = clippedArea.w;
        subarea.height = clippedArea.h;
        subarea.stride = src.stride;
        subarea.contiguous =
            src.contiguous &&
            clippedArea.x == 0 &&
            clippedArea.w == src.width;

        return WG_SUCCESS;
    }


}
