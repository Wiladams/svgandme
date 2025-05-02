#pragma once

#pragma comment(lib, "blend2d.lib") // Link with Blend2D static library, on Windows

#include <functional>

#include "blend2d.h"
#include "fonthandler.h"
#include "collections.h"
#include "svgenums.h"
#include "imanagesvgstate.h"

namespace waavs
{
    
    // A specialization of state management, connected to a BLContext
    // This is used when rendering a tree of SVG elements
    struct IRenderSVG : public IAccessSVGState // , public BLContext
    {
        SVGDrawingState *fCurrentDrawingState;
        SVGStateStack fStateStack;

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
			fCurrentDrawingState = fStateStack.currentState();
            setDrawingState(fCurrentDrawingState);

            background(BLRgba32(0xFFFFFFFF));
            lineJoin(BL_STROKE_JOIN_MITER_CLIP);
            strokeMiterLimit(4);
            fillRule(BL_FILL_RULE_NON_ZERO);
            fill(BLRgba32(0, 0, 0));
            noStroke();
            strokeWidth(1.0);
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

            fStateStack.pop();
			setDrawingState(fStateStack.currentState());

            //applyToContext(fDrawingContext.get());
            onPop();
        }
        
        virtual void onFlush() {}
        void flush()
        {

            onFlush();

        }

        /*
        void resetState()
        {
            IManageSVGState::reset();
            
            resetTransform();
        }
        */

        // Canvas management
        virtual void onClear() {}
        void clear()
        {
            onClear();
        }

        // Call this before each frame to be drawn
        virtual void onRenew() {}
        void renew()
        {
            // Clear the canvas first
            //fDrawingContext->clearAll();

            // If a background paint is set, use it

            //if (!getBackgroundPaint().isNull())
            //{
            //    fDrawingContext->fillAll(getBackgroundPaint());
            //}

            //resetState();

            // Setup the default drawing state
            // to conform to what SVG expects
            blendMode(BL_COMP_OP_SRC_OVER);
            strokeMiterLimit(4.0);
            lineJoin(BL_STROKE_JOIN_MITER_CLIP);
            fillRule(BL_FILL_RULE_NON_ZERO);

            fill(BLRgba32(0, 0, 0));
            noStroke();
            strokeWidth(1.0);
            onRenew();
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

