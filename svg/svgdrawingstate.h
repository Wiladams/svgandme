#pragma once

#include <functional>


#include "blend2d.h"
#include "fonthandler.h"
#include "collections.h"
#include "svgenums.h"


namespace waavs {

	// Represents the current state of the SVG rendering context
    // this can be used by DOM walkers, as well as rendering context
    //
    struct SVGDrawingState {

        // Coordinate system
        BLRect fClipRect{};
        BLRect fViewport{};
        BLRect fObjectFrame{};

        // Paint
        BLVar fDefaultColor{};
        BLVar fFillPaint{};

        uint32_t fPaintOrder{ PaintOrderKind::SVG_PAINT_ORDER_NORMAL };

        // Stroking
        BLStrokeCap fBeginStrokeCap{ BLStrokeCap::BL_STROKE_CAP_BUTT };
        BLStrokeCap fEndStrokeCap{ BLStrokeCap::BL_STROKE_CAP_BUTT };
        double fStrokeWidth{ 1.0 };
        BLStrokeJoin fStrokeJoin{ BLStrokeJoin::BL_STROKE_JOIN_MITER_ROUND };
        
        BLVar fStrokePaint{};
        
        // Typography
        BLPoint fTextCursor{};
        SVGAlignment fTextHAlignment = SVGAlignment::SVG_ALIGNMENT_START;
        TXTALIGNMENT fTextVAlignment = BASELINE;

        // Fontography
        BLFont fFont{};
        ByteSpan fFamilyNames{ "Arial" };
        float fFontSize{ 16 };
        BLFontStyle fFontStyle = BL_FONT_STYLE_NORMAL;
        BLFontWeight fFontWeight = BL_FONT_WEIGHT_NORMAL;
        BLFontStretch fFontStretch = BL_FONT_STRETCH_NORMAL;


        SVGDrawingState()
        {
			fFillPaint = BLRgba32(0, 0, 0, 255);
            fStrokePaint = BLVar::null();
        }

        // Copy Constructor
        SVGDrawingState(const SVGDrawingState& other) noexcept
            : fFont(other.fFont)

        {
            this->operator=(other);
        }

        // Assignment operator
        virtual SVGDrawingState& operator=(const SVGDrawingState& other) noexcept
        {
            if (this == &other)
                return *this;


            fClipRect = other.fClipRect;
            fViewport = other.fViewport;
            fObjectFrame = other.fObjectFrame;

            fTextCursor = other.fTextCursor;

            fPaintOrder = other.fPaintOrder;
            fDefaultColor.assign(other.fDefaultColor);
            fFillPaint.assign(other.fFillPaint);
            

            fStrokePaint.assign(other.fStrokePaint);
			fStrokeWidth = other.fStrokeWidth;
            fBeginStrokeCap = other.fBeginStrokeCap;
            fEndStrokeCap = other.fEndStrokeCap;

            fTextHAlignment = other.fTextHAlignment;
            fTextVAlignment = other.fTextVAlignment;

            //fFontHandler = other.fFontHandler;
            fFont = other.fFont;

            fFamilyNames = other.fFamilyNames;
            fFontSize = other.fFontSize;
            fFontStyle = other.fFontStyle;
            fFontWeight = other.fFontWeight;
            fFontStretch = other.fFontStretch;


            return *this;
        }

        // Resetting the state
        virtual void resetFont() {}
        
        void reset()
        {
            fClipRect = BLRect{};
            fViewport = BLRect();
            fObjectFrame = BLRect();

            fDefaultColor = BLVar::null();
            fFillPaint = BLRgba32(0xff000000);
            fStrokePaint = BLVar::null();
            fPaintOrder = PaintOrderKind::SVG_PAINT_ORDER_NORMAL;

            fTextHAlignment = SVGAlignment::SVG_ALIGNMENT_START;
            fTextVAlignment = BASELINE;

            fFont.reset();
            fFamilyNames = "Arial";
            fFontSize = 16;
            fFontStyle = BL_FONT_STYLE_NORMAL;
            fFontWeight = BL_FONT_WEIGHT_NORMAL;
            fFontStretch = BL_FONT_STRETCH_NORMAL;

            resetFont();
        }


        /// <summary>
        ///  Set various attributes of the state
        /// </summary>
        /// <param name="r"></param>
        void setViewport(const BLRect& r) {
            fViewport = r;
        }
        BLRect viewport() const { return fViewport; }
        
        void objectFrame(const BLRect& r) {
            fObjectFrame = r;
        }
        BLRect objectFrame() const { return fObjectFrame; }

        
        const BLRect& getClipRect() const { return fClipRect; }
        virtual void clipRect(const BLRect& aRect) { fClipRect = aRect; }

        uint32_t paintOrder() const { return fPaintOrder; }
        virtual void paintOrder(const uint32_t order) { fPaintOrder = order; }
        
        const BLVar& defaultColor() const { return fDefaultColor; }
        void defaultColor(const BLVar& color) { fDefaultColor.assign(color); }

		const BLVar& fillPaint() const { return fFillPaint; }
		void fillPaint(const BLVar& paint) { fFillPaint.assign(paint); }

		const BLVar& strokePaint() const { return fStrokePaint; }
		void strokePaint(const BLVar& paint) { fStrokePaint.assign(paint); }
        
        

        virtual void strokeWidth(double sw) { 
            fStrokeWidth = sw; 
        }
        virtual double getStrokeWidth() const { return fStrokeWidth; }

		void lineJoin(BLStrokeJoin join) { fStrokeJoin = join; }
		BLStrokeJoin getLineJoin() const { return fStrokeJoin; }
        


        // Typography
        SVGAlignment textAnchor() const { return fTextHAlignment; }
        void textAnchor(SVGAlignment anchor)
        {
            fTextHAlignment = anchor;
        }

        TXTALIGNMENT textAlignment() const { return fTextVAlignment; }
        void textAlignment(TXTALIGNMENT align)
        {
            fTextVAlignment = align;
        }

        BLPoint textCursor() const { return fTextCursor; }
        void textCursor(const BLPoint& cursor) { fTextCursor = cursor; }


        // Fontography
        const BLFont& font() const { return fFont; }
        virtual void font(BLFont& afont) { fFont = afont; }
        
        void fontFamily(const ByteSpan& familyNames) noexcept
        {
            fFamilyNames = familyNames;
            resetFont();
        }




        double fontSize() const noexcept { return fFontSize; }
        void fontSize(float size) noexcept
        {
            fFontSize = size;
            resetFont();
        }

        void fontStyle(BLFontStyle style) noexcept
        {
            fFontStyle = style;
            resetFont();
        }

        void fontWeight(BLFontWeight weight) noexcept
        {
            fFontWeight = weight;
            resetFont();
        }

        void fontStretch(BLFontStretch stretch) noexcept
        {
            fFontStretch = stretch;
            resetFont();
        }

        // setAttribute()
        // Set a specific attribute of the drawing state
        // in some cases, we can use the loadChunk() call on the attribute
        // if it has one.
        bool setAttribute(const ByteSpan& attName, const ByteSpan& attValue)
        {

    
            return true;
        }


        virtual bool applyStateSelf(BLContext* ctx)
        {
            return true;
        }
        
        virtual bool applyState(BLContext* ctx)
        {
            // Fill attributes
            ctx->setFillStyle(fFillPaint);
            
            // stroke caps
            // stroke linejoin
			ctx->setStrokeStyle(fStrokePaint);
            ctx->setStrokeJoin(getLineJoin());
            ctx->setStrokeWidth(getStrokeWidth());

            // font



            // probably applying font characteristics
            // probably apply paint
            this->applyStateSelf(ctx);
            
            return true;
        }
        
    };
}