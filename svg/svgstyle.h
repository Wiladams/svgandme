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
	// Content node is a CDATA section that contains a style sheet
	// Or it is just the plain content of the node
	// This node exists to load the stylesheet into the root document
	// object.  It should not become a part of the document tree
	// itself.
	//============================================================
	struct SVGStyleNode :public SVGGraphicsElement
	{
		static void registerFactory()
		{
			registerContainerNode("style",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGStyleNode>(groot);
					node->loadFromXmlIterator(iter, groot);
					return node;
				});
			
		}


		SVGStyleNode(IAmGroot* ) :SVGGraphicsElement()
		{
			isStructural(false);
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