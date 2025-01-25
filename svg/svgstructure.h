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
#include "svgcontainer.h"


namespace waavs {
	//====================================================
	// SVGSVGElement
	// 
	// SVGSVGElement
	// This is the root node of an entire SVG tree
	//====================================================
	struct SVGSVGElement : public SVGGraphicsElement // SVGContainer
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

		SVGPortal fPortal;

		
		SVGSVGElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			needsBinding(true);
		}

		BLRect frame() const override
		{
			return fPortal.viewportFrame();
			//return fPortal.getBBox();
		}

		BLRect getBBox() const override
		{
			return fPortal.getBBox();
		}

		
		void fixupSelfStyleAttributes(IRenderSVG*, IAmGroot*) override
		{
			// printf("fixupSelfStyleAttributes\n");
			fPortal.loadFromAttributes(fAttributes);

		}

		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			fPortal.bindToContext(ctx, groot);
		}

		
		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			// Clipping doesn't quite work out, because it's a non-transformed
			// rectangle on the context, and it's only a rectangle, not a shape
			// it will not transform along with the context
			//ctx->clip(getBBox());

			// We do an 'applyTransform' instead of 'setTransform'
			// because there might already be a transform on the context
			// and we want to build upon that, rather than replace it.
			//ctx->setTransform(fViewport.sceneToSurfaceTransform());
			ctx->applyTransform(fPortal.viewBoxToViewportTransform());
			ctx->setViewport(getBBox());
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


		//double x{ 0 };
		//double y{ 0 };
		//double width{ 0 };
		//double height{ 0 };
		BLRect fBoundingBox{};
		
		SVGVariableSize fDimX{};
		SVGVariableSize fDimY{};
		SVGVariableSize fDimWidth{};
		SVGVariableSize fDimHeight{};


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

		BLRect getBBox() const override
		{
			return fBoundingBox;
		}
		
		void update(IAmGroot *groot) override 
		{ 
			if (!fWrappedNode)
				return;
			
			fWrappedNode->update(groot);
		}

		virtual void fixupSelfStyleAttributes(IRenderSVG*, IAmGroot*)
		{
			fDimX.loadFromChunk(getAttribute("x"));
			fDimY.loadFromChunk(getAttribute("y"));
			fDimWidth.loadFromChunk(getAttribute("width"));
			fDimHeight.loadFromChunk(getAttribute("height"));

			// look for the href, or xlink:href attribute
			auto href = getAttribute("xlink:href");
			if (!href)
			{
				href = getAttribute("href");
			}

			fWrappedID = chunk_trim(href, chrWspChars);
		}
		
		
		void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
		{
			double dpi = 96;


			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}


			BLRect objectBoundingBox = ctx->objectFrame();
			BLRect viewport = ctx->viewport();
			
			fDimX.parseValue(fBoundingBox.x, ctx->font(), objectBoundingBox.w, 0, dpi);
			fDimY.parseValue(fBoundingBox.y, ctx->font(), objectBoundingBox.h, 0, dpi);
			fDimWidth.parseValue(fBoundingBox.w, ctx->font(), objectBoundingBox.w, 0, dpi);
			fDimHeight.parseValue(fBoundingBox.h, ctx->font(), objectBoundingBox.h, 0, dpi);
			


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
			ctx->translate(fBoundingBox.x, fBoundingBox.y);
			
			// Draw the wrapped graphic
			ctx->objectFrame(fBoundingBox);
			ctx->setViewport(BLRect{0,0,fBoundingBox.w, fBoundingBox.h});

			fWrappedNode->draw(ctx, groot);

			ctx->pop();
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
		SVGDefsNode(IAmGroot*)
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
		SVGDescNode(IAmGroot*)
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
		SVGTitleNode(IAmGroot*)
			: SVGGraphicsElement() {}

		const ByteSpan& content() const { return fContent; }

		// Load the text content if it exists
		void loadContentNode(const XmlElement& elem, IAmGroot* groot) override
		{
			fContent = elem.data();
		}
	};
}
