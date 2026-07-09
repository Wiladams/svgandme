// viewport.h

#pragma once


#include "maths.h"
#include "svgenums.h"
#include "lang_charset.h"
#include "svgdatatypes.h"


namespace waavs {
    // Low level data type parsers

    // parsePreserveAspectRatio
    // 
    // Parse the preserveAspectRatio attribute, 
    // returning the alignment and meetOrSlice values.
    // 
    //
    static bool preserveAspectRatio_parse(const ByteSpan& inChunk, AspectRatioAlignKind& alignment, AspectRatioMeetOrSliceKind& meetOrSlice)
    {
        alignment = AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMID;
        meetOrSlice = AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_MEET;

        ByteSpan s = inChunk;
        bspan_skip_spaces(s);
        if (s.empty())
            return false;

        // Get first token, which should be alignment
        ByteSpan align = bspan_read_until(s, chrWspChars);

        if (align.empty())
            return false;

        // We have an alignment token, convert to numeric value
        uint32_t parsedAlign = 0;
        if (!getEnumValue(SVGAspectRatioAlignEnum, align, parsedAlign))
            return false;
        
        alignment = AspectRatioAlignKind(parsedAlign);

        // Now, see if there is a slice value
        bspan_skip_spaces(s);

        // if there is no meetOrSlice token, we should set the 
        // meetOrSlice to 'meet' by default
        if (!s.empty()) {
            ByteSpan mos = bspan_read_until(s, chrWspChars);
            uint32_t parsedMOS = 0;
            if (!mos.empty() && !getEnumValue(SVGAspectRatioMeetOrSliceEnum, mos, parsedMOS))
                return false;

            meetOrSlice = AspectRatioMeetOrSliceKind(parsedMOS);
        }

        return true;
    }

    // viewBox_parse()
    //
    // Parse a viewBox attribute, returning the rectangle values.
    // The viewBox attribute is a list of four numbers: 
    // min-x, min-y, width, height
    // Separators can be whitespace and/or commas.
    // The values are numbers without any units, so an early parse
    // directly to a rectangle works fine.
    //
    static bool viewBox_parse(const ByteSpan& inChunk, WGRectD& r) noexcept
    {
        if (!inChunk)
            return false;

        ByteSpan s = inChunk;
        double x, y, w, h = 0.0;

        if (!svgList_readNumber(s, x)) return false;
        if (!svgList_readNumber(s, y)) return false;
        if (!svgList_readNumber(s, w)) return false;
        if (!svgList_readNumber(s, h)) return false;

        svgList_skipSeparators(s);
        if (!s.empty())
            return false;

        r = { x, y, w, h };
        return true;
    }

}

namespace waavs {

    struct PreserveAspectRatio final
    {
    private:
        static constexpr AspectRatioAlignKind DEFAULT_ALIGNMENT = AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMID;
        static constexpr AspectRatioMeetOrSliceKind DEFAULT_MEET_OR_SLICE = AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_MEET;

        AspectRatioAlignKind fAlignment = DEFAULT_ALIGNMENT;
        AspectRatioMeetOrSliceKind fMeetOrSlice = DEFAULT_MEET_OR_SLICE;
        bool fIsSet{ false };

    public:
        PreserveAspectRatio() = default;

        PreserveAspectRatio(const char* cstr) noexcept
        {
            ByteSpan aspan(cstr);
            loadFromChunk(aspan);
        }

        PreserveAspectRatio(const ByteSpan& inChunk) noexcept
        {
            loadFromChunk(inChunk);
        }

        constexpr AspectRatioMeetOrSliceKind meetOrSlice() const noexcept { return fMeetOrSlice; }
        void setMeetOrSlice(AspectRatioMeetOrSliceKind m) noexcept { fMeetOrSlice = m; }

        constexpr AspectRatioAlignKind align() const noexcept { return fAlignment; }
        void setAlign(AspectRatioAlignKind a) noexcept { fAlignment = a; }

        static void splitAlignment(AspectRatioAlignKind aligned, SVGAlignment& xAlign, SVGAlignment& yAlign) noexcept
        {
            switch (aligned)
            {
            case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMINYMIN:
                xAlign = SVGAlignment::SVG_ALIGNMENT_START;
                yAlign = SVGAlignment::SVG_ALIGNMENT_START;
                return;

            case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMINYMID:
                xAlign = SVGAlignment::SVG_ALIGNMENT_START;
                yAlign = SVGAlignment::SVG_ALIGNMENT_MIDDLE;
                return;


            case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMINYMAX:
                xAlign = SVGAlignment::SVG_ALIGNMENT_START;
                yAlign = SVGAlignment::SVG_ALIGNMENT_END;
                return;


                // X Middle
            case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMIN:
                xAlign = SVGAlignment::SVG_ALIGNMENT_MIDDLE;
                yAlign = SVGAlignment::SVG_ALIGNMENT_START;
                return;

            case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMID:
                xAlign = SVGAlignment::SVG_ALIGNMENT_MIDDLE;
                yAlign = SVGAlignment::SVG_ALIGNMENT_MIDDLE;
                return;

            case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMAX:
                xAlign = SVGAlignment::SVG_ALIGNMENT_MIDDLE;
                yAlign = SVGAlignment::SVG_ALIGNMENT_END;
                return;


                // X End
            case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMAXYMIN:
                xAlign = SVGAlignment::SVG_ALIGNMENT_END;
                yAlign = SVGAlignment::SVG_ALIGNMENT_START;
                return;

            case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMAXYMID:
                xAlign = SVGAlignment::SVG_ALIGNMENT_END;
                yAlign = SVGAlignment::SVG_ALIGNMENT_MIDDLE;
                return;

            case AspectRatioAlignKind::SVG_ASPECT_RATIO_XMAXYMAX:
                xAlign = SVGAlignment::SVG_ALIGNMENT_END;
                yAlign = SVGAlignment::SVG_ALIGNMENT_END;
                return;

                // NONE
            case AspectRatioAlignKind::SVG_ASPECT_RATIO_NONE:
            default:
                xAlign = SVGAlignment::SVG_ALIGNMENT_NONE;
                yAlign = SVGAlignment::SVG_ALIGNMENT_NONE;
                return;

            }
        }


        // Load the data type from a single ByteSpan
        bool loadFromChunk(const ByteSpan& inChunk) noexcept
        {
            fIsSet = preserveAspectRatio_parse(inChunk, fAlignment, fMeetOrSlice);
            return fIsSet;
        }


    };



    // ViewStateAuthoring
    //
    // Represents the as-authored state of the viewport and viewbox settings for an element.
    // This is the state that is loaded from the XML attributes, and it preserves the original units and values.
    // This is separate from the resolved state (SVGViewportState) because we may need to re-resolve the viewport 
    // settings if the context changes (e.g. parent element's viewport changes, or the canvas size changes).
    struct DocViewportState
    {
        // as-authored values (after style merge)
        SVGLengthValue x{};
        SVGLengthValue y{};
        SVGLengthValue width{};
        SVGLengthValue height{};

        PreserveAspectRatio par{};

        bool hasViewBox{ false };
        WGRectD viewBox{};
    };

    struct SVGViewportState
    {
        WGRectD fViewport;   // The viewport rectangle

        bool fHasViewBox{ false };
        WGRectD fViewBox;    // The viewBox rectangle

        PreserveAspectRatio fPreserveAspect{};

        // The transformation from viewBox to viewport,
        // Calculated based on the viewport, viewbox, and preserveAspectRatio settings
        WGMatrix3x3 viewBoxToViewportXform{};

        // Whether the viewBoxToViewportXform has been resolved based on the current settings
        bool fResolved{ false };
    };

    // onLoadedFromXmlPull
    //
    // This is called after we've loaded the element and all its children from the XML
    // In the case of a top level svg element, the entirety of the document has been loaded
    // so it's safe to resolve style attributes now.
    //
    // For non-top level svg elements, it's not guaranteed that the 
    // style information has been fully loaded yet, so we just
    // resolve what we can.
    //
    static void loadDocViewportState(DocViewportState& vps, const XmlAttributeCollection& attrs)
    {

        // Load the non-bound attribute values here, for processing later
        // when we bind.
        // x, y, width, height
        lengthValue_parse(attrs.getValue(svgattr::x()), vps.x);
        lengthValue_parse(attrs.getValue(svgattr::y()), vps.y);
        lengthValue_parse(attrs.getValue(svgattr::width()), vps.width);
        lengthValue_parse(attrs.getValue(svgattr::height()), vps.height);


        // preserAspectRatio
        ByteSpan parAttr{};
        if (attrs.getValue(svgattr::preserveAspectRatio(), parAttr))
        {
            vps.par.loadFromChunk(parAttr);
        }

        // viewBox (structural)
        ByteSpan vbAttr{};
        WGRectD vbFrame{};
        if (attrs.getValue(svgattr::viewBox(), vbAttr) && viewBox_parse(vbAttr, vbFrame))
        {
            vps.hasViewBox = true;
            vps.viewBox = vbFrame;
        }


    }
}



namespace waavs
{

    static bool computeViewBoxToViewport(
        const WGRectD& viewport,
        const WGRectD& viewBox,
        const PreserveAspectRatio& par,
        WGMatrix3x3& out)
    {
        if (viewport.w <= 0 || viewport.h <= 0) return false;
        if (viewBox.w <= 0 || viewBox.h <= 0) return false;

        const double sx0 = viewport.w / viewBox.w;
        const double sy0 = viewport.h / viewBox.h;

        double sx = sx0, sy = sy0;
        double ax = 0.0, ay = 0.0;

        if (par.align() != AspectRatioAlignKind::SVG_ASPECT_RATIO_NONE) {
            double s = sx0;
            if (par.meetOrSlice() == AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_SLICE)
                s = max(sx0, sy0);
            else
                s = min(sx0, sy0);

            sx = sy = s;

            const double fitW = viewBox.w * s;
            const double fitH = viewBox.h * s;

            SVGAlignment xA{}, yA{};
            PreserveAspectRatio::splitAlignment(par.align(), xA, yA);

            if (xA == SVGAlignment::SVG_ALIGNMENT_MIDDLE) ax = (viewport.w - fitW) * 0.5;
            else if (xA == SVGAlignment::SVG_ALIGNMENT_END) ax = (viewport.w - fitW);

            if (yA == SVGAlignment::SVG_ALIGNMENT_MIDDLE) ay = (viewport.h - fitH) * 0.5;
            else if (yA == SVGAlignment::SVG_ALIGNMENT_END) ay = (viewport.h - fitH);
        }

        out = WGMatrix3x3::makeIdentity();
        out.translate(viewport.x, viewport.y);
        out.translate(ax, ay);
        out.scale(sx, sy);
        out.translate(-viewBox.x, -viewBox.y);

        return true;
    }


    // Minimal resolve:
// - containingVP: parent viewport rect (user units)
// - authored: parsed attribute state (lengths + viewBox + par)
// - isTopLevel: top-level <svg> ignores x/y; nested honors x/y
// - dpi/font: only needed for em/ex; pass nullptr font if you want
    static bool resolveViewState(
        const WGRectD& containingVP,
        const DocViewportState& authored,
        bool isTopLevel,
        double dpi,
        const BLFont* fontOpt,
        SVGViewportState& out) noexcept
    {
        static constexpr double DEFAULT_CANVAS_WIDTH = 300.0;
        static constexpr double DEFAULT_CANVAS_HEIGHT = 150.0;

        out = SVGViewportState{}; // reset

        // If containingVP is not meaningful, fall back to SVG defaults.
        // 300x150 is the default viewport size per SVG spec, 
        // but it can be overridden by the document's root <svg> element
        const double containW = (containingVP.w > 0.0) ? containingVP.w : DEFAULT_CANVAS_WIDTH;
        const double containH = (containingVP.h > 0.0) ? containingVP.h : DEFAULT_CANVAS_HEIGHT;

        // Defaults:
        // - nested <svg>: width/height default 100% of containingVP
        // - top-level <svg>: if containingVP is meaningful, use it; else 300x150
        //const double defaultW = isTopLevel ? containW : containW;
        //const double defaultH = isTopLevel ? containH : containH;

        // If the viewport has relative units (%), they are relative to 
        // the containing viewport dimensions (containW, containH).
        const LengthResolveCtx ctxX = makeLengthCtxUser(containW, 0.0, dpi, fontOpt);
        const LengthResolveCtx ctxY = makeLengthCtxUser(containH, 0.0, dpi, fontOpt);
        const LengthResolveCtx ctxW = ctxX, ctxH = ctxY;

        // x/y:
        // top-level ignores x/y; nested honors them.
        double x = isTopLevel ? 0.0 : (resolveLengthOr(authored.x, ctxX, 0.0) + containingVP.x);
        double y = isTopLevel ? 0.0 : (resolveLengthOr(authored.y, ctxY, 0.0) + containingVP.y);

        // width/height:
        double w = resolveLengthOr(authored.width, ctxW, containW);
        double h = resolveLengthOr(authored.height, ctxH, containH);

        // Clamp negatives => non-renderable viewport (minimal policy)
        if (w < 0.0) w = 0.0;
        if (h < 0.0) h = 0.0;

        out.fViewport = { x, y, w, h };

        if (out.fViewport.w <= 0.0 || out.fViewport.h <= 0.0) {
            out.fResolved = false;
            return false;
        }

        // viewBox:
        out.fPreserveAspect = authored.par;

        if (authored.hasViewBox)
        {
            out.fHasViewBox = true;
            out.fViewBox = authored.viewBox;
            if (!computeViewBoxToViewport(out.fViewport, out.fViewBox, out.fPreserveAspect, out.viewBoxToViewportXform))
            {
                out.fResolved = false;
                return false;
            }
        }
        else {
            out.fHasViewBox = false;
            out.fViewBox = { 0.0, 0.0, out.fViewport.w, out.fViewport.h };

            // no viewBox means, no aspect fit transform, so viewBoxToViewportXform is identity
            out.viewBoxToViewportXform = WGMatrix3x3::makeIdentity();
        }

        out.fResolved = true;
        return true;
    }

}


