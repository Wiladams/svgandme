#pragma once


//
// http://www.w3.org/TR/SVG11/feature#Mask
//

#include <functional>
#include <unordered_map>

#include "svgattributes.h"
#include "svgstructuretypes.h"


namespace waavs {

	//================================================
	// SVGMaskNode
	// 'mask' element
	// Similar to SVGClipPath, except uses SRC_OUT
	//================================================
	struct SVGMaskElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			getSVGSingularCreationMap()["mask"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGMaskElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			registerContainerNode("mask",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGMaskElement>(groot);
					node->loadFromXmlIterator(iter, groot);

					return node;
				});
			

			registerSingularNode();
		}


		// Instance Constructor
		SVGMaskElement(IAmGroot* groot)
			: SVGGraphicsElement()
		{
			isStructural(false);
			//fUseCacheIsolation = true;
		}


		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			//The parent graphic should be stuffed into a BLPattern
			// and set as a fill style, then we can 
			// do this fillMask
			// parent->getVar()
			// BLPattern* pattern = new BLPattern(parent->getVar());
			// ctx->setFillStyle(pattern);
			//ctx->fillMask(BLPoint(0, 0), fCachedImage);

		}
	};
}