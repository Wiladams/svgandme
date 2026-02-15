#pragma once

//
// References for text layout and measurement
// https://jenkov.com/tutorials/svg/text-element.html#text-definitions
// https://www.w3.org/TR/SVG11/text.html#TextLayout
// https://www.w3.org/TR/SVG11/text.html#TextAnchorProperty

#include "svgstructuretypes.h"
#include "svgattributes.h"
#include "twobitpu.h"

namespace waavs
{
    struct Fontography
    {

        // Measure how big a piece of text will be with a given font
        static BLPoint textMeasure(const BLFont& font, const ByteSpan& txt) noexcept
        {
            BLTextMetrics tm;
            BLGlyphBuffer gb;
            BLFontMetrics fm = font.metrics();

            gb.setUtf8Text(txt.data(), txt.size());
            font.shape(gb);
            font.getTextMetrics(gb, tm);


            // if we're hit testing, use the following
            //float cx = (float)(tm.boundingBox.x1 - tm.boundingBox.x0);
            // when we're trying to know the extent of the text, use the advance
            float advanceX = tm.advance.x;
            float cy = fm.ascent + fm.descent;

            return BLPoint(advanceX, cy);
        }


        // calcTextPosition
        // Given a piece of text, and a coordinate
        // calculate its baseline given the a specified alignment
        static BLRect calcTextPosition(BLFont font, const ByteSpan& txt, double x, double y, SVGAlignment hAlignment = SVGAlignment::SVG_ALIGNMENT_START, TXTALIGNMENT vAlignment = TXTALIGNMENT::BASELINE, DOMINANTBASELINE baseline = DOMINANTBASELINE::AUTO)
        {
            BLPoint txtSize = textMeasure(font, txt);
            double cx = txtSize.x;
            double cy = txtSize.y;

            switch (hAlignment)
            {
            case SVGAlignment::SVG_ALIGNMENT_START:
                // do nothing
                // x = x;
                break;
            case SVGAlignment::SVG_ALIGNMENT_MIDDLE:
                x = x - (cx / 2);
                break;
            case SVGAlignment::SVG_ALIGNMENT_END:
                x = x - cx;
                break;

            default:
                break;
            }

            switch (vAlignment)
            {
            case TXTALIGNMENT::TOP:
                y = y + cy - FontHandler::descent(font);
                break;
            case TXTALIGNMENT::CENTER:
                y = y + (cy / 2);
                break;

            case TXTALIGNMENT::MIDLINE:
                //should use the design metrics xheight
                break;

            case TXTALIGNMENT::BASELINE:
                // If what was passed as y is the baseline
                // do nothing to it because blend2d draws
                // text from baseline
                break;

            case TXTALIGNMENT::BOTTOM:
                // Adjust from the bottom as blend2d
                // prints from the baseline, so adjust
                // by the amount of the descent
                y = y - FontHandler::descent(font);
                break;

            default:
                break;
            }

            switch (baseline)
            {
            case DOMINANTBASELINE::HANGING:
                y = y + FontHandler::capHeight(font);
                break;

            case DOMINANTBASELINE::MATHEMATICAL:
                y = y + FontHandler::exHeight(font);
                break;

            case DOMINANTBASELINE::TEXT_BEFORE_EDGE:
                y = y + FontHandler::emHeight(font);
                break;

            case DOMINANTBASELINE::CENTRAL:
            case DOMINANTBASELINE::MIDDLE:
                // adjust by half the height
                y = y + (FontHandler::exHeight(font) / 2);
                break;
            }

            return { x, y, cx, cy };
        }
    };
}
