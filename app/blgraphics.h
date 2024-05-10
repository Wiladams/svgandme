#ifndef BLGraphics_h
#define BLGraphics_h

//#pragma once


    //Binding P5 graphics API to a blend2d context
    // we don't know what the blend2d context is attached to
    // so we make no assumptions about the API, particularly in 
    // how to get raw bits in and out.



#include "Graphics.h"
#include "fonthandler.h"


#include <vector>

namespace waavs
{
    class BLGraphics : public IGraphics
    {
    protected:
        int fCommandCount = 0;  // How many commands since last flush
        int fCommandThreshold = 256;
        
        
		FontHandler * fFontHandler=nullptr;
        BLContext fCtx{};
        BLImageData fImageData{};

        ANGLEMODE fAngleMode = ANGLEMODE::RADIANS;
        ELLIPSEMODE fEllipseMode = ELLIPSEMODE::CENTER;
        RECTMODE fRectMode = RECTMODE::CORNER;

        bool fUseFill = true;
        // Drawing Attributes
        double fDimensionScale = 1.0;

        // Typography
        BLFontFace fFontFace{};
        BLFont fFont{};
        double fFontSize = 12;
        ALIGNMENT fTextHAlignment = ALIGNMENT::LEFT;
        ALIGNMENT fTextVAlignment = ALIGNMENT::BASELINE;
        double fTextX{ 0 };
        double fTextY{ 0 };
        
        // Vertex shaping
        SHAPEMODE fShapeMode = SHAPEMODE::NONE;
        std::vector<BLPoint> fShapeVertices{};

    private:
        static BLEllipse calcEllipseParams(ELLIPSEMODE mode, double a, double b, double c, double d)
        {
            double cx = 0;
            double cy = 0;
            double rx = 0;
            double ry = 0;

            switch (mode) {
            case ELLIPSEMODE::CORNER:
                rx = c / 2;
                ry = d / 2;
                cx = a + rx;
                cy = b + ry;
                break;
            case ELLIPSEMODE::CORNERS:
                rx = (c - a) / 2;
                ry = (d - b) / 2;
                cx = a + rx;
                cy = b + ry;
                break;
            case ELLIPSEMODE::CENTER:
                rx = c;
                ry = d;
                cx = a;
                cy = b;
                break;
            case ELLIPSEMODE::RADIUS:
                cx = a;
                cy = b;
                rx = c;
                ry = d;
            }

            return { cx, cy, rx, ry };
        }

        static BLRect calcRectParams(RECTMODE mode, double  a, double b, double c, double d)
        {
            double x = 0;
            double y = 0;
            double w = 0;
            double h = 0;

            switch (mode) {
            case RECTMODE::CORNER:
                x = a;
                y = b;
                w = c;
                h = d;
                break;
            case RECTMODE::CORNERS:
                x = a;
                y = b;
                w = c - a;
                h = d - b;
                break;
            case RECTMODE::CENTER:
                x = a - (c / 2);
                y = b - (d / 2);
                w = c;
                h = d;
                break;
            }

            return { x, y, w, h };
        }


        // Here we are crudely doing a minimal amount necessary
        // to see some text.  

        rectf calcTextPosition(const char* txt, double x, double y, double x2, double y2)
        {
            vec2f txtSize = textMeasure(txt);
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

			return { (float)x, (float)y, (float)cx, (float)cy };
        }



        // Increment command count since last flush
    // We track number of commands since last flush so that
    // we can flush after a certain limit
        void incrCmd() {
            fCommandCount += 1;

            if (fCommandCount >= fCommandThreshold) {
                //flush();
                resetCommandCount();
            }
        }
        // Reset command count to zero since last flush
        void resetCommandCount() { fCommandCount = 0; }

    protected:
        // Create initial state
        void initContext(const BLContext& ctxt)
        {
            fCtx = ctxt;

            // white fill
            fill(Pixel(255, 255, 255, 255));

            // black stroke
            stroke(Pixel(0, 0, 0, 255));

            // Start with a default font so we can start drawing text
            //textFont("Consolas");
        }


    public:
        BLGraphics(const BLGraphics& other) = delete;

        BLGraphics(FontHandler *fontHandler)
            :fUseFill(true),
            fEllipseMode(ELLIPSEMODE::RADIUS),
            fRectMode(RECTMODE::CORNER),
            fAngleMode(ANGLEMODE::RADIANS)
            , fFontHandler(fontHandler)
        {
            initContext(BLContext());
        }

        virtual ~BLGraphics() = default;

		FontHandler * fontHandler() const { return fFontHandler; }
		void fontHandler(FontHandler * fh) { fFontHandler = fh; }
        
        BLContext& getBlend2dContext()
        {
            return fCtx;
        }

        // how many threads is the context using
        int threadCount() { return fCtx.threadCount(); }

        void setCommandThreshold(int maxCommands)
        {
            fCommandThreshold = maxCommands;
        }

        // Set the underlying pixels per inch 
        // as well as how many user space units per inch
        // doing this will allow us to specify all graphics
        // in some user specified units
        //
        // BUGBUG - this kinda works.  It will scale everything, which 
        // is not quite what we want.  What we really want is to just
        // add to our own transformation window for coordinate conversion


        void setDpiUnits(const int ppi, const float units) override
        {
            fDimensionScale = ppi / units;
            fCtx.scale(fDimensionScale);
            fCtx.userToMeta();
        }

		// This allows us to keep our stroke size in user space
		// units, but when we draw we will scale it to pixels
        // If you strokeBeforeTransform(true), the stroke will not scale
        // If you strokeBeforeTransform(false), the stroke will scale with the rest of the geometry
        virtual void strokeBeforeTransform(bool b) override
        {
            fCtx.setStrokeTransformOrder(b ? BL_STROKE_TRANSFORM_ORDER_BEFORE : BL_STROKE_TRANSFORM_ORDER_AFTER);
        }

        // Various Modes
        void angleMode(ANGLEMODE mode) override { fAngleMode = mode; }
        void ellipseMode(const ELLIPSEMODE mode) override { fEllipseMode = mode; }
        void rectMode(const RECTMODE mode) override { fRectMode = mode; }
        void blendMode(int op) override { fCtx.setCompOp((BLCompOp)op); }
        void globalOpacity(double opacity) override { fCtx.setGlobalAlpha(opacity); }

        // stroking attributes
        void strokeCap(int cap, int position) override { fCtx.setStrokeCap((BLStrokeCapPosition)position, (BLStrokeCap)cap); }
        void strokeCaps(int caps) override { fCtx.setStrokeCaps((BLStrokeCap)caps); }
        void strokeJoin(int style) override { fCtx.setStrokeJoin((BLStrokeJoin)style); }
        void strokeMiterLimit(double limit) override { fCtx.setStrokeMiterLimit(limit); }
        void strokeWidth(double weight) override { fCtx.setStrokeWidth(weight); }

        // Attribute State Stack
        bool push() override
        {   
            auto res = fCtx.save();
            return res == BL_SUCCESS;
        }

        bool pop() override
        {
            auto res = fCtx.restore();
            return res == BL_SUCCESS;
        }

        // Coordinate transformation
        void transform(const BLMatrix2D& t) override { fCtx.applyTransform(t); }
        //void transform(double* values) override { fCtx.transform(BLMatrix2D(values[0], values[1], values[2], values[3], values[4], values[5])); }
        void translate(double dx, double dy) override { fCtx.translate(dx, dy); }
        void scale(double sx, double sy) override { fCtx.scale(sx, sy); }
        void rotate(double angle, double cx, double cy) override { fCtx.rotate(angle, cx, cy); }


        // Pixel management
        void noFill() override { fUseFill = false; fCtx.setFillStyle(BLRgba32(0, 0, 0, 0)); }
        void fill(const BLVar& s) override { fUseFill = true; fCtx.setFillStyle(s); }
        void fill(const Pixel& c) override { fUseFill = true; fCtx.setFillStyle(c); }
        void fillOpacity(double opacity) override { fCtx.setFillAlpha(opacity); }
        void fillRule(int rule) override { fCtx.setFillRule((BLFillRule)rule); }

        void noStroke() override { fCtx.setStrokeStyle(BLRgba32(0, 0, 0, 0)); }
        void stroke(const BLVar& s) override { fCtx.setStrokeStyle(s); }
        void stroke(const Pixel& c) override { fCtx.setStrokeStyle(c); }
		void strokeOpacity(double opacity) override { fCtx.setStrokeAlpha(opacity); }

        // Synchronization
        // Wait for any outstanding drawing commands to be applied
        //
        bool flush() override
        {
            BLResult bResult = fCtx.flush(BL_CONTEXT_FLUSH_SYNC);
            if (bResult != BL_SUCCESS)
            {
                printf("BLGraphics.flush(), ERROR: %d\n", bResult);
            }

            resetCommandCount();

            return bResult == BL_SUCCESS;
        }

        void loadPixels()
        {
            // First, flush all queued commands
            flush();

            // The get a hold of the pixels from the image
            // if one exists
            BLImage* img = fCtx.targetImage();
            if (nullptr == img)
                return;

            // We have the image, so get the image data so 
            // we can get a pointer to the pixel data
            BLResult err = img->getData(&fImageData);
            if (BL_SUCCESS != err)
                return;
        }

        void updatePixels()
        {
            flush();
            fImageData.reset();
        }

        // Background management
        void clear() override
        {
            //printf("BLGraphics.clear\n");
            fCtx.save();
            fCtx.clearAll();
            fCtx.restore();
            flush();
        }

        void clearRect(double x, double y, double w, double h) override
        {
            fCtx.clearRect(x, y, w, h);
            incrCmd();
        }

        void background(const Pixel& c) override
        {
            fCtx.save();
            if (c.value == 0) {
                fCtx.clearAll();
            }
            else {
                fCtx.setFillStyle(c);
                fCtx.fillAll();
            }
            fCtx.restore();
        }

        // Clipping
        void clip(const rectf& c) override
        {
            auto sz = size(c);
            fCtx.clipToRect(BLRect(c.x, c.y, c.w, c.h));
        }

        void noClip() override { fCtx.restoreClipping(); }

        // Geometry
        // hard set a specfic pixel value
        void set(int x, int y, const Pixel& c) override
        {
            if (nullptr == fImageData.pixelData)
                return;

            ((BLRgba32*)(fImageData.pixelData))[(int)y * fImageData.size.w + (int)x] = c;
        }

        void point(double x, double y) override
        {
            line(x, y, x + 1, y);
        }

        void line(double x1, double y1, double x2, double y2) override
        {
            fCtx.strokeLine(x1, y1, x2, y2);

            incrCmd();
        }

        void arc(double cx, double cy, double r, double start, double sweep) override
        {
            fCtx.strokeArc(cx, cy, r, r, start, sweep);
            incrCmd();
        }

        void rect(const BLRect& rr)
        {
            fCtx.strokeRect(rr);
            fCtx.fillRect(rr);

            incrCmd();
        }

        void rect(double x, double y, double width, double height, double xradius, double yradius) override
        {
            fCtx.fillRoundRect(x, y, width, height, xradius, yradius);
            fCtx.strokeRoundRect(x, y, width, height, xradius, yradius);

            incrCmd();
        }


        void rect(double a, double b, double c, double d) override
        {
            BLRect params = BLGraphics::calcRectParams(fRectMode, a, b, c, d);
            rect(params.x, params.y, params.w, params.h, 1, 1);
        }

        void ellipse(double a, double b, double c, double d) override
        {
            BLEllipse params = BLGraphics::calcEllipseParams(fEllipseMode, a, b, c, d);

            if (fUseFill) {
                fCtx.fillEllipse(params);
            }

            fCtx.strokeEllipse(params);

            incrCmd();
        }

        void circle(double cx, double cy, double diameter) override { ellipse(cx, cy, diameter, diameter); }

        void triangle(double x1, double y1, double x2, double y2, double x3, double y3) override
        {
            BLTriangle tri = { x1,y1, x2,y2,x3,y3 };

            fCtx.strokeGeometry(BL_GEOMETRY_TYPE_TRIANGLE, &tri);

            if (fUseFill) {
                fCtx.fillGeometry(BL_GEOMETRY_TYPE_TRIANGLE, &tri);
            }

            incrCmd();
        }

        void bezier(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4) override
        {
            BLPath path;
            path.moveTo(x1, y1);
            path.cubicTo(x2, y2, x3, y3, x4, y4);
            fCtx.strokePath(path);

            incrCmd();
        }

        void polyline(const BLPoint* pts, size_t n) override
        {
            fCtx.strokePolyline(pts, n);
            incrCmd();
        }

        void polygon(const BLPoint* pts, size_t n) override
        {
            fCtx.strokePolygon(pts, n);

            if (fUseFill) {
                fCtx.fillPolygon(pts, n);
            }

            incrCmd();
        }

        void quad(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4) override
        {
            BLPoint pts[] = { {x1,y1},{x2,y2},{x3,y3},{x4,y4} };
            polygon(pts, 4);
            incrCmd();
        }

        void path(const BLPath& path) override
        {
            if (fUseFill) {
                fCtx.fillPath(path);
            }

            fCtx.strokePath(path);

            incrCmd();
        }

        // Bitmaps
        void image(const BLImage& img, int x, int y) override
        {
            //printf("BLGraphics::image(img, %d, %d, %dx%d)\n", x, y, img.width(), img.height());

            fCtx.blitImage(BLPointI(x, y), img);
            incrCmd();
        }

        void scaleImage(const BLImageCore& src,
            double srcX, double srcY, double srcWidth, double srcHeight,
            double dstX, double dstY, double dstWidth, double dstHeight) override
        {
            BLRect dst{ dstX,dstY,dstWidth,dstHeight };
            BLRectI srcArea{ (int)srcX,(int)srcY,(int)srcWidth,(int)srcHeight };

            fCtx.blitImage(dst, src, srcArea);
        }



        void textAlign(ALIGNMENT horizontal, ALIGNMENT vertical) override
        {
            fTextHAlignment = horizontal;
            fTextVAlignment = vertical;
        }

        void textFace(const BLFontFace& face) override
        {
            if (face.isValid()) {
                fFontFace = face;
                textSize(fFontSize);
            }
        }

        void setFontSize(const double size)
        {
            fFont.reset();
            fFont.createFromFace(fFontFace, (float)size);
        }

        void textFont(const char* fontname) override
        {
            // query the font manager
            // set the found face as the current face

            BLFontFace face;
            fontHandler()->queryFontFace(fontname, face);

            if (face.isValid()) {
                fFontFace = face;
                setFontSize(fFontSize);
            }
        }

        void textSize(double size) override
        {
            fFontSize = fontHandler()->getAdjustedFontSize((float)size);
            setFontSize(fFontSize);
        }

		vec2f textEmSize() override
		{
			vec2f size = textMeasure("M");
            return size;
		}
        
        // Get the width (in font units) of the text
        // BUGBUG - This is NOT in the base interface.  
        // maybe it should be?
        vec2f textMeasure(const char* txt) override
        {
            BLTextMetrics tm;
            BLGlyphBuffer gb;

            gb.setUtf8Text(txt);
            fFont.shape(gb);
            fFont.getTextMetrics(gb, tm);

            float cx = (float)(tm.boundingBox.x1 - tm.boundingBox.x0);
            float cy = fFont.size();


            return { cx, cy };
        }

        float textAscent() override { return fFont.metrics().ascent; }
        float textDescent() override { return fFont.metrics().descent; }

        void textAtBaseline(const char* txt, double x, double y, double x2 = 0, double y2 = 0) override
        {
            fCtx.fillUtf8Text(BLPoint(x, y), fFont, txt);
            fCtx.strokeUtf8Text(BLPoint(x, y), fFont, txt);
        }

        void textPosition(double x, double y) override
        {
			fTextX = x;
			fTextY = y;
        }

		void text(const char* txt) override
		{
			text(txt, fTextX, fTextY);
		}
        
        void text(const char* txt, double x, double y, double x2 = 0, double y2 = 0) override
        {
            auto xy = calcTextPosition(txt, x, y, x2, y2);
            //printf("text: (%3.2f,%3.2f) => (%3.2f,%3.2f)\n", x, y, xy.x, xy.y);

            fCtx.fillUtf8Text(BLPoint(xy.x, xy.y), fFont, txt);

            fCtx.strokeUtf8Text(BLPoint(xy.x, xy.y), fFont, txt);

			// BUGBUG - increment textX, textY
			fTextX += xy.w + textEmSize().x/2;
            //fTextY += xy.h;
            
            // there are proably a whole lot more 'commands'
            // generated when rendering text, but we have no 
            // way of knowing how many.
            incrCmd();
        }


        // Vertex shaping
        void beginShape(SHAPEMODE shapeKind = SHAPEMODE::OPEN) override
        {
            // To begin a shape, clear out existing shape
            // and set the shape mode to that specified
            fShapeMode = shapeKind;
            fShapeVertices.clear();
        }

        void vertex(double x, double y) override
        {
            // Make sure beginShape() has been called
            if (fShapeMode == SHAPEMODE::NONE)
                return;

            fShapeVertices.push_back({ x,y });
        }

        // endKind - SHAPEEND
        void endShape(SHAPEEND endKind) override
        {
            // Sanity check to ensure we're in a shape
            if (fShapeMode == SHAPEMODE::NONE)
                return;

            push();

            switch (fShapeMode) {
            case SHAPEMODE::OPEN: {
                BLPath path;
                if (endKind == SHAPEEND::CLOSE) {
                    polygon(fShapeVertices.data(), fShapeVertices.size());
                }
                else if (endKind == SHAPEEND::STROKE) {
                    if (fUseFill) {
                        fCtx.fillPolygon(fShapeVertices.data(), fShapeVertices.size());
                    }

                    polyline(fShapeVertices.data(), fShapeVertices.size());
                }
                break;
            }
            case SHAPEMODE::POINTS: {
                for (size_t i = 1; i <= fShapeVertices.size(); i++) {
                    point(fShapeVertices[i - 1].x, fShapeVertices[i - 1].y);
                }
            }
                                  break;
            case SHAPEMODE::LINES: {
                for (size_t i = 0; i < fShapeVertices.size(); i += 2) {
                    BLPoint p1 = fShapeVertices[i];
                    BLPoint p2 = fShapeVertices[i + 1];
                    line(p1.x, p1.y, p2.x, p2.y);
                }
            }
                                 break;
            case SHAPEMODE::TRIANGLES: {
                // consume 3 points at a time doing triangles
                for (size_t i = 0; i < fShapeVertices.size(); i += 3) {
                    BLPoint p1 = fShapeVertices[i];
                    BLPoint p2 = fShapeVertices[i + 1];
                    BLPoint p3 = fShapeVertices[i + 2];
                    triangle(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
                }
            }
                                     break;

            case SHAPEMODE::TRIANGLE_STRIP: {
                // consume 3 points at a time doing triangles
                for (size_t i = 0; i < fShapeVertices.size() - 2; i += 1) {
                    BLPoint p0 = fShapeVertices[i + 0];
                    BLPoint p1 = fShapeVertices[i + 1];
                    BLPoint p2 = fShapeVertices[i + 2];
                    triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y);
                }
            }
                                          break;

            case SHAPEMODE::TRIANGLE_FAN: {
                // consume 3 points at a time doing triangles
                // get the first point
                BLPoint p0 = fShapeVertices[0];
                for (size_t i = 1; i < fShapeVertices.size() - 1; i += 1) {
                    BLPoint p1 = fShapeVertices[i];
                    BLPoint p2 = fShapeVertices[i + 1];
                    triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y);
                }
            }
                                        break;

            case SHAPEMODE::QUADS: {
                // consume 4 points at a time doing quads
                for (size_t i = 0; i < fShapeVertices.size(); i += 4) {
                    BLPoint p1 = fShapeVertices[i];
                    BLPoint p2 = fShapeVertices[i + 1];
                    BLPoint p3 = fShapeVertices[i + 2];
                    BLPoint p4 = fShapeVertices[i + 3];
                    quad(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y);
                }
            }
                                 break;

            case SHAPEMODE::QUAD_STRIP: {
                // consume 4 points at a time doing quads
                for (size_t i = 0; i < fShapeVertices.size() - 2; i += 2) {
                    BLPoint p1 = fShapeVertices[i + 0];
                    BLPoint p2 = fShapeVertices[i + 1];
                    BLPoint p3 = fShapeVertices[i + 2];
                    BLPoint p4 = fShapeVertices[i + 3];
                    quad(p1.x, p1.y, p2.x, p2.y, p4.x, p4.y, p3.x, p3.y);
                }
            }
                                      break;

            case SHAPEMODE::NONE:
                // do nothing
                break;
            }

            pop();
        }
    };
}

#endif  // BLGraphics_h
