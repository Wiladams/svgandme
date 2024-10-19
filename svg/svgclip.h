#pragma once


//
// http://www.w3.org/TR/SVG11/feature#Clip
//

#include <functional>
#include <unordered_map>

#include "svgattributes.h"
#include "svgstructuretypes.h"


namespace waavs {

	//============================================================
	// SVGClipPath
	// Create a SVGSurface that we can draw into
	// get the size by asking for the extent of the enclosed
	// visuals.
	// 
	// At render time, use the clip path in a pattern and fill
	// based on that.
	//============================================================
	struct SVGClipPathElement : public SVGGraphicsElement
	{
		// Static constructor to register factory method in map
		static void registerFactory()
		{
			registerContainerNode("clipPath", [](IAmGroot * groot, XmlElementIterator & iter) {
				auto node = std::make_shared<SVGClipPathElement>(groot);
				node->loadFromXmlIterator(iter, groot);

				return node;
			});
			
		}

		BLImage fImage;		// Where we'll render the mask

		// Instance Constructor
		SVGClipPathElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			isStructural(false);
		}

		// 
		// BUGBUG - this needs to happen in resolvePaint, or bingToGroot()
		//
		const BLVar getVariant(IRenderSVG*, IAmGroot*) noexcept override
		{
			if (!fVar.isNull())
				return fVar;

			// get our frame() extent
			BLRect extent = frame();

			// create a surface of that size
			// if it's valid
			if (extent.w > 0 && extent.h > 0)
			{
				fImage.create((int)floor((float)extent.w + 0.5f), (int)floor((float)extent.h + 0.5f), BL_FORMAT_A8);

				// Draw our content into the image
				{
					IRenderSVG ctx(nullptr);
					ctx.begin(fImage);

					ctx.setCompOp(BL_COMP_OP_SRC_COPY);
					ctx.clearAll();
					ctx.setFillStyle(BLRgba32(0xffffffff));
					ctx.translate(-extent.x, -extent.y);
					draw(&ctx, nullptr);
					ctx.flush();
				}

				// bind our image to fVar for later retrieval
				fVar.assign(fImage);
			}

			return fVar;
		}


	};
}