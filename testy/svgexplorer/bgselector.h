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
			BLPath apath;
			apath.addRect(0, 0, frame().w, frame().h);
			ctx->fill(bgcolor);
			ctx->fillShape(apath);
		}

		void drawForeground(IRenderSVG* ctx) override
		{
			ctx->strokeWidth(3);
			BLPath apath;
			apath.addRect(1, 1, frame().w - 2, frame().h - 2);
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