#pragma once


#include "svgstructuretypes.h"
#include "svgattributes.h"
#include "xmlentity.h"


namespace waavs {
	struct Fontography {
		static BLPoint textMeasure(const BLFont& font, const ByteSpan& txt) noexcept
		{
			BLTextMetrics tm;
			BLGlyphBuffer gb;
			BLFontMetrics fm = font.metrics();

			gb.setUtf8Text(txt.data(), txt.size());
			font.shape(gb);
			font.getTextMetrics(gb, tm);

			float cx = (float)(tm.boundingBox.x1 - tm.boundingBox.x0);
			float cy = fm.ascent + fm.descent;

			return BLPoint(cx, cy);
		}

		static double ascent(const BLFont& font) noexcept
		{
			return font.metrics().ascent;
		}

		static double descent(const BLFont& font) noexcept
		{
			return font.metrics().descent;
		}

		static double capHeight(const BLFont& font) noexcept
		{
			return font.metrics().capHeight;
		}

		static double emHeight(const BLFont& font) noexcept
		{
			auto h = font.metrics().ascent + font.metrics().descent;
			return h;
		}

		static double exHeight(const BLFont& font) noexcept
		{
			return font.metrics().xHeight;
		}

		// calcTextPosition
		// Given a piece of text, and a coordinate
		// calculate its baseline given the a specified alignment
		static BLRect calcTextPosition(BLFont font, const ByteSpan& txt, double x, double y, SVGAlignment hAlignment = SVGAlignment::SVG_ALIGNMENT_START, TXTALIGNMENT vAlignment = TXTALIGNMENT::BASELINE, DOMINANTBASELINE baseline = DOMINANTBASELINE::AUTO)
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
				y = y + capHeight(font);
				break;

			case DOMINANTBASELINE::MATHEMATICAL:
				y = y + exHeight(font);
				break;

			case DOMINANTBASELINE::TEXT_BEFORE_EDGE:
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
	};
}






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
	struct SVGTextRun : public SVGGraphicsElement
	{
		ByteSpan fText{};
		BLPoint fTextSize{};
		BLRect fBBox{};
		
		SVGTextRun(const ByteSpan &txt, IAmGroot* groot)
			: SVGGraphicsElement()
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
			registerContainerNode("tspan",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGTSpanNode>(groot);
					node->loadFromXmlIterator(iter, groot);

					return node;
				});
			
		}


		TXTALIGNMENT fTextHAlignment = TXTALIGNMENT::LEFT;
		TXTALIGNMENT fTextVAlignment = TXTALIGNMENT::BASELINE;
		DOMINANTBASELINE fDominantBaseline = DOMINANTBASELINE::AUTO;
		

		BLRect fBBox{};
		
		double fX{ 0 };
		double fY{ 0 };
		double fDx{ 0 };
		double fDy{ 0 };
		
		SVGVariableSize fDimX{};
		SVGVariableSize fDimY{};
		SVGVariableSize fDimDy{};
		SVGVariableSize fDimDx{};

		
		SVGTSpanNode(IAmGroot* ) :SVGGraphicsElement() 
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
			//auto& elem = *iter;
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

		void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
		{
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}


			BLRect cFrame = ctx->viewport();
			w = cFrame.w;
			h = cFrame.h;




			fDimX.loadFromChunk(getAttribute("x"));
			fDimY.loadFromChunk(getAttribute("y"));
			fDimDy.loadFromChunk(getAttribute("dy"));
			fDimDx.loadFromChunk(getAttribute("dx"));


			getEnumValue(SVGTextAnchor, getAttribute("text-anchor"), (uint32_t &)fTextHAlignment);
			getEnumValue(SVGTextAlign, getAttribute("text-align"), (uint32_t &)fTextVAlignment);
			getEnumValue(SVGDominantBaseline, getAttribute("dominant-baseline"), (uint32_t &)fDominantBaseline);
			getEnumValue(SVGDominantBaseline, getAttribute("alignment-baseline"), (uint32_t&)fDominantBaseline);

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
					SVGAlignment anchor = ctx->textAnchor();
					ByteSpan txt = textNode->text();
					BLPoint pos = ctx->textCursor();
					BLRect pRect = Fontography::calcTextPosition(ctx->font(), txt, pos.x, pos.y, anchor, fTextVAlignment, fDominantBaseline);
					expandRect(fBBox, pRect);
					
					// Get the paint order from the context
					uint32_t porder = ctx->paintOrder();

					for (int slot = 0; slot < 3; slot++)
					{
						uint32_t ins = porder & 0x03;	// get two lowest bits, which are a single instruction

						switch (ins)
						{
						case PaintOrderKind::SVG_PAINT_ORDER_FILL:
							ctx->fillText(txt, pRect.x, pRect.y);
							break;

						case PaintOrderKind::SVG_PAINT_ORDER_STROKE:
							ctx->strokeText(txt, pRect.x, pRect.y);
							break;

						case PaintOrderKind::SVG_PAINT_ORDER_MARKERS:
						{
							//drawMarkers(ctx, groot);
						}
						break;
						}

						// discard instruction, shift down to get the next one ready
						porder = porder >> 2;
					}

					ctx->textCursor(BLPoint(pRect.x + pRect.w, pRect.y));
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

		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{	
			// For a span, the default position is wherever the container left the cursor
			// but if we have x, y, then we set the position explicitly
			BLRect cFrame = ctx->viewport();
			auto w = cFrame.w;
			auto h = cFrame.h;

			
			// For a tspan, the default position is wherever the container left the cursor
			BLPoint cursor = ctx->textCursor();
			fX = cursor.x;
			fY = cursor.y;

			if (fDimX.isSet())
				fX = fDimX.calculatePixels(ctx->font(), w, 0, 96);
			if (fDimY.isSet())
				fY = fDimY.calculatePixels(ctx->font(), h, 0, 96);
			if (fDimDx.isSet())
				fDx = fDimDx.calculatePixels(ctx->font(), w, 0, 96);
			if (fDimDy.isSet())
				fDy = fDimDy.calculatePixels(ctx->font(), h, 0, 96);


			fX = fX + fDx;
			fY = fY + fDy;
			
			ctx->textCursor(BLPoint(fX, fY));

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
			registerContainerNode("text",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGTextNode>(groot);
					node->loadFromXmlIterator(iter, groot);
					return node;
				});
			
		}
		

		SVGTextNode(IAmGroot* groot) 
			:SVGTSpanNode(groot)
		{
		}


		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			ctx->textCursor(BLPoint(0, 0));
			
			BLRect cFrame = ctx->viewport();
			auto w = cFrame.w;
			auto h = cFrame.h;
			
			double fSize = ctx->fontSize();
			
			// For a text node, the default position is zero
			fX = 0;
			fY = 0;
			
			if (fDimX.isSet())
				fX = fDimX.calculatePixels(ctx->font(), w, 0, 96);
			if (fDimY.isSet())
				fY = fDimY.calculatePixels(ctx->font(), h, 0, 96);
			if (fDimDx.isSet())
				fDx = fDimDx.calculatePixels(ctx->font(), w, 0, 96);
			if (fDimDy.isSet())
				fDy = fDimDy.calculatePixels(ctx->font(), h, 0, 96);

			
			fX = fX + fDx;
			fY = fY + fDy;			

			ctx->textCursor(BLPoint(fX, fY));

		}
	};
}

