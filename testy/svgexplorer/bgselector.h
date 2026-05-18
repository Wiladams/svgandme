#pragma once

#include "graphicview.h"


namespace waavs {


    struct BackgroundSelector : public GraphicView
    {
        BackgroundSelector(const WGRectD& fr)
            :GraphicView(fr)
        {

        }

        void drawBackground(IRenderSVG* ctx) override
        {
            ColorSRGB srgb;
            if (get_color_by_name_as_srgb("beige", srgb) == WG_SUCCESS)
            {
                BLRgba32 bgcolor = BLRgba32_premultiplied_from_ColorSRGB(srgb);
                BLPath apath;
                apath.add_rect(0, 0, frame().w, frame().h);
                ctx->fill(bgcolor);
                ctx->fillShape(apath);
            }
        }

        void drawForeground(IRenderSVG* ctx) override
        {
            ctx->strokeWidth(3);
            BLPath apath;
            apath.add_rect(1, 1, frame().w - 2, frame().h - 2);
            ctx->stroke(BLRgba32(0xff7fA0A0));
            ctx->strokeShape(apath);
        }

        void drawSelf(IRenderSVG* ctx) override 
        {
            // draw all the stuff
            // square for checkboard
            // square for nothing
            // squares for multiple colors

        }
    };

}