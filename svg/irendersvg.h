#pragma once

#pragma comment(lib, "blend2d.lib") // Link with Blend2D static library, on Windows

#include <functional>

#include "blend2d.h"
#include "fonthandler.h"



namespace waavs
{
    // RectMode
    enum class RECTMODE : unsigned
    {
        CORNER,
        CORNERS,
        CENTER,
    };

    // EllipseMode
    enum class ELLIPSEMODE : unsigned
    {
        CORNER,
        CORNERS,
        CENTER,
        RADIUS
    };

    // Text Alignment
    enum class ALIGNMENT : unsigned
    {
        CENTER = 0x01,

        LEFT = 0x02,
        RIGHT = 0x04,

        TOP = 0x10,
        BASELINE = 0x20,
        BOTTOM = 0x40,
        MIDLINE = 0x80,

    };

    // Text Wrap
    // 
    enum class TEXTWRAP : unsigned
    {
        WORD,
        CHAR
    };


    enum class FILLRULE : unsigned
    {
        NON_ZERO = 0,
        EVEN_ODD = 1,
    };
}

namespace waavs
{
    /*
        IGraphics defines the essential interface for doing vector graphics
        This is a pure virtual interface for the most part, so a sub-class must
        implement a fair bit of stuff.
    */
    struct IRenderSVG : public BLContext
    {
        // Typography
        FontHandler* fFontHandler = nullptr;
        BLFontFace fFontFace{};
        BLFont fFont{};
        double fFontSize = 16;
        
        ALIGNMENT fTextHAlignment = ALIGNMENT::LEFT;
        ALIGNMENT fTextVAlignment = ALIGNMENT::BASELINE;
        double fTextX{ 0 };
        double fTextY{ 0 };
        double fTextAdvance = 0;

        // local width/height
		double fLocalWidth{ 0 };
        double fLocalHeight{ 0 };
        BLRect fLocalFrame{};
        
        
    public:
        IRenderSVG(FontHandler* fh) :fFontHandler(fh) 
        {
            //clearAll();
            fillAll(BLRgba32(0xff000000u));
            
            // Start with default state
            setCompOp(BL_COMP_OP_SRC_OVER);

            //ctx->setStrokeMiterLimit(4.0);
            strokeJoin(BL_STROKE_JOIN_MITER_CLIP);


            setFillRule(BL_FILL_RULE_NON_ZERO);

            fill(BLRgba32(0, 0, 0));
            noStroke();
            strokeWidth(1.0);
            textAlign(ALIGNMENT::LEFT, ALIGNMENT::BASELINE);
            textFamily("Arial");
            textSize(16);
        }
        
        virtual ~IRenderSVG() {}

        FontHandler* fontHandler() const { return fFontHandler; }
        void fontHandler(FontHandler* fh) { fFontHandler = fh; }

        
        // Execute generic operation on this context
        // This supports interface expansion without adding new function prototypes
        virtual void exec(std::function<void(IRenderSVG*)> f) { f(this); }

        //virtual void setDpiUnits(const int dpi, const float units) = 0;
        void localSize(double w, double h) { fLocalWidth = w; fLocalHeight = h; }
        BLPoint localSize() const { return BLPoint(fLocalWidth, fLocalHeight); }
        
        void localFrame(const BLRect& r) { fLocalFrame = r; }
		BLRect localFrame() const { return fLocalFrame; }
        
        
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
        
        virtual bool push() {
            auto res = save();
            return res == BL_SUCCESS;
        }
        
        virtual bool pop() {
            auto res = restore();
            return res == BL_SUCCESS;
        }
        
        virtual bool flush() {
            BLResult bResult = BLContext::flush(BL_CONTEXT_FLUSH_SYNC);
            if (bResult != BL_SUCCESS)
            {
                printf("IRenderSVG.flush(), ERROR: %d\n", bResult);
            }

            return bResult == BL_SUCCESS;
        }

        // paint for filling polygons
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

        /*
        virtual void path(const BLPath& path) {
            BLContext::fillPath(path);
            BLContext::strokePath(path);
        }

        virtual void rect(const BLRect& geom) {
			BLContext::fillRect(geom);
			BLContext::strokeRect(geom);
        }
        */
        
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

        // Text Font selection
		const BLFont& font() const { return fFont; }
        virtual void font(BLFont& afont)
        {
			fFont = afont;
        }
        
        virtual void textFace(const BLFontFace& face) {
            if (face.isValid()) {
                fFontFace = face;
                textSize(fFontSize);
            }
        }
        
        virtual void textFamily(const char* familyName) {
            // query the font manager
            // set the found face as the current face

            BLFontFace face{};
			auto success = fontHandler()->selectFontFamily(familyName, face);
            
            //auto success = fontHandler()->selectFont (familyName, fFont, fFontSize);

            if (success) {
                fFontFace = face;
                setFontSize(fFontSize);
            }
        }

        // Measuring Text
        BLRect calcTextPosition(const ByteSpan & txt, double x, double y)
        {
            BLPoint txtSize = textMeasure(txt);
            double cx = txtSize.x;
            double cy = txtSize.y;

            switch (fTextHAlignment)
            {
            case ALIGNMENT::LEFT:
                // do nothing
                // x = x;
                break;
            case ALIGNMENT::CENTER:
                x = x - (cx / 2);
                break;
            case ALIGNMENT::RIGHT:
                x = x - cx;
                break;

            default:
                break;
            }

            switch (fTextVAlignment)
            {
            case ALIGNMENT::TOP:
                y = y + cy - fFont.metrics().descent;
                break;
            case ALIGNMENT::CENTER:
                y = y + (cy / 2);
                break;

            case ALIGNMENT::MIDLINE:
                //should use the design metrics xheight
                break;

            case ALIGNMENT::BASELINE:
                // If what was passed as y is the baseline
                // do nothing to it because blend2d draws
                // text from baseline
                break;

            case ALIGNMENT::BOTTOM:
                // Adjust from the bottom as blend2d
                // prints from the baseline, so adjust
                // by the amount of the descent
                y = y - fFont.metrics().descent;
                break;

            default:
                break;
            }

            return { x, y, cx, cy };
        }

        
        virtual BLPoint textMeasure(const ByteSpan & txt) 
        {
            BLTextMetrics tm;
            BLGlyphBuffer gb;

			gb.setUtf8Text(txt.data(), txt.size());
            fFont.shape(gb);
            fFont.getTextMetrics(gb, tm);

            float cx = (float)(tm.boundingBox.x1 - tm.boundingBox.x0);
            float cy = fFont.size();


            return BLPoint( cx, cy );
        }
        
        virtual BLPoint textEmSize() {
            auto size = textMeasure("M");
            return size;
        }
        virtual float textAscent() { return fFont.metrics().ascent; }
        virtual float textDescent() { return fFont.metrics().descent; }

        // Text Sizing and positioning
        void setFontSize(const double size)
        {
            fFont.reset();
            fFont.createFromFace(fFontFace, (float)size);
        }
        
        virtual void textAlign(ALIGNMENT horizontal, ALIGNMENT vertical)
        {
            fTextHAlignment = horizontal;
            fTextVAlignment = vertical;
        }
        
        virtual void textSize(double size) {
            fFontSize = fontHandler()->getAdjustedFontSize((float)size);
            setFontSize(fFontSize);
        }
        
        virtual void textPosition(double x, double y) {
            fTextX = x;
            fTextY = y;
        }

        // Text Drawing
        virtual void text(const ByteSpan &txt) 
        {
            auto xy = calcTextPosition(txt, fTextX, fTextY);
            fTextAdvance = xy.w;

            text(txt, xy.x, xy.y);
        }
        
        virtual void text(const char* txt) {
            auto xy = calcTextPosition(txt, fTextX, fTextY);
            fTextAdvance = xy.w;

            text(txt, xy.x, xy.y);
        }
        
        virtual void text(const ByteSpan &txt, double x, double y)
        {
            // BUGBUG - Drawing order should be determined by 
            // the drawing order attribute
            BLContext::strokeUtf8Text(BLPoint(x, y), fFont, (char *)txt.data(), txt.size());
            BLContext::fillUtf8Text(BLPoint(x, y), fFont, (char *)txt.data(), txt.size());

            fTextX += fTextAdvance;
        }
        
        
        virtual void text(const char* txt, double x, double y) 
        {
            // BUGBUG - Drawing order should be determined by 
            // the drawing order attribute
            BLContext::strokeUtf8Text(BLPoint(x, y), fFont, txt);
            BLContext::fillUtf8Text(BLPoint(x, y), fFont, txt);
            
			fTextX += fTextAdvance;
        }

    };


}

