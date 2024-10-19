#pragma once

//
// http://www.w3.org/TR/SVG11/feature#Extensibility
//

#include <functional>
#include <unordered_map>

#include "svgattributes.h"
#include "svgstructuretypes.h"


namespace waavs {

	//================================================
	// SVGForeignObjectElement
	//================================================
	struct SVGForeignObjectElement : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			registerContainerNode("foreignObject",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGForeignObjectElement>(groot);
					node->loadFromXmlIterator(iter, groot);

					return node;
				});
		}

		double fX{ 0 };
		double fY{ 0 };
		double fWidth{ 0 };
		double fHeight{ 0 };



		// Instance Constructor
		SVGForeignObjectElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot) {}

		void resolvePosition(IRenderSVG *ctx, IAmGroot* groot) override
		{
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (groot)
			{
				dpi = groot->dpi();
			}
			
			auto cFrame = ctx->localFrame();
			w = cFrame.w;
			h = cFrame.h;
			

			// The intended destination
			SVGDimension fDimX{};
			SVGDimension fDimY{};
			SVGDimension fDimWidth{};
			SVGDimension fDimHeight{};
			
			fDimX.loadFromChunk(getAttribute("x"));
			fDimY.loadFromChunk(getAttribute("y"));
			fDimWidth.loadFromChunk(getAttribute("width"));
			fDimHeight.loadFromChunk(getAttribute("height"));
			
			fX = fDimX.calculatePixels(w, 0, dpi);
			fY = fDimY.calculatePixels(h, 0, dpi);
			fWidth = fDimWidth.calculatePixels(w, 0, dpi);
			fHeight = fDimHeight.calculatePixels(h, 0, dpi);

		}



	};
}