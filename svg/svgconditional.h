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
			registerContainerNodeByName("switch",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGSwitchElement>(groot);
					node->loadFromXmlPull(iter, groot);

					return node;
				});
			
		}


		ByteSpan fSystemLanguage;

		std::unordered_map<ByteSpan, std::shared_ptr<IViewable>, ByteSpanHash, ByteSpanEquivalent> fLanguageNodes{};
		std::shared_ptr<IViewable> fDefaultNode{ nullptr };
		std::shared_ptr<IViewable> fSelectedNode{ nullptr };

		SVGSwitchElement(IAmGroot* root) noexcept : SVGGraphicsElement() {}

		bool addNode(std::shared_ptr<IViewable> node, IAmGroot* groot) override
		{
			// If the node has a language attribute, add it to the language map
			// IViewable doesn't have the getVisualProperty method, so we need to dynamic_cast to ISVGElement
			auto svgNode = std::dynamic_pointer_cast<ISVGElement>(node);

			ByteSpan lang{};
            if (svgNode->getRawAttributeBySpan(svgattr::systemLanguage(), lang))
			{
				fLanguageNodes[lang] = node;
			}
			else {
				fDefaultNode = node;
			}

			return true;
		}


		void fixupSelfStyleAttributes(IAmGroot* groot) override
		{
			// systemLanguage
			fSystemLanguage = getAttribute(svgattr::systemLanguage());
        }

		void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
		{

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




	};

}
