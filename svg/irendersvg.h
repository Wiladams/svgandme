#pragma once

#pragma comment(lib, "blend2d.lib") // Link with Blend2D static library, on Windows

#include <functional>

#include "blend2d.h"

#include "fonthandler.h"
#include "svgenums.h"
#include "svgdrawingstate.h"
#include "imanagesvgstate.h"


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
    struct IRenderSVG : public IAccessSVGState // , public BLContext
    {
        SVGDrawingState *fCurrentDrawingState;
        SVGStateStack fStateStack;
        
        // Text position management
        std::vector<waavs::SVGTextPosFrame> fTextPosStack;


    public:
        IRenderSVG()
        {
            // Create a context by default, so we have 
            // something initially, so we don't have to use
            // null checks everywhere

            initState();
        }

        
        virtual ~IRenderSVG() = default;

        void initState()
        {
            fTextPosStack.clear();
            fTextPosStack.reserve(8);   // reserve some space for text position frames

            fStateStack.reset();
			fCurrentDrawingState = fStateStack.currentState();
            setDrawingState(fCurrentDrawingState);

            background(BLRgba32(0x00000000));
            lineJoin(BL_STROKE_JOIN_MITER_CLIP);
            strokeMiterLimit(4);
            fillRule(BL_FILL_RULE_NON_ZERO);
            fill(BLRgba32(0, 0, 0));
            noStroke();
            strokeWidth(1.0);

            // BUGBUG - Need to set font for those who are getting default
            // font information from the context
            //setFontFamily("sans-serif");
            resetFont();
        }
        
        virtual void onAttach(BLImageCore& image, const BLContextCreateInfo* createInfo)
        {}

        void attach(BLImageCore& image, const BLContextCreateInfo* createInfo = nullptr) noexcept
        {
            onAttach(image, createInfo);
        }
        
        virtual void onDetach() {}

        void detach()
        {
            flush();
            onDetach();
        }
        

        

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






        virtual void onPush() {}
        void push()  
        {
            //printf("IRenderSVG.push()\n");
            fStateStack.push();
            onPush();
        }

        virtual void onPop() {}

        void pop()
        {
            //printf("IRenderSVG.pop()\n");
            const BLPoint tCursor = textCursor();

            fStateStack.pop();
			setDrawingState(fStateStack.currentState());

            
            // Restore progressed cursor (do NOT rewind)
            setTextCursor(tCursor);

            //applyToContext(fDrawingContext.get());
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




        // Coordinate system transformation management
        // The difference between transform(), and applyTransform() is
        // transform() - will set the transformation absolutely
        // applyTransform() - will add whatever transform is supplied to 
        //   the existing transform
        virtual void onTransform(const BLMatrix2D& value) {}
        void transform(const BLMatrix2D& value)
        {
            setTransform(value);
            onTransform(value);
        }

        virtual void onApplyTransform(const BLMatrix2D& value) {}
        void applyTransform(const BLMatrix2D& value)
        {
            onApplyTransform(value);
        }

        virtual void onScale(double sx, double sy) {}
		void scale(double x, double y)
		{
            auto t = getTransform();
            t.scale(x, y);
			setTransform(t);

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
            auto t = getTransform();
            t.rotate(angle, cx, cy);

            setTransform(t);

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
            auto t = getTransform();
            t.translate(x, y);

            // apply the translation to it
            setTransform(t);

            // call onTranslate
            onTranslate(x, y);
        }

        void translate(const BLPoint& pt)
        {
            translate(pt.x, pt.y);
        }


        // Manipulation of drawing state parameters
        virtual void onStrokeBeforeTransform() {}
        void strokeBeforeTransform(bool b) 
        {
            setStrokeBeforeTransform(b);
            onStrokeBeforeTransform();
        }
        
        virtual void onBlendMode() {}
        void blendMode(int mode) 
        { 
			setCompositeMode(mode);
            onBlendMode();
        }
        
        virtual void onGlobalOpacity() {}
        void globalOpacity(double opacity) 
        { 
            setGlobalOpacity(opacity);
            onGlobalOpacity();
        }

        virtual void onStrokeCap() {}
        void strokeCap(BLStrokeCap kind, int position) 
        { 
            if (position == BLStrokeCapPosition::BL_STROKE_CAP_POSITION_START)
            {
                setStrokeStartCap(kind);
            }
            else {
                setStrokeEndCap(kind);
            }
            onStrokeCap();
        }

        virtual void onStrokeCaps(BLStrokeCap caps) {}

        void strokeCaps(BLStrokeCap caps) 
        { 
            setStrokeCaps(caps);
            onStrokeCaps(caps);
        }
        
        virtual void onStrokeWidth() {}
        void strokeWidth(double width)
        {
            setStrokeWidth(width);
            onStrokeWidth();
        }

        virtual void onLineJoin() {}
        void lineJoin(BLStrokeJoin kind)
        {
            setLineJoin(kind);
            onLineJoin();
        }

        virtual void onStrokeMiterLimit(){}
        void strokeMiterLimit(double value) 
        { 
            setStrokeMiterLimit(value);
            onStrokeMiterLimit();
        }

        virtual void onDashArray() {}
        void dashArray(const std::vector<SVGLengthValue> &dashes)
        {
            setStrokeDashArrayRaw(dashes);
            onDashArray();
        }

        virtual void onDashOffset() {}
        void dashOffset(const SVGLengthValue &offset)
        {
            setStrokeDashOffsetRaw(offset);
            onDashOffset();
        }

        // paint for filling shapes
        virtual void onFill() {}
        template <typename StyleT>
        void fill(const StyleT& paint)
        {
            setFillPaint(paint);
            onFill();
        }

        virtual void onNoFill() {}
        void noFill() {
            onNoFill();
        }


        virtual void onFillOpacity() {}
        void fillOpacity(double o)
        {
            setFillOpacity(o);
            onFillOpacity();
        }



        // Geometry
        virtual void onFillRule() {}
        void fillRule(BLFillRule rule)
        {
            setFillRule(rule);
            onFillRule();
        }

        // paint for stroking lines
        virtual void onStroke() {}
        template <typename StyleT>
        void stroke(const StyleT& paint)
        {
            setStrokePaint(paint);
            onStroke();
        }

        virtual void onNoStroke() {}
        void noStroke() {
            setStrokePaint(BLVar::null());
            onNoStroke();
        }

        virtual void onStrokeOpacity() {}
        void strokeOpacity(double o)
        {
            setStrokeOpacity(o);
            onStrokeOpacity();
        }



        // Set a background that will be used
        // to fill the canvas before any drawing
        virtual void onBackground() {}
        template <typename StyleT>
        void background(const StyleT& bg) noexcept
        {
            setBackgroundPaint(bg);
            onBackground();
        }


        // Typography
        virtual void onTextCursor() {}
        BLPoint textCursor() const { return getTextCursor(); }
        void textCursor(const BLPoint& cursor)
        {
            setTextCursor(cursor);
            onTextCursor();
        }

        // BUGBUG - this should become a part of the state management
        virtual void onFillMask() {}
        void setFillMask(BLImage& mask, const BLRectI& maskArea)
        {
            BLPointI origin(maskArea.x, maskArea.y);
            onFillMask();
        }

        // Clipping
        virtual void onClipRect() {}
        void clipRect(const BLRect& cRect)
        {
            setClipRect(cRect);
            onClipRect();
        }

        virtual void onNoClip() {}
        virtual void noClip()
        {
            setClipRect(BLRect());
            onNoClip();
        }

        // Path handling
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
        virtual void onDrawShape(const BLPath& aPath) {}
        void drawShape(const BLPath &aPath)
        {
            onDrawShape(aPath);
        }
        
        
        // Bitmap drawing
        virtual void onImage(const BLImage& img, double x, double y)
        {
        }

        void image(const BLImage& img, double x, double y)
        {
            onImage(img, x, y);
        }
        
        virtual void onScaleImage(const BLImage& src,
            int srcX, int srcY, int srcWidth, int srcHeight,
            double dstX, double dstY, double dstWidth, double dstHeight)
        {
        }

        void scaleImage(const BLImage& src,
            int srcX, int srcY, int srcWidth, int srcHeight,
            double dstX, double dstY, double dstWidth, double dstHeight) 
        {
            onScaleImage(src, srcX, srcY, srcWidth, srcHeight,
                dstX, dstY, dstWidth, dstHeight);
        }





        // Text Drawing
        virtual void onResetFont() {}

        void resetFont() override
        {
            auto fh = FontHandler::getFontHandler();

            if (nullptr != fh)
            {
                BLFont aFont;
                auto success = fh->selectFont(getFontFamily(),
                    aFont, getFontSize(), getFontStyle(), getFontWeight(), getFontStretch());

                if (success)
                {
                    setFont(aFont);
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
			setFontStretch(stretch);
			onFontStretch(stretch);
		}

		void fontStyle(BLFontStyle style)
		{
			setFontStyle(style);
			onFontStyle(style);
		}
		
        void fontWeight(BLFontWeight weight)
		{
			setFontWeight(weight);
			onFontWeight(weight);
		}

		void fontSize(double size)
		{
			setFontSize(size);
			onFontSize(size);
		}

		void fontFamily(const ByteSpan& family)
		{
			setFontFamily(family);
			onResetFont();
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
        
        virtual void onDrawText(const ByteSpan& txt, double x, double y) {}
        void drawText(const ByteSpan& txt, double x, double y)
        {
            onDrawText(txt, x, y);
        }






    };

}

