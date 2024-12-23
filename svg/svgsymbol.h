#pragma once

#include "svgcontainer.h"

namespace waavs {
	//===========================================
	// SVGSymbolNode
	// Notes:  An SVGSymbol can create its own local coordinate system
	// if a viewBox is specified.  We need to take into account the aspectRatio
	// as well to be totally correct.
	// As an enhancement, we'd like to take into account style information
	// such as allowing the width and height to be specified in an style
	// sheet or inline style attribute
	//===========================================
	struct SVGSymbolNode : public SVGContainer
	{
		static void registerFactory()
		{
			registerContainerNode("symbol",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGSymbolNode>(groot);
					node->loadFromXmlIterator(iter, groot);

					return node;
				});
		}

		SVGDimension fDimRefX{};
		SVGDimension fDimRefY{};

		//BLRect fSymbolBoundingBox{};
		//BLPoint fSymbolContentScale{1,1};
		BLPoint fSymbolContentTranslation{};



		SVGSymbolNode(IAmGroot* root)
			: SVGContainer()
		{
			isStructural(false);
		}


		void createPortal(IRenderSVG* ctx, IAmGroot* groot)
		{
			/*
			BLRect objectBoundingBox = ctx->objectFrame();
			//BLRect containerBoundingBox = ctx->localFrame();

			// Load parameters for the portal
			SVGDimension fDimX{};
			SVGDimension fDimY{};
			SVGDimension fDimWidth{};
			SVGDimension fDimHeight{};

			fDimX.loadFromChunk(getAttribute("x"));
			fDimY.loadFromChunk(getAttribute("y"));
			fDimWidth.loadFromChunk(getAttribute("width"));
			fDimHeight.loadFromChunk(getAttribute("height"));

			// Load parameters for the viewbox
			haveViewbox = parseViewBox(getAttribute("viewBox"), viewboxRect);

			double x, y, w, h = 0;

			x = fDimX.calculatePixels(objectBoundingBox.w);
			y = fDimY.calculatePixels(objectBoundingBox.h);
			w = fDimWidth.calculatePixels(objectBoundingBox.w);
			h = fDimHeight.calculatePixels(objectBoundingBox.h);

			// If we have a viewbox, we need to calculate the scale and translation
			if (haveViewbox)
			{
				// Calculate the scale
				fSymbolContentScale.x = objectBoundingBox.w / viewboxRect.w;
				fSymbolContentScale.y = objectBoundingBox.h / viewboxRect.h;

				// Calculate the translation
				fSymbolContentTranslation.x = viewboxRect.x;
				fSymbolContentTranslation.y = viewboxRect.y;
			}
			else
			{
				// If we don't have a viewbox, we need to calculate the scale
				fSymbolContentScale.x = w / objectBoundingBox.w;
				fSymbolContentScale.y = h / objectBoundingBox.h;

			}
					*/
		}


		virtual void fixupSelfStyleAttributes(IRenderSVG*, IAmGroot*)
		{
			//SVGContainer::fixupSelfStyleAttributes(nullptr, nullptr);

			fDimRefX.loadFromChunk(getAttribute("refX"));
			fDimRefY.loadFromChunk(getAttribute("refY"));
		}


		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			SVGContainer::drawSelf(ctx, groot);

			//ctx->translate(-fSymbolContentTranslation.x, -fSymbolContentTranslation.y);

		}

	};
}