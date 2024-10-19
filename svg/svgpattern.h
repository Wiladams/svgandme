#pragma once

// Support for SVGPatternElement
// http://www.w3.org/TR/SVG11/feature#Pattern
// pattern
//


#include <functional>

#include "svgattributes.h"
#include "svgstructuretypes.h"
#include "viewport.h"

namespace waavs {

	//============================================================
	// SVGPatternElement
	// https://www.svgbackgrounds.com/svg-pattern-guide/#:~:text=1%20Background-size%3A%20cover%3B%20This%20declaration%20is%20good%20when,5%20Background-repeat%3A%20repeat%3B%206%20Background-attachment%3A%20scroll%20%7C%20fixed%3B
	// https://www.svgbackgrounds.com/category/pattern/
	// https://www.visiwig.com/patterns/
	//============================================================
	struct SVGPatternElement :public SVGContainer
	{
		static void registerSingularNode()
		{
			getSVGSingularCreationMap()["pattern"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGPatternElement>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
				};
		}


		static void registerFactory()
		{
			registerContainerNode("pattern",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGPatternElement>(groot);
					node->loadFromXmlIterator(iter, groot);
					return node;
				});
			

			registerSingularNode();
		}


		SpaceUnitsKind fPatternUnits{ SpaceUnitsKind::SVG_SPACE_OBJECT };
		SpaceUnitsKind fPatternContentUnits{ SpaceUnitsKind::SVG_SPACE_USER };
		BLExtendMode fExtendMode{ BL_EXTEND_MODE_REPEAT };
		
		
		BLMatrix2D fPatternTransform{};
		BLMatrix2D fContentTransform{};
		bool fHasPatternTransform{ false };
		
		ByteSpan fTemplateReference{};
		
		BLImage fCachedImage{};

		BLRect fBBox{};

		ViewPort fViewport{};
		SVGViewbox fViewbox{};


		BLPattern fPattern{};
		BLVar fPatternVar{};

		SVGDimension fDimX{};
		SVGDimension fDimY{};
		SVGDimension fDimWidth{};
		SVGDimension fDimHeight{};
		
		
		SVGPatternElement(IAmGroot* )
			:SVGContainer()
		{
			fPattern.setExtendMode(BL_EXTEND_MODE_PAD);
			fPatternTransform = BLMatrix2D::makeIdentity();
			fContentTransform = BLMatrix2D::makeIdentity();

			isStructural(false);
		}


		
		const BLVar getVariant(IRenderSVG *ctx, IAmGroot *groot) noexcept override
		{ 
			bindToContext(ctx, groot);
			//needsBinding(true);

			// Create image and draw into it right here
			// Att this point, all attributes have been captured
			// and all dimensions are known.
			// So, we can size a bitmap and draw into it
			drawIntoCache(ctx, groot);
			
			BLVar tmpVar{};
			tmpVar = fPattern;
			return tmpVar;
		}
		
		BLRect getBBox() const override
		{
			return fBBox;
		}


		void resolveReference(IRenderSVG *ctx, IAmGroot* groot)
		{
			// Quick return if there is no template reference
			if (!fTemplateReference)
				return;
			
			// return early if we can't do a lookup
			if (!groot)
				return;

			// Try to find the referenced node
			auto node = groot->findNodeByHref(fTemplateReference);

			if (!node)
				return;

			// Make sure the template binds, so we can get values out of it
			BLVar aVar = node->getVariant(ctx, groot);
			
			// Cast the node to a SVGPatternElement, so we get get some properties from it
			auto pattNode = dynamic_pointer_cast<SVGPatternElement>(node);

			if (!pattNode)
				return;


			// save patternUnits
			// save patternContentUnits
			fPatternUnits = pattNode->fPatternUnits;
			fPatternContentUnits = pattNode->fPatternContentUnits;
			
			
			if (aVar.isPattern())
			{
				BLPattern& tmpPattern = aVar.as<BLPattern>();

				// pull out whatever values we can inherit
				fPattern = tmpPattern;

				if (tmpPattern.hasTransform())
				{
					fPattern.setTransform(tmpPattern.transform());
				}
			}
		}

		void fixupSelfStyleAttributes(IRenderSVG*, IAmGroot*) override
		{
			// See if we have a template reference
			if (getAttribute("href"))
				fTemplateReference = getAttribute("href");
			else if (getAttribute("xlink:href"))
				fTemplateReference = getAttribute("xlink:href");

		}
		
		// 
		// Resolve template reference
		// fix coordinate system
		//

		void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
		{
			resolveReference(ctx, groot);
			createPortal(ctx, groot);
			
			// We need to resolve the size of the user space
			// start out with some information from groot
			double dpi = 96;


			// The width and height can default to the size of the canvas
			// we are rendering to.
			if (groot)
				dpi = groot->dpi();





			fDimX.loadFromChunk(getAttribute("x"));
			fDimY.loadFromChunk(getAttribute("y"));
			fDimWidth.loadFromChunk(getAttribute("width"));
			fDimHeight.loadFromChunk(getAttribute("height"));
			fViewbox.loadFromChunk(getAttribute("viewBox"));
			
			getEnumValue(SVGSpaceUnits, getAttribute("patternUnits"), (uint32_t&)fPatternUnits);
			getEnumValue(SVGSpaceUnits, getAttribute("patternContentUnits"), (uint32_t&)fPatternContentUnits);
			fHasPatternTransform = parseTransform(getAttribute("patternTransform"), fPatternTransform);

			// We use the 'patternUnits' to determine the coordinate system for evaluating
			// x,y,width,height
			// Which are then used to calculate the size of the backing store
			// we will draw into (fBBox)
			if (fPatternUnits == SVG_SPACE_OBJECT)
			{
				// If we're using the object bounding box, we need to get the bounding box
				// of the currently active object
				// then coordinates should be percentage, or decimal (0..1) of those dimensions 
				BLRect oFrame = ctx->objectFrame();
				double w = oFrame.w;
				double h = oFrame.h;
				auto x = oFrame.x;
				auto y = oFrame.y;
				
				// X position
				if (fDimX.isSet()) {
					if (fDimX.fUnits == SVG_LENGTHTYPE_NUMBER) {
						if (fDimX.value() <= 1.0)
							fBBox.x = x + (fDimX.value() * w);
						else
							fBBox.x = x + fDimX.value();
					}
					else
						fBBox.x = x + fDimX.calculatePixels(w, 0, dpi);
				}
				else
					fBBox.x = x + 0;
				
				// Y position
				if (fDimY.isSet()) {
					if (fDimY.fUnits == SVG_LENGTHTYPE_NUMBER) {
						if (fDimY.value() <= 1.0)
							fBBox.y = y + (fDimY.value() * h);
						else
							fBBox.y = y + fDimY.value();
					}
					else
						fBBox.y = y + fDimY.calculatePixels(h, 0, dpi);
				}
				else
					fBBox.y = y + 0;


				// Width
				if (fDimWidth.isSet()) {
					if (fDimWidth.fUnits == SVG_LENGTHTYPE_NUMBER) {
						if (fDimWidth.value() <= 1.0)
							fBBox.w = fDimWidth.value() * w;
						else
							fBBox.w = fDimWidth.value();
					}
					else
						fBBox.w = fDimWidth.calculatePixels(w, 0, dpi);
				}
				else
					fBBox.w = w;

				// Height
				if (fDimHeight.isSet()) {
					if (fDimHeight.fUnits == SVG_LENGTHTYPE_NUMBER) {
						if (fDimHeight.value() <= 1.0)
							fBBox.h = fDimHeight.value() * h;
						else
							fBBox.h = fDimHeight.value();
					}
					else
						fBBox.h = fDimHeight.calculatePixels(h, 0, dpi);
				}
				else
					fBBox.h = h;
			}
			else if (fPatternUnits == SVG_SPACE_USER)
			{
				BLRect cFrame = ctx->localFrame();
				double w = cFrame.w;
				double h = cFrame.h;

				fBBox.x = fDimX.calculatePixels(w, 0, dpi);
				fBBox.y = fDimY.calculatePixels(h, 0, dpi);
				fBBox.w = fDimWidth.calculatePixels(w, 0, dpi);
				fBBox.h = fDimHeight.calculatePixels(h, 0, dpi);
			}

			fViewport.surfaceFrame(BLRect(0,0,fBBox.w, fBBox.h));
			fViewport.sceneFrame(BLRect(0, 0, fBBox.w, fBBox.h));

			// If objectBoundingBox, then coordinates are normalized
			// so we set a viewport of 0,0,1,1, to streth to fit bounding box
			if (fPatternContentUnits == SVG_SPACE_OBJECT) {

				fViewport.sceneFrame(BLRect(0,0,1,1));
				fContentTransform = fViewport.sceneToSurfaceTransform();
			}
			//else if (fPatternContentUnits == SVG_SPACE_USER)
			//{
			//	BLRect cFrame = ctx->localFrame();
			//	double w = cFrame.w;
			//	double h = cFrame.h;

			//	fViewport.sceneFrame(BLRect(0, 0, w, h));
			//}

			fPattern.translate(fBBox.x, fBBox.y);
			
			fPatternTransform = fViewport.sceneToSurfaceTransform();
			//fPatternTransform = fViewport.surfaceToSceneTransform();


			//fPattern.setTransform(fPatternTransform);

			// Whether it was a reference or not, set the extendMode
			getEnumValue(SVGExtendMode, getAttribute("extendMode"), (uint32_t&)fExtendMode);
			fPattern.setExtendMode(fExtendMode);

		}

		void drawIntoCache(IRenderSVG* ctx, IAmGroot* groot)
		{
			auto box = getBBox();

			fCachedImage.create(static_cast<int>(box.w), static_cast<int>(box.h), BL_FORMAT_PRGB32);
			IRenderSVG ictx(ctx->fontHandler());
			ictx.begin(fCachedImage);


			ictx.renew();
			ictx.clear();


			draw(&ictx, groot);

			ictx.flush();
			ictx.end();

			fPattern.setImage(fCachedImage);
		}

		
		void draw(IRenderSVG* ctx, IAmGroot* groot) override
		{
			if (!visible())
				return;

			if (needsBinding())
			{
				bindToContext(ctx, groot);
			}
			
			ctx->push();
			
			ctx->objectFrame(getBBox());
			
			applyProperties(ctx, groot);
			drawSelf(ctx, groot);

			drawChildren(ctx, groot);

			ctx->pop();
		}
		
	};
	
}