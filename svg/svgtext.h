#pragma once


#include "svgstructuretypes.h"
#include "svgattributes.h"

//
// Text - text and tspan elements
//
namespace waavs {


	static bool parseTextAnchor(const ByteSpan& inChunk, ALIGNMENT& value)
	{
		if (inChunk == "start")
			value = ALIGNMENT::LEFT;
		else if (inChunk == "middle")
			value = ALIGNMENT::CENTER;
		else if (inChunk == "end")
			value = ALIGNMENT::RIGHT;
		else
			return false;

		return true;
	}

	static bool parseTextAlign(const ByteSpan& inChunk, ALIGNMENT& value)
	{
		if (inChunk == "start")
			value = ALIGNMENT::LEFT;
		else if (inChunk == "middle")
			value = ALIGNMENT::CENTER;
		else if (inChunk == "end")
			value = ALIGNMENT::RIGHT;
		else
			return false;

		return true;
	}

	
	
}





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

		const BLFont& font() const { return fFont; }
		void font(const BLFont& aFont) { fFont = aFont; set(true); }
		double descent() const {
			return fFont.metrics().descent;
		}
			
		void bindToGroot(IAmGroot* groot, SVGViewable* container) override
		{
			if (!isSet()) {
				return;
			}

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
		
		// calcTextPosition
// Given a piece of text, and a coordinate
// calculate its baseline given the a specified alignment
		BLRect calcTextPosition(const ByteSpan& txt, double x, double y, ALIGNMENT hAlignment = ALIGNMENT::LEFT, ALIGNMENT vAlignment = ALIGNMENT::BASELINE) const
		{
			BLPoint txtSize = textMeasure(txt);
			double cx = txtSize.x;
			double cy = txtSize.y;

			switch (hAlignment)
			{
			case ALIGNMENT::LEFT:
				// do nothing
				// x = x;
				break;
			case ALIGNMENT::CENTER:
				x = x - (cx / 2);
				break;
			case ALIGNMENT::RIGHT:
				x = x - cx;
				break;

			default:
				break;
			}

			switch (vAlignment)
			{
			case ALIGNMENT::TOP:
				y = y + cy - descent();
				break;
			case ALIGNMENT::CENTER:
				y = y + (cy / 2);
				break;

			case ALIGNMENT::MIDLINE:
				//should use the design metrics xheight
				break;

			case ALIGNMENT::BASELINE:
				// If what was passed as y is the baseline
				// do nothing to it because blend2d draws
				// text from baseline
				break;

			case ALIGNMENT::BOTTOM:
				// Adjust from the bottom as blend2d
				// prints from the baseline, so adjust
				// by the amount of the descent
				y = y - descent();
				break;

			default:
				break;
			}

			return { x, y, cx, cy };
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

		
		SVGTextRun(const ByteSpan &txt, IAmGroot* groot)
			: SVGGraphicsElement(groot)
			, fText(txt)
		{
			name("textrun");
			needsBinding(true);
		}

		ByteSpan text() const { return fText; }
		
		BLRect getBBox() const override { return BLRect{}; }


		
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

		BLPoint fTextCursor{};
		//BLPoint fPosition{};	// The next position where text will be drawn
		ALIGNMENT fTextHAlignment = ALIGNMENT::LEFT;
		ALIGNMENT fTextVAlignment = ALIGNMENT::BASELINE;

		SVGFontSelection fFontSelection{ nullptr };

		double fX{ 0 };
		double fY{ 0 };
		double fDx{ 0 };
		double fDy{ 0 };
		
		SVGDimension fDimX{};
		SVGDimension fDimY{};
		SVGDimension fDimDy{};
		SVGDimension fDimDx{};

		
		SVGTSpanNode(IAmGroot* aroot) :SVGGraphicsElement(aroot) 
		{
			needsBinding(true);
		}


		void textCursor(const BLPoint& pos) { fTextCursor = pos; }
		BLPoint textCursor() const { return fTextCursor; }
		
		
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

			addNode(node, groot);
		}
		
		
		void loadSelfClosingNode(const XmlElement& elem, IAmGroot* groot) override
		{
			if (elem.tagName() == "tspan")
			{
				auto node = std::make_shared<SVGTSpanNode>(groot);
				node->loadFromXmlElement(elem, groot);
				addNode(node, groot);
			}
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
			fDimDy.loadFromChunk(getAttribute("dy"));
			fDimDx.loadFromChunk(getAttribute("dx"));

			fX = fTextCursor.x;
			fY = fTextCursor.y;
			
			if (fDimX.isSet())
				fX = fDimX.calculatePixels(w, 0, 96);
			if (fDimY.isSet())
				fY = fDimY.calculatePixels(h, 0, 96);
			if (fDimDx.isSet())
				fDx = fDimDx.calculatePixels(w, 0, 96);
			if (fDimDy.isSet())
				fDy = fDimDy.calculatePixels(h, 0, 96);

			fTextCursor.x = fX + fDx;
			fTextCursor.y = fY + fDy;

			parseTextAnchor(getAttribute("text-anchor"), fTextHAlignment);
			parseTextAlign(getAttribute("text-align"), fTextVAlignment);


			fFontSelection.loadFromXmlAttributes(*this, groot);
			fFontSelection.bindToGroot(groot, container);
		}

		// Contains styling attributes
		void applyAttributes(IRenderSVG* ctx, IAmGroot* groot) override
		{
			// Apply transform if it's not the identity transform
			if (fHasTransform)  //fTransform.type() != BL_MATRIX2D_TYPE_IDENTITY)
				ctx->applyTransform(fTransform);

			// BUGBUG - It might be useful to pass in the visual object
			// as additional context for attributes such as gradients
			// that might need that
			for (auto& prop : fVisualProperties) {
				if (prop.second->autoDraw() && prop.second->isSet())
					prop.second->draw(ctx, groot);
			}
		}
		
		void drawChildren(IRenderSVG* ctx, IAmGroot* groot) override
		{
			if (!fFontSelection.isSet())
			{
				fFontSelection.font(ctx->font());
			}

			if (fDimX.isSet())
				fTextCursor.x = fX;
			if (fDimY.isSet())
				fTextCursor.y = fY;
			


			for (auto& node : fNodes) {


				// dynamic cast the node to a SVGTextRun if possible
				auto textNode = std::dynamic_pointer_cast<SVGTextRun>(node);
				if (nullptr != textNode)
				{
					BLRect pRect = fFontSelection.calcTextPosition(textNode->text(), fTextCursor.x, fTextCursor.y, fTextHAlignment, fTextVAlignment);
					fFontSelection.draw(ctx, groot);
					ctx->text(textNode->text(), pRect.x, pRect.y);
					fTextCursor.x += pRect.w;
				}
				else
				{
					// dynamic cast the node to a SVGTSpanNode if possible
					auto tspanNode = std::dynamic_pointer_cast<SVGTSpanNode>(node);
					
					if (nullptr != tspanNode)
					{
						tspanNode->textCursor(fTextCursor);
						tspanNode->draw(ctx, groot);
						fTextCursor = tspanNode->textCursor();
					}
				}

			}
			
			// tell the outside world where we left off
			//fPosition = fStartPosition;
		}

		void draw(IRenderSVG* ctx, IAmGroot* groot) override
		{
			if (!visible())
				return;

			ctx->push();

			applyAttributes(ctx, groot);

			drawChildren(ctx, groot);

			ctx->pop();
		}
		

		// BUGBUG - this needs to be restructured
// such that the css and style attributes are applied
// and then the display properties can override them
		void loadFromXmlElement(const XmlElement& elem, IAmGroot* groot) override
		{
			scanAttributes(elem.data());

			// save the id if we've got an id attribute
			id(getAttribute("id"));

			// Save the name if we've got one
			name(elem.name());




			loadCommonVisualProperties(groot);
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
	struct SVGTextNode : public SVGTSpanNode
	{
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["text"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGTextNode>(groot);
				node->loadFromXmlIterator(iter, groot);
				return node;
				};
		}
		

		SVGTextNode(IAmGroot* groot) 
			:SVGTSpanNode(groot)
		{
			needsBinding(true);
		}



	};
}

