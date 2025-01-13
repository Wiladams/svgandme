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
        BLVar fDefaultColor;
        BLVar fFillPaint;
        BLVar fStrokePaint;
        uint32_t fPaintOrder{ PaintOrderKind::SVG_PAINT_ORDER_NORMAL };

        // Typography
        BLPoint fTextCursor{};
        TXTALIGNMENT fTextHAlignment = LEFT;
        TXTALIGNMENT fTextVAlignment = BASELINE;

        // Fontography
        FontHandler* fFontHandler{ nullptr };
        BLFont fFont{};
        ByteSpan fFamilyNames{ "Arial" };
        float fFontSize{ 16 };
        BLFontStyle fFontStyle = BL_FONT_STYLE_NORMAL;
        BLFontWeight fFontWeight = BL_FONT_WEIGHT_NORMAL;
        BLFontStretch fFontStretch = BL_FONT_STRETCH_NORMAL;


        SVGDrawingState() = default;

        // Copy Constructor
        SVGDrawingState(const SVGDrawingState& other) noexcept
            : fFont(other.fFont)

        {
            fClipRect = other.fClipRect;
            fViewport = other.fViewport;
            fObjectFrame = other.fObjectFrame;

            fTextCursor = other.fTextCursor;

            fPaintOrder = other.fPaintOrder;
            fDefaultColor.assign(other.fDefaultColor);
            fFillPaint.assign(other.fFillPaint);
            fStrokePaint.assign(other.fStrokePaint);
            


            fTextHAlignment = other.fTextHAlignment;
            fTextVAlignment = other.fTextVAlignment;

            fFontHandler = other.fFontHandler;
            fFont = other.fFont;

            fFamilyNames = other.fFamilyNames;
            fFontSize = other.fFontSize;
            fFontStyle = other.fFontStyle;
            fFontWeight = other.fFontWeight;
            fFontStretch = other.fFontStretch;
        }

        // Assignment operator
        SVGDrawingState& operator=(const SVGDrawingState& other) noexcept
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


            fTextHAlignment = other.fTextHAlignment;
            fTextVAlignment = other.fTextVAlignment;

            fFontHandler = other.fFontHandler;
            fFont = other.fFont;

            fFamilyNames = other.fFamilyNames;
            fFontSize = other.fFontSize;
            fFontStyle = other.fFontStyle;
            fFontWeight = other.fFontWeight;
            fFontStretch = other.fFontStretch;


            return *this;
        }

        void reset()
        {
            fClipRect = BLRect{};
            fViewport = BLRect();
            fObjectFrame = BLRect();

            fDefaultColor = BLVar::null();
            fFillPaint = BLRgba32(0xff000000);
            fStrokePaint = BLVar::null();
            fPaintOrder = PaintOrderKind::SVG_PAINT_ORDER_NORMAL;

            fTextHAlignment = LEFT;
            fTextVAlignment = BASELINE;

            fFamilyNames = "Arial";
            fFontSize = 16;
            fFontStyle = BL_FONT_STYLE_NORMAL;
            fFontWeight = BL_FONT_WEIGHT_NORMAL;
            fFontStretch = BL_FONT_STRETCH_NORMAL;

            resetFont();
        }

        void resetFont()
        {
            if (nullptr != fFontHandler)
            {
                BLFont aFont;
                if (fFontHandler->selectFont(fFamilyNames, aFont, fFontSize, fFontStyle, fFontWeight, fFontStretch))
                    fFont = aFont;
            }

        }

        const BLRect& clipRect() const { return fClipRect; }
        void setClipRect(const BLRect& aRect) { fClipRect = aRect; }

        uint32_t paintOrder() const { return fPaintOrder; }
        void paintOrder(const uint32_t order) { fPaintOrder = order; }
        
        const BLVar& defaultColor() const { return fDefaultColor; }
        void defaultColor(const BLVar& color) { fDefaultColor.assign(color); }

		const BLVar& fillPaint() const { return fFillPaint; }
		void fillPaint(const BLVar& paint) { fFillPaint.assign(paint); }

		const BLVar& strokePaint() const { return fStrokePaint; }
		void strokePaint(const BLVar& paint) { fStrokePaint.assign(paint); }
        
        

        // Typography State
        void fontHandler(FontHandler* handler) noexcept
        {
            fFontHandler = handler;

            // Select a default faunt to start
            if (fFontHandler != nullptr) {
                resetFont();
            }
        }



        TXTALIGNMENT textAnchor() const { return fTextHAlignment; }
        void textAnchor(TXTALIGNMENT anchor)
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
    };
}