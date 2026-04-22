// surface_info.h - Defines the Surface_ARGB32 structure for representing ARGB32 surfaces in the waavs library.
#pragma once

#include "definitions.h"
#include "wggeometry.h"


namespace waavs
{
    // ----------------------------------------------------
    // Surface type + rows + basic fills/blends for ARGB32 
    // ----------------------------------------------------
    typedef struct {
        uint8_t* data;          // base pointer
        int32_t  width;         // in pixels
        int32_t  height;        // in pixels
        intptr_t stride;        // in bytes between rows
        bool     contiguous;    // whether the memory is contiguous (no gap between rows)
    } Surface_ARGB32;


    // rectangle convenience
    static INLINE WGRectI Surface_ARGB32_bounds(const Surface_ARGB32* s) noexcept
    {
        return WGRectI{ 0, 0, s->width, s->height };
    }

    static  INLINE uint32_t* Surface_ARGB32_row_pointer(const Surface_ARGB32* s, int y) {
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
    static  WGErrorCode Surface_ARGB32_get_subarea(const Surface_ARGB32& src, const WGRectI& area, Surface_ARGB32& subarea) noexcept
    {
        if (!src.data || src.width == 0 || src.height == 0)
            return WG_ERROR_Invalid_Argument;
        if (area.w <= 0 || area.h <= 0)
            return WG_ERROR_Invalid_Argument;
        if (area.x < 0 || area.y < 0 || area.x + area.w >(int)src.width || area.y + area.h >(int)src.height)
            return WG_ERROR_Invalid_Argument;


        subarea.data = src.data + (size_t)area.y * (size_t)src.stride + (size_t)area.x * 4;
        subarea.width = area.w;
        subarea.height = area.h;
        subarea.stride = src.stride;
        subarea.contiguous = false; // Subareas might not contiguous if they don't.
        //. cover the whole area

        return WG_SUCCESS;
    }


}
