#pragma once


//==================================================
// SVGImageNode
// https://www.w3.org/TR/SVG11/struct.html#ImageElement
// Stores embedded or referenced images
//==================================================

#include <functional>
#include <unordered_map>
#include <string>

#include "svgattributes.h"
#include "svgstructuretypes.h"
#include "converters.h"

namespace waavs {
	
	static INLINE std::string toString(const ByteSpan& inChunk) noexcept
	{
		if (!inChunk)
			return std::string();

		return std::string(inChunk.fStart, inChunk.fEnd);
	}

	
	struct SVGImageElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["image"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGImageElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["image"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGImageElement>(groot);
				node->loadFromXmlIterator(iter, groot);

				return node;
				};

			registerSingularNode();
		}


		BLImage fImage{};
		ByteSpan fImageRef{};
		BLVar fImageVar{};

		double fX{ 0 };
		double fY{ 0 };
		double fWidth{ 0 };
		double fHeight{ 0 };




		SVGImageElement(IAmGroot* root)
			: SVGGraphicsElement(root) 
		{
			needsBinding(true);
		}

		BLRect frame() const override
		{

			if (fHasTransform) {
				auto leftTop = fTransform.mapPoint(fX, fY);
				auto rightBottom = fTransform.mapPoint(fX + fWidth, fY + fHeight);
				return BLRect(leftTop.x, leftTop.y, rightBottom.x - leftTop.x, rightBottom.y - leftTop.y);
			}

			return BLRect(fX, fY, fWidth, fHeight);
		}

		BLRect getBBox() const override
		{
			return BLRect(fX, fY, fWidth, fHeight);
		}
		
		const BLVar getVariant() noexcept override
		{
			return fImageVar;
		}

		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
		{
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}
			
			if (nullptr != container)
			{
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}

			SVGDimension fDimX{};
			SVGDimension fDimY{};
			SVGDimension fDimWidth{};
			SVGDimension fDimHeight{};
			
			fDimX.loadFromChunk(getAttribute("x"));
			fDimY.loadFromChunk(getAttribute("y"));
			fDimWidth.loadFromChunk(getAttribute("width"));
			fDimHeight.loadFromChunk(getAttribute("height"));

			fImageRef = getAttribute("href");
			if (!fImageRef)
				fImageRef = getAttribute("xlink:href");

			
			// Parse the image so we can get its dimensions
			if (fImageRef)
			{
				// First, see if it's embedded data
				if (chunk_starts_with_cstr(fImageRef, "data:"))
				{
					bool success = parseImage(fImageRef, fImage);
				}
				else {
					// Otherwise, assume it's a file reference
					auto path = toString(fImageRef);
					if (path.size() > 0)
					{
						fImage.readFromFile(path.c_str());
					}
				}
			}


			fX = 0;
			fY = 0;
			fWidth = fImage.size().w;
			fHeight = fImage.size().h;

			if (fDimX.isSet())
				fX = fDimX.calculatePixels(w, 0, dpi);
			if (fDimY.isSet())
				fY = fDimY.calculatePixels(h, 0, dpi);
			if (fDimWidth.isSet())
				fWidth = fDimWidth.calculatePixels(w, 0, dpi);
			if (fDimHeight.isSet())
				fHeight = fDimHeight.calculatePixels(h, 0, dpi);

			fImageVar = fImage;
		}


		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			if (fImage.empty())
				return;

			BLRect dst{ fX,fY, fWidth,fHeight };
			BLRectI src{ 0,0,fImage.size().w,fImage.size().h };

			ctx->scaleImage(fImage, src.x, src.y, src.w, src.h, fX, fY, fWidth, fHeight);
		}

	};
}
