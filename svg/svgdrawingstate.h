#pragma once

#include <functional>
#include <stack>
#include <vector>
#include <memory>
#include <deque>

#include "blend2d.h"
#include "fonthandler.h"
#include "svgenums.h"
#include "svgdatatypes.h"


namespace waavs {


	// Represents the current state of the SVG rendering context
    // this can be used by DOM walkers, as well as rendering context
    //


    // SVGDrawingState 
    // Brings all the state management together
    struct SVGDrawingState 
    {
		uint8_t fCompositeMode{ BL_COMP_OP_SRC_OVER };
		uint8_t fFillRule{ BL_FILL_RULE_NON_ZERO };
        bool fStrokeBeforeTransform{ false };

        // PaintState
        uint32_t fPaintOrder{ PaintOrderKind::SVG_PAINT_ORDER_NORMAL };
        BLVar fStrokePaint{};
        BLVar fFillPaint{};
        BLVar fDefaultColor{};
        BLVar fBackgroundPaint{};
        double fGlobalOpacity{ 1.0 };
        double fStrokeOpacity{ 1.0 };
        double fFillOpacity{ 1.0 };

        // StrokeState
		BLStrokeOptions fStrokeOptions{};
        StrokeDashState fDash{};

        // FontState
        BLFont fFont{};
        ByteSpan fFamilyNames{ "Arial" };
        float fFontSize{ 16 };
        BLFontStyle fFontStyle = BL_FONT_STYLE_NORMAL;
        BLFontWeight fFontWeight = BL_FONT_WEIGHT_NORMAL;
        BLFontStretch fFontStretch = BL_FONT_STRETCH_NORMAL;

        // TextState
        BLPoint fTextCursor{};
        SVGAlignment fTextHAlignment = SVGAlignment::SVG_ALIGNMENT_START;
        TXTALIGNMENT fTextVAlignment = BASELINE;

        // ViewportState
		BLMatrix2D fTransform{};
        BLRect fClipRect{};
        BLRect fViewport{};
        BLRect fObjectFrame{};
        
        bool modifiedSinceLastPush = false;
        int fErrorState{ 0 };

        // Begin Constructors
        SVGDrawingState() 
        {
            fFillPaint = BLRgba32(0, 0, 0, 255);
            fStrokePaint = BLVar::null();
			fDefaultColor = BLRgba32(0, 0, 0, 255);
            fBackgroundPaint = BLVar::null();
			fTransform = BLMatrix2D::makeIdentity();
        }

        SVGDrawingState(const SVGDrawingState& other)
        {
            *this = other;
        }

        SVGDrawingState& operator=(const SVGDrawingState& other)
        {
            if (this == &other) return *this;

			// Composite Mode
			fCompositeMode = other.fCompositeMode;

            // Fill Options
            fFillRule = other.fFillRule;
            
            // Stroke Options
            fStrokeOptions = other.fStrokeOptions;
            fDash = other.fDash;

            // Paints
            fStrokePaint.assign(other.fStrokePaint);
            fFillPaint.assign(other.fFillPaint);
            fDefaultColor.assign(other.fDefaultColor);
            fBackgroundPaint.assign(other.fBackgroundPaint);
            fGlobalOpacity = other.fGlobalOpacity;
            fFillOpacity = other.fFillOpacity;
            fStrokeOpacity = other.fStrokeOpacity;
            fPaintOrder = other.fPaintOrder;

            // Fontography
            fFont = other.fFont;
            fFamilyNames = other.fFamilyNames;
            fFontSize = other.fFontSize;
            fFontStyle = other.fFontStyle;
            fFontWeight = other.fFontWeight;
            fFontStretch = other.fFontStretch;

			// Textography
            fTextCursor = other.fTextCursor;
            fTextHAlignment = other.fTextHAlignment;
            fTextVAlignment = other.fTextVAlignment;

			// Viewport
			fTransform = other.fTransform;
            fClipRect = other.fClipRect;
            fViewport = other.fViewport;
            fObjectFrame = other.fObjectFrame;

            modifiedSinceLastPush = other.modifiedSinceLastPush;

            return *this;
        }

        void markModified() {
            modifiedSinceLastPush = true;
        }

		bool isModified() const {
			return modifiedSinceLastPush;
		}
    };

    // Contains all the accessor methods used
    // to change the fields of the state
    struct IAccessSVGState
    {
        SVGDrawingState* fDrawingState{};

        // Begin Constructors
        IAccessSVGState() = default;
        IAccessSVGState(SVGDrawingState* st) :fDrawingState(st) {}


		void setDrawingState(SVGDrawingState* state)
		{
			fDrawingState = state;
		}



        void markModified() 
        {
            fDrawingState->modifiedSinceLastPush = true;
        }


        uint8_t getCompositeMode() const { return fDrawingState->fCompositeMode; }
        void setCompositeMode(uint8_t mode) {
            fDrawingState->fCompositeMode = mode;
            markModified();
        }

        /// <summary>
        ///  Set various attributes of the state
        /// </summary>
        /// <param name="r"></param>
        /// 
        BLMatrix2D getTransform() const { return fDrawingState->fTransform; }
        void setTransform(const BLMatrix2D& r) {
            fDrawingState->fTransform = r;
            markModified();
        }

        void setViewport(const BLRect& r) {
            fDrawingState->fViewport = r;
            markModified();
        }
        BLRect viewport() const { return fDrawingState->fViewport; }

        BLRect getObjectFrame() const { return fDrawingState->fObjectFrame; }
        void setObjectFrame(const BLRect& r) {
            fDrawingState->fObjectFrame = r;
            markModified();
        }


        const BLRect& getClipRect() const { return fDrawingState->fClipRect; }
        virtual void setClipRect(const BLRect& aRect)
        {
            fDrawingState->fClipRect = aRect;
            markModified();
        }

        uint32_t getPaintOrder() const { return fDrawingState->fPaintOrder; }
        virtual void setPaintOrder(const uint32_t order)
        {
            fDrawingState->fPaintOrder = order;
            markModified();
        }

		const BLVar& getBackgroundPaint() const { return fDrawingState->fBackgroundPaint; }
		template<typename StyleT>
		void setBackgroundPaint(const StyleT& paint)
		{
			fDrawingState->fBackgroundPaint.assign(paint);
			markModified();
		}

        const BLVar& getDefaultColor() const { return fDrawingState->fDefaultColor; }
        void setDefaultColor(const BLVar& color)
        {
            fDrawingState->fDefaultColor.assign(color);
            markModified();
        }

        double getGlobalOpacity() const { return fDrawingState->fGlobalOpacity; }
        void setGlobalOpacity(double opacity)
        {
            fDrawingState->fGlobalOpacity = opacity;
            markModified();
        }

        // Stroke attributes
        bool getStrokeBeforeTransform()
        {
            return fDrawingState->fStrokeBeforeTransform;
        }

        void setStrokeBeforeTransform(bool b)
        {
            fDrawingState->fStrokeBeforeTransform = b;
            markModified();
        }

        const BLVar& getStrokePaint() const { return fDrawingState->fStrokePaint; }
        template<typename StyleT>
        void setStrokePaint(const StyleT& paint)
        {
            fDrawingState->fStrokePaint.assign(paint);
            markModified();
        }

        double getStrokeOpacity() const { return fDrawingState->fStrokeOpacity; }
        void setStrokeOpacity(double opacity)
        {
            fDrawingState->fStrokeOpacity = opacity;
            markModified();
        }

        uint8_t startStrokeCap() const { return fDrawingState->fStrokeOptions.startCap; }
        void setStrokeStartCap(uint8_t kind) {
            fDrawingState->fStrokeOptions.startCap = kind;
            markModified();
        }

        uint8_t getEndStrokeCap() const { return fDrawingState->fStrokeOptions.endCap; }
        void setStrokeEndCap(uint8_t kind)
        {
            fDrawingState->fStrokeOptions.endCap = kind;
            markModified();
        }

        void setStrokeCaps(BLStrokeCap caps)
        {
            fDrawingState->fStrokeOptions.setCaps(caps);
            markModified();
        }

        double getStrokeMiterLimit() const { return fDrawingState->fStrokeOptions.miterLimit; }
        void setStrokeMiterLimit(double limit) {
            fDrawingState->fStrokeOptions.miterLimit = limit;
            markModified();
        }

        double getStrokeWidth() const { return fDrawingState->fStrokeOptions.width; }
        virtual void setStrokeWidth(double sw)
        {
            fDrawingState->fStrokeOptions.width = sw;
            markModified();
        }

        uint8_t getLineJoin() const { return fDrawingState->fStrokeOptions.join; }
        void setLineJoin(BLStrokeJoin join)
        {
            fDrawingState->fStrokeOptions.join = join;
            markModified();
        }
        
        const StrokeDashState& getStrokeDashState() const { return fDrawingState->fDash; }
        void setStrokeDashArrayRaw(const std::vector<SVGDimension>& arr)
        {
            fDrawingState->fDash.fArray = arr;
            fDrawingState->fDash.fHasArray = !arr.empty();
            markModified();
        }

        void clearStrokeDashArray()
        {
            fDrawingState->fDash.clearArray();
            markModified();
        }

        void setStrokeDashOffsetRaw(const SVGDimension& off)
        {
            fDrawingState->fDash.fOffset = off;
            fDrawingState->fDash.fHasOffset = off.isSet();
            markModified();
        }

        void clearStrokeDashOffset() 
        {
            fDrawingState->fDash.clearOffset();
            markModified();
        }


        //const std::vector<SVGLength>& getStrokeDashArray() const { return std::vector<double>{}; }
        void setStrokeDashArray(const double *dashes, size_t count)
        {
            //fDrawingState->fStrokeDashArray = dashes;
            markModified();
        }

        // Fill Attributes
        const BLVar& getFillPaint() const { return fDrawingState->fFillPaint; }
        template<typename StyleT>
        void setFillPaint(const StyleT& paint)
        {
            fDrawingState->fFillPaint.assign(paint);
            markModified();
        }

        double getFillOpacity() const { return fDrawingState->fFillOpacity; }
        void setFillOpacity(double opacity) {
            fDrawingState->fFillOpacity = opacity;
            markModified();
        }

        uint8_t getFillRule() const { return fDrawingState->fFillRule; }
        void setFillRule(BLFillRule fRule) { fDrawingState->fFillRule = fRule; markModified(); }

        // Typography
        SVGAlignment getTextAnchor() const { return fDrawingState->fTextHAlignment; }
        void setTextAnchor(SVGAlignment anchor)
        {
            fDrawingState->fTextHAlignment = anchor;
            markModified();
        }

        TXTALIGNMENT getTextAlignment() const { return fDrawingState->fTextVAlignment; }
        void setTextAlignment(TXTALIGNMENT align)
        {
            fDrawingState->fTextVAlignment = align;
            markModified();
        }

        BLPoint getTextCursor() const { return fDrawingState->fTextCursor; }
        void setTextCursor(const BLPoint& cursor)
        {
            fDrawingState->fTextCursor = cursor; markModified();
        }


        // Fontography
        virtual void resetFont() {}

        const BLFont& getFont() const { return fDrawingState->fFont; }
        virtual void setFont(BLFont& afont)
        {
            fDrawingState->fFont = afont; markModified();
        }

        const ByteSpan& getFontFamily() const noexcept { return fDrawingState->fFamilyNames; }
        void setFontFamily(const ByteSpan& familyNames) noexcept
        {
            fDrawingState->fFamilyNames = familyNames;
            resetFont();
            markModified();
        }

        double getFontSize() const noexcept { return fDrawingState->fFontSize; }
        void setFontSize(float size) noexcept
        {
            fDrawingState->fFontSize = size;
            resetFont();
            markModified();
        }

        BLFontStyle getFontStyle() const noexcept { return fDrawingState->fFontStyle; }
        void setFontStyle(BLFontStyle style) noexcept
        {
            fDrawingState->fFontStyle = style;
            resetFont();
            markModified();
        }

        BLFontWeight getFontWeight() const noexcept { return fDrawingState->fFontWeight; }
        void setFontWeight(BLFontWeight weight) noexcept
        {
            fDrawingState->fFontWeight = weight;
            resetFont();
            markModified();
        }

        BLFontStretch getFontStretch() const noexcept { return fDrawingState->fFontStretch; }
        void setFontStretch(BLFontStretch stretch) noexcept
        {
            fDrawingState->fFontStretch = stretch;
            resetFont();
            markModified();
        }

        // Apply those attributes that need to be on 
        // the BLContext
        virtual bool applyToContext(BLContext* ctx)
        {
            // clear the clipping state
            ctx->restoreClipping();
            const BLRect& cRect = getClipRect();
            if ((cRect.w > 0) && (cRect.h > 0))
                ctx->clipToRect(cRect);


            ctx->setCompOp((BLCompOp)getCompositeMode());
            ctx->setFillRule((BLFillRule)getFillRule());

            // Set the transform
            ctx->setTransform(getTransform());

            // Stroke Options
            ctx->setStrokeOptions(fDrawingState->fStrokeOptions);

            // Paints
            ctx->setStrokeStyle(getStrokePaint());
            ctx->setFillStyle(getFillPaint());
            ctx->setGlobalAlpha(getGlobalOpacity());
            ctx->setStrokeAlpha(getStrokeOpacity());
            ctx->setFillAlpha(getFillOpacity());


            return true;
        }

    };
}