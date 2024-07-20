#pragma once


#include "svgstructuretypes.h"
#include "svgattributes.h"

//
// Text - text and tspan elements
//

namespace waavs {
	//
	// SVGTextcontentNode
	// Not strictly a part of the DOM, but a useful representation 
	// of text content.  This is the node that actually displays text
	// They are contained as child nodes either of the 'text' or 'tspan'
	// elements.
	struct SVGTextContentNode : public SVGVisualNode
	{
		ByteSpan fText{};
		
		SVGTextContentNode(IAmGroot* root) 
			: SVGVisualNode(root) 
		{ 
			name("text"); 
		}

		void text(const ByteSpan& aSpan)
		{
			// BUGBUG - We still want to represent the text as a ByteSpan
			// So we can use the BLStringView when we eventually draw the text

			//fText = expandStandardEntities(aSpan);
			fText = aSpan;
			//expandBasicEntities(aSpan, fText);
		}

		void draw(IRenderSVG* ctx) override
		{
			ctx->text(fText);
			//ctx->text(fText.c_str());
		}
	};


	// A span has coordinate attributes, and possibly text content
	// It can also be a container of more spans as well, so it is
	// a compound node.
	struct SVGTSpanNode : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["tspan"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGTSpanNode>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
				};
		}

		double fXOffset = 0;
		double fYOffset = 0;

		SVGDimension fX{};
		SVGDimension fY{};
		SVGDimension fDy{};
		SVGDimension fDx{};
		SVGFontSelection fFontSelection{ nullptr };


		SVGTSpanNode(IAmGroot* aroot) :SVGGraphicsElement(aroot) {}

		void fontSelection(const SVGFontSelection& aSelection)
		{
			fFontSelection = aSelection;
		}

		void bindSelfToGroot(IAmGroot* groot) override
		{
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}


			if (fX.isSet())
				fXOffset = fX.calculatePixels(w, 0, 96);
			if (fY.isSet())
				fYOffset = fY.calculatePixels(h, 0, 96);
			if (fDx.isSet())
				fXOffset += fDx.calculatePixels(w, 0, 96);
			if (fDy.isSet())
				fYOffset += fDy.calculatePixels(h, 0, 96);

			if (fFontSelection.isSet())
				fFontSelection.bindToGroot(groot);

			needsBinding(false);
		}

		void applyAttributes(IRenderSVG* ctx) override
		{
			SVGGraphicsElement::applyAttributes(ctx);

			if (fX.isSet())
				ctx->textPosition(fXOffset, fYOffset);

			fFontSelection.draw(ctx);
		}

		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGraphicsElement::loadVisualProperties(attrs);
			fFontSelection.loadFromXmlAttributes(attrs);

			if (attrs.getAttribute("x"))
				fX.loadFromChunk(attrs.getAttribute("x"));
			if (attrs.getAttribute("y"))
				fY.loadFromChunk(attrs.getAttribute("y"));
			if (attrs.getAttribute("dx"))
				fDx.loadFromChunk(attrs.getAttribute("dx"));
			if (attrs.getAttribute("dy"))
				fDy.loadFromChunk(attrs.getAttribute("dy"));

			needsBinding(true);
		}


		// Load the text content if it exists
		void loadContentNode(const XmlElement& elem) override
		{
			// Create a text content node and 
			// add it to our node set
			auto node = std::make_shared<SVGTextContentNode>(root());
			node->text(elem.data());
			addNode(node);
		}

		void loadSelfClosingNode(const XmlElement& elem) override
		{
			auto node = std::make_shared<SVGTSpanNode>(root());
			node->fontSelection(fFontSelection);

			node->loadFromXmlElement(elem);
			addNode(node);
		}

		void loadCompoundNode(XmlElementIterator& iter) override
		{
			// Most likely a <tspan>
			auto& elem = *iter;
			if ((*iter).tagName() == "tspan")
			{
				auto node = std::make_shared<SVGTSpanNode>(root());
				node->fontSelection(fFontSelection);

				node->loadFromXmlIterator(iter);
				addNode(node);
			}
			else
			{
				// Load the sub-node, and throw it away
				SVGGraphicsElement::loadCompoundNode(iter);
			}
		}

	};


	//=====================================================
// SVGTextNode
// There is a reasonable temptation to make this a sub-class
// of tspan, as most of their behavior is the same.  There
// are subtle enough differences though that it is probably
// better to keep them separate, and introduce a core base
// class later for better factoring.
//=====================================================
	struct SVGTextNode : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["text"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGTextNode>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
				};
		}

		double fXOffset = 0;
		double fYOffset = 0;

		SVGDimension fX{};
		SVGDimension fY{};
		SVGDimension fDy{};
		SVGDimension fDx{};
		SVGFontSelection fFontSelection{ nullptr };


		SVGTextNode(IAmGroot* aroot) 
			:SVGGraphicsElement(aroot)
		{
		}

		void fontSelection(const SVGFontSelection& fs)
		{
			fFontSelection = fs;
		}

		void applyAttributes(IRenderSVG* ctx) override
		{
			SVGGraphicsElement::applyAttributes(ctx);

			// set the text alignment to left, baseline
			//ctx->textAlign(ALIGNMENT::LEFT, ALIGNMENT::BASELINE);
			ctx->textPosition(fXOffset, fYOffset);
			fFontSelection.draw(ctx);


		}



		void bindSelfToGroot(IAmGroot* groot) override
		{

			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}



			if (fX.isSet())
				fXOffset = fX.calculatePixels(w, 0, dpi);
			if (fY.isSet())
				fYOffset = fY.calculatePixels(h, 0, dpi);
			if (fDx.isSet())
				fXOffset += fDx.calculatePixels(w, 0, dpi);
			if (fDy.isSet())
				fYOffset += fDy.calculatePixels(h, 0, dpi);


			if (fFontSelection.isSet())
				fFontSelection.bindToGroot(groot);

			needsBinding(false);
		}

		// This is where we can grab the font attributes
		// whether they are inline presentation attributes
		// or coming from inline style attribute
		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGraphicsElement::loadVisualProperties(attrs);

			fX.loadFromChunk(getAttribute("x"));
			fY.loadFromChunk(getAttribute("y"));
			fDy.loadFromChunk(getAttribute("dy"));
			fDx.loadFromChunk(getAttribute("dx"));

			fFontSelection.loadFromXmlAttributes(attrs);

			needsBinding(true);
		}



		// Load the text content if it exists
		void loadContentNode(const XmlElement& elem) override
		{
			// Create a text content node and 
			// add it to our node set
			auto node = std::make_shared<SVGTextContentNode>(root());
			node->text(elem.data());
			addNode(node);
		}

		// Typically a TSpan with no content?
		void loadSelfClosingNode(const XmlElement& elem) override
		{
			auto node = std::make_shared<SVGTSpanNode>(root());
			node->fontSelection(fFontSelection);

			node->loadFromXmlElement(elem);
			addNode(node);
		}

		// Typically a tspan
		void loadCompoundNode(XmlElementIterator& iter) override
		{
			// Most likely a <tspan>
			auto& elem = *iter;
			if ((*iter).tagName() == "tspan")
			{
				auto node = std::make_shared<SVGTSpanNode>(root());
				node->fontSelection(fFontSelection);

				node->loadFromXmlIterator(iter);
				addNode(node);
			}
			else
			{
				// Some compound node we don't know about
				// just load its whole sub-tree to ignore it
				SVGGraphicsElement::loadCompoundNode(iter);
			}
		}

	};
}

