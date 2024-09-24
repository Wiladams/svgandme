#pragma once


#include "svgstructuretypes.h"
#include "svgattributes.h"
#include "xmlentity.h"


namespace waavs {
	static BLPoint textMeasure(const BLFont& font, const ByteSpan& txt) noexcept
	{
		BLTextMetrics tm;
		BLGlyphBuffer gb;

		gb.setUtf8Text(txt.data(), txt.size());
		font.shape(gb);
		font.getTextMetrics(gb, tm);

		float cx = (float)(tm.boundingBox.x1 - tm.boundingBox.x0);
		float cy = font.metrics().ascent + font.metrics().descent;

		return BLPoint(cx, cy);
	}

	static double descent(const BLFont& font) noexcept
	{
		return font.metrics().descent;
	}

	static double emHeight(const BLFont& font) noexcept
	{
		auto size = textMeasure(font, "M");
		return size.y;
	}

	static double exHeight(const BLFont& font) noexcept
	{
		auto size = textMeasure(font, "x");
		return size.y;
	}

	// calcTextPosition
// Given a piece of text, and a coordinate
// calculate its baseline given the a specified alignment
	static BLRect calcTextPosition(BLFont font, const ByteSpan& txt, double x, double y, TXTALIGNMENT hAlignment = TXTALIGNMENT::LEFT, TXTALIGNMENT vAlignment = TXTALIGNMENT::BASELINE, DOMINANTBASELINE baseline = DOMINANTBASELINE::AUTO)
	{
		BLPoint txtSize = textMeasure(font, txt);
		double cx = txtSize.x;
		double cy = txtSize.y;

		switch (hAlignment)
		{
		case TXTALIGNMENT::LEFT:
			// do nothing
			// x = x;
			break;
		case TXTALIGNMENT::CENTER:
			x = x - (cx / 2);
			break;
		case TXTALIGNMENT::RIGHT:
			x = x - cx;
			break;

		default:
			break;
		}

		switch (vAlignment)
		{
		case TXTALIGNMENT::TOP:
			y = y + cy - descent(font);
			break;
		case TXTALIGNMENT::CENTER:
			y = y + (cy / 2);
			break;

		case TXTALIGNMENT::MIDLINE:
			//should use the design metrics xheight
			break;

		case TXTALIGNMENT::BASELINE:
			// If what was passed as y is the baseline
			// do nothing to it because blend2d draws
			// text from baseline
			break;

		case TXTALIGNMENT::BOTTOM:
			// Adjust from the bottom as blend2d
			// prints from the baseline, so adjust
			// by the amount of the descent
			y = y - descent(font);
			break;

		default:
			break;
		}

		switch (baseline)
		{
		case DOMINANTBASELINE::HANGING:
			y = y + emHeight(font);
			break;

		case DOMINANTBASELINE::CENTRAL:
		case DOMINANTBASELINE::MIDDLE:
			// adjust by half the height
			y = y + (exHeight(font) / 2);
			break;
		}

		return { x, y, cx, cy };
	}
}


/*
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

		

		

		
		void bindToGroot(IAmGroot* groot, SVGViewable* container) noexcept override
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


		

		
		void draw(IRenderSVG* ctx, IAmGroot* groot) override
		{
			// BUGBUG - not quite sure if we need both checks
			//if (isSet() && visible())
			//if (isSet())
			//	ctx->font(fFont);
		}
	};

}
*/



//
// 'text' and 'tspan'
// 
// The difference between these two are:
//  - 'tspan', while it can be a container (containing more tspans), it can 
//    not appear outside a 'text' container
//  - When style is applied in a 'tspan', it remains local to that tspan, and will not
//    affect sibling tspans
//  - 'x' and 'y' positions, 'text' element, default to '0', while 'tspan' default to the 
//    whereever the text cursor currently sits.
//
namespace waavs {
	// SVGTextRun
	// This takes care of all the details of rendering a run of text
	// It contains the text itself
	// positioning information
	// style information

	//
	// SVGTextRun
	//
	// Encapsulates a run of text.  That is, a piece of text that has a 
	// particular style.  It is found either as direct nodes of a 'text'
	// element, or as the content of a 'tspan'.
	struct SVGTextRun : public SVGVisualNode
	{
		ByteSpan fText{};
		BLPoint fTextSize{};
		BLRect fBBox;
		
		SVGTextRun(const ByteSpan &txt, IAmGroot* groot)
			: SVGVisualNode(groot)
			, fText(txt)
		{
			name("textrun");
			needsBinding(true);
			
			expandXmlEntities(fText, fText);
		}

		ByteSpan text() const { return fText; }
		
		BLRect getBBox() const override { return fBBox; }
		void setBBox(const BLRect& box) { fBBox = box; }

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


		TXTALIGNMENT fTextHAlignment = TXTALIGNMENT::LEFT;
		TXTALIGNMENT fTextVAlignment = TXTALIGNMENT::BASELINE;
		DOMINANTBASELINE fDominantBaseline = DOMINANTBASELINE::AUTO;
		
		//SVGFontSelection fFontSelection{ nullptr };

		BLRect fBBox{};
		
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

		
		BLRect getBBox() const override { return fBBox; }
		
		
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


			getEnumValue(SVGTextAnchor, getAttribute("text-anchor"), (int &)fTextHAlignment);
			getEnumValue(SVGTextAlign, getAttribute("text-align"), (int&)fTextVAlignment);
			getEnumValue(SVGDominantBaseline, getAttribute("dominant-baseline"), (int&)fDominantBaseline);
			
		}

		
		void drawChildren(IRenderSVG* ctx, IAmGroot* groot) override
		{
			fBBox = {};
			
			for (auto& node : fNodes) 
			{

				// dynamic cast the node to a SVGTextRun if possible
				auto textNode = std::dynamic_pointer_cast<SVGTextRun>(node);
				if (nullptr != textNode)
				{
					TXTALIGNMENT anchor = ctx->textAnchor();
					ByteSpan txt = textNode->text();
					BLPoint pos = ctx->textCursor();
					BLRect pRect = calcTextPosition(ctx->font(), txt, pos.x, pos.y, anchor, fTextVAlignment, fDominantBaseline);
					expandRect(fBBox, pRect);
					
					ByteSpan porder = getAttribute("paint-order");
					
					if (!porder || porder=="normal") {
						ctx->fillText(txt, pRect.x, pRect.y);
						ctx->strokeText(txt, pRect.x, pRect.y);
					}
					else {
						std::list<ByteSpan> alist = {
							ByteSpan("fill"),
							ByteSpan("stroke"),
							ByteSpan("markers")
						};
						
						// get paint order tokens one at a time
						while (porder) {
							auto ptoken = chunk_token(porder, chrWspChars);
							if (ptoken.empty())
								break;

							if (ptoken == "fill")
								ctx->fillText(txt, pRect.x, pRect.y);
							else if (ptoken == "stroke")
								ctx->strokeText(txt, pRect.x, pRect.y);

							alist.remove(ptoken);
						}

						// If there's anything still in the list, then draw that
						for (auto& token : alist) {
							if (token == "fill")
								ctx->fillText(txt, pRect.x, pRect.y);
							else if (token == "stroke")
								ctx->strokeText(txt, pRect.x, pRect.y);
							else if (token == "markers")
							{
								// draw markers if we have any
								//if (fHasMarkers)
								//	drawMarkers(ctx, groot);
							}
						}
						
					}

					ctx->textCursor(BLPoint(pRect.x + pRect.w, pRect.y));
				}
				else
				{
					// dynamic cast the node to a SVGTSpanNode if possible
					auto tspanNode = std::dynamic_pointer_cast<SVGTSpanNode>(node);
					
					if (nullptr != tspanNode)
					{
						//ctx->textCursor(BLPoint(fX, fY));
						tspanNode->draw(ctx, groot);
					}
				}
			}
		}

		void draw(IRenderSVG* ctx, IAmGroot* groot) override
		{
			if (!visible())
				return;

			ctx->push();

			applyAttributes(ctx, groot);

			
			// For a span, the default position is wherever the container left the cursor
			// but if we have x, y, then we set the position explicitly
			BLRect cFrame = ctx->localFrame();
			auto w = cFrame.w;
			auto h = cFrame.h;
			double fSize = ctx->fontSize();
			
			// For a tspan, the default position is wherever the container left the cursor
			BLPoint cursor = ctx->textCursor();
			fX = cursor.x;
			fY = cursor.y;

			if (fDimX.isSet())
				fX = fDimX.calculatePixels(fSize, 0, 96);
			if (fDimY.isSet())
				fY = fDimY.calculatePixels(fSize, 0, 96);
			if (fDimDx.isSet())
				fDx = fDimDx.calculatePixels(fSize, 0, 96);
			if (fDimDy.isSet())
				fDy = fDimDy.calculatePixels(fSize, 0, 96);


			fX = fX + fDx;
			fY = fY + fDy;
			
			ctx->textCursor(BLPoint(fX, fY));

			drawChildren(ctx, groot);
			ctx->pop();
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
		}


		void draw(IRenderSVG* ctx, IAmGroot* groot) override
		{
			if (!visible())
				return;

			BLRect cFrame = ctx->localFrame();
			auto w = cFrame.w;
			auto h = cFrame.h;
			
			ctx->push();
			
			applyAttributes(ctx, groot);
			
			double fSize = ctx->fontSize();
			
			// For a text node, the default position is zero
			fX = 0;
			fY = 0;
			
			if (fDimX.isSet())
				fX = fDimX.calculatePixels(fSize, 0, 96);
			if (fDimY.isSet())
				fY = fDimY.calculatePixels(fSize, 0, 96);
			if (fDimDx.isSet())
				fDx = fDimDx.calculatePixels(fSize, 0, 96);
			if (fDimDy.isSet())
				fDy = fDimDy.calculatePixels(fSize, 0, 96);

			
			fX = fX + fDx;
			fY = fY + fDy;
			

			

			ctx->textCursor(BLPoint(fX, fY));

			drawChildren(ctx, groot);
			ctx->pop();
		}
	};
}

