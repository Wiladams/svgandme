#pragma once

//
// http://www.w3.org/TR/SVG11/feature#Hyperlinking
//

#include <functional>
#include <unordered_map>

#include "svgattributes.h"
#include "svgstructuretypes.h"


namespace waavs {

	//================================================
	// SVGAElement
	// 'a' element
	//================================================
	struct SVGAElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("a", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGAElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			registerContainerNodeByName("a",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGAElement>(groot);
					node->loadFromXmlPull(iter, groot);

					return node;
				});

			registerSingularNode();
		}


		// Instance Constructor
		SVGAElement(IAmGroot* )
			: SVGGraphicsElement() {}

	};
}