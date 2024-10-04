#pragma once

#include "graphicview.h"


namespace waavs {


	struct BackgroundSelector : public GraphicView
	{
		BackgroundSelector(const BLRect& fr)
			:GraphicView(fr)
		{

		}

		void drawBackground(IRenderSVG* ctx) override
		{
			auto bgcolor = getSVGColorByName("beige");

			ctx->fillRect(0, 0, frame().w, frame().h,bgcolor);
		}

		void drawForeground(IRenderSVG* ctx) override
		{
			ctx->strokeWidth(3);
			ctx->strokeRect(BLRect(1, 1, frame().w - 2, frame().h - 2), BLRgba32(0xff7fA0A0));
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