#pragma once


#include "svgstructuretypes.h"
#include "svgattributes.h"

//
// Text - text and tspan elements
//

namespace waavs {
	struct SVGFontSelection : public SVGVisualProperty
	{
		BLFont fFont;

		std::string fFamilyName{};
		SVGFontSize fFontSize;
		uint32_t fFontStyle = BL_FONT_STYLE_NORMAL;
		uint32_t fFontWeight = BL_FONT_WEIGHT_NORMAL;
		uint32_t fFontStretch = BL_FONT_STRETCH_NORMAL;


		SVGFontSelection(IAmGroot* groot)
			: SVGVisualProperty(groot)
			, fFontSize(groot)
		{
			needsBinding(true);
			set(false);
		}

		SVGFontSelection& operator=(const SVGFontSelection& rhs)
		{
			fFont.reset();

			fFamilyName = rhs.fFamilyName;
			fFontSize = rhs.fFontSize;
			fFontStyle = rhs.fFontStyle;
			fFontWeight = rhs.fFontWeight;
			fFontStretch = rhs.fFontStretch;

			set(false);
			needsBinding(true);

			return *this;
		}

		void bindToGroot(IAmGroot* groot, SVGViewable* container) override
		{
			if (!isSet())
				return;

			FontHandler* fh = groot->fontHandler();

			// resolve the size
			// lookup the font face
			fFontSize.bindToGroot(groot, container);
			auto fsize = fFontSize.value();

			bool success = fh->selectFont(fFamilyName.c_str(), fFont, (float)fsize, fFontStyle, fFontWeight, fFontStretch);
			if (success)
				set(true);
		}

		void loadFromXmlAttributes(const XmlAttributeCollection& elem, IAmGroot* groot)
		{
			// look for font-family
			auto familyChunk = elem.getAttribute("font-family");
			if (familyChunk) {
				fFamilyName = std::string(familyChunk.fStart, familyChunk.fEnd);
				set(true);
			}

			// look for font-size
			// This can get resolved at binding time
			fFontSize.loadFromChunk(elem.getAttribute("font-size"));
			if (fFontSize.isSet())
				set(true);

			// look for font-style
			SVGFontStyleAttribute styleAttribute;
			styleAttribute.loadFromChunk(elem.getAttribute("font-style"));
			if (styleAttribute.isSet()) {
				fFontStyle = styleAttribute.value();
				set(true);
			}

			// look for font-weight
			SVGFontWeightAttribute weightAttribute;
			weightAttribute.loadFromChunk(elem.getAttribute("font-weight"));
			if (weightAttribute.isSet()) {
				fFontWeight = weightAttribute.value();
				set(true);
			}

			// look for font-stretch
			SVGFontStretchAttribute stretchAttribute;
			stretchAttribute.loadFromChunk(elem.getAttribute("font-stretch"));
			if (stretchAttribute.isSet()) {
				fFontStretch = stretchAttribute.value();
				set(true);
			}
		}

		void draw(IRenderSVG* ctx, IAmGroot* groot) override
		{
			// BUGBUG - not quite sure if we need both checks
			//if (isSet() && visible())
			if (isSet())
				ctx->font(fFont);
		}
	};
}

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

		void draw(IRenderSVG* ctx, IAmGroot* groot) override
		{
			ctx->text(fText);
		}
	};
}

namespace waavs {
	// SVGTextRun
	// This takes care of all the details of rendering a run of text
	// It contains the text itself
	// positioning information
	// style information
	struct SVGTextRun : public SVGGraphicsElement
	{
		ByteSpan fText{};
		
		//BLPoint fPosition{};
		//BLFontMetrics fMetrics{};
		//BLTextMetrics fTextMetrics{};
		
		SVGFontSelection fFontSelection{ nullptr };
		
		double fXOffset = 0;
		double fYOffset = 0;
		
		SVGDimension fX{};
		SVGDimension fY{};
		SVGDimension fDy{};
		SVGDimension fDx{};
		
		
		SVGTextRun(const ByteSpan &txt, IAmGroot* groot)
			: SVGGraphicsElement(groot)
			, fText(txt)
			, fFontSelection(groot)
		{
			name("textrun");
		}


		void resolveStyle(IAmGroot* groot, SVGViewable* container) override
		{
			fFontSelection.bindToGroot(groot, container);
		}
		

		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
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

			fX.loadFromChunk(getAttribute("x"));
			fY.loadFromChunk(getAttribute("y"));
			fDy.loadFromChunk(getAttribute("dy"));
			fDx.loadFromChunk(getAttribute("dx"));
			
			
			if (fX.isSet())
				fXOffset = fX.calculatePixels(w, 0, 96);
			if (fY.isSet())
				fYOffset = fY.calculatePixels(h, 0, 96);
			if (fDx.isSet())
				fXOffset += fDx.calculatePixels(w, 0, 96);
			if (fDy.isSet())
				fYOffset += fDy.calculatePixels(h, 0, 96);

		}

		
		void draw(IRenderSVG* ctx, IAmGroot* groot) override
		{
			ctx->textPosition(fXOffset, fYOffset);
			fFontSelection.draw(ctx, groot);

			ctx->text(fText);
		}
	};
}


namespace waavs {

	// A span has coordinate attributes, and possibly text content
	// It can also be a container of more spans as well, so it is
	// a compound node.
	struct SVGTSpanNode : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["tspan"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGTSpanNode>(groot);
				node->loadFromXmlIterator(iter, groot);
				
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

		//void fontSelection(const SVGFontSelection& aSelection)
		//{
		//	fFontSelection = aSelection;
		//}

		/*
		void applyAttributes(IRenderSVG* ctx, IAmGroot* groot) override
		{
			SVGGraphicsElement::applyAttributes(ctx, groot);

			//if (fX.isSet())
			//	ctx->textPosition(fXOffset, fYOffset);

			//fFontSelection.draw(ctx, groot);
		}
		*/
		/*
		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
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
				fFontSelection.bindToGroot(groot, container);

			needsBinding(false);
		}
		*/
		/*
		void resolveStyle(IAmGroot* groot, SVGViewable* container) override
		{
			SVGGraphicsElement::resolveStyle(groot, container);

			fFontSelection.loadFromXmlAttributes(*this, groot);

			if (fFontSelection.isSet())
				fFontSelection.bindToGroot(groot, container);
		}
		*/
		/*
		void loadVisualProperties(const XmlAttributeCollection& attrs, IAmGroot* groot) override
		{
			SVGGraphicsElement::loadVisualProperties(attrs, groot);

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
		*/

		// Load the text content if it exists
				// Load the text content if it exists
		void loadContentNode(const XmlElement& elem, IAmGroot* groot) override
		{
			// Create a text content node and 
			// add it to our node set
			auto node = std::make_shared<SVGTextRun>(elem.data(), groot);

			// fail fast if the node was not created
			if (nullptr == node)
				return;

			// If we have the node, then tell it to load itself up
			// based on the attributes we contain
			node->mergeAttributes(*this);

			addNode(node, groot);
		}
		
		
		void loadSelfClosingNode(const XmlElement& elem, IAmGroot* groot) override
		{
			auto node = std::make_shared<SVGTSpanNode>(groot);
			//node->fontSelection(fFontSelection);

			node->loadFromXmlElement(elem, groot);
			addNode(node, groot);
		}

		void loadCompoundNode(XmlElementIterator& iter, IAmGroot* groot) override
		{
			// Most likely a <tspan>
			auto& elem = *iter;
			if ((*iter).tagName() == "tspan")
			{
				auto node = std::make_shared<SVGTSpanNode>(groot);
				//node->fontSelection(fFontSelection);

				node->loadFromXmlIterator(iter, groot);
				addNode(node, groot);
			}
			else
			{
				// Load the sub-node, and throw it away
				SVGGraphicsElement::loadCompoundNode(iter, groot);
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
			gSVGGraphicsElementCreation["text"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGTextNode>(groot);
				node->loadFromXmlIterator(iter, groot);
				return node;
				};
		}

		SVGTextNode(IAmGroot* aroot) 
			:SVGGraphicsElement(aroot)
		{
		}

		void applyAttributes(IRenderSVG* ctx, IAmGroot* groot) override
		{
			// BUGBUG - kind of  a hack
			// we need the alignment to start at a default, and be reset
			// by text-align, and text-anchor.
			// we need this here as the tspan element might have changed it
			// and the context does not pop the text styling.
			ctx->textAlign(ALIGNMENT::LEFT, ALIGNMENT::BASELINE);

			SVGGraphicsElement::applyAttributes(ctx, groot);
		}

		// Load the text content if it exists
		void loadContentNode(const XmlElement& elem, IAmGroot* groot) override
		{
			// Create a text content node and 
			// add it to our node set
			auto node = std::make_shared<SVGTextRun>(elem.data(), groot);
			
			// fail fast if the node was not created
			if (nullptr == node)
				return;
			
			// If we have the node, then tell it to load itself up
			// based on the attributes we contain
			node->mergeAttributes(*this);
			
			addNode(node, groot);
		}

		// Typically a TSpan with no content?
		void loadSelfClosingNode(const XmlElement& elem, IAmGroot* groot) override
		{
			auto node = std::make_shared<SVGTSpanNode>(nullptr);

			node->loadFromXmlElement(elem, groot);
			addNode(node, groot);
		}

		// Typically a tspan
		void loadCompoundNode(XmlElementIterator& iter, IAmGroot* groot) override
		{
			// Most likely a <tspan>
			auto& elem = *iter;
			if ((*iter).tagName() == "tspan")
			{
				auto node = std::make_shared<SVGTSpanNode>(nullptr);

				node->loadFromXmlIterator(iter, groot);
				addNode(node, groot);
			}
			else
			{
				// Some compound node we don't know about
				// just load its whole sub-tree to ignore it
				SVGGraphicsElement::loadCompoundNode(iter, groot);
			}
		}

	};
}

