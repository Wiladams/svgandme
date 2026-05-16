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
        BLImage fTargetImage;  // the image we are currently drawing to.

    public:



        SVGB2DDriver()
        {
            // Create a context by default, so we have 
            // something initially, so we don't have to use
            // null checks everywhere
            fDrawingContext = std::make_unique<BLContext>();

            initState();
            //fDrawingContext->setCompOp(BLCompOp::BL_COMP_OP_SRC_OVER);
        }


        virtual ~SVGB2DDriver() = default;

        BLImage* currentTarget() const noexcept override
        {
            return fDrawingContext->target_image();
        }

        void onCopyDrawingState(const SVGDrawingState& state) override
        {
            IAccessSVGState stateAccessor{ const_cast<SVGDrawingState*>(&state) };
            stateAccessor.applyToContext(fDrawingContext.get());
        }

        void onAttach(Surface& surf, int threadCount, const SVGDrawingState *state) override
        {
            BLContextCreateInfo ctxInfo{};
            ctxInfo.thread_count = threadCount;
            fTargetImage.create_from_data((int)surf.info().width, (int)surf.info().height, BL_FORMAT_PRGB32, surf.info().data, surf.info().stride);

            BLResult res = fDrawingContext->begin(fTargetImage, ctxInfo);

            if (state)
                copyDrawingState(*state);
            else
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
        // Clear the canvas to be fully transparent
        void onClear() override
        {
            fDrawingContext->clear_all();
        }

        // Clear the canvas to the background style specified
        // by the user.
        void onClearToBackground() override
        {
            const BLVar &bgpaint = getBackgroundPaint();
            if (!bgpaint.is_null())
            {
            	fDrawingContext->fill_all(bgpaint);
            }
            else
            {
            	fDrawingContext->clear_all();
            }
        }


        // Coordinate system transformation
        void onTransform(const WGMatrix3x3& value) override
        {
            BLMatrix2D m = blMatrix_from_WGMatrix3x3(value);
            fDrawingContext->set_transform(m);
            WGMatrix3x3 wgM = wgMatrix_from_BLMatrix2D(fDrawingContext->user_transform());
            setTransform(wgM);
        }

        void onApplyTransform(const WGMatrix3x3& value) override
        {
            BLMatrix2D m = blMatrix_from_WGMatrix3x3(value);

            fDrawingContext->apply_transform(m);
            WGMatrix3x3 wgM = wgMatrix_from_BLMatrix2D(fDrawingContext->user_transform());

            setTransform(wgM);
        }

        void onScale(double x, double y) override
        {
            fDrawingContext->scale(x, y);
            WGMatrix3x3 wgM = wgMatrix_from_BLMatrix2D(fDrawingContext->user_transform());

            setTransform(wgM);
        }

        void onTranslate(double x, double y) override
        {
            fDrawingContext->translate(x, y);
            WGMatrix3x3 wgM = wgMatrix_from_BLMatrix2D(fDrawingContext->user_transform());

            setTransform(wgM);
        }


        void onRotate(double angle, double cx, double cy) override
        {
            fDrawingContext->rotate(angle);
            WGMatrix3x3 wgM = wgMatrix_from_BLMatrix2D(fDrawingContext->user_transform());

            setTransform(wgM);
        }

        // Drawing attributes
        void onStrokeBeforeTransform() override
        {
            fDrawingContext->set_stroke_transform_order(getStrokeBeforeTransform() ? BL_STROKE_TRANSFORM_ORDER_BEFORE : BL_STROKE_TRANSFORM_ORDER_AFTER);
        }

        void onBlendMode(int mode) override
        {
            BLCompOp compOp = (BLCompOp)mode;
            fDrawingContext->set_comp_op(compOp);
        }

        void onGlobalOpacity() override
        {
            fDrawingContext->set_global_alpha(getGlobalOpacity());
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
            fDrawingContext->set_stroke_caps((BLStrokeCap)caps);
        }

        void onStrokeWidth() override
        {
            fDrawingContext->set_stroke_width(getStrokeWidth());
        }

        void onLineJoin() override
        {
            fDrawingContext->set_stroke_join((BLStrokeJoin)getLineJoin());
        }

        void onStrokeMiterLimit() override
        {
            fDrawingContext->set_stroke_miter_limit(getStrokeMiterLimit());
        }



        // paint for filling shapes
        void onFill() override
        {
            if (!fDrawingContext)
                return;

            BLVar paint = getFillPaint();
            if (paint.is_null()) {
                fDrawingContext->disable_fill_style();
                return;
            }

            fDrawingContext->set_fill_style(paint);
        }


        void onNoFill() override
        {
            fDrawingContext->disable_fill_style();
        }


        void onFillOpacity() override
        {
            double opa = getFillOpacity();
            fDrawingContext->set_fill_alpha(opa);
        }

        // Geometry
        void onFillRule() override
        {
            fDrawingContext->set_fill_rule((BLFillRule)getFillRule());
        }


        // paint for stroking lines
        void onStroke() override
        {
            BLVar paint = getStrokePaint();

            if (paint.is_null()) {
                fDrawingContext->disable_stroke_style();
                return;
            }

            fDrawingContext->set_stroke_style(paint);

        }


        void onNoStroke() override
        {
            fDrawingContext->disable_stroke_style();
        }


        void onStrokeOpacity() override
        {
            fDrawingContext->set_stroke_alpha(getStrokeOpacity());
        }



        // Set a background that will be used
        // to fill the canvas before any drawing
        void onBackground() override 
        {
            
        }



        // Typography
        void onTextCursor() override {}


        // BUGBUG - this should become a part of the state management
        void onFillMask() override
        {
            //fDrawingContext->fillMask(origin, mask, maskArea);
        }

		//virtual void onFillMask(BLImage& mask, const WGRectI& maskArea) override
		//{
		//	BLPointI origin(maskArea.x, maskArea.y);
		//	fDrawingContext->fillMask(origin, mask, maskArea);
		//}


        // Clipping
        void onClipRect() override
        {
            WGRectD cRect = getClipRect();
            BLRect blr{ cRect.x, cRect.y, cRect.w, cRect.h };
            fDrawingContext->clip_to_rect(blr);
        }

        void onNoClip() override
        {
            fDrawingContext->restore_clipping();
        }

        void onBeginDrawShape(const BLPath& apath) override
        {
        }

        void onEndDrawShape() override
        {
        }

        void onStrokeShape(const BLPath &apath) override
        {
			fDrawingContext->stroke_path(apath);
        }
		
        void onFillShape(const BLPath& apath) override
		{
			fDrawingContext->fill_path(apath);
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
                    fDrawingContext->fill_path(aPath, getFillPaint());
                    break;

                case PaintOrderKind::SVG_PAINT_ORDER_STROKE:
                    fDrawingContext->stroke_path(aPath, getStrokePaint());
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
        void onImage(const Surface& surf, double x, double y) override
        {
            // bail out if there is no image data
            if (!surf.data())
                return;

            BLImage blImg = blImageFromSurface(surf);
            fDrawingContext->blit_image(BLPoint(x, y), blImg);
        }

        void onScaleImage(const Surface& surf,
            int srcX, int srcY, int srcWidth, int srcHeight,
            double dstX, double dstY, double dstWidth, double dstHeight) override
        {
            BLImage blImg = blImageFromSurface(surf);

            BLRect dst{ dstX,dstY,dstWidth,dstHeight };
            BLRectI srcArea{ srcX,srcY,srcWidth,srcHeight };

            fDrawingContext->blit_image(dst, blImg, srcArea);
        }

        // example in your concrete BLContext-backed renderer:
        void onFillGlyphRun(const BLFont& font, const BLGlyphRun& run, double x, double y) override
        {
            fDrawingContext->fill_glyph_run(BLPoint(x, y), font, run);
        }

        void onStrokeGlyphRun(const BLFont& font, const BLGlyphRun& run, double x, double y) override
        {
            fDrawingContext->stroke_glyph_run(BLPoint(x, y), font, run);
        }

        // Text Drawing
        void onStrokeText(const ByteSpan& txt, double x, double y) override
        {
            fDrawingContext->stroke_utf8_text(BLPoint(x, y), getFont(), (char*)txt.data(), txt.size());
        }

        void onFillText(const ByteSpan& txt, double x, double y) override
        {
            fDrawingContext->fill_utf8_text(BLPoint(x, y), getFont(), (char*)txt.data(), txt.size());
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

