#pragma once

#include "svgstructuretypes.h"

#include "screensnapshot.h"
#include "gmonitor.h"


#include <memory>
#include <vector>


namespace waavs {
	//
	// DisplayCaptureElement
	// This is a source of paint, just like a SVGImage
	// It captures from the user's screen
	// Return value as variant(), and it can be used in all 
	// the same places a SVGImage can be used 
	//
	struct DisplayCaptureElement : public SVGVisualNode
	{
		static void registerFactory() {
			registerSVGSingularNode("displayCapture",
				[](IAmGroot* root, const XmlElement& elem) {
					auto node = std::make_shared<DisplayCaptureElement>(root);
					node->loadFromXmlElement(elem);
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



		DisplayCaptureElement(IAmGroot* aroot) :SVGVisualNode(aroot) {}

		const BLVar getVariant() override
		{
			if (fVar.isNull())
			{
				fSnapper.update();
				fPattern.setImage(fSnapper.getImage());
				fVar.assign(fPattern);
			}
			
			return fVar;
		}

		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
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

			if (nullptr != container) {
				BLRect cFrame = container->getBBox();
				w = cFrame.w;
				h = cFrame.h;
			}

			
			fSrcSpan = getAttribute("src");

			if (getAttribute("capX"))
				fCapX = toInteger(getAttribute("capX"));
			if (getAttribute("capY"))
				fCapY = toInteger(getAttribute("capY"));
			if (getAttribute("capWidth"))
				fCapWidth = toInteger(getAttribute("capWidth"));
			if (getAttribute("capHeight"))
				fCapHeight = toInteger(getAttribute("capHeight"));

			fDimX.loadFromChunk(getAttribute("x"));
			fDimY.loadFromChunk(getAttribute("y"));
			fDimWidth.loadFromChunk(getAttribute("width"));
			fDimHeight.loadFromChunk(getAttribute("height"));
		}
		
		void bindToGroot(IAmGroot* groot, SVGViewable* container) override
		{
			SVGVisualNode::bindToGroot(groot, container);

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
				//HDC dc = nullptr;
				
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

		}

		void loadVisualProperties(const XmlAttributeCollection& attrs, IAmGroot* groot) override
		{
			SVGVisualNode::loadVisualProperties(attrs, groot);



		}
		
		void update(IAmGroot* groot) override
		{
			fSnapper.update();
		}
		
		void drawSelf(IRenderSVG *ctx)
		{
			fSnapper.update();
			
			int lWidth = (int)fSnapper.width();
			int lHeight = (int)fSnapper.height();

			ctx->scaleImage(this->fSnapper.getImage(), 0, 0, lWidth, lHeight, fX, fY, fWidth, fHeight);
			ctx->flush();

		}
		
	};
}