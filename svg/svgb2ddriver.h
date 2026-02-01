#pragma once

#pragma comment(lib, "blend2d.lib") // Link with Blend2D static library, on Windows

#include "irendersvg.h"


namespace waavs
{

    // A specialization of state management, connected to a BLContext
    // This is used when rendering a tree of SVG elements
    struct SVGB2DDriver : public IRenderSVG
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


        void onAttach(BLImageCore& image, const BLContextCreateInfo* createInfo) override
        {
            BLResult res = fDrawingContext->begin(image, createInfo);
            applyToContext(fDrawingContext.get());
        }



        void onDetach() override
        {
            fDrawingContext->end();
        }

        void onResetFont() override
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

        void onPush() override
        {
            fDrawingContext->save();
        }


        void onPop() override
        {
            fDrawingContext->restore();

            //applyToContext(fDrawingContext.get());
        }

        void onFlush() override
        {
            BLResult bResult = fDrawingContext->flush(BL_CONTEXT_FLUSH_SYNC);
            if (bResult != BL_SUCCESS)
            {
                printf("svgb2ddriver.flush(), ERROR: %d\n", bResult);
            }
        }


        // Canvas management
        void onClear() override
        {
			const BLVar &bgpaint = getBackgroundPaint();
			if (!bgpaint.isNull())
			{
				fDrawingContext->fillAll(bgpaint);
			}
			else
			{
				fDrawingContext->clearAll();
			}
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
            //setTransform(fDrawingContext->userTransform());
        }

        void onApplyTransform(const BLMatrix2D& value) override
        {
            fDrawingContext->applyTransform(value);
        }

        void onScale(double x, double y) override
        {
            fDrawingContext->scale(x, y);
        }

        void onTranslate(double x, double y) override
        {
            fDrawingContext->translate(x, y);
        }


        void onRotate(double angle, double cx, double cy) override
        {
            fDrawingContext->rotate(angle);
        }

        // Drawing attributes
        void onStrokeBeforeTransform() override
        {
            fDrawingContext->setStrokeTransformOrder(getStrokeBeforeTransform() ? BL_STROKE_TRANSFORM_ORDER_BEFORE : BL_STROKE_TRANSFORM_ORDER_AFTER);
        }

        void onBlendMode() override
        {
            fDrawingContext->setCompOp((BLCompOp)getCompositeMode());
        }

        void onGlobalOpacity() override
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
        void onBackground() override {}



        // Typography
        void onTextCursor() override {}


        // BUGBUG - this should become a part of the state management
        void onFillMask() override
        {
            //fDrawingContext->fillMask(origin, mask, maskArea);
        }

		//virtual void onFillMask(BLImage& mask, const BLRectI& maskArea) override
		//{
		//	BLPointI origin(maskArea.x, maskArea.y);
		//	fDrawingContext->fillMask(origin, mask, maskArea);
		//}


        // Clipping
        void onClipRect() override
        {
            fDrawingContext->clipToRect(getClipRect());
        }

        void onNoClip() override
        {
            fDrawingContext->restoreClipping();
        }

        void onBeginDrawShape(const BLPath& apath) override
        {
        }

        void onEndDrawShape() override
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
        void onDrawShape(const BLPath& aPath) override
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
        void onImage(const BLImage& img, double x, double y) override
        {
            fDrawingContext->blitImage(BLPoint(x, y), img);
        }

        void onScaleImage(const BLImage& src,
            int srcX, int srcY, int srcWidth, int srcHeight,
            double dstX, double dstY, double dstWidth, double dstHeight) override
        {
            BLRect dst{ dstX,dstY,dstWidth,dstHeight };
            BLRectI srcArea{ srcX,srcY,srcWidth,srcHeight };

            fDrawingContext->blitImage(dst, src, srcArea);
        }

        // example in your concrete BLContext-backed renderer:
        void onFillGlyphRun(const BLFont& font, const BLGlyphRun& run, double x, double y) override
        {
            fDrawingContext->fillGlyphRun(BLPoint(x, y), font, run);
        }

        void onStrokeGlyphRun(const BLFont& font, const BLGlyphRun& run, double x, double y) override
        {
            fDrawingContext->strokeGlyphRun(BLPoint(x, y), font, run);
        }

        // Text Drawing
        void onStrokeText(const ByteSpan& txt, double x, double y) override
        {
            fDrawingContext->strokeUtf8Text(BLPoint(x, y), getFont(), (char*)txt.data(), txt.size());
        }

        void onFillText(const ByteSpan& txt, double x, double y) override
        {
            fDrawingContext->fillUtf8Text(BLPoint(x, y), getFont(), (char*)txt.data(), txt.size());
        }

        void onDrawText(const ByteSpan& txt, double x, double y) override
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

