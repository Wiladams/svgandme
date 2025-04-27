#pragma once

#pragma comment(lib, "blend2d.lib") // Link with Blend2D static library, on Windows

#include "irendersvg.h"


namespace waavs
{

    // A specialization of state management, connected to a BLContext
    // This is used when rendering a tree of SVG elements
    struct SVGB2DDriver : public IRenderSVG // , public BLContext
    {
        std::unique_ptr<BLContext> fDrawingContext;

    public:
        SVGB2DDriver()
        {
            // Create a context by default, so we have 
            // something initially, so we don't have to use
            // null checks everywhere
            fDrawingContext = std::make_unique<BLContext>();

            initState();
        }


        virtual ~SVGB2DDriver() = default;
        /*
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
        */

        virtual void onAttach(BLImageCore& image, const BLContextCreateInfo* createInfo)
        {
            BLResult res = fDrawingContext->begin(image, createInfo);
            applyToContext(fDrawingContext.get());
        }



        virtual void onDetach()
        {
            fDrawingContext->end();
        }

        virtual void onResetFont() 
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

        virtual void onPush() 
        {
            fDrawingContext->save();
        }


        virtual void onPop() 
        {
            fDrawingContext->restore();

            //applyToContext(fDrawingContext.get());
        }

        virtual void onFlush() 
        {
            BLResult bResult = fDrawingContext->flush(BL_CONTEXT_FLUSH_SYNC);
            if (bResult != BL_SUCCESS)
            {
                printf("svgb2ddriver.flush(), ERROR: %d\n", bResult);
            }
        }


        /*
        void resetState()
        {
            IManageSVGState::reset();

            resetTransform();
        }
        */

        // Canvas management
        void onClear() override
        {
            fDrawingContext->clearAll();
        }


        // Call this before each frame to be drawn
        void onRenew() override
        {
            // Clear the canvas first
            fDrawingContext->clearAll();

            // If a background paint is set, use it
            if (!getBackgroundPaint().isNull())
            {
                fDrawingContext->fillAll(getBackgroundPaint());
            }

        }


        // Coordinate system transformation
        void onTransform(const BLMatrix2D& value) override
        {
            fDrawingContext->setTransform(value);
            setTransform(fDrawingContext->userTransform());
        }

        void onApplyTransform(const BLMatrix2D& value) override
        {
            fDrawingContext->applyTransform(value);
            setTransform(fDrawingContext->userTransform());
        }

        void onScale(double x, double y) override
        {
            fDrawingContext->scale(x, y);
            //setTransform(fDrawingContext->userTransform());
        }

        void onTranslate(double x, double y) override
        {
            fDrawingContext->translate(x, y);
            //setTransform(fDrawingContext->userTransform());
        }


        void onRotate(double angle, double cx, double cy) override
        {
            fDrawingContext->rotate(angle);
            //setTransform(fDrawingContext->userTransform());
        }

        // Drawing attributes
        virtual void onStrokeBeforeTransform() 
        {
            fDrawingContext->setStrokeTransformOrder(getStrokeBeforeTransform() ? BL_STROKE_TRANSFORM_ORDER_BEFORE : BL_STROKE_TRANSFORM_ORDER_AFTER);
        }

        virtual void onBlendMode() 
        {
            fDrawingContext->setCompOp((BLCompOp)getCompositeMode());
        }

        virtual void onGlobalOpacity() 
        {
            fDrawingContext->setGlobalAlpha(getGlobalOpacity());
        }

        /*
        virtual void onStrokeCap() 
        {
            if (position == BLStrokeCapPosition::BL_STROKE_CAP_POSITION_START)
            {
                setStrokeStartCap(kind);
                fDrawingContext->setStrokeCap(BLStrokeCapPosition::BL_STROKE_CAP_POSITION_START, kind);
            }
            else {
                setStrokeEndCap(kind);
                fDrawingContext->setStrokeCap(BLStrokeCapPosition::BL_STROKE_CAP_POSITION_END, kind);
            }

        }
        */

        void onStrokeCaps(BLStrokeCap caps) override
        {
            fDrawingContext->setStrokeCaps((BLStrokeCap)caps);
        }

        void onStrokeWidth() override
        {
            fDrawingContext->setStrokeWidth(getStrokeWidth());
        }

        void onLineJoin() override
        {
            fDrawingContext->setStrokeJoin((BLStrokeJoin)getLineJoin());
        }

        void onStrokeMiterLimit() override
        {
            fDrawingContext->setStrokeMiterLimit(getStrokeMiterLimit());
        }



        // paint for filling shapes
        void onFill() override
        {
            fDrawingContext->setFillStyle(getFillPaint());
        }


        void onNoFill() override
        {
            fDrawingContext->disableFillStyle();
        }


        void onFillOpacity() override
        {
            fDrawingContext->setFillAlpha(getFillOpacity());
        }

        // Geometry
        void onFillRule() override
        {
            fDrawingContext->setFillRule((BLFillRule)getFillRule());
        }


        // paint for stroking lines
        void onStroke() override
        {
            fDrawingContext->setStrokeStyle(getStrokePaint());
        }


        void onNoStroke() override
        {
            fDrawingContext->disableStrokeStyle();
        }


        void onStrokeOpacity() override
        {
            fDrawingContext->setStrokeAlpha(getStrokeOpacity());
        }



        // Set a background that will be used
        // to fill the canvas before any drawing
        virtual void onBackground() {}



        // Typography
        virtual void onTextCursor() {}


        // BUGBUG - this should become a part of the state management
        virtual void onFillMask() 
        {
            //fDrawingContext->fillMask(origin, mask, maskArea);
        }

        virtual void onFillMask(BLImage& mask, const BLRectI& maskArea)
        {

        }

        // Clipping
        virtual void onClipRect() 
        {
            fDrawingContext->clipToRect(getClipRect());
        }

        virtual void onNoClip() 
        {
            fDrawingContext->restoreClipping();
        }

        void onBeginShape(const BLPath& apath) override
        {
        }

        void onEndShape() override
        {
        }

        void onStrokeShape(const BLPath &apath) override
        {
			fDrawingContext->strokePath(apath);
        }
		
        void onFillShape(const BLPath& apath) override
		{
			fDrawingContext->fillPath(apath);
		}

        // Drawing Shapes
        // This is a general shape drawing.
        // It can handle the order of drawing, as well
        // as do isolated drawing (stroke, or fill only)
        virtual void onDrawShape(const BLPath& aPath)
        {
            // Get the paint order from the context
            uint32_t porder = getPaintOrder();

            for (int slot = 0; slot < 3; slot++)
            {
                uint32_t ins = porder & 0x03;	// get two lowest bits, which are a single instruction

                switch (ins)
                {
                case PaintOrderKind::SVG_PAINT_ORDER_FILL:
                    fDrawingContext->fillPath(aPath, getFillPaint());
                    break;

                case PaintOrderKind::SVG_PAINT_ORDER_STROKE:
                    fDrawingContext->strokePath(aPath, getStrokePaint());
                    break;

                case PaintOrderKind::SVG_PAINT_ORDER_MARKERS:
                {
                    // We don't do markers at this level
                }
                break;
                }

                // move past current instruction, 
                // shift down to get the next one ready
                porder = porder >> 2;
            }
        }


        // Bitmap drawing
        virtual void onImage(const BLImage& img, double x, double y)
        {
            fDrawingContext->blitImage(BLPoint(x, y), img);
        }

        virtual void onScaleImage(const BLImage& src,
            int srcX, int srcY, int srcWidth, int srcHeight,
            double dstX, double dstY, double dstWidth, double dstHeight)
        {
            BLRect dst{ dstX,dstY,dstWidth,dstHeight };
            BLRectI srcArea{ srcX,srcY,srcWidth,srcHeight };

            fDrawingContext->blitImage(dst, src, srcArea);
        }


        // Text Drawing
        virtual void onStrokeText(const ByteSpan& txt, double x, double y)
        {
            fDrawingContext->strokeUtf8Text(BLPoint(x, y), getFont(), (char*)txt.data(), txt.size());
        }

        virtual void onFillText(const ByteSpan& txt, double x, double y)
        {
            fDrawingContext->fillUtf8Text(BLPoint(x, y), getFont(), (char*)txt.data(), txt.size());
        }

        virtual void onDrawText(const ByteSpan& txt, double x, double y) 
        {
            // Get the paint order from the context
            uint32_t porder = getPaintOrder();

            for (int slot = 0; slot < 3; slot++)
            {
                uint32_t ins = porder & 0x03;	// get two lowest bits, which are a single instruction

                switch (ins)
                {
                case PaintOrderKind::SVG_PAINT_ORDER_FILL:
                    onFillText(txt, x, y);
                    break;

                case PaintOrderKind::SVG_PAINT_ORDER_STROKE:
                    onStrokeText(txt, x, y);
                    break;

                case PaintOrderKind::SVG_PAINT_ORDER_MARKERS:
                {
                    // We don't do markers at this level
                }
                break;
                }

                // move past current instruction, 
                // shift down to get the next one ready
                porder = porder >> 2;
            }
        }

    };

}

