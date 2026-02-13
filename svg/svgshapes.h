#pragma once

#include <string>
#include <array>
#include <list>
#include <functional>
#include <unordered_map>

#include "svgattributes.h"
#include "svgstructuretypes.h"
#include "b2dpathbuilder.h"
#include "svgtext.h"
#include "viewport.h"
#include "svgmarker.h"



namespace waavs 
{
    // A helper to paint a path with fill/stroke/markers
    // deals with the paint order

    struct PathPainter
    {
        IRenderSVG* fCtx{};
        const BLPath& fFillPath;
        const BLPath* fStrokePathOpt{ nullptr };

        PathPainter(IRenderSVG* ctx, const BLPath& fillPath, const BLPath* strokeOpt) noexcept
            : fCtx(ctx), fFillPath(fillPath), fStrokePathOpt(strokeOpt) {
        }

        void begin() noexcept { fCtx->beginDrawShape(fFillPath); }
        void end()   noexcept { fCtx->endDrawShape(); }

        void onFill() noexcept { fCtx->fillShape(fFillPath); }

        void onStroke() noexcept {
            const BLPath& p = fStrokePathOpt ? *fStrokePathOpt : fFillPath;
            fCtx->strokeShape(p);
        }
    };


    // MarkersPainter
    //
    // A painter that only draws markers for a given path program
    struct MarkersPainter
    {
        ISVGElement* fOwner{ nullptr };
        IRenderSVG* fCtx{ nullptr };
        IAmGroot* fGroot{ nullptr };
        const PathProgram* fProg{nullptr};

        void onFill() noexcept {}
        void onStroke() noexcept {}
        void onMarkers() noexcept
        {
            if (!fOwner || !fProg || !fCtx)
                return;

            drawMarkersForPathProgram(fOwner, fCtx, fGroot, *fProg);
        }
    };

    // ShapePaintOps
    // 
    // This is a composite painter that can do fill, stroke, and markers
    struct ShapePaintOps
    {
        PathPainter& path;
        MarkersPainter& markers;

        void onFill()    noexcept { path.onFill(); }
        void onStroke()  noexcept { path.onStroke(); }
        void onMarkers() noexcept { markers.onMarkers(); }
    };
} // namespace waavs



namespace waavs 
{
    // SVGPathBasedGeometry
    //
    // All SVG shapes are represented using the BLPath
    // ultimately.  This serves as a base object for all the shapes
    // It handles the various attributes, and drawing of markers.
    // 
    // The core representation of the shape is the PathProgram fProg,
    // This program can be transformed and ultimately run to create 
    // a BLPath object.
    //
    struct SVGPathBasedGeometry : public SVGGraphicsElement
    {
        // Canonical path representation of the shape (normalized SVG semantics)
        PathProgram fProg{};

        mutable BLPath fFillPath{};
        mutable bool fFillPathValid{ false };

        mutable BLPath fStrokePath{};
        mutable uint64_t fStrokeCacheKey{ 0 };


        // Indication of whether we have markers or not
        bool fHasMarkers{ false };


        SVGPathBasedGeometry(IAmGroot* iMap)
            :SVGGraphicsElement()
        {
        }

        // Any time drawing attributes change, we need to invalidate
        void invalidateGeometry() noexcept
        {
            fFillPathValid = false;
            fStrokeCacheKey = 0;
        }

        // Get the path used for filling. We do a lazy evaluation here so we only
        // compute it when needed.
        //
        // Hybrid mode:
        // - If fHasProg is true, materialize from fProg into fFillPath.
        // - Otherwise, mirror the already-built fPath into fFillPath.
        //
        // Notes:
        // - We always return fFillPath so callers have a stable reference.
        // - If both fHasProg==false and fPath is empty, fFillPath will be empty.
        const BLPath& getFillPath() const noexcept
        {
            if (fFillPathValid)
                return fFillPath;

            fFillPath.reset();

            // Canonical path program -> BLPath
            BLPathProgramExec exec(fFillPath);
            runPathProgram(fProg, exec);
            fFillPath.shrink();

            fFillPathValid = true;
            return fFillPath;
        }


        const BLPath & getStrokePath(IRenderSVG* ctx, IAmGroot* groot) const noexcept
        {
            // For now, stroke path is same as fill path
            return getFillPath();
        }

        // objectBoundingBox
        //
        // The bounding box of the shape, in the shape's own user space.  
        // This is used for percentage sizing, and marker placement.
        BLRect objectBoundingBox() const override
        {
            //double x{}, y{}, width{}, height{};
            //getBoundingBox(fProg, x, y, width, height);

            //return BLRect(x, y, width, height);

            BLBox bbox{};
            auto& path = getFillPath();

            path.getBoundingBox(&bbox);

            return BLRect(bbox.x0, bbox.y0, (bbox.x1 - bbox.x0), (bbox.y1 - bbox.y0));
        }

        bool contains(double x, double y) override
        {
            BLPoint localPoint(x, y);

            
            // BUGBUG - should use actual fill rule
            auto& path = getFillPath();

            BLHitTest ahit = path.hitTest(localPoint, BLFillRule::BL_FILL_RULE_EVEN_ODD);
            
            return (ahit == BLHitTest::BL_HIT_TEST_IN);
        }
        


        bool checkForMarkers()
        {
            // figure out if we have any markers set
            auto mStart = getAttribute(svgattr::marker_start());
            auto mMid = getAttribute(svgattr::marker_mid());
            auto mEnd = getAttribute(svgattr::marker_end());
            auto m = getAttribute(svgattr::marker());
            
            if ((!mStart.empty() && mStart != svgval::none()) || 
                (!mMid.empty() && mMid != svgval::none()) || 
                (!mEnd.empty() && mEnd != svgval::none()) || 
                (!m.empty() && m != svgval::none()))
            {
                fHasMarkers = true;
            }
            
            return fHasMarkers;
        }

        void fixupSelfStyleAttributes(IAmGroot* ) override
        {
            checkForMarkers();
        }

        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            PaintOrderProgram<uint8_t> prog(ctx->getPaintOrder());

            const BLPath& fillP = getFillPath();
            const BLPath& strokeP = getStrokePath(ctx, groot);

            PathPainter pathOps(ctx, fillP, &strokeP);

            MarkersPainter markerOps{ this, ctx, groot, &fProg };

            ShapePaintOps shapeOps{ pathOps, markerOps };

            pathOps.begin();
            prog.run(shapeOps);
            pathOps.end();
        }

    };
}

namespace waavs {
    
    //====================================
    // SVGLineElement
    //====================================
    struct DocLineState
    {
        SVGLengthValue x1{};
        SVGLengthValue y1{};
        SVGLengthValue x2{};
        SVGLengthValue y2{};
    };

    // Load the authored document state from the attributes
    static INLINE void loadDocLineState(DocLineState& state, const XmlAttributeCollection& attrs) noexcept
    {
        ByteSpan fX1{}, fY1{}, fX2{}, fY2{};
        attrs.getValue(svgattr::x1(), fX1);
        attrs.getValue(svgattr::y1(), fY1);
        attrs.getValue(svgattr::x2(), fX2);
        attrs.getValue(svgattr::y2(), fY2);

        state.x1 = parseLengthAttr(fX1);
        state.y1 = parseLengthAttr(fY1);
        state.x2 = parseLengthAttr(fX2);
        state.y2 = parseLengthAttr(fY2);
    }

    struct SVGLineElement : public SVGPathBasedGeometry
    {
        static void registerFactory() {
            registerSVGSingularNodeByName("line", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGLineElement>(groot);
                node->loadFromXmlElement(elem, groot);
                return node;
            });
        }


        BLLine geom{};
        DocLineState fDocState{};

        SVGLineElement(IAmGroot* iMap)
            :SVGPathBasedGeometry(iMap) {}

        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            loadDocLineState(fDocState, fAttributes);
        }

        void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {	
            double dpi = groot ? groot->dpi() : 96;

            const BLFont* fontOpt = nullptr;    // font not relevant here
            const BLRect paintVP = ctx->viewport();

            
            LengthResolveCtx cx{}, cy{};
            cx = makeLengthCtxUser(paintVP.w, 0.0, dpi, fontOpt);
            cy = makeLengthCtxUser(paintVP.h, 0.0, dpi, fontOpt);


            geom.x0 = resolveLengthOr(fDocState.x1, cx, 0);
            geom.y0 = resolveLengthOr(fDocState.y1, cy, 0);
            geom.x1 = resolveLengthOr(fDocState.x2, cx, 0);
            geom.y1 = resolveLengthOr(fDocState.y2, cy, 0);
            


            PathProgramBuilder pb;
            pb.moveTo(geom.x0, geom.y0);
            pb.lineTo(geom.x1, geom.y1);
            pb.end();

            fProg = std::move(pb.prog);

            invalidateGeometry();
        }
        
    };
    

    //====================================
    // SVGRectElement
    //====================================
    // SVG <rect> path emission.
    // - If w<=0 or h<=0 => not rendered => returns false.
    // - Corner radii follow SVG rules:
    //   * rx/ry default to 0.
    //   * if only one specified, the other equals it.
    //   * negative radii -> treat as 0.
    //   * clamp rx <= w/2, ry <= h/2.
    //
    // Emits:
    //   - sharp corners: M (x,y) L ... Z
    //   - rounded corners: M (x+rx,y) (L + A)*4 Z
    //
    static INLINE bool buildRectPathProgram(
        PathProgram& outProg,
        float x, float y, float w, float h,
        float rx, float ry) noexcept
    {
        outProg.clear();

        if (!(w > 0.0f) || !(h > 0.0f))
            return false;

        // Normalize radii per SVG
        if (rx < 0.0f) rx = 0.0f;
        if (ry < 0.0f) ry = 0.0f;

        // If exactly one radius is non-zero, mirror it to the other.
        // (This matches SVG's behavior where unspecified rx/ry defaults to the other if that one is set.)
        if (rx > 0.0f && ry == 0.0f) ry = rx;
        if (ry > 0.0f && rx == 0.0f) rx = ry;

        // Clamp to half extents
        const float hw = 0.5f * w;
        const float hh = 0.5f * h;
        if (rx > hw) rx = hw;
        if (ry > hh) ry = hh;

        PathProgramBuilder b;
        b.reset();

        // Sharp-corner rect
        if (rx == 0.0f || ry == 0.0f)
        {
            b.moveTo(x, y);
            b.lineTo(x + w, y);
            b.lineTo(x + w, y + h);
            b.lineTo(x, y + h);
            b.close();
            b.end();

            outProg = std::move(b.prog);
            return true;
        }

        // Rounded rect: use 4 quarter-ellipse arcs (A)
        //
        // Arc params: rx ry xAxisRotation largeArcFlag sweepFlag x y
        // For quarter arcs: largeArcFlag=0 always.
        // sweepFlag controls direction; keep consistent (1) for clockwise outline.
        constexpr float kXRot = 0.0f;
        constexpr float kLarge = 0.0f;
        constexpr float kSweep = 1.0f;

        const float x0 = x;
        const float y0 = y;
        const float x1 = x + w;
        const float y1 = y + h;

        // Start at top edge, after top-left corner
        b.moveTo(x0 + rx, y0);

        // Top edge -> top-right corner
        b.lineTo(x1 - rx, y0);
        b.arcTo(rx, ry, kXRot, kLarge, kSweep, x1, y0 + ry);

        // Right edge -> bottom-right corner
b.lineTo(x1, y1 - ry);
b.arcTo(rx, ry, kXRot, kLarge, kSweep, x1 - rx, y1);

// Bottom edge -> bottom-left corner
b.lineTo(x0 + rx, y1);
b.arcTo(rx, ry, kXRot, kLarge, kSweep, x0, y1 - ry);

// Left edge -> top-left corner
b.lineTo(x0, y0 + ry);
b.arcTo(rx, ry, kXRot, kLarge, kSweep, x0 + rx, y0);

b.close();
b.end();

outProg = std::move(b.prog);
return true;
    }

    struct DocRectState
    {
        SVGLengthValue x{};
        SVGLengthValue y{};
        SVGLengthValue width{};
        SVGLengthValue height{};
        SVGLengthValue rx{};
        SVGLengthValue ry{};
    };

    static INLINE void loadDocRectState(DocRectState& state, const XmlAttributeCollection& attrs) noexcept
    {
        ByteSpan fX{}, fY{}, fWidth{}, fHeight{}, fRx{}, fRy{};

        attrs.getValue(svgattr::x(), fX);
        attrs.getValue(svgattr::y(), fY);
        attrs.getValue(svgattr::width(), fWidth);
        attrs.getValue(svgattr::height(), fHeight);
        attrs.getValue(svgattr::rx(), fRx);
        attrs.getValue(svgattr::ry(), fRy);

        state.x = parseLengthAttr(fX);
        state.y = parseLengthAttr(fY);
        state.width = parseLengthAttr(fWidth);
        state.height = parseLengthAttr(fHeight);
        state.rx = parseLengthAttr(fRx);
        state.ry = parseLengthAttr(fRy);
    }

    struct SVGRectElement : public SVGPathBasedGeometry
    {
        static void registerSingular() {
            registerSVGSingularNodeByName("rect", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGRectElement>(groot);
                node->loadFromXmlElement(elem, groot);
                return node;
                });
        }

        static void registerFactory() {
            registerContainerNodeByName("rect",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGRectElement>(groot);
                    node->loadFromXmlPull(iter, groot);

                    return node;
                });

            registerSingular();
        }

        BLRoundRect geom{};
        DocRectState fDocState{};

        SVGRectElement(IAmGroot* iMap) :SVGPathBasedGeometry(iMap) {}

        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            loadDocRectState(fDocState, fAttributes);
        }

        void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            double dpi = groot ? groot->dpi() : 96.0;
            double w = 1.0;
            double h = 1.0;

            const BLFont* fontOpt = nullptr;    // font not relevant here
            const BLRect paintVP = ctx->viewport();

            LengthResolveCtx cx{}, cy{};
            cx = makeLengthCtxUser(paintVP.w, 0.0, dpi, fontOpt);
            cy = makeLengthCtxUser(paintVP.h, 0.0, dpi, fontOpt);

            geom.x = resolveLengthOr(fDocState.x, cx, 0);
            geom.y = resolveLengthOr(fDocState.y, cy, 0);
            geom.w = resolveLengthOr(fDocState.width, cx, 0);
            geom.h = resolveLengthOr(fDocState.height, cy, 0);

            // Reject if width or height is not positive
            if (!(geom.w > 0.0) || !(geom.h > 0.0))
            {
                fProg.clear();
                invalidateGeometry();
                return;
            }

            geom.rx = resolveLengthOr(fDocState.rx, cx, 0);
            geom.ry = resolveLengthOr(fDocState.ry, cy, 0);

            fProg.clear();

            buildRectPathProgram(fProg,
                (float)geom.x, (float)geom.y,
                (float)geom.w, (float)geom.h,
                (float)geom.rx, (float)geom.ry);

            // Invalidate cached fill/stroke materializations
            invalidateGeometry();
        }
    
    };
    

    //====================================
    //  SVGCircleElement 
    //====================================
    // Emit a circle into a PathProgram using two SVG arc segments.
    // Returns true if it emitted a drawable program (r > 0).
    static INLINE bool buildCirclePathProgram(PathProgram& outProg, float cx, float cy, float r) noexcept
    {
        outProg.clear();

        // Per SVG, r <= 0 => no rendering.
        if (!(r > 0.0f))
            return false;

        PathProgramBuilder b;
        b.reset();

        // Start at (cx + r, cy)
        b.moveTo(cx + r, cy);

        // Two 180-degree arcs to complete the circle.
        // Use largeArcFlag=1, sweepFlag=0 (or 1) consistently; either way works,
        // as long as the two arcs connect endpoints correctly.
        //
        // rx ry xAxisRotation largeArcFlag sweepFlag x y
        b.arcTo(r, r, 0.0f, 1.0f, 0.0f, cx - r, cy);
        b.arcTo(r, r, 0.0f, 1.0f, 0.0f, cx + r, cy);

        b.close();
        b.end();

        outProg = std::move(b.prog);
        return true;
    }



    struct SVGCircleElement : public SVGPathBasedGeometry
    {
        static void registerSingular() {
            registerSVGSingularNodeByName("circle", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGCircleElement>(groot);
                node->loadFromXmlElement(elem, groot);
                return node;
                });
        }

        static void registerFactory() {
            registerContainerNodeByName("circle",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGCircleElement>(groot);
                    node->loadFromXmlPull(iter, groot);

                    return node;
                });
            

            registerSingular();
        }

        
        BLCircle geom{};


        SVGCircleElement(IAmGroot* iMap) :SVGPathBasedGeometry(iMap) {}

        
        void bindSelfToContext(IRenderSVG *ctx, IAmGroot *groot) override
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

            
            SVGDimension fCx{};
            SVGDimension fCy{};
            SVGDimension fR{};
            
            fCx.loadFromChunk(getAttributeByName("cx"));
            fCy.loadFromChunk(getAttributeByName("cy"));
            fR.loadFromChunk(getAttributeByName("r"));
            
            geom.cx = fCx.calculatePixels(w, 0, dpi);
            geom.cy = fCy.calculatePixels(h, 0, dpi);
            geom.r = fR.calculatePixels(w, h, dpi);

            fProg.clear();
            buildCirclePathProgram(fProg, (float)geom.cx, (float)geom.cy, (float)geom.r);

            // Invalidate cached paths
            invalidateGeometry();

        }
    };
    

    //====================================
    //	SVGEllipseElement 
    //====================================
    // Emit an ellipse into a PathProgram using two SVG arc segments.
    // Returns true if it emitted a drawable program (rx > 0 && ry > 0).
    static INLINE bool buildEllipsePathProgram(PathProgram& outProg, float cx, float cy, float rx, float ry) noexcept
    {
        outProg.clear();

        // Per SVG, if rx<=0 or ry<=0 => no rendering.
        if (!(rx > 0.0f) || !(ry > 0.0f))
            return false;

        PathProgramBuilder b;
        b.reset();

        // Start at (cx + rx, cy)
        b.moveTo(cx + rx, cy);

        // Two 180-degree arcs.
        // rx ry xAxisRotation largeArcFlag sweepFlag x y
        b.arcTo(rx, ry, 0.0f, 1.0f, 0.0f, cx - rx, cy);
        b.arcTo(rx, ry, 0.0f, 1.0f, 0.0f, cx + rx, cy);

        b.close();
        b.end();

        outProg = std::move(b.prog);
        return true;
    }

    struct SVGEllipseElement : public SVGPathBasedGeometry
    {
        static void registerFactory() {
            registerSVGSingularNodeByName("ellipse", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGEllipseElement>(groot);
                node->loadFromXmlElement(elem, groot);
                
                return node; });
        }


        BLEllipse geom{};

        
        SVGEllipseElement(IAmGroot* iMap)
            :SVGPathBasedGeometry(iMap) {}
        
        
        BLRect objectBoundingBox() const override
        {
            return BLRect(geom.cx - geom.rx, geom.cy - geom.ry, geom.rx * 2, geom.ry * 2);
        }
        
        void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
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

            

            SVGDimension fCx{};
            SVGDimension fCy{};
            SVGDimension fRx{};
            SVGDimension fRy{};
            
            fCx.loadFromChunk(getAttributeByName("cx"));
            fCy.loadFromChunk(getAttributeByName("cy"));
            fRx.loadFromChunk(getAttributeByName("rx"));
            fRy.loadFromChunk(getAttributeByName("ry"));
            
            
            geom.cx = fCx.calculatePixels(w, 0, dpi);
            geom.cy = fCy.calculatePixels(h, 0, dpi);
            geom.rx = fRx.calculatePixels(w, 0, dpi);
            geom.ry = fRy.calculatePixels(h, 0, dpi);


            // Program-first (no legacy fPath)
            fProg.clear();
            //fHasProg = false;

            if (buildEllipsePathProgram(fProg,
                (float)geom.cx, (float)geom.cy,
                (float)geom.rx, (float)geom.ry))
            {
                //fHasProg = true;
            }

            // Invalidate caches
            invalidateGeometry();
        }

    };
    

    //====================================
    // SVGPolylineElement
    //
    //====================================
        // Parse SVG "points" attribute into a PathProgram.
    // - closePath=false => polyline
    // - closePath=true  => polygon
    //
    // Returns true if it produced at least a moveto.
    static bool parsePointsToPathProgram(const ByteSpan& inChunk, PathProgram& outProg, bool closePath) noexcept
    {
        outProg.clear();

        ByteSpan s = chunk_trim(inChunk, chrWspChars);
        if (!s)
            return false;

        SVGTokenListView view(s);

        ByteSpan tok{};
        double dx = 0.0;
        double dy = 0.0;

        // Need first x
        if (!view.nextNumberToken(tok))
            return false;
        {
            ByteSpan t = tok;
            if (!readNumber(t, dx))
                return false;
        }

        // Need first y
        if (!view.nextNumberToken(tok))
            return false;
        {
            ByteSpan t = tok;
            if (!readNumber(t, dy))
                return false;
        }

        PathProgramBuilder b;
        b.reset();

        // moveTo first pair
        b.moveTo((float)dx, (float)dy);

        // Remaining pairs => lineTo
        while (view.nextNumberToken(tok))
        {
            ByteSpan t = tok;
            if (!readNumber(t, dx))
                break;

            if (!view.nextNumberToken(tok))
                break; // trailing odd number (permissive)

            t = tok;
            if (!readNumber(t, dy))
                break;

            b.lineTo((float)dx, (float)dy);
        }

        if (closePath)
            b.close();

        b.end();
        outProg = std::move(b.prog);

        // Optional strictness:
        // ByteSpan rem = view.remaining();
        // rem.skipWhile(SVGTokenListView::sepChars());
        // if (rem) return false;

        return true;
    }


    struct SVGPolylineElement : public SVGPathBasedGeometry
    {
        static void registerFactory() {
            registerSVGSingularNodeByName("polyline", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGPolylineElement>(groot);
                node->loadFromXmlElement(elem, groot);
                
                return node;
            });
        }

        
        SVGPolylineElement(IAmGroot* groot) :SVGPathBasedGeometry(groot) {}


        void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {

            ByteSpan pts{};
            pts = getAttribute(svgattr::points());
            if (!pts.empty())
            {
                if (!parsePointsToPathProgram(pts, fProg, false)) {
                    // Failed to parse points; clear program
                    fProg.clear();
                    //fHasProg = false;
                }
                else {
                    //fHasProg = true;
                }
            }

            fFillPathValid = false;
            fStrokeCacheKey = 0;

        }
    };
    

    //====================================
    //
    // SVGPolygonElement
    //
    //====================================
    struct SVGPolygonElement : public SVGPathBasedGeometry
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("polygon", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGPolygonElement>(groot);
                node->loadFromXmlElement(elem, groot);
                return node;
                });
        }

        static void registerFactory() {
            registerContainerNodeByName("polygon",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGPolygonElement>(groot);
                    node->loadFromXmlPull(iter, groot);

                    return node;
                });
            

            registerSingularNode();
        }

        SVGPolygonElement(IAmGroot* iMap) 
            :SVGPathBasedGeometry(iMap) {}

        void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {

            ByteSpan pts{};
            pts = getAttribute(svgattr::points());
            if (!pts.empty())
            {
                if (!parsePointsToPathProgram(pts, fProg, true)) {
                    fProg.clear();
                    //fHasProg = false;
                }
                else {
                    //fHasProg = true;
                }
            }

            fFillPathValid = false;
            fStrokeCacheKey = 0;
        }
    };
    

    //====================================
    // SVGPath
    // <path>
    //====================================
    struct SVGPathElement : public SVGPathBasedGeometry
    {
        static void registerSingularNode() {
            registerSVGSingularNodeByName(svgtag::tag_path(), [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGPathElement>(groot);
                node->loadFromXmlElement(elem, groot);
                
                return node;
            });
        }

        static void registerFactory()
        {
            registerContainerNodeByName(svgtag::tag_path(),
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGPathElement>(groot);
                    node->loadFromXmlPull(iter, groot);

                    return node;
                });
            

            registerSingularNode();
        }
        
        SVGPathElement(IAmGroot* iMap) 
            :SVGPathBasedGeometry(iMap)
        {
        }
        
        virtual void bindSelfToContext(IRenderSVG*, IAmGroot*) override
        {
            fProg.clear();
            //fHasProg = false;

            auto d = getAttribute(svgattr::d());
            if (d) {
                parsePathProgram(d, fProg);
                //fHasProg = true;
            }

            fFillPathValid = false;
            fStrokeCacheKey = 0;
        }


    };

}
