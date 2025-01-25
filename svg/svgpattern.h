#pragma once

// Support for SVGPatternElement
// http://www.w3.org/TR/SVG11/feature#Pattern
// pattern
//


#include <functional>
#include <memory>

#include "svgattributes.h"
#include "svgstructuretypes.h"
//#include "viewport.h"
#include "svgportal.h"


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

			isStructural(false);
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
			if (!getAttribute("x"))
			{
				if (elem->getAttribute("x"))
					setAttribute("x", elem->getAttribute("x"));
			}

			if (!getAttribute("y"))
			{
				if (elem->getAttribute("y"))
					setAttribute("y", elem->getAttribute("y"));
			}
			
			if (!getAttribute("width"))
			{
				if (elem->getAttribute("width"))
					setAttribute("width", elem->getAttribute("width"));

			}

			if (!getAttribute("height"))
			{
				if (elem->getAttribute("height"))
					setAttribute("height", elem->getAttribute("height"));
			}
			
			if (!getAttribute("patternUnits"))
			{
				if (elem->getAttribute("patternUnits"))
					setAttribute("patternUnits", elem->getAttribute("patternUnits"));
			}
			
			if (!getAttribute("patternContentUnits"))
			{
				if (elem->getAttribute("patternContentUnits"))
					setAttribute("patternContentUnits", elem->getAttribute("patternContentUnits"));
			}

			if (!getAttribute("patternTransform"))
			{
				if (elem->getAttribute("patternTransform"))
					setAttribute("patternTransform", elem->getAttribute("patternTransform"));
			}

			if (!getAttribute("viewBox"))
			{
				if (elem->getAttribute("viewBox"))
					setAttribute("viewBox", elem->getAttribute("viewBox"));
			}

			if (!getAttribute("preserveAspectRatio"))
			{
				if (elem->getAttribute("preserveAspectRatio"))
					setAttribute("preserveAspectRatio", elem->getAttribute("preserveAspectRatio"));
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
			if (getAttribute("href"))
				fTemplateReference = getAttribute("href");
			else if (getAttribute("xlink:href"))
				fTemplateReference = getAttribute("xlink:href");

			resolveReference(ctx, groot);
			
			// Get the aspect ratio, and spacial units
			//getEnumValue(SVGAspectRatioEnum, getAttribute("preserveAspectRatio"), (uint32_t&)fPreserveAspectRatio);
			getEnumValue(SVGSpaceUnits, getAttribute("patternUnits"), (uint32_t&)fPatternUnits);
			getEnumValue(SVGSpaceUnits, getAttribute("patternContentUnits"), (uint32_t&)fPatternContentUnits);


			haveViewbox = parseViewBox(getAttribute("viewBox"), viewboxRect);

			fHasPatternTransform = parseTransform(getAttribute("patternTransform"), fPatternTransform);

			getEnumValue(SVGExtendMode, getAttribute("extendMode"), (uint32_t&)fExtendMode);
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
			BLRect objectBoundingBox = ctx->objectFrame();
			// We also want to know the size of the container the object is in
			BLRect containerBoundingBox = ctx->viewport();
			
			// Load parameters for the portal
			SVGVariableSize fDimX{};
			SVGVariableSize fDimY{};
			SVGVariableSize fDimWidth{};
			SVGVariableSize fDimHeight{};


			fDimX.loadFromChunk(getAttribute("x"));
			fDimY.loadFromChunk(getAttribute("y"));
			fDimWidth.loadFromChunk(getAttribute("width"));
			fDimHeight.loadFromChunk(getAttribute("height"));

			// We need to calculate the fPatternBoundingBox
			// this is based on the patternUnits
			// Set default surfaceFrame
			if (fPatternUnits == SVG_SPACE_OBJECT)
			{
				// Start with the offset
				if (fDimX.isSet())
					fPatternBoundingBox.x = fDimX.calculatePixels(ctx->font(), objectBoundingBox.w, 0, dpi, fPatternUnits);
				if (fDimY.isSet())
					fPatternBoundingBox.y = fDimY.calculatePixels(ctx->font(), objectBoundingBox.h, 0, dpi, fPatternUnits);

				
				if (fDimWidth.isSet())
					fPatternBoundingBox.w = fDimWidth.calculatePixels(ctx->font(), objectBoundingBox.w, 0, dpi, fPatternUnits);
				if (fDimHeight.isSet())
					fPatternBoundingBox.h = fDimHeight.calculatePixels(ctx->font(), objectBoundingBox.h, 0, dpi, fPatternUnits);

				fPatternOffset.x = fPatternBoundingBox.x;
				fPatternOffset.y = fPatternBoundingBox.y;


				
			}
			else if (fPatternUnits == SVG_SPACE_USER)
			{
				// Start with the offset					// Start with the offset
				if (fDimX.isSet())
					fPatternBoundingBox.x = fDimX.calculatePixels(ctx->font(), containerBoundingBox.w, 0, dpi, fPatternUnits);
				if (fDimY.isSet())
					fPatternBoundingBox.y = fDimY.calculatePixels(ctx->font(), containerBoundingBox.h, 0, dpi, fPatternUnits);


				if (fDimWidth.isSet())
					fPatternBoundingBox.w = fDimWidth.calculatePixels(ctx->font(), containerBoundingBox.w, 0, dpi, fPatternUnits);
				if (fDimHeight.isSet())
					fPatternBoundingBox.h = fDimHeight.calculatePixels(ctx->font(), containerBoundingBox.h, 0, dpi, fPatternUnits);

				
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
			IRenderSVG ictx(ctx->fontHandler());
			ictx.begin(fPatternCache);


			ictx.renew();
			ictx.clear();
			ictx.scale(fPatternContentScale.x, fPatternContentScale.y);

			draw(&ictx, groot);

			ictx.flush();
			ictx.end();

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
			
			ctx->objectFrame(getBBox());
			
			applyProperties(ctx, groot);
			drawSelf(ctx, groot);

			drawChildren(ctx, groot);

			ctx->pop();
		}
		
	};
	
}