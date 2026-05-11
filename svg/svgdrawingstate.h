#pragma once

#include <functional>
#include <stack>
#include <vector>
#include <memory>
#include <deque>

#include "blend2d_connect.h"
#include "fonthandler.h"
#include "svgenums.h"
#include "svgdatatypes.h"
#include "svgobject.h"


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
        
        IServePaint*fStrokePaintServer{};
        BLVar fStrokePaint{};
        
        IServePaint*fFillPaintServer{};
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
        WGPointD fTextCursor{};
        SVGAlignment fTextHAlignment = SVGAlignment::SVG_ALIGNMENT_START;
        TXTALIGNMENT fTextVAlignment = BASELINE;

        // ViewportState
		WGMatrix3x3 fTransform{};
        WGRectD fClipRect{};
        WGRectD fViewport{};
        WGRectD fObjectFrame{};
        
        bool modifiedSinceLastPush = false;
        int fErrorState{ 0 };

        // Begin Constructors
        SVGDrawingState() 
        {
            fFillPaint = BLRgba32(0, 0, 0, 255);
            fStrokePaint = BLVar::null();
			fDefaultColor = BLRgba32(0, 0, 0, 255);
            fBackgroundPaint = BLVar::null();
			fTransform = WGMatrix3x3::makeIdentity();
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

            // Paint Servers
            fFillPaintServer = other.fFillPaintServer;
            fStrokePaintServer = other.fStrokePaintServer;

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
        SVGDrawingState* getDrawingState() const { return fDrawingState; }


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
        WGMatrix3x3 getTransform() const { return fDrawingState->fTransform; }
        void setTransform(const WGMatrix3x3& r) {
            fDrawingState->fTransform = r;
            markModified();
        }

        void setViewport(const WGRectD& r) {
            fDrawingState->fViewport = r;
            markModified();
        }
        WGRectD viewport() const {
            return fDrawingState->fViewport; 
        }

        // Map viewport to user space using inverse transform
        WGRectD getViewportUserSpace() const 
        {
            WGMatrix3x3 invTransform = getTransform();
            if (!invTransform.invert())
            {
                printf("IAccessDrawingState::getViewportUserSpace, ERROR: Transform is not invertible\n");
                return WGRectD{};
            }

            // Map the viewport rectangle to user space 
            // using the inverse transform
            WGRectD vport = viewport();
            WGRectD viewportUserSpace{};
            WGPointD origin = invTransform.mapPoint(vport.x, vport.y);
            WGPointD corner = invTransform.mapPoint(vport.x + vport.w, vport.y + vport.h);

            return WGRectD{
                origin.x,
                origin.y,
                corner.x - origin.x,
                corner.y - origin.y
            };
        }

        WGRectD getObjectFrame() const
        {
            if (!fDrawingState) {
                printf("IAccessDrawingState::getObjectFrame, ERROR: No drawing state set\n");
                return WGRectD{};
            }

            return fDrawingState->fObjectFrame; 
        }
        void setObjectFrame(const WGRectD& r) {
            fDrawingState->fObjectFrame = r;
            markModified();
        }


        const WGRectD getClipRect() const { return fDrawingState->fClipRect; }
        virtual void setClipRect(const WGRectD& aRect)
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

		BLVar getBackgroundPaint() const { return fDrawingState->fBackgroundPaint; }
		template<typename StyleT>
		void setBackgroundPaint(const StyleT& paint)
		{
			fDrawingState->fBackgroundPaint.assign(paint);
			markModified();
		}

        BLVar getDefaultColor() const { return fDrawingState->fDefaultColor; }
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

        BLVar getStrokePaint() const  { return fDrawingState->fStrokePaint; }
        template<typename StyleT>
        void setStrokePaint(const StyleT& paint)
        {
            fDrawingState->fStrokePaint.assign(paint);
            markModified();
        }
        
        IServePaint* getStrokeServer() const {return fDrawingState->fStrokePaintServer;}
        void setStrokeServer(IServePaint* obj) { fDrawingState->fStrokePaintServer = obj; }

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
        
        bool hasDashing() const { 
            return fDrawingState->fDash.fHasArray; 
        }
        const StrokeDashState& getStrokeDashState() const { return fDrawingState->fDash; }
        void setStrokeDashArrayRaw(const std::vector<float>& arr)
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

        void setStrokeDashOffsetRaw(const float off)
        {
            fDrawingState->fDash.fOffset = off;
            fDrawingState->fDash.fHasOffset = true;

            markModified();
        }

        void clearStrokeDashOffset() 
        {
            fDrawingState->fDash.clearOffset();
            markModified();
        }


        void setStrokeDashArray(const std::vector<float> &dasharray)
        {
            setStrokeDashArrayRaw(dasharray);

            markModified();
        }

        // Fill Paint Server
        IServePaint* getFillPaintServer() const { return fDrawingState->fFillPaintServer; }
        void setFillPaintServer(IServePaint* obj) { fDrawingState->fFillPaintServer = obj; }

        // Fill Attributes
        BLVar getFillPaint() const { return fDrawingState->fFillPaint; }
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

        WGPointD getTextCursor() const { return fDrawingState->fTextCursor; }
        void setTextCursor(const WGPointD& cursor)
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

        virtual bool applyToContext(BLContext* ctx)
        {
            // clear the clipping state
            ctx->restoreClipping();
            WGRectD cRect = getClipRect();
            if ((cRect.w > 0) && (cRect.h > 0))
            {
                BLRect blr{ cRect.x, cRect.y, cRect.w, cRect.h };
                ctx->clipToRect(blr);
            }

            ctx->setCompOp((BLCompOp)getCompositeMode());
            ctx->setFillRule((BLFillRule)getFillRule());

            // Set the transform
            BLMatrix2D t = blMatrix_from_WGMatrix3x3(getTransform());
            ctx->setTransform(t);

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