#pragma once


#include <memory>
#include <vector>

#include "wsenum.h"

#include "svgstructuretypes.h"
#include "converters.h"


namespace waavs {

	/*
		<strokeProfile
			id="..."
			units="objectBoundingBox|userSpaceOnUse"
			spreadMethod="pad|repeat|reflect">

			<profileStop offset="number" width="number" easing="linear|ease-in|ease-out|ease-in-out|step-start|step-end|cubic-bezier(...)"/>
			...
			</strokeProfile>

		| Attribute      | Description                                                                      |
		| -------------- | -------------------------------------------------------------------------------- |
		| `id`           | Identifier for use with `url(#...)` reference                                    |
		| `units`        | Interprets `offset` values as `0.0�1.0` (objectBoundingBox) or absolute length (userSpaceOnUse         |
		| `spreadMethod` | Controls how values beyond offset range are handled (`pad`, `repeat`, `reflect`) |

	*/



	// Use enum: SVGSpaceUnits from gradients (userSpaceOnUse, objectBoundingBox)
	// Use enum: SVGSpreadMethod from gradients (pad, repeat, reflect)

	struct WaavsStrokeProfile : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			registerContainerNode("strokeProfile",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<WaavsStrokeProfile>();
					node->loadFromXmlIterator(iter, groot);

					return node;
				});

			//registerSingularNode();
		}


		SpaceUnitsKind fGradientUnits{ SVG_SPACE_OBJECT };
		BLExtendMode fSpreadMethod{ BL_EXTEND_MODE_PAD };


		WaavsStrokeProfile()
			:SVGGraphicsElement() 
		{
			isStructural(false);

		}


		void fixupSelfStyleAttributes(IRenderSVG*, IAmGroot*) override
		{
			// The id has already been grabbed separately

			getEnumValue(SVGSpreadMethod, getAttribute("spreadMethod"), (uint32_t&)fSpreadMethod);
			getEnumValue(SVGSpaceUnits, getAttribute("gradientUnits"), (uint32_t&)fGradientUnits);

		}

	};

	//
	// WaavsStrokeBrush
	// This is a source of paint, just like a SVGImage
	// It captures from the user's screen
	// Return value as variant(), and it can be used in all 
	// the same places a SVGImage can be used 
	//
	struct WaavsStrokeBrush : public SVGGraphicsElement
	{
		static void registerFactory() {
			registerSVGSingularNode("strokeBrush",
				[](IAmGroot* groot, const XmlElement& elem) {
					auto node = std::make_shared<WaavsStrokeBrush>(groot);
					node->loadFromXmlElement(elem, groot);
					return node;
				});
		}
		WaavsStrokeBrush(IAmGroot*)
			:SVGGraphicsElement() {
		}

		const BLVar getVariant(IRenderSVG* ctx, IAmGroot* groot) noexcept override
		{
			return {};
		}
	};
}