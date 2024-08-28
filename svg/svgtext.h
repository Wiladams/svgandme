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

		BLPoint textMeasure(const ByteSpan& txt) const
		{
			BLTextMetrics tm;
			BLGlyphBuffer gb;

			gb.setUtf8Text(txt.data(), txt.size());
			fFont.shape(gb);
			fFont.getTextMetrics(gb, tm);

			float cx = (float)(tm.boundingBox.x1 - tm.boundingBox.x0);
			//float cy = fFont.size();
			float cy = fFont.metrics().ascent + fFont.metrics().descent;

			return BLPoint(cx, cy);
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
	// SVGTextRun
	// This takes care of all the details of rendering a run of text
	// It contains the text itself
	// positioning information
	// style information
	struct SVGTextRun : public SVGGraphicsElement
	{
		ByteSpan fText{};
		BLPoint fTextSize{};
		BLPoint fPosition{};

		
		SVGFontSelection fFontSelection{ nullptr };
		
		

		
		
		SVGTextRun(const ByteSpan &txt, IAmGroot* groot)
			: SVGGraphicsElement(groot)
			, fText(txt)
			, fFontSelection(groot)
		{
			name("textrun");
			needsBinding(true);
		}

		BLPoint textSize() const { return fTextSize; }
		
		void resolveStyle(IAmGroot* groot, SVGViewable* container) override
		{
			fFontSelection.loadFromXmlAttributes(*this, groot);
			fFontSelection.bindToGroot(groot, container);

			// Measure the text to see how big it is
			fTextSize = fFontSelection.textMeasure(fText);
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

			SVGDimension fX{};
			SVGDimension fY{};
			SVGDimension fDy{};
			SVGDimension fDx{};
			
			fX.loadFromChunk(getAttribute("x"));
			fY.loadFromChunk(getAttribute("y"));
			fDy.loadFromChunk(getAttribute("dy"));
			fDx.loadFromChunk(getAttribute("dx"));
			
			
			if (fX.isSet())
				fPosition.x = fX.calculatePixels(w, 0, 96);
			if (fY.isSet())
				fPosition.y = fY.calculatePixels(h, 0, 96);
			if (fDx.isSet())
				fPosition.x += fDx.calculatePixels(w, 0, 96);
			if (fDy.isSet())
				fPosition.y += fDy.calculatePixels(h, 0, 96);


		}

		
		void draw(IRenderSVG* ctx, IAmGroot* groot) override
		{
			ctx->textPosition(fPosition.x, fPosition.y);
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


		SVGTSpanNode(IAmGroot* aroot) :SVGGraphicsElement(aroot) 
		{
			needsBinding(true);
		}


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

		BLPoint fPosition{};	// The next position where text will be drawn
		

		SVGTextNode(IAmGroot* aroot) 
			:SVGGraphicsElement(aroot)
		{
			needsBinding(true);
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

			SVGDimension fX{};
			SVGDimension fY{};
			SVGDimension fDy{};
			SVGDimension fDx{};
			
			fX.loadFromChunk(getAttribute("x"));
			fY.loadFromChunk(getAttribute("y"));
			fDy.loadFromChunk(getAttribute("dy"));
			fDx.loadFromChunk(getAttribute("dx"));


			if (fX.isSet())
				fPosition.x = fX.calculatePixels(w, 0, 96);
			if (fY.isSet())
				fPosition.y = fY.calculatePixels(h, 0, 96);
			if (fDx.isSet())
				fPosition.x += fDx.calculatePixels(w, 0, 96);
			if (fDy.isSet())
				fPosition.y += fDy.calculatePixels(h, 0, 96);

		}

		
		void bindToGroot(IAmGroot* groot, SVGViewable* container) override
		{
			resolvePosition(groot, container);
			resolveStyle(groot, container);
			
			bindPropertiesToGroot(groot, container);
			
			bindChildrenToGroot(groot, container);

			needsBinding(false);
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

		// Load some text content
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

		void drawChildren(IRenderSVG* ctx, IAmGroot* groot) override
		{
			for (auto& node : fNodes) {


				// dynamic cast the node to a SVGTextRun if possible
				auto textNode = std::dynamic_pointer_cast<SVGTextRun>(node);
				if (nullptr != textNode)
				{
					printf("TEXT SIZE: %f %f\n", textNode->textSize().x, textNode->textSize().y);
					ctx->textPosition(fPosition.x, fPosition.y);
					textNode->draw(ctx, groot);
				}
				else
				{
					// dynamic cast the node to a SVGTSpanNode if possible
					auto tspanNode = std::dynamic_pointer_cast<SVGTSpanNode>(node);
					if (nullptr != tspanNode)
					{
						tspanNode->draw(ctx, groot);
					}
				}

			}
		}
		

	};
}

