#pragma once

#include "opentype_parser.h"

using namespace waavs;
using namespace waavs::opentype;

void processColorLayers(const OpenTypeParser& parser, uint32_t glyphId)
{
    size_t layerCount = 0;

    const OpenTypeColorLayer* layers =
        parser.getColorLayers(glyphId, layerCount);

    for (size_t i = 0; i < layerCount; ++i)
    {
        const auto& layer = layers[i];

        // layer.glyphId
        // layer.paletteIndex
    }
}

inline void renderCOLRv0Glyph(
    const OpenTypeParser& parser,
    uint16_t glyphId,
    uint16_t paletteIndex)
{
    size_t layerCount = 0;

    const OpenTypeColorLayer* layers =
        parser.getColorLayers(
            glyphId,
            layerCount);

    if (!layers)
        return;

    for (size_t i = 0; i < layerCount; ++i)
    {
        const auto& layer = layers[i];

        if (layer.paletteIndex == 0xFFFF)
        {
            // Draw layer.glyphId using current
            // application foreground color.
        }
        else
        {
            const OpenTypeCPALColor* color =
                parser.getPaletteColor(
                    paletteIndex,
                    layer.paletteIndex);

            if (!color)
                continue;

            // Draw layer.glyphId using:
            //
            // color->red
            // color->green
            // color->blue
            // color->alpha
        }
    }
}
