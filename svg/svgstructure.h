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
	struct SVGSVGElement : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			registerContainerNode("svg",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGSVGElement>(groot);
					node->loadFromXmlPull(iter, groot);

					return node;
				});
			

		}

		SVGPortal fPortal;			// The coordinate system
		bool fIsTopLevel{ false };	// Is this a top level svg element
		
		SVGSVGElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			setNeedsBinding(true);
		}

		// setTopLevel
		//
		// A top level node is different than an SVG node which might
		// be located deeper in the DOM tree.  The top level node ignores
		// any x,y attributes, only paying attention to the dimensions
		// width, and height.
		//
		// Deeper into the tree, the x,y attributes are used to position the node
		// relative to the parent node.  The width and height attributes are used
		// to set the size of the node, but not the position.
		// And svg node is NOT top level by default, and is explicitly set
		// to be top level by the DOM constructor
		void setTopLevel(bool isTop) 
		{ 
			fIsTopLevel = isTop; 
		}
		bool isTopLevel() const { return fIsTopLevel; }


		BLRect frame() const override
		{
			BLRect vpFrame{};
			fPortal.getViewportFrame(vpFrame);
			return vpFrame;
		}

		BLRect getBBox() const override
		{
			return fPortal.getBBox();
		}

		
		void fixupSelfStyleAttributes(IRenderSVG*, IAmGroot *groot) override
		{
			// printf("fixupSelfStyleAttributes\n");
			//fPortal.loadFromAttributes(fAttributes);
		}

		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			// We primarily need to establish the coordinate system here
			// It is a mix of intrinsic size, the canvas size, and the viewbox

			BLRect intrinsicRect{};

			double intrinsicW = 0;
			double intrinsicH = 0;
			double dpi = 96.0;

			// Set canvas size and dpi from groot, if we have it
			if (groot != nullptr)
			{
				intrinsicW = groot->canvasWidth();
				intrinsicH = groot->canvasHeight();
				dpi = groot->dpi();
			}

			// If the canvas size is not set, we use a default size
			if (intrinsicW <= 0)
				intrinsicW = 300;
			if (intrinsicH <= 0)
				intrinsicH = 150;

			intrinsicRect = BLRect{ 0,0, intrinsicW, intrinsicH };


			// 
			// At this point, we know whether we're the top level svg element
			// We need to figure out the viewport and viewbox settings
            ByteSpan xAttr{}, yAttr{}, wAttr{}, hAttr{};

            fAttributes.getAttribute("x", xAttr);
            fAttributes.getAttribute("y", yAttr);
            fAttributes.getAttribute("width", wAttr);
            fAttributes.getAttribute("height", hAttr);

			SVGVariableSize dimX; dimX.loadFromChunk(xAttr);
			SVGVariableSize dimY;  dimY.loadFromChunk(yAttr);
			SVGVariableSize dimWidth;  dimWidth.loadFromChunk(wAttr);
			SVGVariableSize dimHeight; dimHeight.loadFromChunk(hAttr);

			//fDimX.parseValue(srfFrame.x, ctx->getFont(), viewport.w, origin, dpi, SpaceUnitsKind::SVG_SPACE_USER);
			//fDimY.parseValue(srfFrame.y, ctx->getFont(), viewport.h, origin, dpi, SpaceUnitsKind::SVG_SPACE_USER);
			//fDimWidth.parseValue(srfFrame.w, ctx->getFont(), viewport.w, origin, dpi, SpaceUnitsKind::SVG_SPACE_USER);
			//fDimHeight.parseValue(srfFrame.h, ctx->getFont(), viewport.h, origin, dpi, SpaceUnitsKind::SVG_SPACE_USER);

			fPortal.loadFromAttributes(fAttributes);
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
			registerSVGSingularNode("g", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGGElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			registerContainerNode("g",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGGElement>(groot);
					node->loadFromXmlPull(iter, groot);

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
			registerSVGSingularNode("use", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGUseElement>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
				});
		}

		static void registerFactory()
		{
			registerContainerNode("use",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGUseElement>(groot);
					node->loadFromXmlPull(iter, groot);
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
			fDimX.loadFromChunk(getAttributeByName("x"));
			fDimY.loadFromChunk(getAttributeByName("y"));
			fDimWidth.loadFromChunk(getAttributeByName("width"));
			fDimHeight.loadFromChunk(getAttributeByName("height"));

			// look for the href, or xlink:href attribute
			auto href = getAttributeByName("xlink:href");
			if (href.empty())
			{
				href = getAttributeByName("href");
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

			BLRect objectBoundingBox = ctx->getObjectFrame();
			BLRect viewport = ctx->viewport();
			BLRect cartFrame{};

			cartFrame = objectBoundingBox;
			if ((cartFrame.w <= 0) || (cartFrame.h <=0))
                cartFrame = viewport;

			fDimX.parseValue(fBoundingBox.x, ctx->getFont(), cartFrame.w, 0, dpi);
			fDimY.parseValue(fBoundingBox.y, ctx->getFont(), cartFrame.h, 0, dpi);
			fDimWidth.parseValue(fBoundingBox.w, ctx->getFont(), cartFrame.w, 0, dpi);
			fDimHeight.parseValue(fBoundingBox.h, ctx->getFont(), cartFrame.h, 0, dpi);
			


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
			ctx->setObjectFrame(fBoundingBox);
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
			registerSVGSingularNode("defs",  [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGDefsNode>(groot);
				node->loadFromXmlElement(elem, groot);
				//node->visible(false);

				return node;
				});
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			registerContainerNode("defs",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGDefsNode>(groot);
					node->loadFromXmlPull(iter, groot);
					//node->visible(false);

					return node;
				});


			registerSingularNode();
		}

		// Instance Constructor
		SVGDefsNode(IAmGroot*)
			: SVGGraphicsElement()
		{
			setIsStructural(false);
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
			registerSVGSingularNode("desc", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGDescNode>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			registerContainerNode("desc",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGDescNode>(groot);
					node->loadFromXmlPull(iter, groot);

					return node;
				});


			registerSingularNode();
		}

		ByteSpan fContent{};

		// Instance Constructor
		SVGDescNode(IAmGroot*)
			: SVGGraphicsElement()
		{
			setIsStructural(false);
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
			registerSVGSingularNode("title", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGTitleNode>(groot);
				node->loadFromXmlElement(elem, groot);
				node->visible(false);

				return node;
				});
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			registerContainerNode("title",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGTitleNode>(groot);
					node->loadFromXmlPull(iter, groot);
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
