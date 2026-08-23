#pragma once

#pragma comment(lib, "blend2d.lib") // Link with Blend2D static library, on Windows

#include "graphics_draw.h"


namespace waavs
{

    // A specialization of state management, connected to a BLContext
    // This is used when rendering a tree of SVG elements
    struct SVGB2DDriver : public IDrawGraphics
    {
        BLContext fCtx;
        BLImage fTargetImage;  // the image we are currently drawing to.

    public:



        SVGB2DDriver()
        {
            initState();
            fCtx.set_comp_op(BLCompOp::BL_COMP_OP_SRC_OVER);
        }


        virtual ~SVGB2DDriver() = default;

        BLImage* currentTarget() const noexcept override
        {
            return fCtx.target_image();
        }

//        void onApplyDrawingState(const SVGDrawingState& st) override

        void onCopyDrawingState(const SVGDrawingState& st) override
        {
            // clear the clipping state
            fCtx.restore_clipping();

            WGRectD cRect = state().getClipRect();
            if ((cRect.w > 0) && (cRect.h > 0))
            {
                fCtx.clip_to_rect(BLRect{ 
                    cRect.x, 
                    cRect.y, 
                    cRect.w, 
                    cRect.h });
            }

            fCtx.set_comp_op((BLCompOp)st.fCompositeMode);
            fCtx.set_fill_rule((BLFillRule)st.fFillRule);
            fCtx.set_transform(blMatrix_from_WGMatrix3x3(st.fTransform));
            fCtx.set_stroke_options(st.fStrokeOptions);

            // Paints
            if (st.fFillPaintServer)
                fCtx.set_fill_style(st.fFillPaintServer->getVariant(this, nullptr));
            //else
            //    fCtx.disable_fill_style();

            if (st.fStrokePaintServer)
                fCtx.set_stroke_style(st.fStrokePaintServer->getVariant(this, nullptr));
            //else
            //    fCtx.disable_stroke_style();

        }


        void onAttach(Surface& surf, int threadCount, const SVGDrawingState *state) override
        {
            BLContextCreateInfo ctxInfo{};
            ctxInfo.thread_count = threadCount;
            fTargetImage.create_from_data((int)surf.info().width, (int)surf.info().height, BL_FORMAT_PRGB32, surf.info().data, surf.info().stride);

            BLResult res = fCtx.begin(fTargetImage, ctxInfo);

            if (state)
                copyDrawingState(*state);
            //else
            //    applyToContext(fDrawingContext.get());
        }



        void onDetach() override
        {
            fCtx.end();
        }

        void onResetFont() override
        {
            auto fh = FontHandler::getFontHandler();

            if (nullptr != fh)
            {
                BLFont aFont;
                auto success = fh->selectFont(state().getFontFamily(),
                    aFont, 
                    state().getFontSize(), 
                    state().getFontStyle(), 
                    state().getFontWeight(), 
                    state().getFontStretch());

                if (success)
                {
                    state().setFont(aFont);
                }
            }
        }

        void onPush() override
        {
            fCtx.save();
        }

        void onPop() override
        {
            fCtx.restore();
        }



        void onFlush() override
        {
            BLResult bResult = fCtx.flush(BL_CONTEXT_FLUSH_SYNC);
            if (bResult != BL_SUCCESS)
            {
                printf("svgb2ddriver.flush(), ERROR: %d\n", bResult);
            }
        }


        // Canvas management
        // Clear the canvas to be fully transparent
        void onClear() override
        {
            fCtx.clear_all();
        }

        // Clear the canvas to the background style specified
        // by the user.
        void onClearToBackground() override
        {
            const BLVar &bgpaint = state().getBackgroundPaint();
            if (!bgpaint.is_null())
            {
            	fCtx.fill_all(bgpaint);
            }
            else
            {
            	fCtx.clear_all();
            }
        }


        // Coordinate system transformation
        void onSetTransform(const WGMatrix3x3& value) override
        {
            //WGMatrix3x3 wgM = wgMatrix_from_BLMatrix2D(fCtx.user_transform());
            BLMatrix2D m = blMatrix_from_WGMatrix3x3(value);
            fCtx.set_transform(m);
        }

        
        void onApplyTransform(const WGMatrix3x3& value) override
        {
            BLMatrix2D m = blMatrix_from_WGMatrix3x3(value);

            fCtx.apply_transform(m);

            //WGMatrix3x3 wgM = wgMatrix_from_BLMatrix2D(fCtx.user_transform());
            //state().setTransform(wgM);
        }

        void onScale(double x, double y) override
        {
            fCtx.scale(x, y);

            //WGMatrix3x3 wgM = wgMatrix_from_BLMatrix2D(fCtx.user_transform());
            //state().setTransform(wgM);
        }

        void onTranslate(double x, double y) override
        {
            fCtx.translate(x, y);
            
            //WGMatrix3x3 wgM = wgMatrix_from_BLMatrix2D(fCtx.user_transform());
            //state().setTransform(wgM);
        }


        void onRotate(double angle, double cx, double cy) override
        {
            fCtx.rotate(angle, cx, cy);
            
            //WGMatrix3x3 wgM = wgMatrix_from_BLMatrix2D(fCtx.user_transform());
            //state().setTransform(wgM);
        }
        

        // Drawing attributes
        void onStrokeBeforeTransform(bool b) override
        {
            fCtx.set_stroke_transform_order(b ? 
                BL_STROKE_TRANSFORM_ORDER_BEFORE : 
                BL_STROKE_TRANSFORM_ORDER_AFTER);
        }

        void onBlendMode(int mode) override
        {
            BLCompOp compOp = (BLCompOp)mode;
            fCtx.set_comp_op(compOp);
        }

        void onGlobalOpacity(double alpha) override
        {
            fCtx.set_global_alpha(alpha);
        }

        /*
        virtual void onStrokeCap() 
        {
            if (position == BLStrokeCapPosition::BL_STROKE_CAP_POSITION_START)
            {
                setStrokeStartCap(kind);
                fCtx.setStrokeCap(BLStrokeCapPosition::BL_STROKE_CAP_POSITION_START, kind);
            }
            else {
                setStrokeEndCap(kind);
                fCtx.setStrokeCap(BLStrokeCapPosition::BL_STROKE_CAP_POSITION_END, kind);
            }

        }
        */

        void onStrokeCaps(BLStrokeCap caps) override
        {
            fCtx.set_stroke_caps((BLStrokeCap)caps);
        }

        void onStrokeWidth(double width) override
        {
            fCtx.set_stroke_width(width);
        }

        void onLineJoin(BLStrokeJoin kind) override
        {
            fCtx.set_stroke_join((BLStrokeJoin)kind);
        }

        void onStrokeMiterLimit(double ml) override
        {
            fCtx.set_stroke_miter_limit(ml);
        }

        // paint for filling shapes
        void onApplyFill(BLVar paint) override
        {
            //if (!fDrawingContext)
            //    return;
            
            //BLVar paint = getFillPaint();
            //if (paint.is_null()) {
            //    fCtx.disable_fill_style();
            //    return;
            //}

            fCtx.set_fill_style(paint);
        }


        void onNoFill() override
        {
            //fCtx.set_fill_style(SVGDrawingState::emptyPaintServer()->getVariant(nullptr,nullptr));
            fCtx.disable_fill_style();
        }


        void onFillOpacity(double opa) override
        {
            fCtx.set_fill_alpha(opa);
        }

        // Geometry
        void onFillRule(BLFillRule rule) override
        {
            fCtx.set_fill_rule(rule);
        }


        // paint for stroking lines
        // BUGBUG - this should not actually do anything
        // as it's being called when setting up a paintServer
        void onApplyStroke(BLVar strokePaint) override
        {
            fCtx.set_stroke_style(strokePaint);
        }


        void onNoStroke() override
        {
            //fCtx.set_stroke_style(SVGDrawingState::emptyPaintServer()->getVariant(nullptr, nullptr));
            fCtx.disable_stroke_style();
        }


        void onStrokeOpacity(double opa) override
        {
            fCtx.set_stroke_alpha(opa);
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
            //fCtx.fillMask(origin, mask, maskArea);
        }

		//virtual void onFillMask(BLImage& mask, const WGRectI& maskArea) override
		//{
		//	BLPointI origin(maskArea.x, maskArea.y);
		//	fCtx.fillMask(origin, mask, maskArea);
		//}


        // Clipping
        void onClipRect(const WGRectD &cRect) override
        {
            BLRect blr{ cRect.x, cRect.y, cRect.w, cRect.h };
            fCtx.clip_to_rect(blr);
        }

        void onNoClip() override
        {
            fCtx.restore_clipping();
        }

        void onBeginDrawShape(const BLPath& apath) override
        {
        }

        void onEndDrawShape() override
        {
        }

        void onStrokeShape(const BLPath &apath) override
        {
			fCtx.stroke_path(apath);
        }
		
        void onFillShape(const BLPath& apath) override
		{
			fCtx.fill_path(apath);
		}

        // Drawing Shapes
        // This is a general shape drawing.
        // It can handle the order of drawing, as well
        // as do isolated drawing (stroke, or fill only)
        void onDrawShape(const BLPath& aPath, uint32_t porder) override
        {

            for (int slot = 0; slot < 3; slot++)
            {
                uint32_t ins = porder & 0x03;	// get two lowest bits, which are a single instruction

                switch (ins)
                {
                case PaintOrderKind::SVG_PAINT_ORDER_FILL:
                    fCtx.fill_path(aPath); // , getFillPaint());
                    break;

                case PaintOrderKind::SVG_PAINT_ORDER_STROKE:
                    fCtx.stroke_path(aPath); // , getStrokePaint());
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
            fCtx.blit_image(BLPoint(x, y), blImg);
        }

        void onScaleImage(const Surface& surf,
            int srcX, int srcY, int srcWidth, int srcHeight,
            double dstX, double dstY, double dstWidth, double dstHeight) override
        {
            BLImage blImg = blImageFromSurface(surf);

            BLRect dst{ dstX,dstY,dstWidth,dstHeight };
            BLRectI srcArea{ srcX,srcY,srcWidth,srcHeight };

            fCtx.blit_image(dst, blImg, srcArea);
        }

        // example in your concrete BLContext-backed renderer:
        void onFillGlyphRun(const BLFont& font, const BLGlyphRun& run, double x, double y) override
        {
            fCtx.fill_glyph_run(BLPoint(x, y), font, run);
        }

        void onStrokeGlyphRun(const BLFont& font, const BLGlyphRun& run, double x, double y) override
        {
            fCtx.stroke_glyph_run(BLPoint(x, y), font, run);
        }

        // Text Drawing
        void onStrokeText(const ByteSpan& txt, double x, double y) override
        {
            fCtx.stroke_utf8_text(BLPoint(x, y), state().getFont(), (char*)txt.data(), txt.size());
        }

        void onFillText(const ByteSpan& txt, double x, double y) override
        {
            fCtx.fill_utf8_text(BLPoint(x, y), state().getFont(), (char*)txt.data(), txt.size());
        }

        void onDrawText(const ByteSpan& txt, double x, double y, uint32_t porder) override
        {

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

