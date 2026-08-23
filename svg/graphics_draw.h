#pragma once


#include <functional>


#include "blend2d_connect.h"

#include "fonthandler.h"
#include "svgenums.h"
#include "svgdatatypes.h"
#include "surface.h"
#include "render_state.h"

namespace waavs
{
    // SVGTextPosStream
    // 
    // Used to manage the 'dx' and 'dy' attributes
    struct SVGTextPosStream {
        SVGTokenListView x{};
        SVGTokenListView y{};
        SVGTokenListView dx{};
        SVGTokenListView dy{};
        SVGTokenListView rotate{};

        bool hasX{ false };
        bool hasY{ false };
        bool hasDx{ false };
        bool hasDy{ false };
        bool hasRotate{ false };

        void reset() {
            x.reset({});
            y.reset({});
            dx.reset({});
            dy.reset({});
            rotate.reset({});

            hasX = false;
            hasY = false;
            hasDx = false;
            hasDy = false;
            hasRotate = false;
        }

        static const SVGTextPosStream & empty() noexcept
        {
            static const SVGTextPosStream sEmpty;
            return sEmpty;
        }

    };

    // A stack frame is the effective stream at a particular nesting level
    struct SVGTextPosFrame
    {
        SVGTextPosStream eff{};
    };
}




namespace waavs
{
    
    // A specialization of state management, connected to a BLContext
    // This is used when rendering a tree of SVG elements
    struct IDrawGraphics
    {
        SVGStateManager fStateManager;
        
        // Text position management
        std::vector<waavs::SVGTextPosFrame> fTextPosStack;


    public:
        IDrawGraphics()
        {
            // Create a context by default, so we have 
            // something initially, so we don't have to use
            // null checks everywhere

            initState();
        }

        
        virtual ~IDrawGraphics() = default;

        SVGDrawingState& state() noexcept { return fStateManager.current(); }
        const SVGDrawingState& state() const noexcept { return fStateManager.current(); }

        virtual void onApplyDrawingState(const SVGDrawingState &) {}


        void initState()
        {
            fTextPosStack.clear();
            fTextPosStack.reserve(8);   // reserve some space for text position frames

            fStateManager.reset();

            background(BLRgba32(0x00000000));
            lineJoin(BL_STROKE_JOIN_MITER_CLIP);
            strokeMiterLimit(4);
            noStroke();
            strokeWidth(1.0);


            fillRule(BL_FILL_RULE_NON_ZERO);
            state().setFillPaintServer(SVGDrawingState::defaultFillServer());


            // BUGBUG - Need to set font for those who are getting default
            // font information from the context
            //setFontFamily("sans-serif");
            resetFont();
        }
        
        virtual void onAttach(Surface& surf, int threadCount, const SVGDrawingState* state)
        {}

        void attach(Surface& surf, int threadCount, const SVGDrawingState *state = nullptr) noexcept
        {
            onAttach(surf, threadCount, state);
        }
        
        virtual void onDetach() {}

        void detach()
        {
            flush();
            onDetach();
        }
        

        virtual BLImage* currentTarget() const noexcept { return nullptr; }

        // Call this before each frame to be drawn
        virtual void onRenew() {}
        void renew()
        {
            // Clear the canvas first
            clear();

            //resetState();
            initState();

            // Setup the default drawing state
            // to conform to what SVG expects
            blendMode(BL_COMP_OP_SRC_OVER);
            //strokeMiterLimit(4.0);
            //lineJoin(BL_STROKE_JOIN_MITER_CLIP);
            //fillRule(BL_FILL_RULE_NON_ZERO);

            //fill(BLRgba32(0, 0, 0));
            //noStroke();
            //strokeWidth(1.0);
            onRenew();
        }

        // Text position stack management
        INLINE void pushTextPosStream(const SVGTextPosStream& ps) noexcept
        {
            SVGTextPosFrame frame{};

            // Inherit from previous frame if there is one
            if (!fTextPosStack.empty())
            {
                frame = fTextPosStack.back();
            }
            else
            {
                frame.eff.reset();
            }

            // override only what the incoming stream provides
            if (ps.hasX) { frame.eff.x = ps.x; frame.eff.hasX = true;}
            if (ps.hasY) { frame.eff.y = ps.y; frame.eff.hasY = true; }
            if (ps.hasDx) { frame.eff.dx = ps.dx; frame.eff.hasDx = true; }
            if (ps.hasDy) { frame.eff.dy = ps.dy; frame.eff.hasDy = true; }
            if (ps.hasRotate) { frame.eff.rotate = ps.rotate; frame.eff.hasRotate = true; }

            fTextPosStack.push_back(frame);
        }

        INLINE void popTextPosStream() noexcept
        {
            if (!fTextPosStack.empty())
            {
                fTextPosStack.pop_back();
            }
        }

        INLINE bool hasTextPosStream() const noexcept { return !fTextPosStack.empty(); }

        INLINE const SVGTextPosStream& textPosStream() const noexcept
        {
            if (!fTextPosStack.empty())
            {
                return fTextPosStack.back().eff;
            }
            return SVGTextPosStream::empty();
        }

        bool consumeNextDxToken(ByteSpan& tok) noexcept
        {
            tok.reset();
            if (fTextPosStack.empty())
                return false;

            auto& eff = fTextPosStack.back().eff;
            if (!eff.hasDx)
                return false;

            return eff.dx.nextLengthToken(tok);
        }

        bool consumeNextDyToken(ByteSpan& tok) noexcept
        {
            tok.reset();
            if (fTextPosStack.empty())
                return false;

            auto& eff = fTextPosStack.back().eff;
            if (!eff.hasDy)
                return false;

            return eff.dy.nextLengthToken(tok);
        }

        bool consumeNextRotateToken(ByteSpan& tok) noexcept
        {
            tok.reset();
            if (fTextPosStack.empty())
                return false;

            auto& eff = fTextPosStack.back().eff;
            if (!eff.hasRotate)
                return false;

            return eff.rotate.nextNumberToken(tok);
        }



        virtual void onCopyDrawingState(const SVGDrawingState& state) {}
        void copyDrawingState(const SVGDrawingState& st) 
        {
            state() = st;
            onCopyDrawingState(st);
        }

        virtual void onPush() {}

        void push()
        {
            fStateManager.push();
            onPush();
        }
        
        virtual void onPop() {}
        void pop()  
        {
            WGPointD progressedCursor = state().getTextCursor();
           
            if (!fStateManager.pop())
                return;

            // SVG text cursor is logically progressed, not normally rewound.
            state().setTextCursor(progressedCursor);

            onPop();
        }



        
        virtual void onFlush() {}
        void flush()
        {
            onFlush();
        }



        // Canvas management
        virtual void onClear() {}
        void clear()
        {
            onClear();
        }

        virtual void onClearToBackground() {}
        void clearToBackground()
        {
            onClearToBackground();
        }



        // Coordinate system transformation management
        // The difference between transform(), and applyTransform() is
        // transform() - will set the transformation absolutely
        // applyTransform() - will add whatever transform is supplied to 
        //   the existing transform
        virtual void onSetTransform(const WGMatrix3x3& value) {}
        void setTransform(const WGMatrix3x3& value)
        {
            state().setTransform(value);
            onSetTransform(value);
        }

        virtual void onApplyTransform(const WGMatrix3x3& value) {}
        void applyTransform(const WGMatrix3x3& value)
        {
            auto t = state().getTransform();
            t.transform(value);
            state().setTransform(t);

            onApplyTransform(value);
        }

        virtual void onScale(double sx, double sy) {}
        void scale(double x, double y)
        {
            auto t = state().getTransform();
            t.scale(x, y);
            state().setTransform(t);
            //onTransform(t);

            onScale(x,y);
        }

        void scale(double s)
        {
            scale(s, s);
        }

        // Rotate around a specified point
        // default to 0,0
        virtual void onRotate(double angle, double cx, double xy) {}
        void rotate(double angle, double cx, double cy)
        {
            auto t = state().getTransform();
            t.rotate(angle, cx, cy);
            state().setTransform(t);
            //onTransform(t);

            onRotate(angle, cx, cy);
        }

        void rotate(double angle)
        {
            rotate(angle, 0, 0);
        }

        virtual void onTranslate(double x, double y) {}
        void translate(double x, double y)
        {
            // get current transform
            auto t = state().getTransform();
            t.translate(x, y);
            state().setTransform(t);
            //onTransform(t);

            // call onTranslate
            onTranslate(x, y);
        }


        // Manipulation of drawing state parameters
        virtual void onStrokeBeforeTransform(bool b) {}
        void strokeBeforeTransform(bool b) 
        {
            state().setStrokeBeforeTransform(b);
            onStrokeBeforeTransform(b);
        }
        
        virtual void onBlendMode(int mode) {}
        void blendMode(int mode) 
        { 
            state().setCompositeMode(mode);
            onBlendMode(mode);
        }
        
        virtual void onGlobalOpacity(double alpha) {}
        void globalOpacity(double opa) 
        { 
            state().setGlobalOpacity(opa);
            onGlobalOpacity(opa);
        }

        virtual void onStrokeCap() {}
        void strokeCap(BLStrokeCap kind, int position) 
        { 
            if (position == BLStrokeCapPosition::BL_STROKE_CAP_POSITION_START)
            {
                state().setStrokeStartCap(kind);
            }
            else {
                state().setStrokeEndCap(kind);
            }
            onStrokeCap();
        }

        virtual void onStrokeCaps(BLStrokeCap caps) {}

        void strokeCaps(BLStrokeCap caps) 
        { 
            state().setStrokeCaps(caps);
            onStrokeCaps(caps);
        }
        
        virtual void onStrokeWidth(double width) {}
        void strokeWidth(double width)
        {
            state().setStrokeWidth(width);
            onStrokeWidth(width);
        }

        virtual void onLineJoin(BLStrokeJoin kind) {}
        void lineJoin(BLStrokeJoin kind)
        {
            state().setLineJoin(kind);
            onLineJoin(kind);
        }

        virtual void onStrokeMiterLimit(double ml){}
        void strokeMiterLimit(double value) 
        { 
            state().setStrokeMiterLimit(value);
            onStrokeMiterLimit(value);
        }

        virtual void onDashArray() {}
        void dashArray(const std::vector <float> & dashes)
        {
            state().setStrokeDashArrayRaw(dashes);
            onDashArray();
        }

        virtual void onDashOffset(float offset) { (void)offset; }
        void dashOffset(const float offset)
        {
            state().setStrokeDashOffsetRaw(offset);
            onDashOffset(offset);
        }

        // paint for filling shapes
        void fillPaintServer(IServePaint *pServer)
        {
            state().setFillPaintServer(pServer);
        }

        virtual void onNoFill() {}
        void noFill() {
            onNoFill();
        }

        virtual void onApplyFill(BLVar) {}
        void applyFillPaint(BLVar paint)
        {
            onApplyFill(paint);
        }

        virtual void onFillOpacity(double o) {}
        void fillOpacity(double o)
        {
            state().setFillOpacity(o);
            onFillOpacity(o);
        }


        // Geometry
        virtual void onFillRule(BLFillRule rule) {}
        void fillRule(BLFillRule rule)
        {
            state().setFillRule(rule);
            onFillRule(rule);
        }

        // ---------------------------------------
        // paint for stroking lines
        // -------------------------------
        void strokePaintServer(IServePaint* pServer)
        {
            state().setStrokePaintServer(pServer);
        }

        virtual void onApplyStroke(BLVar ) {}

        void applyStrokePaint(BLVar paint)
        {
            onApplyStroke(paint);
        }

        virtual void onNoStroke() {}
        void noStroke() {
            //setStrokePaintServer(nullptr);
            onNoStroke();
        }

        virtual void onStrokeOpacity(double opa) {}
        void strokeOpacity(double o)
        {
            state().setStrokeOpacity(o);
            onStrokeOpacity(o);
        }



        // Set a background that will be used
        // to fill the canvas before any drawing
        virtual void onBackground() {}
        template <typename StyleT>
        void background(const StyleT& bg) noexcept
        {
            state().setBackgroundPaint(bg);
            onBackground();
        }


        // Typography
        virtual void onTextCursor() {}
        WGPointD textCursor() const { return state().getTextCursor(); }
        void textCursor(const WGPointD& cursor)
        {
            state().setTextCursor(cursor);
            onTextCursor();
        }

        // BUGBUG - this should become a part of the state management
        virtual void onFillMask() {}
        void setFillMask(BLImage& mask, const WGRectI& maskArea)
        {
            BLPointI origin(maskArea.x, maskArea.y);
            onFillMask();
        }

        // Clipping
        virtual void onClipRect(const WGRectD& cRect) {}
        void clipRect(const WGRectD& cRect)
        {
            state().setClipRect(cRect);
            onClipRect(cRect);
        }

        virtual void onNoClip() {}
        virtual void noClip()
        {
            state().setClipRect(WGRectD{});
            onNoClip();
        }

        // Path handling
        void paintOrder(uint8_t order)
        {
            state().setPaintOrder(order);
        }

        virtual void onBeginDrawShape(const BLPath& apath) {}
        void beginDrawShape(const BLPath& apath)
        {
            onBeginDrawShape(apath);
        }

        virtual void onEndDrawShape() {}
        void endDrawShape()
        {
            onEndDrawShape();
        }

        // Stroke shapes
        // based on the current drawing state
        virtual void onStrokeShape(const BLPath& aPath) {}
        void strokeShape(const BLPath& aPath)
        {
            onStrokeShape(aPath);
        }
        
        // Fill shapes
        // based on the current drawing state
        virtual void onFillShape(const BLPath& aPath) {}
        void fillShape(const BLPath& aPath)
        {
            onFillShape(aPath);
        }

        // Drawing Shapes
        // This is a general shape drawing.
        // It can handle the order of drawing, as well
        // as do isolated drawing (stroke, or fill only)
        virtual void onDrawShape(const BLPath& aPath, uint32_t porder) {}
        void drawShape(const BLPath &aPath)
        {
            // Get the paint order from the context
            uint32_t porder = state().getPaintOrder();

            onDrawShape(aPath, porder);
        }
        
        
        // Bitmap drawing
        virtual void onImage(const Surface& img, double x, double y)
        {
        }

        void image(const Surface& img, double x, double y)
        {
            onImage(img, x, y);
        }
        
        virtual void onScaleImage(const Surface& src,
            int srcX, int srcY, int srcWidth, int srcHeight,
            double dstX, double dstY, double dstWidth, double dstHeight)
        {
        }

        void scaleImage(const Surface& src,
            int srcX, int srcY, int srcWidth, int srcHeight,
            double dstX, double dstY, double dstWidth, double dstHeight) 
        {
            onScaleImage(src, srcX, srcY, srcWidth, srcHeight,
                dstX, dstY, dstWidth, dstHeight);
        }





        // Text Drawing
        virtual void onResetFont() {}

        void resetFont()
        {
            auto fh = FontHandler::getFontHandler();

            if (nullptr != fh)
            {
                BLFont aFont;
                auto success = fh->selectFont(state().getFontFamily(),
                    aFont, state().getFontSize(), state().getFontStyle(), 
                    state().getFontWeight(), state().getFontStretch());

                if (success)
                {
                    state().setFont(aFont);
                }
            }
        }

        // Font management attributes
        virtual void onFontFamily(const ByteSpan& family) {}
        virtual void onFontSize(double size) {}
        virtual void onFontStyle(BLFontStyle style) {}
        virtual void onFontWeight(BLFontWeight weight) {}
        virtual void onFontStretch(BLFontStretch stretch) {}

        void fontStretch(BLFontStretch stretch)
        {
            state().setFontStretch(stretch);
            onFontStretch(stretch);
        }

        void fontStyle(BLFontStyle style)
        {
            state().setFontStyle(style);
            onFontStyle(style);
        }
        
        void fontWeight(BLFontWeight weight)
        {
            state().setFontWeight(weight);
            onFontWeight(weight);
        }

        void fontSize(double size)
        {
            state().setFontSize(size);
            onFontSize(size);
        }

        void fontFamily(const ByteSpan& family)
        {
            state().setFontFamily(family);
            resetFont();
        }


        // Text drawing (glyph run)
        virtual void onFillGlyphRun(const BLFont& , const BLGlyphRun& , double x, double y) {}
        void fillGlyphRun(const BLFont& font, const BLGlyphRun& run, double x, double y)
        {
            onFillGlyphRun(font, run, x, y);
        }

        virtual void onStrokeGlyphRun(const BLFont& , const BLGlyphRun& , double x, double y) {}
        void strokeGlyphRun(const BLFont& font, const BLGlyphRun& run, double x, double y)
        {
            onStrokeGlyphRun(font, run, x, y);
        }


        // Text drawing
        void textAnchor(SVGAlignment anchor)
        {
            state().setTextAnchor(anchor);
        }

        virtual void onStrokeText(const ByteSpan& txt, double x, double y) {}
        void strokeText(const ByteSpan& txt, double x, double y) 
        {
            onStrokeText(txt, x, y);
        }
        
        virtual void onFillText(const ByteSpan& txt, double x, double y) {}
        void fillText(const ByteSpan& txt, double x, double y) 
        {
            onFillText(txt, x, y);
        }
        
        virtual void onDrawText(const ByteSpan& txt, double x, double y, uint32_t porder) {}
        void drawText(const ByteSpan& txt, double x, double y)
        {
            // Get the paint order from the context
            uint32_t porder = state().getPaintOrder();

            onDrawText(txt, x, y, porder);
        }






    };

    //using IDrawGraphics = IRenderSVG;
}

