#pragma once

// SVG Feature String
// http://www.w3.org/TR/SVG11/feature#ConditionalProcessing
//

#include <functional>
#include <unordered_map>

#include "svgattributes.h"
#include "svgstructuretypes.h"


namespace waavs {

	//============================================================
	// 	SVGSwitchElement
	//============================================================
	struct SVGSwitchElement : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			registerContainerNode("switch",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGSwitchElement>(groot);
					node->loadFromXmlIterator(iter, groot);

					return node;
				});
			
		}


		ByteSpan fSystemLanguage;

		std::unordered_map<ByteSpan, std::shared_ptr<IViewable>, ByteSpanHash, ByteSpanEquivalent> fLanguageNodes{};
		std::shared_ptr<IViewable> fDefaultNode{ nullptr };
		std::shared_ptr<IViewable> fSelectedNode{ nullptr };

		SVGSwitchElement(IAmGroot* root) noexcept : SVGGraphicsElement() {}


		void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
		{

			// Get the system language
			fSystemLanguage = groot->systemLanguage();

			// Find the language specific node
			auto iter = fLanguageNodes.find(fSystemLanguage);
			if (iter != fLanguageNodes.end()) {
				fSelectedNode = iter->second;
			}
			else {
				fSelectedNode = fDefaultNode;
			}

			if (nullptr != fSelectedNode)
				fSelectedNode->bindToContext(ctx, groot);
		}

		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			if (fSelectedNode) {
				fSelectedNode->draw(ctx, groot);
			}
		}

		bool addNode(std::shared_ptr<ISVGElement> node, IAmGroot* groot) override
		{
			// If the node has a language attribute, add it to the language map
			auto lang = node->getVisualProperty("systemLanguage");
			if (lang) {
				fLanguageNodes[lang->rawValue()] = node;
			}
			else {
				fDefaultNode = node;
			}

			return true;
		}


	};

}
