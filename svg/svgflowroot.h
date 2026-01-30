// svgflowroot.h
//
// Inkscape <flowRoot> compatibility node, written to match WAAVS idioms:
//
// - Uses ByteSpan + charset (chrWspChars, etc.) for parsing and trimming
// - Avoids std::string as the primary substrate; stores paragraph text in owned buffers
// - Uses your readNumber/readNextNumber style where needed
// - Uses Blend2D font shaping/metrics for measurement, but stays conservative
//
// What it implements (pragmatic):
//   <flowRoot ...>
//      <flowRegion><rect x y width height /></flowRegion>
//      <flowPara> ...text... </flowPara>
//      <flowPara> ... </flowPara>
//   </flowRoot>
//
// Behavior:
//   - Word-wrap into rect width
//   - Advances baseline by line-height (supports %, unitless, px)
//   - Clips to the flow rect
//   - Preserves global textCursor (doesn't disturb non-flow text)
//
// Drop-in:
//   1) Include this header.
//   2) Call SVGFlowRoot::registerFactory() during node registration.

#ifndef SVGFLOWROOT_H
#define SVGFLOWROOT_H

#include <vector>
#include <cstdint>
#include <cstring>   // memchr/memcpy/memcmp
#include <cstdlib>   // strtod fallback if you want, but we avoid it
#include <algorithm>

#include "svgstructuretypes.h"
#include "converters.h"      // readNumber / readNextNumber
#include "charset.h"
#include "bspan.h"


namespace waavs {

    //============================================================
    // Small owned UTF-8 string for paragraphs/lines
    //============================================================
    struct OwnedSpan
    {
        std::vector<uint8_t> fBuf{};

        OwnedSpan() = default;
        explicit OwnedSpan(const ByteSpan& s) { assign(s); }

        void clear() { fBuf.clear(); }

        void assign(const ByteSpan& s)
        {
            fBuf.assign(s.begin(), s.end());
        }

        void append(const ByteSpan& s)
        {
            if (!s) return;
            fBuf.insert(fBuf.end(), s.begin(), s.end());
        }

        void appendChar(uint8_t c)
        {
            fBuf.push_back(c);
        }

        ByteSpan span() const noexcept
        {
            if (fBuf.empty()) return {};
            return ByteSpan(fBuf.data(), fBuf.data() + fBuf.size());
        }

        bool empty() const noexcept { return fBuf.empty(); }
        size_t size() const noexcept { return fBuf.size(); }
    };

    //============================================================
    // Minimal XML entity decode into OwnedSpan
    // (covers the common ones you actually see in these files)
    //============================================================
    static inline void appendDecodedXmlText(OwnedSpan& out, const ByteSpan& src)
    {
        // Fast path: no '&' present
        if (!src) return;
        ByteSpan scan = src;
        ByteSpan found{};
        if (!chunk_find(scan, ByteSpan("&"), found)) {
            out.append(src);
            return;
        }

        const uint8_t* p = src.fStart;
        const uint8_t* e = src.fEnd;

        while (p < e)
        {
            const uint8_t* amp = (const uint8_t*)std::memchr(p, '&', size_t(e - p));
            if (!amp) {
                out.append(ByteSpan(p, e));
                break;
            }

            // copy bytes before '&'
            if (amp > p)
                out.append(ByteSpan(p, amp));

            // attempt decode entity
            const uint8_t* semi = (const uint8_t*)std::memchr(amp, ';', size_t(e - amp));
            if (!semi) {
                // malformed: just copy '&' and continue
                out.appendChar(*amp);
                p = amp + 1;
                continue;
            }

            ByteSpan ent(amp, semi + 1); // includes & ... ;
            // Common entities
            if (ent == "&quot;") out.appendChar('"');
            else if (ent == "&apos;") out.appendChar('\'');
            else if (ent == "&lt;")   out.appendChar('<');
            else if (ent == "&gt;")   out.appendChar('>');
            else if (ent == "&amp;")  out.appendChar('&');
            else {
                // Unknown: keep literal
                out.append(ent);
            }

            p = semi + 1;
        }
    }

    //============================================================
    // Whitespace normalization helpers using charset
    //============================================================
    static inline bool spanAllWsp(const ByteSpan& s) noexcept
    {
        return isAll(s, chrWspChars);
    }

    // Trim left/right whitespace with your chunk_trim()
    static inline ByteSpan trimOuterWsp(const ByteSpan& s) noexcept
    {
        return chunk_trim(s, chrWspChars);
    }

    // Collapse runs of whitespace to single ' ' (ASCII space)
    // and trim ends. (Only affects ASCII whitespace bytes.)
    static inline OwnedSpan collapseWhitespace(const ByteSpan& in)
    {
        OwnedSpan out;
        if (!in) return out;

        const uint8_t* p = in.fStart;
        const uint8_t* e = in.fEnd;

        // trim leading
        while (p < e && chrWspChars(*p)) 
            ++p;

        // trim trailing: find last non-wsp
        while (e > p && chrWspChars(*(e - 1))) 
            --e;

        bool inWsp = false;
        while (p < e)
        {
            const uint8_t c = *p++;
            if (chrWspChars(c)) {
                if (!inWsp) {
                    out.appendChar(' ');
                    inWsp = true;
                }
            }
            else {
                out.appendChar(c);
                inWsp = false;
            }
        }

        return out;
    }

    //============================================================
    // Measure text width using Blend2D shaping
    //============================================================
    static inline double measureTextWidth(const BLFont& font, const ByteSpan& utf8) noexcept
    {
        if (!utf8) return 0.0;

        BLTextMetrics tm;
        BLGlyphBuffer gb;
        gb.setUtf8Text((const char*)utf8.fStart, utf8.size());
        font.shape(gb);
        font.getTextMetrics(gb, tm);

        return double(tm.boundingBox.x1 - tm.boundingBox.x0);
    }

    //============================================================
    // Parse line-height from ByteSpan (supports: %, unitless, px)
    // Returns:
    //   if absolutePx==true => outValue is px
    //   else outValue is multiplier
    //============================================================
    static inline double parseLineHeight(const ByteSpan& s, bool& absolutePx) noexcept
    {
        absolutePx = false;

        ByteSpan t = trimOuterWsp(s);
        if (!t) return 1.25;

        // percent
        if (t.size() >= 2 && t.fEnd[-1] == '%') {
            ByteSpan num(t.fStart, t.fEnd - 1);
            double v = 0.0;
            if (parseNumber(num, v) && v > 0.0)
                return v / 100.0;
            return 1.25;
        }

        // px
        if (t.size() >= 2 && t.fEnd[-2] == 'p' && t.fEnd[-1] == 'x') {
            ByteSpan num(t.fStart, t.fEnd - 2);
            double v = 0.0;
            if (parseNumber(num, v) && v > 0.0) {
                absolutePx = true;
                return v;
            }
            return 1.25;
        }

        // unitless multiplier
        {
            double v = 0.0;
            if (parseNumber(t, v) && v > 0.0)
                return v;
        }

        return 1.25;
    }

    //============================================================
    // Word wrap
    //   Input: paragraph text already decoded (UTF-8)
    //   Output: lines (OwnedSpan), without trailing whitespace in collapsed mode
    //============================================================
    static inline void wrapParagraph(
        std::vector<OwnedSpan>& outLines,
        const BLFont& font,
        const ByteSpan& para,
        const double maxWidth,
        const bool preserveSpace)
    {
        outLines.clear();

        if (!para || maxWidth <= 0.0) {
            outLines.emplace_back();
            return;
        }

        // If not preserving, collapse whitespace now.
        OwnedSpan normalizedOwned;
        ByteSpan normalized = para;
        if (!preserveSpace) {
            normalizedOwned = collapseWhitespace(para);
            normalized = normalizedOwned.span();
            if (!normalized) {
                outLines.emplace_back();
                return;
            }
        }

        // We'll tokenize by whitespace boundaries using ByteSpan scanning.
        // tokens: sequences of non-whitespace bytes
        ByteSpan s = normalized;

        // delimiter set for "split on whitespace"
        const charset wsp = chrWspChars;

        OwnedSpan currentLine;
        bool firstTokenInLine = true;

        auto flushLine = [&]() {
            outLines.push_back(currentLine);
            currentLine.clear();
            firstTokenInLine = true;
            };

        while (s)
        {
            if (!preserveSpace) {
                // skip leading whitespace (should be none in collapsed mode, but in preserveSpace it may exist)
                s.skipWhile(wsp);
            }
            else {
                // preserveSpace: ... we DO NOT auto-insert spaces
            }

            if (!s) break;

            // token = until whitespace
            ByteSpan token = chunk_token(s, wsp);
            // chunk_token() advances s past delimiter if it sees one; good.

            if (!token) continue;

            // Candidate = currentLine + (space?) + token
            OwnedSpan candidate = currentLine;
            if (!firstTokenInLine)
                candidate.appendChar(' ');

            //if (!preserveSpace) {
            //    if (!firstTokenInLine)
            //        candidate.appendChar(' ');
            //}
            //else {
                // preserveSpace: If currentLine isn't empty, we DO NOT auto-insert spaces;
                // spaces were in the original input. Here we only add the token itself.
            //}
            candidate.append(token);

            // Measure candidate
            const double w = measureTextWidth(font, candidate.span());

            if (!firstTokenInLine && w > maxWidth) {
                // emit current line, start new line with token
                flushLine();
                currentLine.append(token);
                firstTokenInLine = false;
            }
            else {
                currentLine = std::move(candidate);
                firstTokenInLine = false;
            }

            // If single token itself exceeds maxWidth and currentLine was empty initially,
            // we still place it (no hyphenation).
        }

        // emit last line (even if empty)
        if (!currentLine.empty() || outLines.empty())
            outLines.push_back(currentLine);
    }

    //============================================================
    // SVGFlowRoot
    //============================================================
    struct SVGFlowRoot final : public SVGGraphicsElement
    {
        static void registerFactory()
        {
            registerContainerNodeByName("flowRoot",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGFlowRoot>(groot);
                    node->loadFromXmlPull(iter, groot);
                    return node;
                });
        }

        // flowRegion rect dims (unresolved until bind)
        SVGVariableSize fX{};
        SVGVariableSize fY{};
        SVGVariableSize fW{};
        SVGVariableSize fH{};
        bool fHasFlowRect{ false };

        // paragraphs (decoded utf-8, owned)
        std::vector<OwnedSpan> fParas{};

        // resolved
        BLRect fFlowBox{};
        bool fPreserveSpace{ false };

        // line height
        double fLineHeightMul{ 1.25 };
        bool   fLineHeightAbsPx{ false };
        double fLineHeightPx{ 0.0 };

        SVGFlowRoot(IAmGroot*) : SVGGraphicsElement()
        {
            setNeedsBinding(true);
        }

        BLRect getBBox() const override { return fFlowBox; }

        // Parse <rect ...> attributes (x,y,width,height) from element.data()
        void parseRectFromElement(const XmlElement& elem)
        {
            ByteSpan src = elem.data();
            ByteSpan k{}, v{};

            ByteSpan x{}, y{}, w{}, h{};

            while (readNextKeyAttribute(src, k, v))
            {
                InternedKey key = PSNameTable::INTERN(k);

                // avoid repeated INTERN calls for constants by caching once
                static InternedKey kx = svgattr::x();
                static InternedKey ky = svgattr::y();
                static InternedKey kw = svgattr::width();
                static InternedKey kh = svgattr::height();

                if (key == kx) x = v;
                else if (key == ky) y = v;
                else if (key == kw) w = v;
                else if (key == kh) h = v;
            }

            if (x) fX.loadFromChunk(x);
            if (y) fY.loadFromChunk(y);
            if (w) fW.loadFromChunk(w);
            if (h) fH.loadFromChunk(h);

            fHasFlowRect = true;
        }

        // Consume <flowRegion> subtree, looking for first <rect>
        void parseFlowRegion(XmlPull& iter)
        {
            int depth = 1;
            while (depth > 0 && iter.next())
            {
                const XmlElement& e = *iter;
                switch (e.kind())
                {
                case XML_ELEMENT_TYPE_START_TAG:
                    depth++;
                    // attributes are on this start tag
                    if (e.name() == "rect") {
                        parseRectFromElement(e);
                    }
                    break;
                case XML_ELEMENT_TYPE_SELF_CLOSING:
                    if (e.name() == "rect") {
                        parseRectFromElement(e);
                    }
                    break;
                case XML_ELEMENT_TYPE_END_TAG:
                    depth--;
                    break;
                default:
                    break;
                }
            }
        }

        // Consume <flowPara> subtree, collecting CONTENT/CDATA
        void parseFlowPara(XmlPull& iter)
        {
            OwnedSpan raw;
            int depth = 1;

            while (depth > 0 && iter.next())
            {
                const XmlElement& e = *iter;
                switch (e.kind())
                {
                case XML_ELEMENT_TYPE_START_TAG:
                    depth++;
                    break;
                case XML_ELEMENT_TYPE_END_TAG:
                    depth--;
                    break;
                case XML_ELEMENT_TYPE_CONTENT:
                case XML_ELEMENT_TYPE_CDATA:
                {
                    ByteSpan d = e.data();
                    if (d) appendDecodedXmlText(raw, d);
                } break;
                default:
                    break;
                }
            }

            // xml:space handling:
            //   preserve => keep as-is
            //   default  => trim/collapse
            if (!fPreserveSpace) {
                OwnedSpan collapsed = collapseWhitespace(raw.span());
                fParas.push_back(std::move(collapsed));
            }
            else {
                // trim nothing, keep literal
                fParas.push_back(std::move(raw));
            }
        }

        // Override loadStartTag to intercept flowRoot sub-elements
        void loadStartTag(XmlPull& iter, IAmGroot* groot) override
        {
            const XmlElement& elem = *iter;

            if (elem.name() == "flowRegion") {
                parseFlowRegion(iter);
                return;
            }

            if (elem.name() == "flowPara") {
                parseFlowPara(iter);
                return;
            }

            // fallback to your normal mechanism
            auto node = createContainerNode(iter, groot);
            if (node != nullptr) {
                this->addNode(node, groot);
            }
            else {
                skipSubtree(iter);
            }
        }

        void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            (void)groot;

            // xml:space preserve?
            {
                ByteSpan xs = getAttributeByName("xml:space");
                xs = trimOuterWsp(xs);
                if (xs && xs == "preserve")
                    fPreserveSpace = true;
            }

            // line-height
            {
                ByteSpan lh = getAttributeByName("line-height");
                bool absPx = false;
                double v = parseLineHeight(lh, absPx);
                if (absPx) {
                    fLineHeightAbsPx = true;
                    fLineHeightPx = v;
                }
                else {
                    fLineHeightMul = v;
                }
            }

            // Resolve flow rect to user units (pixels)
            if (fHasFlowRect)
            {
                // You use viewport() and/or object frame in other places.
                // We'll use ctx->viewport() for percentage resolution.
                BLRect vp = ctx->viewport();
                const double vw = vp.w;
                const double vh = vp.h;

                const double dpi = groot ? groot->dpi() : 96.0;

                const double x = fX.isSet() ? fX.calculatePixels(ctx->getFont(), vw, 0, dpi) : 0.0;
                const double y = fY.isSet() ? fY.calculatePixels(ctx->getFont(), vh, 0, dpi) : 0.0;
                const double w = fW.isSet() ? fW.calculatePixels(ctx->getFont(), vw, 0, dpi) : 0.0;
                const double h = fH.isSet() ? fH.calculatePixels(ctx->getFont(), vh, 0, dpi) : 0.0;

                fFlowBox = BLRect(x, y, w, h);
            }
        }

        void drawLine(IRenderSVG* ctx, const ByteSpan& txt, double x, double y)
        {
            uint32_t porder = ctx->getPaintOrder();
            for (int slot = 0; slot < 3; ++slot)
            {
                uint32_t ins = porder & 0x03;
                switch (ins)
                {
                case PaintOrderKind::SVG_PAINT_ORDER_FILL:
                    ctx->fillText(txt, x, y);
                    break;
                case PaintOrderKind::SVG_PAINT_ORDER_STROKE:
                    ctx->strokeText(txt, x, y);
                    break;
                case PaintOrderKind::SVG_PAINT_ORDER_MARKERS:
                default:
                    break;
                }
                porder >>= 2;
            }
        }

        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            //(void)groot;

            if (!fHasFlowRect) return;
            if (fFlowBox.w <= 0.0 || fFlowBox.h <= 0.0) return;

            // keep global text cursor unchanged
            const BLPoint savedCursor = ctx->textCursor();

            // clip to flow box
            ctx->clipRect(fFlowBox);

            const BLFont& font = ctx->getFont();
            const BLFontMetrics fm = font.metrics();
            const double ascent = double(fm.ascent);
            const double descent = double(fm.descent);
            const double emHeight = ascent + descent;

            double lineAdvance = 0.0;
            if (fLineHeightAbsPx) lineAdvance = fLineHeightPx;
            else lineAdvance = emHeight * fLineHeightMul;
            if (lineAdvance <= 0.0) lineAdvance = emHeight * 1.25;

            // alignment: from 'text-align' (Inkscape uses it heavily)
            enum Align { ALIGN_START, ALIGN_MIDDLE, ALIGN_END };
            Align align = ALIGN_START;
            {
                ByteSpan ta = trimOuterWsp(getAttributeByName("text-align"));
                if (ta) {
                    InternedKey tak = PSNameTable::INTERN(ta);

                    if (tak == svgval::center()) 
                        align = ALIGN_MIDDLE;
                    else if (tak == svgval::right() || tak == svgval::end()) 
                        align = ALIGN_END;
                }
            }

            double penX0 = fFlowBox.x;
            double penY = fFlowBox.y + ascent;
            const double maxBaselineY = fFlowBox.y + fFlowBox.h - descent;

            std::vector<OwnedSpan> lines;

            for (size_t pi = 0; pi < fParas.size(); ++pi)
            {
                ByteSpan para = fParas[pi].span();

                // blank para => blank line
                if (!para || spanAllWsp(para)) {
                    penY += lineAdvance;
                    if (penY > maxBaselineY) break;
                    continue;
                }

                wrapParagraph(lines, font, para, fFlowBox.w, fPreserveSpace);

                for (size_t li = 0; li < lines.size(); ++li)
                {
                    if (penY > maxBaselineY) break;

                    ByteSpan line = lines[li].span();

                    // compute x for alignment
                    double x = penX0;
                    if (align != ALIGN_START && line)
                    {
                        const double w = measureTextWidth(font, line);
                        if (align == ALIGN_MIDDLE)
                            x = penX0 + (fFlowBox.w * 0.5) - (w * 0.5);
                        else
                            x = penX0 + (fFlowBox.w) - w;
                    }

                    drawLine(ctx, line, x, penY);
                    penY += lineAdvance;
                }

                // small paragraph gap (optional; conservative)
                penY += lineAdvance * 0.10;

                if (penY > maxBaselineY)
                    break;
            }

            ctx->textCursor(savedCursor);
        }
    };

} // namespace waavs

#endif // SVGFLOWROOT_H
