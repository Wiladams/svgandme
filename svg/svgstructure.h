#pragma once

// Support for SVG structure elements
// http://www.w3.org/TR/SVG11/feature#Structure
// svg
// g
// defs
// desc
// title
// metadata
// symbol
// use

#include <string>
#include <array>
#include <functional>
#include <unordered_map>

#include "svgattributes.h"
#include "viewport.h"
#include "svgstructuretypes.h"



namespace waavs {
	//====================================================
	// SVGSVGElement
	// SVGSVGElement
	// This is the root node of an entire SVG tree
	//====================================================
	struct SVGSVGElement : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["svg"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGSVGElement>(groot);
				node->loadFromXmlIterator(iter, groot);

				return node;
				};

		}

		BLRect fBBox{};

		ViewPort fViewport{};

		SVGViewbox fViewbox{};
		SVGDimension fDimX{};
		SVGDimension fDimY{};
		SVGDimension fDimWidth{};
		SVGDimension fDimHeight{};

		SVGSVGElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
		}

		BLRect frame() const override
		{
			if (fViewbox.isSet()) {
				return fViewbox.fRect;
			}

			return fBBox;
		}

		BLRect getBBox() const override
		{
			return fBBox;
		}

		// resolvePosition()
		// Here we need to set the size and viewport of the root SVG element
		// 
		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
		{
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

			// Here we have to do a dance to determine the default 
			// values for the size of the canvas.  It can be set
			// explicitly from either width and height properties
			// or through the viewBox
			// or both
			fDimX.loadFromChunk(getAttribute("x"));
			fDimY.loadFromChunk(getAttribute("y"));
			fDimWidth.loadFromChunk(getAttribute("width"));
			fDimHeight.loadFromChunk(getAttribute("height"));
			fViewbox.loadFromChunk(getAttribute("viewBox"));

			// BUGBUG - if the width and height are not set, then
			// we need to get them out of the viewBox if that was set

			
			fBBox.x = fDimX.calculatePixels(w, 0, dpi);
			fBBox.y = fDimY.calculatePixels(h, 0, dpi);
			fBBox.w = fDimWidth.calculatePixels(w, 0, dpi);
			fBBox.h = fDimHeight.calculatePixels(h, 0, dpi);

			fViewport.sceneFrame(getBBox());
			//fViewport.surfaceFrame(getBBox());


			
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
		}


		virtual void drawChildren(IRenderSVG* ctx, IAmGroot* groot) override
		{
			for (auto& node : fNodes) {
				// Restore our context before drawing each child
				//ctx->localSize(getBBox().w, getBBox().h);
				ctx->localFrame(getBBox());

				node->draw(ctx, groot);
			}
		}

		void draw(IRenderSVG* ctx, IAmGroot* groot) override
		{
			ctx->push();

			// Start with default state
			//ctx->blendMode(BL_COMP_OP_SRC_OVER);

			// Apply attributes that have been gathered
			// in the case of the root node, it's mostly the viewport
			applyAttributes(ctx, groot);

			// Apply scaling transform based on viewbox
			//ctx->setTransform(fViewport.sceneToSurfaceTransform());
			ctx->translate(fBBox.x, fBBox.y);

			// Draw the children
			drawChildren(ctx, groot);

			ctx->pop();
		}

	};
	
	//================================================
	// SVGGroupNode
	// 'g' element
	//================================================
	struct SVGGElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["g"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGGElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["g"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGGElement>(groot);
				node->loadFromXmlIterator(iter, groot);

				return node;
				};

			registerSingularNode();
		}



		// Instance Constructor
		SVGGElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
		}
	};

	//============================================================
	// 	SVGDefsNode
	// This node is here to hold onto definitions of other nodes
	//============================================================
	struct SVGDefsNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["defs"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGDefsNode>(groot);
				node->loadFromXmlElement(elem, groot);
				//node->visible(false);

				return node;
				};
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["defs"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGDefsNode>(groot);
				node->loadFromXmlIterator(iter, groot);
				//node->visible(false);

				return node;
				};

			registerSingularNode();
		}

		// Instance Constructor
		SVGDefsNode(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(false);
			visible(false);
		}

		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			printf("==== ERROR ERROR SVGDefsNode::drawSelf ====\n");
		}
	};

	
	//=================================================
	//
	// SVGDescNode
	// https://www.w3.org/TR/SVG11/struct.html#DescElement
	// 
	struct SVGDescNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["desc"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGDescNode>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["desc"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGDescNode>(groot);
				node->loadFromXmlIterator(iter, groot);

				return node;
				};

			registerSingularNode();
		}

		ByteSpan fContent{};

		// Instance Constructor
		SVGDescNode(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(false);
			visible(false);
		}

		const ByteSpan& content() const { return fContent; }
		
		// Load the text content if it exists
		void loadContentNode(const XmlElement& elem, IAmGroot* groot) override
		{
			fContent = elem.data();
		}
	};
	

	
	//=================================================
	// SVGTitleNode
	// 	   https://www.w3.org/TR/SVG11/struct.html#TitleElement
	// Capture the title of the SVG document
	//=================================================
	struct SVGTitleNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["title"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGTitleNode>(groot);
				node->loadFromXmlElement(elem, groot);
				node->visible(false);

				return node;
				};
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["title"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGTitleNode>(groot);
				node->loadFromXmlIterator(iter, groot);
				node->visible(false);

				return node;
				};

			registerSingularNode();
		}

		ByteSpan fContent{};

		// Instance Constructor
		SVGTitleNode(IAmGroot* aroot)
			: SVGGraphicsElement(aroot) {}

		const ByteSpan& content() const { return fContent; }
		
		// Load the text content if it exists
		void loadContentNode(const XmlElement& elem, IAmGroot* groot) override
		{
			fContent = elem.data();
		}
	};
	

	//===========================================
// SVGSymbolNode
// Notes:  An SVGSymbol can create its own local coordinate system
// if a viewBox is specified.  We need to take into account the aspectRatio
// as well to be totally correct.
// As an enhancement, we'd like to take into account style information
// such as allowing the width and height to be specified in an style
// sheet or inline style attribute
//===========================================
	struct SVGSymbolNode : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["symbol"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGSymbolNode>(groot);
				node->loadFromXmlIterator(iter, groot);

				return node;
				};
		}

		double scaleX = 1.0;
		double scaleY = 1.0;

		SVGViewbox fViewbox{};
		SVGDimension fRefX{};
		SVGDimension fRefY{};
		SVGDimension fDimX{};
		SVGDimension fDimY{};
		SVGDimension fDimWidth{};
		SVGDimension fDimHeight{};

		SVGSymbolNode(IAmGroot* root) : SVGGraphicsElement(root)
		{
			isStructural(false);
		}

		void applyAttributes(IRenderSVG* ctx, IAmGroot* groot) override
		{
			SVGGraphicsElement::applyAttributes(ctx, groot);

			auto localSize = ctx->localFrame();

			// BUGBUG - This is probably supposed to be
			// If the symbol does not specify a width, but
			// the container does, then use that
			// else, if the symbol specifies a width, use that
			if (fViewbox.isSet())
			{
				// BUGBUG - container size needs to be popped with context push/pop
				// first, if our container has specified a size, then use that

				if (fDimWidth.isSet() && fDimHeight.isSet())
				{
					scaleX = fDimWidth.calculatePixels() / fViewbox.width();
					scaleY = fDimHeight.calculatePixels() / fViewbox.height();
				}
				else if (localSize.w > 0 && localSize.h > 0)
				{
					scaleX = localSize.w / fViewbox.width();
					scaleY = localSize.h / fViewbox.height();
				}

				ctx->scale(scaleX, scaleY);
				ctx->translate(-fViewbox.x(), -fViewbox.y());
				ctx->translate(-fRefX.calculatePixels(), -fRefY.calculatePixels());
			}
		}


		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
		{
			fViewbox.loadFromChunk(getAttribute("viewBox"));
			fRefX.loadFromChunk(getAttribute("refX"));
			fRefY.loadFromChunk(getAttribute("refY"));
			fDimX.loadFromChunk(getAttribute("x"));
			fDimY.loadFromChunk(getAttribute("y"));
			fDimWidth.loadFromChunk(getAttribute("width"));
			fDimHeight.loadFromChunk(getAttribute("height"));
		}


	};
	

	
	//====================================
	// SVGUseNode
	// https://www.w3.org/TR/SVG11/struct.html#UseElement
	// <use>
	//====================================
	struct SVGUseElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["use"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGUseElement>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
				};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["use"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGUseElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				return node;
				};

			registerSingularNode();
		}


		ByteSpan fWrappedID{};
		std::shared_ptr<SVGViewable> fWrappedNode{ nullptr };


		double x{ 0 };
		double y{ 0 };
		double width{ 0 };
		double height{ 0 };

		SVGDimension fDimX{};
		SVGDimension fDimY{};
		SVGDimension fDimWidth{};
		SVGDimension fDimHeight{};


		SVGUseElement(const SVGUseElement& other) = delete;
		SVGUseElement(IAmGroot* aroot) : SVGGraphicsElement(aroot) {}

		const BLVar getVariant() noexcept override
		{
			if (fWrappedNode)
				return fWrappedNode->getVariant();
			else
				return SVGGraphicsElement::getVariant();
		}

		BLRect frame() const override
		{
			// BUGBUG: This is not correct
			// Needs to be adjusted for the x/y
			// and the transform
			if (fWrappedNode != nullptr)
				return fWrappedNode->frame();

			return BLRect{ };
		}

		// Apply locally generated attributes
		void applyAttributes(IRenderSVG* ctx, IAmGroot* groot) override
		{
			SVGGraphicsElement::applyAttributes(ctx, groot);

			ctx->translate(x, y);

			// set local size, if width and height were set
			// we don't want to do scaling here, because the
			// wrapped graphic might want to do something different
			// really it applies to symbols, and they're do their 
			// own scaling.
			if (fDimWidth.isSet() && fDimHeight.isSet())
			{
				ctx->localFrame(BLRect{ 0,0,width,height });
			}

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
				BLRect cFrame = container->frame();
				w = cFrame.w;
				h = cFrame.h;
			}

			fDimX.loadFromChunk(getAttribute("x"));
			fDimY.loadFromChunk(getAttribute("y"));
			fDimWidth.loadFromChunk(getAttribute("width"));
			fDimHeight.loadFromChunk(getAttribute("height"));


			if (fDimX.isSet())
				x = fDimX.calculatePixels(w, 0, dpi);
			if (fDimY.isSet())
				y = fDimY.calculatePixels(h, 0, dpi);
			if (fDimWidth.isSet())
				width = fDimWidth.calculatePixels(w, 0, dpi);
			if (fDimHeight.isSet())
				height = fDimHeight.calculatePixels(h, 0, dpi);

			// look for the href, or xlink:href attribute
			auto href = getAttribute("href");
			if (!href)
			{
				href = chunk_trim(getAttribute("xlink:href"), xmlwsp);
			}

			// Capture the href to be looked up later
			if (href && *href == '#')
			{
				href++;
				fWrappedID = href;
			}


			// Use the root to lookup the wrapped node
			fWrappedNode = groot->getElementById(fWrappedID);

			if (fWrappedNode)
			{
				fWrappedNode->bindToGroot(groot, container);
			}
		}


		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			if (fWrappedNode == nullptr)
				return;

			ctx->push();

			// Draw the wrapped graphic
			fWrappedNode->draw(ctx, groot);

			ctx->pop();
		}

	};
}
