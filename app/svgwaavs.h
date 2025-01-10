#pragma once


#include <memory>
#include <vector>


#include "svgstructuretypes.h"

#include "screensnapshot.h"
#include "gmonitor.h"
#include "converters.h"




namespace waavs {
	//
	// DisplayCaptureElement
	// This is a source of paint, just like a SVGImage
	// It captures from the user's screen
	// Return value as variant(), and it can be used in all 
	// the same places a SVGImage can be used 
	//
	struct DisplayCaptureElement : public SVGGraphicsElement
	{
		static void registerFactory() {
			registerSVGSingularNode("displayCapture",
				[](IAmGroot* groot, const XmlElement& elem) {
					auto node = std::make_shared<DisplayCaptureElement>(groot);
					node->loadFromXmlElement(elem, groot);
					return node;
				});
		}


		ScreenSnapper fSnapper{};
		ByteSpan fSrcSpan{};
		BLPattern fPattern{};
		
		int64_t fCapX = 0;
		int64_t fCapY = 0;
		int64_t fCapWidth = 0;
		int64_t fCapHeight = 0;

		double fX{ 0 };
		double fY{ 0 };
		double fWidth{ 0 };
		double fHeight{ 0 };

		// The intended destination
		SVGDimension fDimX{};
		SVGDimension fDimY{};
		SVGDimension fDimWidth{};
		SVGDimension fDimHeight{};



		DisplayCaptureElement(IAmGroot* ) 
			:SVGGraphicsElement() {}

		const BLVar getVariant(IRenderSVG *ctx, IAmGroot *groot) noexcept override
		{
			if (fVar.isNull())
			{
				bindToContext(ctx, groot);
				
				fSnapper.update();
				fPattern.setImage(fSnapper.getImage());
				fVar.assign(fPattern);
			}
			
			return fVar;
		}

		void fixupSelfStyleAttributes(IRenderSVG* , IAmGroot* ) override
		{
			fSrcSpan = getAttribute("src");

			parse64i(getAttribute("capX"), fCapX);
			parse64i(getAttribute("capY"), fCapY);
			parse64i(getAttribute("capWidth"), fCapWidth);
			parse64i(getAttribute("capHeight"), fCapHeight);
		}
		
		void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
		{

			// We need to resolve the size of the user space
			// start out with some information from groot
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			// The width and height can default to the size of the canvas
			// we are rendering to.
			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}


			BLRect cFrame = ctx->viewport();
			w = cFrame.w;
			h = cFrame.h;


			fDimX.loadFromChunk(getAttribute("x"));
			fDimY.loadFromChunk(getAttribute("y"));
			fDimWidth.loadFromChunk(getAttribute("width"));
			fDimHeight.loadFromChunk(getAttribute("height"));
			
			
			// BUGBUG - need to get the dpi and canvas size to calculate these properly
			fX = fDimX.calculatePixels();
			fY = fDimY.calculatePixels();

			fWidth = fDimWidth.calculatePixels();
			fHeight = fDimHeight.calculatePixels();

			if (fSrcSpan)
			{
				auto displayName = toString(fSrcSpan);

				// Setup the screen snapper
				HDC dc = DisplayMonitor::createDC(displayName.c_str());
				
				// BUGBUG
				// If capture width has not been set, then get it from 
				// the DC
				
				// Setup the screen snapshot
				if (fWidth > 0 && fHeight > 0) {
					fSnapper.reset((int)fCapX, (int)fCapY, (int)fCapWidth, (int)fCapHeight, (int)fWidth, (int)fHeight, dc);
				}
				else {
					fSnapper.reset((int)fCapX, (int)fCapY, (int)fCapWidth, (int)fCapHeight, dc);
				}
			}

			if (!fDimWidth.isSet())
				fWidth = (double)fSnapper.width();
			if (!fDimHeight.isSet())
				fHeight = (double)fSnapper.height();

			fSnapper.update();
		}

		
		void updateSelf(IAmGroot* ) override
		{
			fSnapper.update();
		}
		
		void drawSelf(IRenderSVG* ctx, IAmGroot* ) override
		{	
			int lWidth = (int)fSnapper.width();
			int lHeight = (int)fSnapper.height();

			ctx->scaleImage(this->fSnapper.getImage(), 0, 0, lWidth, lHeight, fX, fY, fWidth, fHeight);
			ctx->flush();
		}
		
	};
}