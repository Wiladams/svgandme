#pragma once

// Support for SVGPatternElement
// http://www.w3.org/TR/SVG11/feature#Pattern
// pattern
//


#include <functional>
#include <memory>

#include "svgattributes.h"
#include "svgstructuretypes.h"
#include "svgportal.h"
#include "svgb2ddriver.h"

namespace waavs {

	//============================================================
	// SVGPatternElement
	// https://www.svgbackgrounds.com/svg-pattern-guide/#:~:text=1%20Background-size%3A%20cover%3B%20This%20declaration%20is%20good%20when,5%20Background-repeat%3A%20repeat%3B%206%20Background-attachment%3A%20scroll%20%7C%20fixed%3B
	// https://www.svgbackgrounds.com/category/pattern/
	// https://www.visiwig.com/patterns/
	//============================================================
	struct SVGPatternElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("pattern", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGPatternElement>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
				});
		}


		static void registerFactory()
		{
			registerContainerNodeByName("pattern",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGPatternElement>(groot);
					node->loadFromXmlPull(iter, groot);
					return node;
				});
			

			registerSingularNode();
		}


		
		ByteSpan fTemplateReference{};		// A referred to pattern as template
		
		
		SpaceUnitsKind fPatternUnits{ SpaceUnitsKind::SVG_SPACE_OBJECT };
		SpaceUnitsKind fPatternContentUnits{ SpaceUnitsKind::SVG_SPACE_USER };
		BLExtendMode fExtendMode{ BL_EXTEND_MODE_REPEAT };
		
		
		BLMatrix2D fPatternTransform{};
		BLMatrix2D fContentTransform{};
		bool fHasPatternTransform{ false };
		

		SVGPortal fPortal;
		BLRect viewboxRect{};
		bool haveViewbox{ false };
		PreserveAspectRatio fPreserveAspectRatio{};
		
		
		// Things we'll need to calculate
		BLRect fObjectBoundingBox{};
		BLRect fPatternBoundingBox{};
		BLPoint fPatternOffset{ 0,0 };
		BLPoint fPatternContentScale{ 1.0,1.0 };
		




		BLImage fPatternCache{};
		BLPattern fPattern{};
		BLVar fPatternVar{};

		
		
		SVGPatternElement(IAmGroot* )
			:SVGGraphicsElement()
		{
			fPattern.setExtendMode(BL_EXTEND_MODE_PAD);
			fPatternTransform = BLMatrix2D::makeIdentity();
			fContentTransform = BLMatrix2D::makeIdentity();

			setIsStructural(false);
		}


		
		const BLVar getVariant(IRenderSVG *ctx, IAmGroot *groot) noexcept override
		{ 
			//bindToContext(ctx, groot);


			// Create image and draw into it right here
			// At this point, all attributes have been captured
			// and all dimensions are known.
			// So, we can size a bitmap and draw into it
			drawIntoCache(ctx, groot);
			
			BLVar tmpVar{};
			tmpVar = fPattern;
			return tmpVar;
		}
		
		BLRect getBBox() const override
		{
			return fPatternBoundingBox;
		}

		void inheritProperties(const SVGPatternElement *elem)
		{
			if (!elem)
				return;
			
			// inherit: 
			// x, y, width, height, patternUnits, patternContentUnits, patternTransform
			// viewBox, preserveAspectRatio
			if (!getAttributeByName("x"))
			{
				if (elem->getAttributeByName("x"))
					setAttribute("x", elem->getAttributeByName("x"));
			}

			if (!getAttributeByName("y"))
			{
				if (elem->getAttributeByName("y"))
					setAttribute("y", elem->getAttributeByName("y"));
			}
			
			if (!getAttributeByName("width"))
			{
				if (elem->getAttributeByName("width"))
					setAttribute("width", elem->getAttributeByName("width"));

			}

			if (!getAttributeByName("height"))
			{
				if (elem->getAttributeByName("height"))
					setAttribute("height", elem->getAttributeByName("height"));
			}
			
			if (!getAttributeByName("patternUnits"))
			{
				if (elem->getAttributeByName("patternUnits"))
					setAttribute("patternUnits", elem->getAttributeByName("patternUnits"));
			}
			
			if (!getAttributeByName("patternContentUnits"))
			{
				if (elem->getAttributeByName("patternContentUnits"))
					setAttribute("patternContentUnits", elem->getAttributeByName("patternContentUnits"));
			}

			if (!getAttributeByName("patternTransform"))
			{
				if (elem->getAttributeByName("patternTransform"))
					setAttribute("patternTransform", elem->getAttributeByName("patternTransform"));
			}

			if (!getAttributeByName("viewBox"))
			{
				if (elem->getAttributeByName("viewBox"))
					setAttribute("viewBox", elem->getAttributeByName("viewBox"));
			}

			if (!getAttributeByName("preserveAspectRatio"))
			{
				if (elem->getAttributeByName("preserveAspectRatio"))
					setAttribute("preserveAspectRatio", elem->getAttributeByName("preserveAspectRatio"));
			}
		}

		
		void resolveReference(IRenderSVG *ctx, IAmGroot* groot)
		{
			// return early if we can't do a lookup
			if (!groot)
				return;

			// Quick return if there is no referred to pattern
			if (!fTemplateReference)
				return;
			
			// Try to find the referred to pattern
			auto node = groot->findNodeByHref(fTemplateReference);

			// Didn't find the node, so return empty handed
			if (!node)
				return;

			// Try to cast the node to a SVGPatternElement, so we get get some properties from it
			auto pattNode = std::dynamic_pointer_cast<SVGPatternElement>(node);

			// If it didn't cast, then it's not an SVGPatternElement, so return
			if (!pattNode)
				return;
			
			
			// We need to do the inheritance thing here
			// There are a fixed number of properties that we care to 
			// inherit: x, y, width, height, patternUnits, patternContentUnits, patternTransform
			// viewBox, preserveAspectRatio
			// If we have set them ourselves, then don't bother getting them from the referenced
			// pattern.  But, if we have not set them, then get them from the referenced pattern
			// Use getVisualProperty() to figure out if the property has already been set or not
			// 
			pattNode->bindToContext(ctx, groot);
			inheritProperties(pattNode.get());
		}

		// fixupSelfStyleAttributes
		//
		// This is the earliest opportunity to load raw attribute
		// values, after styling has been applied.
		//
		// We want to load things here that are invariant between 
		// coordinate spaces, mostly enums, transform, and viewbox.
		// 
		// BUGBUG 
		// can we also resolve inheritance here?
		// has the document already been fully loaded.
		//
		void fixupSelfStyleAttributes(IRenderSVG *ctx, IAmGroot *groot) override
		{
			// See if we have a template reference
			if (getAttributeByName("href"))
				fTemplateReference = getAttributeByName("href");
			else if (getAttributeByName("xlink:href"))
				fTemplateReference = getAttributeByName("xlink:href");

			resolveReference(ctx, groot);
			
			// Get the aspect ratio, and spacial units
			//getEnumValue(SVGAspectRatioEnum, getAttribute("preserveAspectRatio"), (uint32_t&)fPreserveAspectRatio);
			getEnumValue(SVGSpaceUnits, getAttributeByName("patternUnits"), (uint32_t&)fPatternUnits);
			getEnumValue(SVGSpaceUnits, getAttributeByName("patternContentUnits"), (uint32_t&)fPatternContentUnits);


			haveViewbox = parseViewBox(getAttributeByName("viewBox"), viewboxRect);

			fHasPatternTransform = parseTransform(getAttributeByName("patternTransform"), fPatternTransform);

			getEnumValue(SVGExtendMode, getAttributeByName("extendMode"), (uint32_t&)fExtendMode);
		}
		
		//
		// createPortal
		// 
		// This code establishes the coordinate space for the element, and 
		// its child nodes.
		// 
		void createPortal(IRenderSVG* ctx, IAmGroot* groot)
		{
			double dpi = 96;
			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}

			// First, we want to know the size of the object we're going to be
			// rendered into
			BLRect objectBoundingBox = ctx->getObjectFrame();
			// We also want to know the size of the container the object is in
			BLRect containerBoundingBox = ctx->viewport();
			
			// Load parameters for the portal
			SVGVariableSize fDimX{};
			SVGVariableSize fDimY{};
			SVGVariableSize fDimWidth{};
			SVGVariableSize fDimHeight{};


			fDimX.loadFromChunk(getAttributeByName("x"));
			fDimY.loadFromChunk(getAttributeByName("y"));
			fDimWidth.loadFromChunk(getAttributeByName("width"));
			fDimHeight.loadFromChunk(getAttributeByName("height"));

			// We need to calculate the fPatternBoundingBox
			// this is based on the patternUnits
			// Set default surfaceFrame
			if (fPatternUnits == SVG_SPACE_OBJECT)
			{
				// Start with the offset
				if (fDimX.isSet())
					fPatternBoundingBox.x = fDimX.calculatePixels(ctx->getFont(), objectBoundingBox.w, 0, dpi, fPatternUnits);
				if (fDimY.isSet())
					fPatternBoundingBox.y = fDimY.calculatePixels(ctx->getFont(), objectBoundingBox.h, 0, dpi, fPatternUnits);

				
				if (fDimWidth.isSet())
					fPatternBoundingBox.w = fDimWidth.calculatePixels(ctx->getFont(), objectBoundingBox.w, 0, dpi, fPatternUnits);
				if (fDimHeight.isSet())
					fPatternBoundingBox.h = fDimHeight.calculatePixels(ctx->getFont(), objectBoundingBox.h, 0, dpi, fPatternUnits);

				fPatternOffset.x = fPatternBoundingBox.x;
				fPatternOffset.y = fPatternBoundingBox.y;


				
			}
			else if (fPatternUnits == SVG_SPACE_USER)
			{
				// Start with the offset					// Start with the offset
				if (fDimX.isSet())
					fPatternBoundingBox.x = fDimX.calculatePixels(ctx->getFont(), containerBoundingBox.w, 0, dpi, fPatternUnits);
				if (fDimY.isSet())
					fPatternBoundingBox.y = fDimY.calculatePixels(ctx->getFont(), containerBoundingBox.h, 0, dpi, fPatternUnits);


				if (fDimWidth.isSet())
					fPatternBoundingBox.w = fDimWidth.calculatePixels(ctx->getFont(), containerBoundingBox.w, 0, dpi, fPatternUnits);
				if (fDimHeight.isSet())
					fPatternBoundingBox.h = fDimHeight.calculatePixels(ctx->getFont(), containerBoundingBox.h, 0, dpi, fPatternUnits);

				
				fPatternOffset.x = fPatternBoundingBox.x;
				fPatternOffset.y = fPatternBoundingBox.y;
				
			}

			if (fPatternContentUnits == SVG_SPACE_OBJECT)
			{
				if (!haveViewbox) {
					fPatternContentScale = { objectBoundingBox.w, objectBoundingBox.h };
				}
				else {
					fPatternContentScale = { fPatternBoundingBox.w / viewboxRect.w, fPatternBoundingBox.h / viewboxRect.h };
				}
			}
			else if (fPatternContentUnits == SVG_SPACE_USER)
			{
				if (!haveViewbox) {
					fPatternContentScale = { 1.0,1.0 };
				}
				else {
					fPatternContentScale = { fPatternBoundingBox.w / viewboxRect.w, fPatternBoundingBox.h / viewboxRect.h };
				}

			}

		}



		void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
		{
			createPortal(ctx, groot);
			fPortal.bindToContext(ctx, groot);

			// We need to resolve the size of the user space
			// start out with some information from groot
			double dpi = 96;


			// The width and height can default to the size of the canvas
			// we are rendering to.
			if (groot)
				dpi = groot->dpi();


			
			// Need to apply the various transformations before drawing
			fPattern.resetTransform();
			fPattern.translate(-fPatternOffset.x, -fPatternOffset.y);
			
			// This is to properly scale the drawing within the context
			//auto & scTform = fViewport.sceneToSurfaceTransform();
			//fPattern.applyTransform(scTform);
			
			if (fHasPatternTransform)
				fPattern.applyTransform(fPatternTransform);



			// Whether it was a reference or not, set the extendMode
			fPattern.setExtendMode(fExtendMode);

		}

		void drawIntoCache(IRenderSVG* ctx, IAmGroot* groot)
		{
			bindToContext(ctx, groot);

			auto box = getBBox();

			fPatternCache.create(static_cast<int>(fPatternBoundingBox.w), static_cast<int>(fPatternBoundingBox.h), BL_FORMAT_PRGB32);
			SVGB2DDriver ictx; 
			ictx.attach(fPatternCache);


			ictx.renew();
			ictx.clear();
			ictx.scale(fPatternContentScale.x, fPatternContentScale.y);

			draw(&ictx, groot);

			ictx.flush();
			ictx.detach();

			fPattern.setImage(fPatternCache);
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
			
			ctx->setObjectFrame(getBBox());
			
			applyProperties(ctx, groot);
			drawSelf(ctx, groot);

			drawChildren(ctx, groot);

			ctx->pop();
		}
		
	};
	
}