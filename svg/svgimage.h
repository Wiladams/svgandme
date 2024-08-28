#pragma once


//==================================================
// SVGImageNode
// https://www.w3.org/TR/SVG11/struct.html#ImageElement
// Stores embedded or referenced images
//==================================================

#include <functional>
#include <unordered_map>

#include "svgattributes.h"
#include "svgstructuretypes.h"

namespace waavs {
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
		
		const BLVar getVariant() override
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

			
			// Parse the image so we can get its dimensions
			if (fImageRef)
			{
				// First, see if it's embedded data
				if (chunk_starts_with_cstr(fImageRef, "data:"))
				{
					bool success = parseImage(fImageRef, fImage);
					//printf("SVGImageNode::fImageRef, parseImage: %d\n", success);
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

			// BUGBUG - sanity check of image data
			//BLImageData imgData{};
			//fImage.getData(&imgData);
			//printf("imgData: %d x %d  depth: %d\n", imgData.size.w, imgData.size.h, fImage.depth());

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

		virtual void loadVisualProperties(const XmlAttributeCollection& attrs, IAmGroot* groot)
		{
			SVGGraphicsElement::loadVisualProperties(attrs, groot);

			//printf("SVGImageNode: %3.0f %3.0f\n", fWidth, fHeight);
			ByteSpan href = attrs.getAttribute("href");
			if (!href)
				href = attrs.getAttribute("xlink:href");

			if (href)
				fImageRef = href;
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
