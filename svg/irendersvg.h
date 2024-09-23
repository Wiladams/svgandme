#pragma once

#pragma comment(lib, "blend2d.lib") // Link with Blend2D static library, on Windows

#include <functional>

#include "blend2d.h"
#include "fonthandler.h"
#include "collections.h"
#include "svgenums.h"

namespace waavs
{
    struct IRenderSVG;
    struct IAmGroot;
    
    /*
        IGraphics defines the essential interface for doing vector graphics
        This is a pure virtual interface for the most part, so a sub-class must
        implement a fair bit of stuff.
    */
    struct SVGDrawingState {
        BLRect fLocalFrame{};

        // Paint
        BLVar fDefaultColor;
        
        // Typography
        TXTALIGNMENT fTextHAlignment = LEFT;
        TXTALIGNMENT fTextVAlignment = BASELINE;
        
        // Fontography
        FontHandler* fFontHandler{ nullptr };   
        BLFont fFont{};
        ByteSpan fFamilyNames{"Arial"};
        float fFontSize{ 16 };
        BLFontStyle fFontStyle = BL_FONT_STYLE_NORMAL;
        BLFontWeight fFontWeight = BL_FONT_WEIGHT_NORMAL;
        BLFontStretch fFontStretch = BL_FONT_STRETCH_NORMAL;

        
        SVGDrawingState() = default;
        
        // Copy Constructor
		SVGDrawingState(const SVGDrawingState& other) noexcept
			: fFont(other.fFont)
			, fLocalFrame(other.fLocalFrame)
		{
            fLocalFrame = other.fLocalFrame;

            fDefaultColor.assign(other.fDefaultColor);
            
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
            if (this != &other)
            {
                fLocalFrame = other.fLocalFrame;
             
                fDefaultColor.assign(other.fDefaultColor);
                
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

            return *this;
        }
        
        const BLVar & defaultColor() const { return fDefaultColor; }
        void defaultColor(const BLVar& color) { fDefaultColor.assign(color); }
        
        // Typography changes
		void fontHandler(FontHandler* handler) noexcept
		{
			fFontHandler = handler;

            // Select a default faunt to start
            if (fFontHandler != nullptr) {
                resetFont();
            }
		}
        
        void reset()
        {
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
        
		void fontFamily(const ByteSpan& familyNames) noexcept
		{
			fFamilyNames = familyNames;
            resetFont();
        }
        
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
    

    
    struct IRenderSVG : public BLContext
    {
        BLVar fBackground{};
        

        // Managing state
        WSStack<SVGDrawingState> fStateStack{};
        SVGDrawingState fCurrentState{};
        
    public:
        IRenderSVG(FontHandler* fh)
        {
			fBackground = BLRgba32(0xFFFFFFFF);
            
            fontHandler(fh);
        }
        
        virtual ~IRenderSVG() {}

        FontHandler* fontHandler() const { return fCurrentState.fFontHandler; }
        void fontHandler(FontHandler* fh) { 
            fCurrentState.fontHandler(fh);
        }

        
        void localFrame(const BLRect& r) { fCurrentState.fLocalFrame = r; }
		BLRect localFrame() const { return fCurrentState.fLocalFrame; }
        
        // Text Font selection
        const BLFont& font() const { return fCurrentState.fFont; }
        virtual void font(BLFont& afont){ fCurrentState.fFont = afont;    }

        
        void applyState()
        {
            // probably applying font characteristics
        }
        
        virtual bool push() {
            // Save the current state to the state stack
            fStateStack.push(fCurrentState);

            // Save the blend2d context as well
            auto res = save();
            return res == BL_SUCCESS;
        }

        virtual bool pop() {
            auto res = restore();
            return res == BL_SUCCESS;

            // Restore the current state from the state stack
            if (!fStateStack.empty()) {
                fCurrentState = fStateStack.top();
                fStateStack.pop();

                // Apply the current state to the context
                applyState();
            }
        }

        void resetState()
        {
            resetTransform();
            
            // Select a default faunt to start
            fStateStack.clear();
            fCurrentState.reset();
        }
        
        // Call this before each frame to be drawn
        void renew()
        {
            if (fBackground.isNull())
                clear();
            else
                fillAll(fBackground);

            // Setup the default drawing state
            // to conform to what SVG expects
            setCompOp(BL_COMP_OP_SRC_OVER);

            //ctx->setStrokeMiterLimit(4.0);
            strokeJoin(BL_STROKE_JOIN_MITER_CLIP);

            setFillRule(BL_FILL_RULE_NON_ZERO);

            fill(BLRgba32(0, 0, 0));
            noStroke();
            strokeWidth(1.0);

            resetState();
        }

        
        const BLVar& defaultColor() const { return fCurrentState.defaultColor(); }
        void defaultColor(const BLVar& color) { fCurrentState.defaultColor(color); }

        
        virtual void strokeBeforeTransform(bool b) 
        {
            setStrokeTransformOrder(b ? BL_STROKE_TRANSFORM_ORDER_BEFORE : BL_STROKE_TRANSFORM_ORDER_AFTER);
        }
        
        virtual void blendMode(int mode) { BLContext::setCompOp((BLCompOp)mode); }
        virtual void globalOpacity(double opacity) { BLContext::setGlobalAlpha(opacity); }

        virtual void strokeCap(int cap, int position) { BLContext::setStrokeCap((BLStrokeCapPosition)position, (BLStrokeCap)cap); }
        virtual void strokeCaps(int caps) { BLContext::setStrokeCaps((BLStrokeCap)caps); }
        virtual void strokeJoin(int join) { BLContext::setStrokeJoin((BLStrokeJoin)join); }
        virtual void strokeMiterLimit(double limit) { BLContext::setStrokeMiterLimit(limit); }
        virtual void strokeWidth(double w) { BLContext::setStrokeWidth(w); }
		double strokeWidth() { return blContextGetStrokeWidth(this); }
        


        
        virtual bool flush() {
            BLResult bResult = BLContext::flush(BL_CONTEXT_FLUSH_SYNC);
            if (bResult != BL_SUCCESS)
            {
                printf("IRenderSVG.flush(), ERROR: %d\n", bResult);
            }

            return bResult == BL_SUCCESS;
        }

        // paint for filling shapes
        virtual void fill(const BLVar& value) { BLContext::setFillStyle(value); }
        virtual void fill(const BLRgba32& value) { BLContext::setFillStyle(value); }
        virtual void fillOpacity(double o) { BLContext::setFillAlpha(o); }

        virtual void noFill() { BLContext::setFillStyle(BLVar::null()); }

        // paint for stroking lines
        virtual void stroke(const BLVar& value) { BLContext::setStrokeStyle(value); }
        virtual void stroke(const BLRgba32& value) { BLContext::setStrokeStyle(value); }
        virtual void strokeOpacity(double o) { BLContext::setStrokeAlpha(o); }

        virtual void noStroke() { setStrokeStyle(BLVar::null()); }


        // Background management
        virtual void clear() {
            BLContext::clearAll();
        }
        
        void background(const BLVar& bg) noexcept
        {
            blVarAssignWeak(&fBackground, &bg);
            
            if (bg.isNull())
				clear();
            else {
                BLContext::save();
                BLContext::setFillStyle(fBackground);
                BLContext::fillAll();
                BLContext::restore();
            }
        }
        
        virtual void background(const BLRgba32& c) {

            if (c.value == 0) {
                clear();
            }
            else {
                // We want to preserved the current state
                // so wrap in a save/restore
                BLContext::save();
                BLContext::setFillStyle(c);
                BLContext::fillAll();
                BLContext::restore();
                flush();
            }
        }

        // Clipping
        virtual void clip(const BLRect& bb) {
            BLContext::clipToRect(bb);
        }
        
        virtual void noClip() { BLContext::restoreClipping(); }

        // Geometry
        // hard set a specfic pixel value
        virtual void fillRule(int rule) { BLContext::setFillRule((BLFillRule)rule); }


        
        // Bitmaps
        void setFillMask(BLImageCore& mask, const BLRectI &maskArea)
        {
            BLPointI origin(maskArea.x , maskArea.y);
            BLResult res = blContextFillMaskI(this, &origin, &mask, &maskArea);
        }
        
        virtual void image(const BLImageCore& img, int x, int y)
        {
            BLContext::blitImage(BLPointI(x, y), img);
        }
        
        virtual void scaleImage(const BLImageCore& src,
            int srcX, int srcY, int srcWidth, int srcHeight,
            double dstX, double dstY, double dstWidth, double dstHeight) {
            
            BLRect dst{ dstX,dstY,dstWidth,dstHeight };
            BLRectI srcArea{ srcX,srcY,srcWidth,srcHeight };

            BLContext::blitImage(dst, src, srcArea);
        }



        // Typography
        TXTALIGNMENT textAnchor() const { return fCurrentState.textAnchor(); }
        void textAnchor(TXTALIGNMENT anchor) { fCurrentState.textAnchor(anchor); }
        
        virtual void fontFamily(const ByteSpan& familyNames) {fCurrentState.fontFamily(familyNames);}
		virtual void fontSize(double sz) { fCurrentState.fontSize(sz); }
		virtual void fontStyle(const BLFontStyle style) { fCurrentState.fontStyle(style); }
		virtual void fontWeight(const BLFontWeight weight) { fCurrentState.fontWeight(weight); }
        virtual void fontStretch(const BLFontStretch stretch) { fCurrentState.fontStretch(stretch); }
        
        // Text Drawing
		virtual void strokeText(const ByteSpan& txt, double x, double y) {
            BLContext::strokeUtf8Text(BLPoint(x, y), font(), (char*)txt.data(), txt.size());
		}
        
        virtual void fillText(const ByteSpan& txt, double x, double y) {
            BLContext::fillUtf8Text(BLPoint(x, y), font(), (char*)txt.data(), txt.size());
        }
        

        
        // Execute generic operation on this context
        // This supports interface expansion without adding new function prototypes
        virtual void exec(std::function<void(IRenderSVG*)> f) { f(this); }
    };


}

