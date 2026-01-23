#pragma once

// http://www.w3.org/TR/SVG11/feature#Style
// 
// 

#include <functional>
#include <unordered_map>

#include "svgattributes.h"
#include "svgstructuretypes.h"

namespace waavs {
	//============================================================
	//	SVGStyleNode
	// 
	// The content can be either a CDATA section that contains a style sheet
    // Or the style sheet can be in the element text itself.
	// 
	// This node exists to load the stylesheet into the root document
	// object.  It should not become a part of the document tree itself.
	//============================================================
	struct SVGStyleNode :public SVGGraphicsElement
	{
		static void registerFactory()
		{
			registerContainerNode("style",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGStyleNode>(groot);
					node->loadFromXmlPull(iter, groot);
					return node;
				});
		}


		SVGStyleNode(IAmGroot* ) :SVGGraphicsElement()
		{
			setIsStructural(false);
		}

		void loadContentNode(const XmlElement& elem, IAmGroot* groot) override
		{
			loadCDataNode(elem, groot);
		}

		void loadCDataNode(const XmlElement& elem, IAmGroot* groot) override
		{
			if (groot)
			{
				groot->styleSheet()->loadFromSpan(elem.data());
			}
		}
	};

}