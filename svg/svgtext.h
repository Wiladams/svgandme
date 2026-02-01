#pragma once


#include "svgstructuretypes.h"
#include "svgattributes.h"
#include "xmlentity.h"
#include "twobitpu.h"

namespace waavs 
{
	struct Fontography 
	{

        // Measure how big a piece of text will be with a given font
		static BLPoint textMeasure(const BLFont& font, const ByteSpan& txt) noexcept
		{
			BLTextMetrics tm;
			BLGlyphBuffer gb;
			BLFontMetrics fm = font.metrics();

			gb.setUtf8Text(txt.data(), txt.size());
			font.shape(gb);
			font.getTextMetrics(gb, tm);


			// if we're hit testing, use the following
			//float cx = (float)(tm.boundingBox.x1 - tm.boundingBox.x0);
            // when we're trying to know the extent of the text, use the advance
			float advanceX = tm.advance.x;
			float cy = fm.ascent + fm.descent;

			return BLPoint(advanceX, cy);
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
			case SVGAlignment::SVG_ALIGNMENT_START:
				// do nothing
				// x = x;
				break;
			case SVGAlignment::SVG_ALIGNMENT_MIDDLE:
				x = x - (cx / 2);
				break;
			case SVGAlignment::SVG_ALIGNMENT_END:
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
//    affect sibling tspans, although it will affect child tspans.
//  - 'x' and 'y' positions, 'text' element, default to '0', while 'tspan' default to the 
//    whereever the text cursor sits after the last text rendering
//	  action.
//
namespace waavs 
{
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
			setNeedsBinding(true);
			
			expandXmlEntities(fText, fText);
		}

		ByteSpan text() const { return fText; }
		
		BLRect getBBox() const override { return fBBox; }
		void setBBox(const BLRect& box) { fBBox = box; }

	};
}

namespace waavs {
	static INLINE const uint8_t* utf8_next(const uint8_t* p, const uint8_t* e) noexcept
	{
		if (p >= e) return e;

		uint8_t c = *p++;
		if (c < 0x80) return p;

		// assumes valid UTF-8; fast skip only
		if ((c >> 5) == 0x6)  return (p + 1 <= e) ? (p + 1) : e; // 2 bytes total
		if ((c >> 4) == 0xE)  return (p + 2 <= e) ? (p + 2) : e; // 3 bytes total
		if ((c >> 3) == 0x1E) return (p + 3 <= e) ? (p + 3) : e; // 4 bytes total
		return e;
	}

	static INLINE bool tokenToPx(IRenderSVG* ctx, const ByteSpan& tok, double range, double& outPx, double dpi=96.0) noexcept
	{
		outPx = 0.0;
		if (!tok) return false;

		SVGVariableSize dim;
		if (!dim.loadFromChunk(tok))
			return false;

		// match your existing 96 dpi assumption for now
		outPx = dim.calculatePixels(ctx->getFont(), range, 0, dpi);
		return true;
	}

	// BUGBUG - should be able to use parseAngle() instead
	static INLINE double parseAngleDeg(const ByteSpan& tok) noexcept
	{
		// Keep it simple: assume degrees numeric like "10" or "10.5".
		// If you support "rad"/"grad"/"turn", convert here.
		double v = 0.0;
		ByteSpan t = tok;
		readNumber(t, v);   // you already have this style helper
		return v;
	}

	static INLINE bool consumeNextRotateDeg(IRenderSVG* ctx, double& ioLastDeg) noexcept
	{
		ByteSpan tok{};
		if (!ctx->consumeNextRotateToken(tok))
			return false;                 // no rotate stream, or rotate list exhausted

		if (tok.empty())
			return false;

		ioLastDeg = parseAngleDeg(tok);
		return true;
	}



    //using BLFixed = int32_t; // 26.6 fixed-point
	static INLINE double consumeNextLengthPxOrZero(IRenderSVG* ctx, bool isDx) noexcept
	{
		ByteSpan tok{};
		bool ok = isDx ? ctx->consumeNextDxToken(tok) : ctx->consumeNextDyToken(tok);
		if (!ok || tok.empty())
			return double(0);

		// Convert the length token to px; you already do this elsewhere via SVGVariableSize.
		// A pragmatic approach is: parse token -> SVGVariableSize -> calculatePixels -> fixed.
		SVGVariableSize dim;
		dim.loadFromChunk(tok);

		// For dx: percent is relative to viewport width; for dy: viewport height.
		// But SVGVariableSize::calculatePixels already takes "relativeTo" input.
		BLRect vp = ctx->viewport();
		const double rel = isDx ? vp.w : vp.h;

		const double px = dim.calculatePixels(ctx->getFont(), rel, 0.0, 96.0);
		return px;
	}




    // GlyphPainter
	// 
    // This is an executor for paint-order rendering of glyph runs.
    // It Works with two-bit paint order encoding.
	struct GlyphPainter {
		IRenderSVG* ctx;
		const BLFont& font;
		const BLGlyphRun& run;
		double x, y;

		void onFill()    noexcept { ctx->fillGlyphRun(font, run, x, y); }
		void onStroke()  noexcept { ctx->strokeGlyphRun(font, run, x, y); }
		void onMarkers() noexcept { /* usually no-op for text */ }
	};


	static  void drawTextRunGlyphLoop(
		IRenderSVG* ctx,
		const ByteSpan& txt,
		TXTALIGNMENT vAlign,
		DOMINANTBASELINE domBase,
		BLRect& ioBBox) noexcept
	{
		const BLFont& font = ctx->getFont();

		// Get font matrix so we can build per-glyph transforms
		BLFontMatrix fm = font.matrix();
		const double sx = fm.m00;
		const double sy = fm.m11;	// negative for y-down

		// 1) Shape
		BLGlyphBuffer gb;
		gb.setUtf8Text(txt.data(), txt.size());
		font.shape(gb);

		// 2) Resolve baseline origin using your existing alignment policy
		const SVGAlignment anchor = ctx->getTextAnchor();
		const BLPoint pos = ctx->textCursor();

		const BLRect pRect = Fontography::calcTextPosition(font, txt, pos.x, pos.y, anchor, vAlign, domBase);
		expandRect(ioBBox, pRect);

		// 3) Access glyph run
		BLGlyphRun grun = gb.glyphRun();
		const size_t n = size_t(grun.size);
		if (!n) {
			ctx->textCursor(BLPoint(pRect.x, pRect.y));
			return;
		}

		// Ensure placements exist
		if (!grun.placementData) {
			font.positionGlyphs(gb);
			grun = gb.glyphRun();
			if (!grun.placementData) {
				ctx->fillText(txt, pRect.x, pRect.y);
				ctx->textCursor(BLPoint(pRect.x + pRect.w, pRect.y));
				return;
			}
		}

		// Mutable view of placements
		BLGlyphPlacement* pl = const_cast<BLGlyphPlacement*>(
			static_cast<const BLGlyphPlacement*>(grun.placementData));

		// 4) Consume SVG dx/dy/rotate streams
		const bool hasPS = ctx->hasTextPosStream();
		const auto& ps = ctx->textPosStream();
		const bool useDx = hasPS && ps.hasDx;
		const bool useDy = hasPS && ps.hasDy;
		const bool useRot = hasPS && ps.hasRotate;

		// Angles per glyph (deg). If rotate list ends, repeat last value.
		std::vector<double> angDeg;
		if (useRot) angDeg.resize(n, 0.0);

		double lastRotDeg = 0.0;

        double cumDxPlace = 0.0;  
        double cumDyPlace = 0.0;  

		// dx/dy in SVG are cumulative offsets.
		double cumDxUser = 0;
		double cumDyUser = 0;

		for (size_t i = 0; i < n; ++i) {
			if (useDx) {
				const double dxUser = consumeNextLengthPxOrZero(ctx, true);  // exhausted => 0
				cumDxUser += dxUser;

				const double dxPlace = (sx != 0.0) ? (dxUser / sx) : 0.0;  // user units
				cumDxPlace += dxPlace;

				pl[i].placement.x += (int32_t)llround(cumDxPlace);
			}

			if (useDy) {
				const double dyUser = consumeNextLengthPxOrZero(ctx, false); // exhausted => 0
				cumDyUser += dyUser;

                const double dyPlace = (sy != 0.0) ? (dyUser / sy) : 0.0;  // user units
                cumDyPlace += dyPlace;
				pl[i].placement.y += (int32_t)llround(cumDyPlace);
			}

			if (useRot) {
				// If rotate list exhausted, keep lastRotDeg (repeat last).
				consumeNextRotateDeg(ctx, lastRotDeg);
				angDeg[i] = lastRotDeg;
			}
		}


		// 5) Paint-order helper
		PaintOrderProgram prog(ctx->getPaintOrder());


		const double originX = pRect.x;
		const double originY = pRect.y;
		


		if (!useRot) {
			// Fast path: whole run at origin
			GlyphPainter painter{ ctx, font, grun, originX, originY };
			prog.run(painter);
		}
		else {


			// ---- Robust rotate path ----
			// Build ABSOLUTE per-glyph positions after dx/dy:
			// abs = pen + placement (pen is sum of advances).
			// Then draw each glyph as a 1-glyph run with placement=abs and advance=0.
			int64_t penX = 0;
			int64_t penY = 0;

			for (size_t i = 0; i < n; ++i) 
			{
				const BLGlyphPlacement& gp = pl[i];

				const int64_t absX = penX + gp.placement.x;
				const int64_t absY = penY + gp.placement.y;

				// Draw one glyph at local origin
				BLGlyphPlacement onePl = gp;
				onePl.placement.x = 0;
                onePl.placement.y = 0;
				onePl.advance.x = 0;
				onePl.advance.y = 0;

				// 1-glyph run view.
				BLGlyphRun one = grun;
				one.glyphData = (uint32_t*)grun.glyphData + i;
				one.placementData = &onePl;
				one.size = 1;

                // convert absX/absY (placement space) to user space using font matrix
				const double tx = originX + double(absX) * sx;
				const double ty = originY + double(absY) * sy;;

				const BLMatrix2D saved = ctx->getTransform();
				BLMatrix2D delta{};
				delta.reset();	// start with identity
				delta.translate(tx, ty);

				// Pivot in user space:
				const double a = angDeg[i];
				if (a != 0.0) {
					const double angRad = a * 0.017453292519943295; // pi/180
					delta.rotate(angRad);
				}

				ctx->applyTransform(delta);

                // create painter for this glyph
				GlyphPainter painter{ ctx, font, one, 0, 0 };
				prog.run(painter);

				ctx->transform(saved);		// restore transform

				// Advance pen using ORIGINAL advances (SVG dx is already baked into placements).
				penX += gp.advance.x;
				penY += gp.advance.y;
			}
		}

		// 6) Advance cursor.
		// Blend2D advance does NOT know about your manual dx/dy mutations reliably.
		// So add the final cumulative dx/dy yourself.
		BLTextMetrics tm;
		font.getTextMetrics(gb, tm);

		const double endX = originX + tm.advance.x + cumDxUser;
		const double endY = originY + tm.advance.y + cumDyUser;

		ctx->textCursor(BLPoint(endX, endY));
	}


}

namespace waavs
{
	struct SVGTextContainerNode : public SVGGraphicsElement
	{
        TXTALIGNMENT fTextHAlignment = TXTALIGNMENT::LEFT;
        TXTALIGNMENT fTextVAlignment = TXTALIGNMENT::BASELINE;
        DOMINANTBASELINE fDominantBaseline = DOMINANTBASELINE::AUTO;

        BLRect fBBox{};

		double fX{ 0 }, fY{ 0 }, fDx{ 0 }, fDy{ 0 }, fRotate{};

        ByteSpan fDxSpan{}, fDySpan{}, fRotateSpan;
        bool fHasDxStream{ false }, fHasDyStream{ false }, fHasRotStream{ false };
        bool fDxIsList{ false }, fDyIsList{ false }, fRotateIsList;
		bool fPushedTextPosition{ false };

        SVGVariableSize fDimX{};
        SVGVariableSize fDimY{};
        SVGVariableSize fDimDx{};
        SVGVariableSize fDimDy{};


		SVGTextContainerNode(IAmGroot*) :SVGGraphicsElement() 
		{
			setNeedsBinding(true);
		}

		// --- initial position policy hooks ---
		virtual BLPoint defaultCursor(IRenderSVG* ctx) const noexcept = 0;


        BLRect getBBox() const override { return fBBox; }

        // child loading shared between 'text' and 'tspan'
		void loadContentNode(const XmlElement& elem, IAmGroot* groot) override
		{
            auto node = std::make_shared<SVGTextRun>(elem.data(), groot);
			if (!node)
                return;
            addNode(node, groot);
		}

		virtual bool acceptsTSpanChildren() const noexcept { return true; }

		void loadSelfClosingNode(const XmlElement& elem, IAmGroot* groot) override
		{
			if (acceptsTSpanChildren() && elem.nameAtom() == svgtag::tag_tspan())
			{
                auto child = createSingularNode(elem, groot);
				if (child)
				{
                    addNode(child, groot);
				}

				// If not registered, we can't load it here
				// No XMLPull available

				return;
            }

			// otherwise, ignore and delegate
		}

		void loadStartTag(XmlPull& iter, IAmGroot* groot) override
		{
			// Most likely a <tspan>
			if (acceptsTSpanChildren() && (*iter).nameAtom() == svgtag::tag_tspan())
			{
				auto child = createContainerNode(iter, groot);
				if (child)
				{
					// child is already fully loaded by the container factory
                    addNode(child, groot);
					return;
				}

				// If not registered, fall through and let base loader
                // discard it.
			}

            // Unknown sub-node, delegate to base class
			SVGGraphicsElement::loadStartTag(iter, groot);
        }

		// --- binding (shared) ---

		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			double dpi = 96;
			if (groot) dpi = groot->dpi();

			BLRect cFrame = ctx->viewport();
			double w = cFrame.w;
			double h = cFrame.h;
			(void)w; (void)h; (void)dpi;

			fDxSpan = getAttribute(svgattr::dx());
			fDySpan = getAttribute(svgattr::dy());
			fRotateSpan = getAttribute(svgattr::rotate());

            fHasDxStream = !fDxSpan.empty();
            fHasDyStream = !fDySpan.empty();
            fHasRotStream = !fRotateSpan.empty();

            // The 'dx' attribute, if present, can be a list of values
			// If it is, we'll just prepare for that

			fDxIsList = false;
			if (!fDxSpan.empty()) 
			{
				SVGTokenListView dxList(fDxSpan);
				ByteSpan a{}, b{};
				if (dxList.nextLengthToken(a) && dxList.nextLengthToken(b))
                    fDxIsList = true;
			}


			fDyIsList = false;
			if (!fDySpan.empty())
			{
				SVGTokenListView dyList(fDySpan);
				ByteSpan a{}, b{};
				if (dyList.nextLengthToken(a) && dyList.nextLengthToken(b))
					fDyIsList = true;
			}


            fRotateIsList = false;
			if (!fRotateSpan.empty())
			{
				SVGTokenListView rotList(fRotateSpan);
				ByteSpan a{}, b{};
				if (rotList.nextNumberToken(a) && rotList.nextNumberToken(b))
					fRotateIsList = true;
            }

			fDimX.loadFromChunk(getAttribute(svgattr::x()));
			fDimY.loadFromChunk(getAttribute(svgattr::y()));
			fDimDx.loadFromChunk(getAttribute(svgattr::dx()));
			fDimDy.loadFromChunk(getAttribute(svgattr::dy()));

			getEnumValue(SVGTextAnchor, getAttributeByName("text-anchor"),
				(uint32_t&)fTextHAlignment);
			getEnumValue(SVGTextAlign, getAttributeByName("text-align"),
				(uint32_t&)fTextVAlignment);
			getEnumValue(SVGDominantBaseline, getAttributeByName("dominant-baseline"),
				(uint32_t&)fDominantBaseline);
			getEnumValue(SVGDominantBaseline, getAttributeByName("alignment-baseline"),
				(uint32_t&)fDominantBaseline);
		}


		// --- draw (shared ) ---

		void drawChildren(IRenderSVG* ctx, IAmGroot* groot) override
		{
			fBBox = {};

			for (auto& node : fNodes) {
				if (auto textNode = std::dynamic_pointer_cast<SVGTextRun>(node)) {
					//drawTextRun(ctx, textNode->text(), fTextVAlignment, fDominantBaseline, fBBox);
					drawTextRunGlyphLoop(ctx, textNode->text(), fTextVAlignment, fDominantBaseline, fBBox);
				}
				else {
                    node->draw(ctx, groot);
                    expandRect(fBBox, node->getBBox());
				}
			}

            // Pop text position stream if we pushed one
			if (fPushedTextPosition)
				ctx->popTextPosStream();
		}


		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			BLRect cFrame = ctx->viewport();
			double w = cFrame.w;
			double h = cFrame.h;

			// Base cursor policy: <text> defaults (0,0), <tspan> defaults current cursor
			BLPoint base = defaultCursor(ctx);
			fX = base.x;
			fY = base.y;

			// Absolute x/y override
			if (fDimX.isSet())
				fX = fDimX.calculatePixels(ctx->getFont(), w, 0, 96);

			if (fDimY.isSet())
				fY = fDimY.calculatePixels(ctx->getFont(), h, 0, 96);

			// dx/dy:
			// - If list: do NOT apply scalar here (list consumption happens per-glyph)
			// - If scalar: apply immediately (legacy behavior)
			fDx = 0.0;
			fDy = 0.0;

			if (!fHasDxStream && fDimDx.isSet())
				fDx = fDimDx.calculatePixels(ctx->getFont(), w, 0, 96);

			if (!fHasDyStream && fDimDy.isSet())
				fDy = fDimDy.calculatePixels(ctx->getFont(), h, 0, 96);

			fX += fDx;
			fY += fDy;

			// Set initial cursor before children draw
			ctx->textCursor(BLPoint(fX, fY));

            fPushedTextPosition = false;
			// Push list streams if present
			if (fHasDxStream || fHasDyStream || fHasRotStream)
			{
				SVGTextPosStream local{};

				// works for single OR list
				if (fHasDxStream) {local.hasDx = true;local.dx.reset(fDxSpan);}
				if (fHasDyStream) {local.hasDy = true;local.dy.reset(fDySpan);}
				if (fHasRotStream) {local.hasRotate = true;local.rotate.reset(fRotateSpan);	}

				ctx->pushTextPosStream(local);
				fPushedTextPosition = true;

			}
		}



	};
}

namespace waavs {
	struct SVGTSpanNode final : public SVGTextContainerNode
	{
		static void registerSingular()
		{
			registerSVGSingularNode(svgtag::tag_tspan(),
				[](IAmGroot* groot, const XmlElement& elem) {
					auto node = std::make_shared<SVGTSpanNode>(groot);
					node->loadFromXmlElement(elem, groot);
					return node;
				});
		}

		static void registerFactory()
		{
			registerContainerNode(svgtag::tag_tspan(),
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGTSpanNode>(groot);
					node->loadFromXmlPull(iter, groot);
					return node;
				});

			registerSingular();
		}

		SVGTSpanNode(IAmGroot* groot) : SVGTextContainerNode(groot) {}

		BLPoint defaultCursor(IRenderSVG* ctx) const noexcept override {
			return ctx->textCursor();
		}
	};



	struct SVGTextNode final : public SVGTextContainerNode
	{
		static void registerFactory()
		{
			registerContainerNode(svgtag::tag_text(),
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGTextNode>(groot);
					node->loadFromXmlPull(iter, groot);
					return node;
				});
		}

		SVGTextNode(IAmGroot* groot) : SVGTextContainerNode(groot) {}

		BLPoint defaultCursor(IRenderSVG* ctx) const noexcept override
		{
			return BLPoint(0, 0);
		}
	};


}

