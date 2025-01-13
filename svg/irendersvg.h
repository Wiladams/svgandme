#pragma once

#pragma comment(lib, "blend2d.lib") // Link with Blend2D static library, on Windows

#include <functional>

#include "blend2d.h"
#include "fonthandler.h"
#include "collections.h"
#include "svgenums.h"
#include "svgdrawingstate.h"

namespace waavs
{
    struct IRenderSVG;
    struct IAmGroot;
    

    struct IRenderSVG : public BLContext
    {
        BLVar fBackground{};
        BLPoint fTextCursor{};

        // Managing state
        WSStack<SVGDrawingState> fStateStack{};
        SVGDrawingState fCurrentState{};
        
    public:
        IRenderSVG(FontHandler* fh)
        {
            initState();
            fontHandler(fh);
        }
        
        virtual ~IRenderSVG() {}

        FontHandler* fontHandler() const { return fCurrentState.fFontHandler; }
        void fontHandler(FontHandler* fh) { 
            fCurrentState.fontHandler(fh);
        }

        void initState()
        {
            fBackground = BLRgba32(0xFFFFFFFF);
            strokeJoin(BL_STROKE_JOIN_MITER_CLIP);
            strokeMiterLimit(4);
            setFillRule(BL_FILL_RULE_NON_ZERO);
            fill(BLRgba32(0, 0, 0));
            noStroke();
            strokeWidth(1.0);
        }
        
        BLResult attach(BLImageCore& image, const BLContextCreateInfo* createInfo= nullptr) noexcept
        {
            BLResult res = BLContext::begin(image, createInfo);
            initState();
            
            return res;
        }
        
        void detach()
        {
            BLContext::end();
        }
        
        void setViewport(const BLRect& r) { 
            fCurrentState.fViewport = r;
        }
		BLRect viewport() const { return fCurrentState.fViewport; }
        

		void objectFrame(const BLRect& r) {
			fCurrentState.fObjectFrame = r;
		}
		BLRect objectFrame() const { return fCurrentState.fObjectFrame; }
        
        // Text Font selection
        const BLFont& font() const { return fCurrentState.fFont; }
        virtual void font(BLFont& afont){ fCurrentState.fFont = afont;    }
        void resetFont() { fCurrentState.resetFont(); }
        
        void applyState()
        {
            // clear the clipping state
            BLContext::restoreClipping();
            
			const BLRect &cRect = fCurrentState.clipRect();
            if ((cRect.w >0) && (cRect.h>0))
				BLContext::clipToRect(cRect);
            
            // probably applying font characteristics
            // probably apply paint
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


            // Restore the current state from the state stack
            if (!fStateStack.empty()) {
                fCurrentState = fStateStack.top();
                fStateStack.pop();

                // Apply the current state to the context
                applyState();
            }

            return res == BL_SUCCESS;
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

            resetState();

            // Setup the default drawing state
            // to conform to what SVG expects
            setCompOp(BL_COMP_OP_SRC_OVER);

            //ctx->setStrokeMiterLimit(4.0);
            strokeJoin(BL_STROKE_JOIN_MITER_CLIP);

            setFillRule(BL_FILL_RULE_NON_ZERO);

            fill(BLRgba32(0, 0, 0));
            noStroke();
            strokeWidth(1.0);
        }

        
        const BLVar& defaultColor() const { return fCurrentState.defaultColor(); }
        void defaultColor(const BLVar& color) { fCurrentState.defaultColor(color); }

        uint32_t paintOrder() const { return fCurrentState.paintOrder(); }
        void paintOrder(const uint32_t order) { fCurrentState.paintOrder(order); }

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
        virtual void fill(const BLRgba32& value) { 
            BLContext::setFillStyle(value); 
        }
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
            // Save the value of the background and...
            blVarAssignWeak(&fBackground, &bg);
            
            // apply the background value immediately
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
        void setClipRect(const BLRect& cRect) {
			fCurrentState.setClipRect(cRect);
			BLContext::clipToRect(cRect);
        }
        
        virtual void clip(const BLRect& bb) {
            BLContext::clipToRect(bb);
        }
        
        virtual void noClip() { 
            BLContext::restoreClipping(); 
        }

        // Geometry
        // hard set a specfic pixel value
        virtual void fillRule(int rule) { 
            BLContext::setFillRule((BLFillRule)rule); 
        }


        
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
        
        BLPoint textCursor() const { return fTextCursor; }
        void textCursor(const BLPoint& cursor) { fTextCursor = cursor; }

        //BLPoint textCursor() const { return fCurrentState.textCursor(); }
        //void textCursor(const BLPoint& cursor) { fCurrentState.textCursor(cursor); }
        
        virtual void fontFamily(const ByteSpan& familyNames) {fCurrentState.fontFamily(familyNames);}
		virtual void fontSize(double sz) { fCurrentState.fontSize(static_cast<float>(sz)); }
		double fontSize() const { return fCurrentState.fontSize(); }
        
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

