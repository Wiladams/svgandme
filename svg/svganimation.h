#pragma once

#include <cmath>
#include <cstdint>

#include "svgstructuretypes.h"
#include "wsenum.h"
#include "smildatatypes.h"




namespace waavs {
	enum AnimRestartKind : uint32_t {
		SVG_ANIM_RESTART_ALWAYS,
		SVG_ANIM_RESTART_NEVER,
		SVG_ANIM_RESTART_WHEN_NOT_ACTIVE
	};

	static WSEnum SVGAnimRestart = {
		{ "always", AnimRestartKind::SVG_ANIM_RESTART_ALWAYS },
		{ "never", AnimRestartKind::SVG_ANIM_RESTART_NEVER },
		{ "whenNotActive", AnimRestartKind::SVG_ANIM_RESTART_WHEN_NOT_ACTIVE }
	};

	enum AnimFillKind : uint32_t {
		SVG_ANIM_FILL_REMOVE,
		SVG_ANIM_FILL_FREEZE
	};

	static WSEnum SVGAnimFill = {
		{ "remove", AnimFillKind::SVG_ANIM_FILL_REMOVE },
		{ "freeze", AnimFillKind::SVG_ANIM_FILL_FREEZE }
	};

	enum AnimAdditiveKind : uint32_t {
		SVG_ANIM_ADD_REPLACE,
		SVG_ANIM_ADD_SUM
	};

	static WSEnum SVGAnimAdditive = {
		{ "replace", AnimAdditiveKind::SVG_ANIM_ADD_REPLACE },
		{ "sum", AnimAdditiveKind::SVG_ANIM_ADD_SUM }
	};

	enum AnimAccumulateKind : uint32_t {
		SVG_ANIM_ACCUM_NONE,
		SVG_ANIM_ACCUM_SUM
	};

	static WSEnum SVGAnimAccumulate = {
		{ "none", AnimAccumulateKind::SVG_ANIM_ACCUM_NONE },
		{ "sum", AnimAccumulateKind::SVG_ANIM_ACCUM_SUM }
	};

	enum AnimCalcModeKind : uint32_t {
		SVG_ANIM_CALC_DISCRETE,
		SVG_ANIM_CALC_LINEAR,
		SVG_ANIM_CALC_PACED,
		SVG_ANIM_CALC_SPLINE
	};

	static WSEnum SVGAnimCalcMode = {
		{ "discrete", AnimCalcModeKind::SVG_ANIM_CALC_DISCRETE },
		{ "linear", AnimCalcModeKind::SVG_ANIM_CALC_LINEAR },
		{ "paced", AnimCalcModeKind::SVG_ANIM_CALC_PACED },
		{ "spline", AnimCalcModeKind::SVG_ANIM_CALC_SPLINE }
	};

	enum AnimAttributeTypeKind : uint32_t {
		SVG_ANIM_ATTR_TYPE_AUTO,
		SVG_ANIM_ATTR_TYPE_CSS,
		SVG_ANIM_ATTR_TYPE_XML
	};

	static WSEnum SVGAnimAttributeType = {
		{ "auto", AnimAttributeTypeKind::SVG_ANIM_ATTR_TYPE_AUTO },
		{ "css", AnimAttributeTypeKind::SVG_ANIM_ATTR_TYPE_CSS },
		{ "xml", AnimAttributeTypeKind::SVG_ANIM_ATTR_TYPE_XML }
	};




	struct SVGAnimateElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNode("animate", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGAnimateElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		static void registerFactory()
		{
			registerContainerNode("animate", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGAnimateElement>(groot);
				node->loadFromXmlPull(iter, groot);
				node->visible(false);

				return node;
				});

			registerSingularNode();
		}

		AnimFillKind fAnimFill{ AnimFillKind::SVG_ANIM_FILL_REMOVE };
		AnimRestartKind fAnimRestart{ AnimRestartKind::SVG_ANIM_RESTART_ALWAYS };
		AnimAdditiveKind fAnimAdditive{ AnimAdditiveKind::SVG_ANIM_ADD_REPLACE };
		AnimAccumulateKind fAnimAccumulate{ AnimAccumulateKind::SVG_ANIM_ACCUM_NONE };
		AnimCalcModeKind fAnimCalcMode{ AnimCalcModeKind::SVG_ANIM_CALC_LINEAR };
		AnimAttributeTypeKind fAnimAttributeType{ AnimAttributeTypeKind::SVG_ANIM_ATTR_TYPE_AUTO };


		SVGAnimateElement(IAmGroot*)
			: SVGGraphicsElement()
		{
			isStructural(true);
		}

		// Attributes
		// %timingAttrs
		//	begin
		//	dur
		//	end
		//	restart (always|never|whenNotActive) "always"
		//	repeatCount
		//	repeatDur
		//	fill (remove|freeze) "remove"
		//
		// %animAttrs
		//	attributeName
		//	attributeType
		//	additive (replace|sum) "replace"
		//	accumulate (none|sum) "none"
		//
		//	calcMode (discrete|linear|paced|spline) "linear"
		//	values
		//	keyTimes
		//	keySplines
		//	from
		//	to
		//	by
		//
		// %animTargetAttrs
		//	targetElement	IDREF	#IMPLIED

		// Read in all the attributes for the animation
		void fixupSelfStyleAttributes(IRenderSVG*, IAmGroot*) override
		{
			getEnumValue(SVGAnimFill, getAttribute("fill"), (uint32_t&)fAnimFill);
			getEnumValue(SVGAnimRestart, getAttribute("restart"), (uint32_t&)fAnimRestart);
			getEnumValue(SVGAnimAdditive, getAttribute("additive"), (uint32_t&)fAnimAdditive);
			getEnumValue(SVGAnimAccumulate, getAttribute("accumulate"), (uint32_t&)fAnimAccumulate);
			getEnumValue(SVGAnimCalcMode, getAttribute("calcMode"), (uint32_t&)fAnimCalcMode);
			getEnumValue(SVGAnimAttributeType, getAttribute("attributeType"), (uint32_t&)fAnimAttributeType);

		}
	};
}
