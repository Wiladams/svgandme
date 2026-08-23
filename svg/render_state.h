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
#include "svg_interface.h"

namespace waavs
{
    struct SolidColorPaint : public IServePaint
    {
        BLRgba32 fColorValue;
        //bool isNone{ false };

        SolidColorPaint()
        {
            fColorValue.value = 0; // default to transparent black
            //isNone = true;
        }

        SolidColorPaint(const BLRgba32& color)
        {
            fColorValue = color;
            //isNone = false;
        }

        SolidColorPaint(const uint32_t r, const uint32_t g, const uint32_t b, const uint32_t a = 255)
        {
            fColorValue.reset(r, g, b, a);
            //isNone = false;
        }

        const BLVar getVariant(IDrawGraphics* ctx, IAmGroot* groot) noexcept override
        {
            //if (isNone)
            //    return BLVar::null();

            BLVar tmpVar{};
            tmpVar = fColorValue;
            return tmpVar;
        }


    };
}

namespace waavs {

	// Represents the current state of the SVG rendering context
    // this can be used by DOM walkers, as well as rendering context
    //


    // SVGDrawingState 
    // Brings all the state management together
    struct SVGDrawingState 
    {
        // Some default paint servers
        static IServePaint* emptyPaintServer()
        {
            static SolidColorPaint kEmptyPaint;
            return &kEmptyPaint;
        }

        static IServePaint* defaultFillServer()
        {
            static SolidColorPaint kFillPaint(0, 0, 0, 255);
            return &kFillPaint;
        }

        static IServePaint* defaultStrokeServer()
        {
            static SolidColorPaint kStrokePaint;
            return &kStrokePaint;
        }

		uint8_t fCompositeMode{ BL_COMP_OP_SRC_OVER };
		uint8_t fFillRule{ BL_FILL_RULE_NON_ZERO };
        bool fStrokeBeforeTransform{ false };

        // PaintState
        uint32_t fPaintOrder{ PaintOrderKind::SVG_PAINT_ORDER_NORMAL };
        
        IServePaint*fStrokePaintServer = defaultStrokeServer();        
        IServePaint*fFillPaintServer= defaultFillServer();
        //BLVar fFillPaint{};

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
            fFillPaintServer = defaultFillServer();
            fStrokePaintServer = defaultStrokeServer();
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
            if (this == &other) 
                return *this;

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

        void markModified() { modifiedSinceLastPush = true; }
		bool isModified() const { return modifiedSinceLastPush; }

        uint8_t getCompositeMode() const { return fCompositeMode; }
        void setCompositeMode(uint8_t mode) {
            fCompositeMode = mode;
            markModified();
        }

        /// <summary>
        ///  Set various attributes of the state
        /// </summary>
        /// <param name="r"></param>
        /// 
        WGMatrix3x3 getTransform() const { return fTransform; }
        void setTransform(const WGMatrix3x3& r) {
            fTransform = r;
            markModified();
        }

        void setViewport(const WGRectD& r) {
            fViewport = r;
            markModified();
        }
        WGRectD getViewport() const { return fViewport; }

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
            WGRectD vport = getViewport();
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
            return fObjectFrame;
        }
        void setObjectFrame(const WGRectD& r) {
            fObjectFrame = r;
            markModified();
        }


        const WGRectD getClipRect() const { return fClipRect; }
        virtual void setClipRect(const WGRectD& aRect)
        {
            fClipRect = aRect;
            markModified();
        }

        uint32_t getPaintOrder() const { return fPaintOrder; }
        virtual void setPaintOrder(const uint32_t order)
        {
            fPaintOrder = order;
            markModified();
        }

        BLVar getBackgroundPaint() const { return fBackgroundPaint; }
        template<typename StyleT>
        void setBackgroundPaint(const StyleT& paint)
        {
            fBackgroundPaint.assign(paint);
            markModified();
        }

        BLVar getDefaultColor() const { return fDefaultColor; }
        void setDefaultColor(const BLVar& color)
        {
            fDefaultColor.assign(color);
            markModified();
        }

        double getGlobalOpacity() const { return fGlobalOpacity; }
        void setGlobalOpacity(double opacity)
        {
            fGlobalOpacity = opacity;
            markModified();
        }

        // Stroke attributes
        bool getStrokeBeforeTransform()
        {
            return fStrokeBeforeTransform;
        }

        void setStrokeBeforeTransform(bool b)
        {
            fStrokeBeforeTransform = b;
            markModified();
        }

        IServePaint* getStrokePaintServer() const { return fStrokePaintServer; }
        void setStrokePaintServer(IServePaint* obj) { fStrokePaintServer = obj; }

        BLVar getStrokePaint() const
        {
            return getStrokePaintServer()->getVariant(nullptr, nullptr);
        }
        //template<typename StyleT>
        //void setStrokePaint(const StyleT& paint)
        //{
        //    fDrawingState->fStrokePaint.assign(paint);
        //    markModified();
        //}


        double getStrokeOpacity() const { return fStrokeOpacity; }
        void setStrokeOpacity(double opacity)
        {
            fStrokeOpacity = opacity;
            markModified();
        }

        uint8_t startStrokeCap() const { return fStrokeOptions.start_cap; }
        void setStrokeStartCap(uint8_t kind) {
            fStrokeOptions.start_cap = kind;
            markModified();
        }

        uint8_t getEndStrokeCap() const { return fStrokeOptions.end_cap; }
        void setStrokeEndCap(uint8_t kind)
        {
            fStrokeOptions.end_cap = kind;
            markModified();
        }

        void setStrokeCaps(BLStrokeCap caps)
        {
            fStrokeOptions.set_caps(caps);
            markModified();
        }

        double getStrokeMiterLimit() const { return fStrokeOptions.miter_limit; }
        void setStrokeMiterLimit(double limit) {
            fStrokeOptions.miter_limit = limit;
            markModified();
        }

        double getStrokeWidth() const { return fStrokeOptions.width; }
        virtual void setStrokeWidth(double sw)
        {
            fStrokeOptions.width = sw;
            markModified();
        }

        uint8_t getLineJoin() const { return fStrokeOptions.join; }
        void setLineJoin(BLStrokeJoin join)
        {
            fStrokeOptions.join = join;
            markModified();
        }

        bool hasDashing() const {
            return fDash.fHasArray;
        }
        const StrokeDashState& getStrokeDashState() const { return fDash; }
        void setStrokeDashArrayRaw(const std::vector<float>& arr)
        {
            fDash.fArray = arr;
            fDash.fHasArray = !arr.empty();
            markModified();
        }

        void clearStrokeDashArray()
        {
            fDash.clearArray();
            markModified();
        }

        void setStrokeDashOffsetRaw(const float off)
        {
            fDash.fOffset = off;
            fDash.fHasOffset = true;

            markModified();
        }

        void clearStrokeDashOffset()
        {
            fDash.clearOffset();
            markModified();
        }


        void setStrokeDashArray(const std::vector<float>& dasharray)
        {
            setStrokeDashArrayRaw(dasharray);
            markModified();
        }

        // Fill Paint Server
        IServePaint* getFillPaintServer() const { return fFillPaintServer; }
        void setFillPaintServer(IServePaint* obj) { fFillPaintServer = obj; }

        // Fill Attributes

        BLVar getFillPaint()
        {
            return getFillPaintServer()->getVariant(nullptr, nullptr);
        }
        //template<typename StyleT>
        //void setFillPaint(const StyleT& paint)
        //{
        //    fDrawingState->fFillPaint.assign(paint);
        //    markModified();
        //}

        double getFillOpacity() const { return fFillOpacity; }
        void setFillOpacity(double opacity) {
            fFillOpacity = opacity;
            markModified();
        }

        uint8_t getFillRule() const { return fFillRule; }
        void setFillRule(BLFillRule fRule) { fFillRule = fRule; markModified(); }

        // Typography
        SVGAlignment getTextAnchor() const { return fTextHAlignment; }
        void setTextAnchor(SVGAlignment anchor)
        {
            fTextHAlignment = anchor;
            markModified();
        }

        TXTALIGNMENT getTextAlignment() const { return fTextVAlignment; }
        void setTextAlignment(TXTALIGNMENT align)
        {
            fTextVAlignment = align;
            markModified();
        }

        WGPointD getTextCursor() const { return fTextCursor; }
        void setTextCursor(const WGPointD& cursor)
        {
            fTextCursor = cursor; markModified();
        }


        // Fontography
        virtual void resetFont() {}

        const BLFont& getFont() const { return fFont; }
        virtual void setFont(BLFont& afont)
        {
            fFont = afont; markModified();
        }

        const ByteSpan& getFontFamily() const noexcept { return fFamilyNames; }
        void setFontFamily(const ByteSpan& familyNames) noexcept
        {
            fFamilyNames = familyNames;
            resetFont();
            markModified();
        }

        double getFontSize() const noexcept { return fFontSize; }
        void setFontSize(float size) noexcept
        {
            fFontSize = size;
            resetFont();
            markModified();
        }

        BLFontStyle getFontStyle() const noexcept { return fFontStyle; }
        void setFontStyle(BLFontStyle style) noexcept
        {
            fFontStyle = style;
            resetFont();
            markModified();
        }

        BLFontWeight getFontWeight() const noexcept { return fFontWeight; }
        void setFontWeight(BLFontWeight weight) noexcept
        {
            fFontWeight = weight;
            resetFont();
            markModified();
        }

        BLFontStretch getFontStretch() const noexcept { return fFontStretch; }
        void setFontStretch(BLFontStretch stretch) noexcept
        {
            fFontStretch = stretch;
            resetFont();
            markModified();
        }

    };

}

namespace waavs
{
    struct SVGStateManager
    {
        SVGDrawingState fCurrent{};
        std::vector<SVGDrawingState> fStack{};

        SVGStateManager()
        {
            fStack.reserve(16);
        }

        void reset()
        {
            fCurrent = SVGDrawingState{};
            fStack.clear();
        }

        SVGDrawingState& current() noexcept { return fCurrent; }
        const SVGDrawingState& current() const noexcept { return fCurrent; }

        void push()
        {
            fStack.push_back(fCurrent);
            fCurrent.modifiedSinceLastPush = false;
        }

        bool pop()
        {
            if (fStack.empty())
                return false;

            fCurrent = fStack.back();
            fStack.pop_back();

            return true;
        }

        size_t depth() const noexcept { return fStack.size(); }
    };
}