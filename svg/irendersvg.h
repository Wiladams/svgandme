#pragma once

#pragma comment(lib, "blend2d.lib") // Link with Blend2D static library, on Windows

#include <functional>

#include "blend2d.h"
#include "fonthandler.h"
#include "collections.h"
#include "svgenums.h"
#include "imanagesvgstate.h"

namespace waavs
{
    struct IRenderSVG;
    struct IAmGroot;
    
    
    

    // A specialization of state management, connected to a BLContext
    // This is used when rendering a tree of SVG elements
    struct IRenderSVG : public IManageSVGState, public BLContext
    {
        BLVar fBackground{};
        BLPoint fTextCursor{};
        FontHandler* fFontHandler{ nullptr };


        
    public:
        IRenderSVG(FontHandler* fh)
        {
            initState();
            fontHandler(fh);
        }
        
        virtual ~IRenderSVG() {}



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
        

        


        void resetFont()
        {
            if (nullptr != fFontHandler)
            {
                BLFont aFont;
                if (fFontHandler->selectFont(fFamilyNames, aFont, fFontSize, fFontStyle, fFontWeight, fFontStretch))
                {
                    fFont = aFont;
                }
            }

        }
        
        
        FontHandler* fontHandler() const { 
            return fFontHandler; 
        }
        void fontHandler(FontHandler* fh) {
            fFontHandler = fh;
            if (fFontHandler != nullptr) {
                resetFont();
            }
        }
        
        void applyState()
        {
            // clear the clipping state
            BLContext::restoreClipping();
            
			const BLRect &cRect = getClipRect();
            if ((cRect.w >0) && (cRect.h>0))
				BLContext::clipToRect(cRect);
            
            // probably applying font characteristics
            // probably apply paint
        }
        
        bool push() override 
        {
            IManageSVGState::push();

            // Save the blend2d context as well
            auto res = save();
            return res == BL_SUCCESS;
        }

        virtual bool pop() 
        {
 
            if (IManageSVGState::pop())
            {
                auto res = restore();
                applyState();
            
				return res == BL_SUCCESS;
            }
            
            return false;
        }

        void resetState()
        {
            IManageSVGState::reset();
            
            resetTransform();
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

        void strokeWidth(double w) override { 
            IManageSVGState::strokeWidth(w);
            BLContext::setStrokeWidth(w); 
        }

        


        
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
        void clipRect(const BLRect& cRect) override 
        {
            IManageSVGState::clipRect(cRect);
			BLContext::clipToRect(cRect);
        }
        
        //virtual void clip(const BLRect& bb) {
        //    BLContext::clipToRect(bb);
        //}
        
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
        
        BLPoint textCursor() const { return fTextCursor; }
        void textCursor(const BLPoint& cursor) { fTextCursor = cursor; }

        //BLPoint textCursor() const { return fCurrentState.textCursor(); }
        //void textCursor(const BLPoint& cursor) { fCurrentState.textCursor(cursor); }
        
        
        
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

