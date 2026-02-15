#pragma once


#include "svgstructuretypes.h"
#include "svgattributes.h"
#include "xmlentity.h"
#include "twobitpu.h"
#include "svgtextlayout.h"









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
    struct SVGTextRun final : public IViewable
    {
        // The original text as read from the XML, without
        // entities unexpanded, and no whitespace treatment.
        ByteSpan fRawText{}; 

        // Expanded UTF-8 bytes used for shaping/draw.  Entities exapanded
        // and whitespace normalized.
        ByteSpan fExpanded{}; 

        // If we had to expand entities, this will own the expanded text.  
        // Otherwise, it's empty and we just point at the original XML buffer.
        MemBuff fOwned{};

        // ObjectBounding box
        BLRect fBBox{};


        SVGTextRun() = default;
        
        SVGTextRun(const ByteSpan& raw) noexcept : fRawText(raw) {}

        
        // Inherited from IViewable
        // Abstracts that MUST have an implementation
        // For text runs, these are all no-ops because 
        // the container ('text' or 'tspan') is responsible for all of these things.
        void draw(IRenderSVG*, IAmGroot*) override {}
        void bindToContext(IRenderSVG*, IAmGroot*) noexcept override {}
        void fixupStyleAttributes(IAmGroot*) override {}

        const BLVar getVariant(IRenderSVG* ctx, IAmGroot* groot) noexcept override { return BLVar{}; }
        BLRect objectBoundingBox() const override { return fBBox; }
        void setBBox(const BLRect& box) { fBBox = box; }

        // no-op
        void update(IAmGroot* ) override {}

        // container does all drawing, so this is a no-op


        // Our properties
        ByteSpan rawText() const { return fRawText; }
        ByteSpan textForLayout() const noexcept 
        { 
            return fExpanded ? fExpanded : fRawText; 
        }

        // Take ownership of raw bytes.  This is typical
        // after merging multiple runs.
        void setRawOwned(MemBuff&& buf) noexcept
        {
            fOwned = std::move(buf);
            fRawText = fOwned.span();
            fExpanded = {};
        }

        bool expandEntitiesExplicit() noexcept
        {
            if (!fRawText) {
                fExpanded = {};
                return true;
            }

            // Fast path: nothing to expand.
            if (memchr(fRawText.data(), '&', fRawText.size()) == nullptr) {
                fExpanded = fRawText;
                return true;
            }

            // Ensure we have a mutable buffer to rewrite.
            if (fOwned.empty()) {
                fOwned.resetFromSpan(fRawText);
                fRawText = fOwned.span();
            }

            uint8_t* buf = fOwned.data();
            uint8_t* r = buf;
            uint8_t* w = buf;
            uint8_t* e = buf + fOwned.size();

            auto appendByte = [&](uint8_t b) noexcept { *w++ = b; };
            auto appendSpan = [&](const uint8_t* p, size_t n) noexcept {
                if (!n) return;
                // w <= r invariant means overlap is safe, but memmove is simplest.
                memmove(w, p, n);
                w += n;
                };

            auto appendLiteralEntity = [&](const uint8_t* amp, const uint8_t* semi) noexcept {
                // copy '&' ... ';' inclusive
                appendSpan(amp, size_t(semi - amp + 1));
                };

            while (r < e)
            {
                if (*r != '&') {
                    *w++ = *r++;
                    continue;
                }

                uint8_t* amp = r++;                 // points at '&' in input
                if (r >= e) { appendByte('&'); break; }

                // find ';'
                uint8_t* semi = r;
                while (semi < e && *semi != ';') ++semi;

                if (semi >= e) {
                    // no ';' => copy literal '&' and remaining bytes
                    appendByte('&');
                    appendSpan(r, size_t(e - r));
                    break;
                }

                uint8_t* body = r;                  // after '&'
                const size_t bodyLen = size_t(semi - body);

                // advance r past ';' for next loop
                r = semi + 1;

                // numeric entity?
                if (bodyLen && body[0] == '#')
                {
                    uint8_t* p = body + 1;
                    bool isHex = false;

                    if (p < semi && (*p == 'x' || *p == 'X')) { isHex = true; ++p; }

                    uint64_t value = 0;
                    bool ok = false;

                    if (p < semi) {
                        //ByteSpan digits(p, semi);
                        ByteSpan digits = ByteSpan::fromPointers(p, semi);
                        ok = isHex ? parseHex64u(digits, value) : parse64u(digits, value);
                    }

                    if (ok) {
                        char outBuff[10] = { 0 };
                        size_t outLen = 0;
                        if (convertUTF32ToUTF8(value, outBuff, outLen) && outLen) {
                            appendSpan(reinterpret_cast<const uint8_t*>(outBuff), outLen);
                            continue;
                        }
                    }

                    // malformed numeric => copy literal
                    appendLiteralEntity(amp, semi);
                    continue;
                }

                // named entities
                if (bodyLen == 2 && body[0] == 'l' && body[1] == 't') { appendByte('<'); continue; }
                if (bodyLen == 2 && body[0] == 'g' && body[1] == 't') { appendByte('>'); continue; }
                if (bodyLen == 3 && body[0] == 'a' && body[1] == 'm' && body[2] == 'p') { appendByte('&'); continue; }
                if (bodyLen == 4 && body[0] == 'a' && body[1] == 'p' && body[2] == 'o' && body[3] == 's') { appendByte('\''); continue; }
                if (bodyLen == 4 && body[0] == 'q' && body[1] == 'u' && body[2] == 'o' && body[3] == 't') { appendByte('"'); continue; }

                // unknown named => copy literal
                appendLiteralEntity(amp, semi);
            }

            // Canonicalize: expanded text is the text (raw no longer needed).
            fExpanded = ByteSpan::fromPointers(buf, w);
            fRawText = fExpanded;

            return true;
        }

        // BUGBUG - Since whitespace normallization comes after expansion
        // We need to use fExpanded bytespan for data and size, NOT
        // the membuff attributes.
        // xml:space default behavior:
        //  - trim leading/trailing XML whitespace
        //  - collapse XML whitespace runs to a single ASCII space
        //
        // If preserve=true, no-op.
        bool normalizeXmlWhitespaceExplicit(bool preserve) noexcept
        {
            if (preserve) {
                // keep as-is (but still point expanded at canonical text)
                if (!fExpanded) 
                    fExpanded = fRawText;
                return true;
            }

            // Ensure mutable owned text exists.
            // Typically after merge you already own; this is just defensive.
            if (fOwned.empty()) {
                fOwned.resetFromSpan(textForLayout());
                fRawText = fOwned.span();
                fExpanded = fRawText;
            }

            //uint8_t* buf = fOwned.data();
            ByteSpan tfl = textForLayout();
            uint8_t* buf = const_cast<uint8_t*>(tfl.data()); // we know we have an owned buffer, so this is safe
            uint8_t* r = buf;
            uint8_t* w = buf;
            uint8_t* e = const_cast<uint8_t*>(tfl.end()); // we know we have an owned buffer, so this is safe

            // 1) trim leading XML ws
            while (r < e && is_space(*r)) ++r;

            bool pendingSpace = false;

            // 2) collapse internal runs
            while (r < e) {
                uint8_t c = *r++;
                if (is_space(c)) {
                    pendingSpace = true;
                    continue;
                }

                if (pendingSpace && w > buf) {
                    *w++ = 0x20; // ASCII space
                }
                pendingSpace = false;

                *w++ = c;
            }

            // No trailing space because we only emit space before a non-ws char.

            fExpanded = ByteSpan::fromPointers(buf, w);
            fRawText = fExpanded; // raw no longer needed after normalization
            return true;
        }

    };
}

namespace waavs {


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



    static INLINE bool consumeNextRotateRad(IRenderSVG* ctx, double& ioLastRad) noexcept
    {
        ByteSpan tok{};
        if (!ctx->consumeNextRotateToken(tok))
            return false;                 // no rotate stream, or rotate list exhausted

        if (tok.empty())
            return false;

        SVGAngleUnits units;
        return parseAngle(tok, ioLastRad, units);
    }


    static INLINE double consumeNextLengthPxOrZero(IRenderSVG* ctx, IAmGroot* groot, bool isDx) noexcept
    {
        double dpi = groot ? groot->dpi() : 96.0;  // default to 96 dpi if no groot

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

        const double px = dim.calculatePixels(ctx->getFont(), rel, 0.0, dpi);
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
        IRenderSVG* ctx, IAmGroot* groot,
        const ByteSpan& txt,
        TXTALIGNMENT vAlign,
        DOMINANTBASELINE domBase,
        BLRect& ioBBox) noexcept
    {
        //=============== TEST =================
        const bool hasPS = ctx->hasTextPosStream();
        const auto& ps = ctx->textPosStream();
        const bool useDy = hasPS && ps.hasDy;

        //if (txt.size() && memchr(txt.data(), 'H', txt.size())) { // any cheap marker
        //    printf("hasPS=%d hasDy=%d\n", int(hasPS), int(useDy));
        //}
        //=============== END TEST =============

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
        //const bool hasPS = ctx->hasTextPosStream();
        //const auto& ps = ctx->textPosStream();
        const bool useDx = hasPS && ps.hasDx;
        //const bool useDy = hasPS && ps.hasDy;
        const bool useRot = hasPS && ps.hasRotate;

        // Angles per glyph (deg). If rotate list ends, repeat last value.
        //std::vector<double> angDeg;
        std::vector<double> angRad;
        
        //if (useRot) angDeg.resize(n, 0.0);
        if (useRot) angRad.resize(n, 0.0);

        //double lastRotDeg = 0.0;
        double lastRotRad = 0.0;

        double cumDxPlace = 0.0;  
        double cumDyPlace = 0.0;  

        // dx/dy in SVG are cumulative offsets.
        double cumDxUser = 0;
        double cumDyUser = 0;

        for (size_t i = 0; i < n; ++i) {
            if (useDx) {
                const double dxUser = consumeNextLengthPxOrZero(ctx, groot, true);  // exhausted => 0
                cumDxUser += dxUser;

                const double dxPlace = (sx != 0.0) ? (dxUser / sx) : 0.0;  // user units
                cumDxPlace += dxPlace;

                pl[i].placement.x += (int32_t)llround(cumDxPlace);
            }

            if (useDy) {
                const double dyUser = consumeNextLengthPxOrZero(ctx, groot, false); // exhausted => 0
                cumDyUser += dyUser;

                const double dyPlace = (sy != 0.0) ? (dyUser / sy) : 0.0;  // user units
                cumDyPlace += dyPlace;
                pl[i].placement.y += (int32_t)llround(cumDyPlace);
            }

            if (useRot) {
                // If rotate list exhausted, keep lastRotDeg (repeat last).
                //consumeNextRotateDeg(ctx, lastRotDeg);
                //angDeg[i] = lastRotDeg;
                consumeNextRotateRad(ctx, lastRotRad);
                angRad[i] = lastRotRad;
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
                const double rads = angRad[i];
                if (rads != 0.0) {
                    delta.rotate(rads);
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
    static inline bool hasAtLeastTwoLengthTokens(const ByteSpan& span) noexcept
    {
        if (span.empty()) return false;
        SVGTokenListView v(span);
        ByteSpan a{}, b{};
        if (!v.nextLengthToken(a)) return false;
        return v.nextLengthToken(b);
    }

    static inline bool hasExactlyOneLengthToken(const ByteSpan& span) noexcept
    {
        if (span.empty()) return false;
        SVGTokenListView v(span);
        ByteSpan a{}, b{};
        if (!v.nextLengthToken(a)) return false;
        return !v.nextLengthToken(b);
    }



    struct SVGTextContainerNode : public SVGGraphicsElement
    {
        SVGAlignment fTextHAlignment = SVGAlignment::SVG_ALIGNMENT_START;
        TXTALIGNMENT fTextVAlignment = TXTALIGNMENT::BASELINE;
        DOMINANTBASELINE fDominantBaseline = DOMINANTBASELINE::AUTO;

        BLRect fBBox{};

        double fX{ 0 }, fY{ 0 };


        ByteSpan fDxSpan{}, fDySpan{}, fRotateSpan;
        bool fHasDxStream{ false }, fHasDyStream{ false }, fHasRotStream{ false };
        bool fDxIsSingle{ false }, fDyIsSingle{ false }, fRotIsSingle{ false };
        
        bool fPushedTextPosition{ false };

        SVGVariableSize fDimX{};
        SVGVariableSize fDimY{};



        SVGTextContainerNode(IAmGroot*) :SVGGraphicsElement() 
        {
            setNeedsBinding(true);
        }

        // --- initial position policy hooks ---
        virtual BLPoint defaultCursor(IRenderSVG* ctx) const noexcept = 0;


        BLRect objectBoundingBox() const override { return fBBox; }

        // child loading shared between 'text' and 'tspan'
        void loadContentNode(const XmlElement& elem, IAmGroot* groot) override
        {
            auto node = std::make_shared<SVGTextRun>(elem.data());
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

        static inline bool isTextRunNode(const std::shared_ptr<IViewable>& n) noexcept
        {
            return (bool)std::dynamic_pointer_cast<SVGTextRun>(n);
        }

        // Once the sub-tree is loaded, we can do some work here.
        // We still don't have style information fully resolved though
        void onEndTag(IAmGroot* groot) override
        {
            SVGGraphicsElement::onEndTag(groot);

            if (fNodes.empty())
                return;

            // Determine xml:space policy for THIS container.
            // Note: xml:space is inheritable; this reads only the local attribute for now.
            bool preserveSpace = false;
            {
                ByteSpan v{};
                if (fPresentationAttributes.getValue(PSNameTable::INTERN("xml:space"), v)) {
                    v = chunk_trim(v, chrWspChars);
                    // accept "preserve" (case-sensitive per XML)
                    preserveSpace = (v == "preserve");
                }
            }

            std::vector<std::shared_ptr<IViewable>> out;
            out.reserve(fNodes.size());

            auto asRun = [&](const std::shared_ptr<IViewable>& n) noexcept -> std::shared_ptr<SVGTextRun>
                {
                    return std::dynamic_pointer_cast<SVGTextRun>(n);
                };

            auto isRun = [&](const std::shared_ptr<IViewable>& n) noexcept -> bool
                {
                    return (bool)asRun(n);
                };

            auto flushRunGroup = [&](size_t i0, size_t i1)
                {
                    // Pass 1: compute total bytes
                    size_t totalBytes = 0;
                    for (size_t i = i0; i < i1; ++i) {
                        auto run = asRun(fNodes[i]);
                        if (!run) continue;
                        totalBytes += run->rawText().size();
                    }

                    auto merged = std::make_shared<SVGTextRun>();
                    if (!merged) return;

                    MemBuff owned(totalBytes);
                    uint8_t* dst = owned.data();

                    // Pass 2: copy bytes
                    for (size_t i = i0; i < i1; ++i) {
                        auto run = asRun(fNodes[i]);
                        if (!run) continue;

                        ByteSpan s = run->rawText();
                        const size_t n = s.size();
                        if (n) {
                            memcpy(dst, s.data(), n);
                            dst += n;
                        }
                    }

                    merged->setRawOwned(std::move(owned));

                    // Expand entities ONCE (in-place)
                    (void)merged->expandEntitiesExplicit();

                    // Apply xml whitespace rules ONCE (in-place)
                    (void)merged->normalizeXmlWhitespaceExplicit(preserveSpace);

                    out.push_back(merged);
                };

            // Merge maximal contiguous groups of runs
            size_t i = 0;
            while (i < fNodes.size())
            {
                if (!isRun(fNodes[i])) {
                    out.push_back(fNodes[i]);
                    ++i;
                    continue;
                }

                const size_t i0 = i;
                ++i;
                while (i < fNodes.size() && isRun(fNodes[i]))
                    ++i;

                flushRunGroup(i0, i);
            }

            fNodes.swap(out);
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
            // dx, dy, rotate, are all treated as lists, even if they only have
            // one value.  
            // If the list is shorter than the text
            //   rotation - the last value is repeated
            //  dx/dy - the missing values are treated as 0
            //
            fDxSpan = getAttribute(svgattr::dx());
            fDySpan = getAttribute(svgattr::dy());
            fRotateSpan = getAttribute(svgattr::rotate());

            fHasDxStream = !fDxSpan.empty();
            fHasDyStream = !fDySpan.empty();
            fHasRotStream = !fRotateSpan.empty();

            fDxIsSingle = fHasDxStream && hasExactlyOneLengthToken(fDxSpan);
            fDyIsSingle = fHasDyStream && hasExactlyOneLengthToken(fDySpan);
            fRotIsSingle = fHasRotStream && hasExactlyOneLengthToken(fRotateSpan);


            fDimX.loadFromChunk(getAttribute(svgattr::x()));
            fDimY.loadFromChunk(getAttribute(svgattr::y()));


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
                    drawTextRunGlyphLoop(ctx, groot, textNode->textForLayout(), fTextVAlignment, fDominantBaseline, fBBox);
                }
                else {
                    node->draw(ctx, groot);
                    expandRect(fBBox, node->objectBoundingBox());
                }
            }

            // Pop text position stream if we pushed one
            if (fPushedTextPosition)
            {
                ctx->popTextPosStream();
                fPushedTextPosition = false;
            }
        }




        static inline double evalSingleLengthPx(IRenderSVG* ctx, IAmGroot* groot,
            const ByteSpan& listSpan,
            double rel /* viewport w or h */) noexcept
        {
            if (listSpan.empty()) 
                return 0.0;

            SVGTokenListView v(listSpan);
            ByteSpan tok{};
            if (!v.nextLengthToken(tok) || tok.empty())
                return 0.0;

            SVGVariableSize dim;
            if (!dim.loadFromChunk(tok))
                return 0.0;

            const double dpi = groot ? groot->dpi() : 96.0;
            return dim.calculatePixels(ctx->getFont(), rel, 0.0, dpi);
        }

        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            const double dpi = groot ? groot->dpi() : 96.0;
            const BLRect cFrame = ctx->viewport();
            const double w = cFrame.w;
            const double h = cFrame.h;

            BLPoint base = defaultCursor(ctx);
            fX = base.x;
            fY = base.y;

            if (fDimX.isSet()) fX = fDimX.calculatePixels(ctx->getFont(), w, 0, dpi);
            if (fDimY.isSet()) fY = fDimY.calculatePixels(ctx->getFont(), h, 0, dpi);

            // ---- apply scalar dx/dy (exactly one token) as run-level offsets ----
            if (fDxIsSingle)
                fX += evalSingleLengthPx(ctx, groot, fDxSpan, /*rel*/w);

            if (fDyIsSingle)
                fY += evalSingleLengthPx(ctx, groot, fDySpan, /*rel*/h);

            // Establish initial cursor (this is what accumulates across tspans)
            ctx->textCursor(BLPoint(fX, fY));

            fPushedTextPosition = false;

            // Push streams ONLY if they are truly per-glyph (2+ tokens),
            // and always push rotate if present (even if single, last repeats).
            const bool pushDx = fHasDxStream && !fDxIsSingle;
            const bool pushDy = fHasDyStream && !fDyIsSingle;
            const bool pushRot = fHasRotStream;

            if (pushDx || pushDy || pushRot)
            {
                SVGTextPosStream local{};

                if (pushDx) { local.hasDx = true; local.dx.reset(fDxSpan); }
                if (pushDy) { local.hasDy = true; local.dy.reset(fDySpan); }
                if (pushRot) { local.hasRotate = true; local.rotate.reset(fRotateSpan); }

                ctx->pushTextPosStream(local);
                fPushedTextPosition = true;
            }
        }



        /*
        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            double dpi = groot ? groot->dpi() : 96.0;
            BLRect cFrame = ctx->viewport();
            double w = cFrame.w;
            double h = cFrame.h;

            // Base cursor policy: <text> defaults (0,0), <tspan> defaults current cursor
            BLPoint base = defaultCursor(ctx);
            fX = base.x;
            fY = base.y;

            // Absolute x/y override
            if (fDimX.isSet())
                fX = fDimX.calculatePixels(ctx->getFont(), w, 0, dpi);

            if (fDimY.isSet())
                fY = fDimY.calculatePixels(ctx->getFont(), h, 0, dpi);


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
        */


    };
}

namespace waavs {
    struct SVGTSpanNode final : public SVGTextContainerNode
    {
        static void registerSingular()
        {
            registerSVGSingularNodeByName(svgtag::tag_tspan(),
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

