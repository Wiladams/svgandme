#pragma once

// Support for SVGPatternElement
// http://www.w3.org/TR/SVG11/feature#Pattern
// pattern
//


#include <functional>

#include "svgattributes.h"
#include "viewport.h"
#include "svgstructuretypes.h"


namespace waavs {

	//============================================================
	// SVGPatternElement
	// https://www.svgbackgrounds.com/svg-pattern-guide/#:~:text=1%20Background-size%3A%20cover%3B%20This%20declaration%20is%20good%20when,5%20Background-repeat%3A%20repeat%3B%206%20Background-attachment%3A%20scroll%20%7C%20fixed%3B
	// https://www.svgbackgrounds.com/category/pattern/
	// https://www.visiwig.com/patterns/
	//============================================================
	struct SVGPatternElement :public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["pattern"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGPatternElement>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
				};
		}


		static void registerFactory()
		{
			gSVGGraphicsElementCreation["pattern"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGPatternElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				return node;
				};

			registerSingularNode();
		}


		SVGSpaceUnits fPatternUnits{ SVGSpaceUnits::SVG_SPACE_OBJECT };
		BLMatrix2D fPatternTransform{};

		BLPattern fPattern{};

		BLImage fCachedImage{};
		ByteSpan fTemplateReference{};
		BLRect fBBox{};

		ViewPort fViewport{};


		// Raw attribute values
		SVGViewbox fViewbox{};

		SVGPatternExtendMode fExtendMode{ nullptr };

		SVGPatternElement(IAmGroot* aroot)
			:SVGGraphicsElement(aroot)
		{
			fPattern.setExtendMode(BL_EXTEND_MODE_PAD);
			fPatternTransform = BLMatrix2D::makeIdentity();

			isStructural(false);
		}

		BLRect getBBox() const override
		{
			return fBBox;
		}


		void resolveReferences(IAmGroot* groot, const ByteSpan& inChunk)
		{
			// return early if we can't do a lookup
			if (nullptr == groot)
			{
				printf("SVGPatternNode::resolveReferences - groot == null\n");
				return;
			}

			ByteSpan str = inChunk;

			auto node = groot->findNodeByHref(inChunk);


			// return early if we could not lookup the node
			if (nullptr == node)
			{
				printf("SVGPatternNode::resolveReferences - node == null\n");
				return;
			}


			// That node itself might need to be bound
			//if (node->needsBinding())
			node->bindToGroot(groot, this);

			const BLVar& aVar = node->getVariant();

			if (aVar.isPattern())
			{
				const BLPattern& tmpPattern = aVar.as<BLPattern>();

				// pull out whatever values we can inherit
				fPattern = tmpPattern;
			}
		}

		// 
		// Resolve template reference
		// fix coordinate system
		//
		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
		{
			if (nullptr == groot)
				return;


			// We need to resolve the size of the user space
			// start out with some information from groot
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			// The width and height can default to the size of the canvas
			// we are rendering to.
			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}

			if (nullptr != container) {
				BLRect cFrame = container->getBBox();
				w = cFrame.w;
				h = cFrame.h;
			}


			SVGDimension fDimX{};
			SVGDimension fDimY{};
			SVGDimension fDimWidth{};
			SVGDimension fDimHeight{};

			fDimX.loadFromChunk(getAttribute("x"));
			fDimY.loadFromChunk(getAttribute("y"));
			fDimWidth.loadFromChunk(getAttribute("width"));
			fDimHeight.loadFromChunk(getAttribute("height"));


			fBBox.x = fDimX.calculatePixels(w, 0, dpi);
			fBBox.y = fDimY.calculatePixels(h, 0, dpi);
			fBBox.w = fDimWidth.calculatePixels(w, 0, dpi);
			fBBox.h = fDimHeight.calculatePixels(h, 0, dpi);

			fViewport.sceneFrame(getBBox());


			parseSpaceUnits(getAttribute("patternUnits"), fPatternUnits);
			fViewbox.loadFromChunk(getAttribute("viewBox"));
			parseTransform(getAttribute("patternTransform"), fPatternTransform);


			if (fViewbox.isSet()) {
				fViewport.sceneFrame(fViewbox.fRect);

				if (!fDimWidth.isSet() || !fDimHeight.isSet()) {
					fBBox = fViewbox.fRect;
				}

			}
			else if (!fDimWidth.isSet() || !fDimHeight.isSet()) {
				fBBox.w = w;
				fBBox.h = h;
				fViewport.sceneFrame(getBBox());
			}

			fViewport.surfaceFrame(getBBox());



			// See if we have a template reference
			if (getAttribute("href"))
				fTemplateReference = getAttribute("href");
			else if (getAttribute("xlink:href"))
				fTemplateReference = getAttribute("xlink:href");

			//
			// Resolve the templateReference if it exists
			if (fTemplateReference) {
				resolveReferences(groot, fTemplateReference);
			}

			// Whether it was a reference or not, set the extendMode
			fExtendMode.loadFromChunk(getAttribute("extendMode"));
			if (fExtendMode.isSet())
			{
				fPattern.setExtendMode(fExtendMode.value());
			}
			else {
				fPattern.setExtendMode(BL_EXTEND_MODE_REPEAT);

			}
			/*
						else {
							//BLRectI bbox{};
							// start out with an initial size for the backing buffer
							int iWidth = (int)cWidth;
							int iHeight = (int)cHeight;

							// If a viewbox exists, then it determines the size
							// of the pattern, and thus the size of the backing store
							if (fViewbox.isSet())
							{
								if (fDimWidth.isSet() && fDimHeight.isSet())
								{
									iWidth = (int)fDimWidth.calculatePixels();
									iHeight = (int)fDimHeight.calculatePixels();
								}
								else {

									// If a viewbox is set, then the 'width' and 'height' parameters
									// are ignored
									//bbox.reset(0, 0, fViewbox.width(), fViewbox.height());
									iWidth = (int)fViewbox.width();
									iHeight = (int)fViewbox.height();
								}
							}
							else {
								// If we don't have a viewbox, then the bbox can be either
								// 1) A specified width and height
								// 2) A percentage width and height of the pattern's children

								if (fDimWidth.isSet() && fDimHeight.isSet())
								{
									BLRect bbox = frame();
									iWidth = (int)bbox.w;
									iHeight = (int)bbox.h;


									// if the units on the fWidth == '%', then take the size of the pattern box
									// and use that as the width, and then calculate the percentage of that
									if (fDimWidth.units() == SVGDimensionUnits::SVG_UNITS_USER)
									{
										if (fDimWidth.value() < 1.0)
										{
											iWidth = (int)(bbox.w * fDimWidth.value());
										}
										else
										{
											iWidth = (int)fDimWidth.value();
										}
										if (fDimHeight.value() < 1.0)
										{
											iHeight = (int)(bbox.h * fDimHeight.value());
										}
										else
										{
											iHeight = (int)fDimHeight.value();
										}
										//iWidth = (int)fWidth.calculatePixels(iWidth,0,dpi);
										//iHeight = (int)fHeight.calculatePixels(iHeight,0,dpi);
									}
									else {
										//iWidth = (int)fWidth.calculatePixels();
										//iHeight = (int)fHeight.calculatePixels();

										//iWidth = (int)fWidth.calculatePixels(bbox.w, 0, dpi);
										//iHeight = (int)fHeight.calculatePixels(bbox.h, 0, dpi);
									}

								}
								else {
									BLRect bbox = frame();
									iWidth = (int)bbox.w;
									iHeight = (int)bbox.h;
								}
							}

							width = iWidth;
							height = iHeight;

						}
						*/



		}

		virtual void bindToGroot(IAmGroot* groot, SVGViewable* container)
		{
			SVGGraphicsElement::bindToGroot(groot, container);

			// create the backing buffer based on the specified sizes
			fCachedImage.create(fBBox.w, fBBox.h, BL_FORMAT_PRGB32);

			// Render out content into the backing buffer
			IRenderSVG ctx(groot->fontHandler());
			ctx.begin(fCachedImage);
			ctx.setCompOp(BL_COMP_OP_SRC_COPY);
			ctx.clearAll();
			ctx.noStroke();

			draw(&ctx, groot);

			ctx.flush();
			ctx.end();

			// apply the patternTransform
			// maybe do a translate on x, y?

			// assign that BLImage to the pattern object
			fPattern.setTransform(fPatternTransform);
			fPattern.setImage(fCachedImage);

			fVar = fPattern;

		}

	};
	
}