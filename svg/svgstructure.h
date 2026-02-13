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

#include <array>
#include <functional>

#include "svgstructuretypes.h"
#include "viewport.h"


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
			registerContainerNodeByName("svg",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGSVGElement>(groot);
					node->loadFromXmlPull(iter, groot);

					return node;
				});
			

		}

		bool fIsTopLevel{ true };	// Is this a top level svg element

		DocViewportState fDocViewport;	// The viewport attributes as authored in the document, before resolution
		bool fHasDocViewport{ false };

		SVGViewportState fViewportState;	// The resolved viewport state
		

		SVGSVGElement(IAmGroot* groot )
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
		void setTopLevel(bool isTop) { fIsTopLevel = isTop; }
		bool isTopLevel() const { return fIsTopLevel; }


		BLRect viewPort() const override
		{
            return fViewportState.fViewport;
		}

		

		void fixupSelfStyleAttributes(IAmGroot *groot) override
		{
			// printf("fixupSelfStyleAttributes\n");
						// load the document representation of the viewport attributes
			DocViewportState vps{};
			loadDocViewportState(vps, fAttributes);

			// Try to resolve the viewport state 
			if (fIsTopLevel && groot)
			{
				const BLRect containingVP{ 0,0, groot->canvasWidth(), groot->canvasHeight() };

				const double dpi = groot->dpi();
				resolveViewState(containingVP, vps, true, dpi, nullptr, fViewportState);
			}
			else {
				fDocViewport = vps;
				fHasDocViewport = true;
				fViewportState.fResolved = false;
			}

		}

		/*
        // onLoadedFromXmlPull
		//
        // This is called after we've loaded the element and all its children from the XML
        // In the case of the top level svg element, the entirety of the document has been loaded
		// and we can resolve style attributes now.
        //
		virtual void onEndTag(IAmGroot* groot)
		{
			if (fIsTopLevel)
				resolveStyleSubtree(groot);

			// load the document representation of the viewport attributes
            DocViewportState vps{};
			loadDocViewportState(vps, fAttributes);

			// Try to resolve the viewport state 
			if (fIsTopLevel && groot)
			{
                const BLRect containingVP{ 0,0, groot->canvasWidth(), groot->canvasHeight() };

                const double dpi = groot->dpi();
				resolveViewState(containingVP, vps, true, dpi, nullptr, fViewportState);
			}
			else {
				fDocViewport = vps;
				fHasDocViewport = true;
				fViewportState.fResolved = false;
			}
		}
		*/
		
		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			if (!fViewportState.fResolved)
			{
				const BLRect containingVP = ctx->viewport();
				const double dpi = groot ? groot->dpi() : 96.0;
				resolveViewState(containingVP, fDocViewport, false, dpi, nullptr, fViewportState);
			}
		}
		
		
		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			// Clipping doesn't quite work out, because it's a non-transformed
			// rectangle on the context, and it's only a rectangle, not a shape
			// it will not transform along with the context
			//ctx->clip(getBBox());



            // Apply mapping from this svg's user space (viewBox) to its 
			// parent user space (viewport)
			// We do an 'applyTransform' instead of 'setTransform'
			// because there might already be a transform on the context
			// and we want to build upon that, rather than replace it.
			ctx->applyTransform(fViewportState.viewBoxToViewportXform);

			// Establish the 'nearest viewport' for children, in THIS
			// svg's user space.  This is what percentage lengths should
			// use as their reference.
			ctx->setViewport(fViewportState.fViewBox);

			// Object frame for <svg> isn't a true geometry bbox; we'll
			// keep it simple, just so there is something.
			// We could just ignore it, and allow whatever was already
			// to remain.
            //ctx->setObjectFrame(fViewportState.fViewBox);
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
			registerSVGSingularNodeByName("g", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGGElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			registerContainerNodeByName("g",
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


	


	

	



	//=================================================
	//
	// SVGDescNode
	// https://www.w3.org/TR/SVG11/struct.html#DescElement
	// 
	struct SVGDescNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("desc", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGDescNode>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			registerContainerNodeByName("desc",
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
			//setIsStructural(false);
			setIsVisible(false);
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
			registerSVGSingularNodeByName("title", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGTitleNode>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			registerContainerNodeByName("title",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGTitleNode>(groot);
					node->loadFromXmlPull(iter, groot);

					return node;
				});


			registerSingularNode();
		}

		ByteSpan fContent{};

		// Instance Constructor
		SVGTitleNode(IAmGroot*)
			: SVGGraphicsElement() 
		{
            setIsVisible(false);
		}

		const ByteSpan& content() const { return fContent; }

		// Load the text content if it exists
		void loadContentNode(const XmlElement& elem, IAmGroot* groot) override
		{
			fContent = elem.data();
		}
	};
}
