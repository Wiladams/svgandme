#pragma once

#include "svg/svgstructuretypes.h"

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
			gShapeCreationMap["displayCapture"] = [](IAmGroot* root, const XmlElement& elem) {
				auto node = std::make_shared<DisplayCaptureElement>(root);
				node->loadFromXmlElement(elem);
				return node;
			};
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

		const BLVar& getVariant() override
		{
			if (fVar.isNull())
			{
				fSnapper.update();
				fPattern.setImage(fSnapper.getImage());
				fVar.assign(fPattern);
			}
			
			return fVar;
		}


		void bindToGroot(IAmGroot* groot) override
		{
			SVGVisualNode::bindToGroot(groot);

			// BUGBUG - need to get the dpi and canvas size to calculate these properly
			fX = fDimX.calculatePixels();
			fY = fDimY.calculatePixels();

			fWidth = fDimWidth.calculatePixels();
			fHeight = fDimHeight.calculatePixels();

			if (fSrcSpan)
			{
				auto displayName = toString(fSrcSpan);

				// Setup the screen snapper
				auto dc = DisplayMonitor::createDC(displayName.c_str());

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

		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGVisualNode::loadVisualProperties(attrs);

			if (attrs.getAttribute("src"))
				fSrcSpan = attrs.getAttribute("src");

			if (attrs.getAttribute("capX"))
				fCapX = toInteger(attrs.getAttribute("capX"));
			if (attrs.getAttribute("capY"))
				fCapY = toInteger(attrs.getAttribute("capY"));
			if (attrs.getAttribute("capWidth"))
				fCapWidth = toInteger(attrs.getAttribute("capWidth"));
			if (attrs.getAttribute("capHeight"))
				fCapHeight = toInteger(attrs.getAttribute("capHeight"));

			if (attrs.getAttribute("x"))
				fDimX.loadFromChunk(attrs.getAttribute("x"));
			if (attrs.getAttribute("y"))
				fDimY.loadFromChunk(attrs.getAttribute("y"));
			if (attrs.getAttribute("width"))
				fDimWidth.loadFromChunk(attrs.getAttribute("width"));
			if (attrs.getAttribute("height"))
				fDimHeight.loadFromChunk(attrs.getAttribute("height"));

		}
		
		void update() override
		{
			fSnapper.update();
		}
		
		void drawSelf(IRenderSVG *ctx)
		{
			int lWidth = (int)fSnapper.width();
			int lHeight = (int)fSnapper.height();

			ctx->scaleImage(this->fSnapper.getImage(), 0, 0, lWidth, lHeight, fX, fY, fWidth, fHeight);
			ctx->flush();

		}
		
	};
}