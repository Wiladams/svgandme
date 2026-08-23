#pragma once

#include "graphicview.h"


namespace waavs {


    struct BackgroundSelector : public GraphicView
    {
        SolidColorPaint fForegroundBorder{ BLRgba32(0xff7fA0A0) };
        SolidColorPaint fBackgroundFill{ BLRgba32(0xffF0EAD6) };

        BackgroundSelector(const WGRectD& fr)
            :GraphicView(fr)
        {
            ColorSRGB srgb;
            if (get_color_by_name_as_srgb("beige", srgb) == WG_SUCCESS)
            {
                BLRgba32 bgcolor = BLRgba32_premultiplied_from_ColorSRGB(srgb);
                fBackgroundFill = SolidColorPaint(bgcolor);
            }

        }

        void drawBackground(IDrawGraphics* ctx) override
        {
                BLPath apath;
                apath.add_rect(0, 0, frame().w, frame().h);
                ctx->fill(&fBackgroundFill);
                ctx->fillShape(apath);
        }

        void drawForeground(IDrawGraphics* ctx) override
        {
            ctx->strokeWidth(3);
            BLPath apath;
            apath.add_rect(1, 1, frame().w - 2, frame().h - 2);
            ctx->stroke(&fForegroundBorder);
            ctx->strokeShape(apath);
        }

        void drawSelf(IDrawGraphics* ctx) override 
        {
            // draw all the stuff
            // square for checkboard
            // square for nothing
            // squares for multiple colors

        }
    };

}