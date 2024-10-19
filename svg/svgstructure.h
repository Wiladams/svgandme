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
	// 
	// SVGSVGElement
	// This is the root node of an entire SVG tree
	//====================================================
	struct SVGSVGElement : public SVGContainer
	{
		static void registerFactory()
		{
			registerContainerNode("svg",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGSVGElement>(groot);
					node->loadFromXmlIterator(iter, groot);

					return node;
				});
			

		}


		
		SVGSVGElement(IAmGroot* )
			: SVGContainer()
		{
			needsBinding(true);
		}

		void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) noexcept override
		{
			createPortal(ctx, groot);
			
			//needsBinding(false);
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
			getSVGSingularCreationMap()["g"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGGElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			registerContainerNode("g",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGGElement>(groot);
					node->loadFromXmlIterator(iter, groot);

					return node;
				});
			

			registerSingularNode();
		}



		// Instance Constructor
		SVGGElement(IAmGroot* )
			: SVGGraphicsElement()
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
			getSVGSingularCreationMap()["defs"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGDefsNode>(groot);
				node->loadFromXmlElement(elem, groot);
				//node->visible(false);

				return node;
				};
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			registerContainerNode("defs",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGDefsNode>(groot);
					node->loadFromXmlIterator(iter, groot);
					//node->visible(false);

					return node;
				});
			

			registerSingularNode();
		}

		// Instance Constructor
		SVGDefsNode(IAmGroot* )
			: SVGGraphicsElement()
		{
			isStructural(false);
			visible(false);
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
			getSVGSingularCreationMap()["desc"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGDescNode>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			registerContainerNode("desc",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGDescNode>(groot);
					node->loadFromXmlIterator(iter, groot);

					return node;
				});
			

			registerSingularNode();
		}

		ByteSpan fContent{};

		// Instance Constructor
		SVGDescNode(IAmGroot* )
			: SVGGraphicsElement()
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
			getSVGSingularCreationMap()["title"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGTitleNode>(groot);
				node->loadFromXmlElement(elem, groot);
				node->visible(false);

				return node;
				};
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			registerContainerNode("title",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGTitleNode>(groot);
					node->loadFromXmlIterator(iter, groot);
					node->visible(false);

					return node;
				});
			

			registerSingularNode();
		}

		ByteSpan fContent{};

		// Instance Constructor
		SVGTitleNode(IAmGroot* )
			: SVGGraphicsElement() {}

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

		SVGDimension fRefX{};
		SVGDimension fRefY{};


		SVGSymbolNode(IAmGroot* root) 
			: SVGContainer()
		{
			isStructural(false);
		}

		
		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			SVGContainer::drawSelf(ctx, groot);
			ctx->translate(-fRefX.calculatePixels(), -fRefY.calculatePixels());
		}
		

		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			fRefX.loadFromChunk(getAttribute("refX"));
			fRefY.loadFromChunk(getAttribute("refY"));

			createPortal(ctx, groot);
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
			getSVGSingularCreationMap()["use"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGUseElement>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
				};
		}

		static void registerFactory()
		{
			registerContainerNode("use",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGUseElement>(groot);
					node->loadFromXmlIterator(iter, groot);
					return node;
				});
			

			registerSingularNode();
		}


		ByteSpan fWrappedID{};
		std::shared_ptr<IViewable> fWrappedNode{ nullptr };


		double x{ 0 };
		double y{ 0 };
		double width{ 0 };
		double height{ 0 };

		SVGDimension fDimX{};
		SVGDimension fDimY{};
		SVGDimension fDimWidth{};
		SVGDimension fDimHeight{};


		SVGUseElement(const SVGUseElement& other) = delete;
		SVGUseElement(IAmGroot* ) 
			: SVGGraphicsElement() {}

		BLRect frame() const override
		{
			// BUGBUG: This is not correct
			// Needs to be adjusted for the x/y
			// and the transform
			if (fWrappedNode != nullptr)
				return fWrappedNode->getBBox();

			return BLRect{ };
		}

		void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
		{
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}


			BLRect cFrame = ctx->localFrame();
			w = cFrame.w;
			h = cFrame.h;


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
			auto href = getAttribute("xlink:href");
			if (!href)
			{
				href = getAttribute("href");
			}

			fWrappedID = chunk_trim(href,chrWspChars);
			if (fWrappedID) {
				fWrappedNode = groot->findNodeByHref(fWrappedID);
			}
		}


		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			if (fWrappedNode == nullptr)
				return;

			// set local size, if width and height were set
			// we don't want to do scaling here, because the
			// wrapped graphic might want to do something different
			// really it applies to symbols, and they're do their 
			// own scaling.
			// width and height only apply when the wrapped graphic
			// is a symbol.  So, we should do that when we lookup the node
			//if (fDimWidth.isSet() && fDimHeight.isSet())
			//{
			//	ctx->localFrame(BLRect{ x,y,width,height });
			//}
			
			
			ctx->push();
			ctx->translate(x, y);
			// Draw the wrapped graphic
			fWrappedNode->draw(ctx, groot);

			ctx->pop();
		}

	};
}
