#pragma once

#include <functional>

#include "blend2d.h"
#include "fonthandler.h"
#include "collections.h"
#include "svgenums.h"
#include "imanagesvgstate.h"
#include "irendersvg.h"
#include "pathcmdprinter.h"
#include "b2dpath.h"

namespace waavs
{

    // A specialization of state management, connected to a BLContext
    // This is used when rendering a tree of SVG elements
    struct SVGAPIPrinter : public IRenderSVG 
    {
        int fStyleCounter{ 1 };

        // Some helper routines
        static void writeTransform(const BLMatrix2D value)
        {
            printf("BLMatrix2D(%g,%g,%g,%g,%g,%g)",
                value.m00, value.m01,
                value.m10, value.m11,
                value.m20, value.m21);
        }

        static void writeRect(const BLRect& rect)
        {
            printf("BLRect(%f,%f,%f,%f)", rect.x, rect.y, rect.w, rect.h);
        }

        // write a style to be embedded
        // Really, this only works for RGBA styles
        // all others should probably be printStyle()
        void writeStyle(const BLVar& style)
        {
            // BLObjectType
            switch (style.type())
            {
            case BLObjectType::BL_OBJECT_TYPE_RGBA32:
            {
                BLRgba32 value = style.as<BLRgba32>();
                printf("BLRgba32(%d,%d,%d,%d)", value.r(), value.g(), value.b(), value.a());
            }
            break;
			case BLObjectType::BL_OBJECT_TYPE_GRADIENT:
			{
				BLGradient value = style.as<BLGradient>();
				printStyleGradient(value, "gradient");
			}
            default:
				printf("NYI Style: %d\n", style.type());
                break;
            }
        }

        bool printStyleGradient(const BLGradient& agrad, const char *name="gradient")
        {
            if (agrad.empty())
                return false;

            switch (agrad.type())
            {
            case BLGradientType::BL_GRADIENT_TYPE_LINEAR:
            {
                const BLLinearGradientValues& values = agrad.linear();
                printf("BLGradient %s(BLLinearGradientValues(%f,%f,%f,%f), (BLExtendMode)%d);\n",
                    name, values.x0, values.y0, values.x1, values.y1,
                    agrad.extendMode());

            }
            break;

            case BLGradientType::BL_GRADIENT_TYPE_RADIAL:
            {
                const BLRadialGradientValues& values = agrad.radial();
                printf("BLGradient %s(BLRadialGradientValues(%f,%f,%f,%f,%f,%f));\n",
                    name, 
                    values.x0, values.y0, values.x1, values.y1, values.r0, values.r1);

            }
            break;

            case BLGradientType::BL_GRADIENT_TYPE_CONIC:
            {
                const BLConicGradientValues& values = agrad.conic();
                printf("BLGradient %s(BLConicGradientValues(%g,%g,%g,%g));",
                    values.x0, values.y0, values.angle, values.repeat);
            }
            }

            // enumerate the source stops, and print them out, adding 
            // to our gradient
            auto stops = agrad.stopsView();
            for (size_t i = 0; i < stops.size; i++)
            {
                printf("%s.addStop(%g,BLRgba64(%d,%d,%d,%d));\n", name,
                    stops[i].offset,
                    stops[i].rgba.r(), stops[i].rgba.g(), stops[i].rgba.b(), stops[i].rgba.a());
            }

            // print the gradient transform if it exists, and isn't just
            // the identity transform
            if (agrad.hasTransform())
            {
                printf("%s.setTransform(", name);
                writeTransform(agrad.transform());
                printf(");\n");
            }

            return true;
        }

        void printStyle(const BLVar& style, const char *name="style")
        {
            // BLObjectType
            switch (style.type())
            {
                case BLObjectType::BL_OBJECT_TYPE_RGBA32:
                {
                    const BLRgba32 &value = style.as<BLRgba32>();
                    printf("BLRgba32 %s(%d,%d,%d,%d);\n", name,
                        value.r(), value.g(), value.b(), value.a());
                }
                break;

                case BLObjectType::BL_OBJECT_TYPE_GRADIENT:
                {
                    const BLGradient &value = style.as<BLGradient>();
                    printStyleGradient(value, name);
                }
                break;

                default:
                {
                    printf("// NYI Style: %d\n", style.type());
                }
            }
        }



        void printPathCommands(const BLPath& path, const char* pathName = "path") {
            BLPathView view = path.view();
            const uint8_t* commands = view.commandData;
            const BLPoint* points = view.vertexData;
            size_t count = view.size;

            printf("BLPath %s;\n", pathName);
            uint8_t lastCmd = 255;
			BLPoint lastPoint = { 0, 0 };

            for (size_t i = 0; i < count; ++i) {
                switch (commands[i]) {
                case BL_PATH_CMD_MOVE:
					//if (lastCmd != BL_PATH_CMD_MOVE)
                        printf("%s.moveTo(%g, %g);\n", pathName, points[i].x, points[i].y);
                    //else
                    //    printf("%s.lineTo(%g, %g);\n", pathName, lastPoint.x+points[i].x, lastPoint.y+points[i].y);
                    break;
                case BL_PATH_CMD_ON:
                    printf("%s.lineTo(%g, %g);\n", pathName, points[i].x, points[i].y);
                    break;
                case BL_PATH_CMD_QUAD:
                    if (i + 1 < count) {
                        printf("%s.quadTo(%g, %g, %g, %g);\n",
                            pathName, points[i].x, points[i].y,
                            points[i + 1].x, points[i + 1].y);
                        i++; // Skip the next point
                    }
                    break;
                case BL_PATH_CMD_CUBIC:
                    if (i + 2 < count) {
                        printf("%s.cubicTo(%g, %g, %g, %g, %g, %g);\n",
                            pathName, points[i].x, points[i].y,
                            points[i + 1].x, points[i + 1].y,
                            points[i + 2].x, points[i + 2].y);
                        i += 2; // Skip the next two points
                    }
                    break;
                case BL_PATH_CMD_CLOSE:
                    printf("%s.close();\n", pathName);
                    break;
                default:
                    printf("// Unknown command: %d\n", commands[i]);
                    break;
                }

                lastCmd = commands[i];
				lastPoint = points[i];
            }
        }


    public:
        SVGAPIPrinter() = default;



        virtual ~SVGAPIPrinter() = default;


        void onAttach(BLImageCore& image, const BLContextCreateInfo* createInfo) override
        {
            printf("//ctx.begin();\n");
        }

        void onDetach() override
        {
            printf("ctx.end();\n");
        }

        void onResetFont() override
        {}


        void onPush() override
        {
            printf("{\n");
            printf("ctx.save();\n");
        }

        void onPop() override
        {
            printf("ctx.restore();\n");
            printf("}\n");
        }

        void onFlush() override
        {
            printf("ctx.flush(BL_CONTEXT_FLUSH_SYNC);\n");
        }

        // Canvas management
        void onClear() override
        {
            printf("ctx.clearAll();\n");
        }

        // Call this before each frame to be drawn
        void onRenew() override
        {
            printf("// renew();");
            printf("ctx.clearAll();\n");

            // If a background paint is set, use it
            if (!getBackgroundPaint().isNull())
            {
                printf("ctx.fillAll(bg);\n");
            }

        }

        void onApplyTransform(const BLMatrix2D& value) override
        {
            printf("ctx.applyTransform(");
            writeTransform(value);
            printf("); \n");
        }

        void onTransform(const BLMatrix2D& value) override
        {
            printf("ctx.setTransform(");
            writeTransform(value);
            printf("); \n");
        }

        void onRotate(double angle, double cx, double cy) override
        {
            printf("/*\n");
            writeTransform(getTransform());
            printf("\n*/\n");

            printf("ctx.rotate(%g, %g, %g);\n", angle, cx, cy);
        }

        void onScale(double sx, double sy) override 
        {
            printf("/*\n");
            writeTransform(getTransform());
            printf("\n*/\n");

            printf("ctx.scale(%g,%g);\n", sx, sy);

        }

        void onTranslate(double x, double y) override
        {
            printf("/*\n");
            writeTransform(getTransform());
            printf("\n*/\n");

            printf("ctx.translate(%g,%g);\n", x, y);
        }

        void onStrokeBeforeTransform() override
        {
            bool b = false;// getStrokeBeforeTransform();
            printf("ctx.setStrokeTransformOrder(%d));\n", b ? BL_STROKE_TRANSFORM_ORDER_BEFORE : BL_STROKE_TRANSFORM_ORDER_AFTER);
        }

        void onBlendMode() override
        {
            int mode = getCompositeMode();
            printf("ctx.setCompOp((BLCompOp)%d);\n", mode);
        }

        void onGlobalOpacity() override
        {
            double value = getGlobalOpacity();
            printf("ctx.setGlobalAlpha(%f);\n", value);
        }

        void onStrokeCap()
        {
            int kind = 0;
            int position = 0;
            printf("ctx.setStrokeCap(%d, (BLStrokeCap)%d);\n", position, kind);
        }

        void onStrokeCaps(BLStrokeCap caps) override
        {
            printf("ctx.setStrokeCaps((BLStrokeCap)%d);", caps);
        }

        void onStrokeWidth() override
        {
            double width = getStrokeWidth();
            printf("ctx.setStrokeWidth(%f);\n", width);
        }


        void onLineJoin() override
        {
            auto value = getLineJoin();
            printf("ctx.setStrokeJoin((BLStrokeJoin)%d);\n", value);
        }

        void onStrokeMiterLimit() override
        {
            auto value = getStrokeMiterLimit();
            printf("ctx.setStrokeMiterLimit(%f);\n", value);
        }


        // paint for filling shapes
        void onFill() override
        {
            auto paint = getFillPaint();
            if (paint.isNull())
                printf("ctx.disableFillStyle();\n");
            else {
                printStyle(paint, "fillStyle");

                printf("ctx.setFillStyle(fillStyle);\n");
            }
        }


        void onFillOpacity() override
        {
            auto value = getFillOpacity();
            printf("ctx.setFillAlpha(%f);\n", value);
        }



        // Geometry
        // hard set a specfic pixel value
        void onFillRule() override
        {
            auto value = getFillRule();
            printf("ctx.setFillRule((BLFillRule)%d);\n", value);
        }

        // paint for stroking lines
        void onStroke()
        {
            auto paint = getStrokePaint();
            if (paint.isNull())
                printf("ctx.disableStrokeStyle();\n");
            else {
                printStyle(paint, "strokeStyle");

                printf("ctx.setStrokeStyle(strokeStyle);\n");
            }
        }

        virtual void onStrokeOpacity()
        {
            auto value = getStrokeOpacity();
			printf("ctx.setStrokeAlpha(%f);\n", value);
        }



        // Set a background that will be used
        // to fill the canvas before any drawing
        void onBackground() override
        {
            printf("// setBackground\n");
        }


        // Typography
        void onTextCursor() override
        {
            auto value = getTextCursor();
            printf("// textCursor(%f,%f);\n", value.x, value.y);
        }


        // BUGBUG - this should become a part of the state management
        void onFillMask() override
        {
            printf("// ctx.fillMask();\n");
        }



        // Clipping
        void onClipRect() override
        {
            auto value = getClipRect();
            printf("ctx.clipToRect(");
			writeRect(value);
			printf(");\n");
        }

        void onNoClip() override
        {
            printf("ctx.restoreClipping();\n");
        }

        // Drawing Shapes
        void onBeginDrawShape(const BLPath& apath) override 
        {
            printf("{\n");

            printPathCommands(apath, "apath");

        }

		void onEndDrawShape() override
		{
			printf("}\n");
		}

        void onStrokeShape(const BLPath &apath) override
        {
            //printf("{\n");
            //printPathCommands(apath, "apath");

            printf("ctx.strokePath(apath);\n");

            //printf("}\n");
        }

        void onFillShape(const BLPath& apath) override
        {
            //printf("{\n");
            //printPathCommands(apath, "apath");

            printf("ctx.fillPath(apath);\n");

            //printf("}\n");
        }

        // This is a general shape drawing.
        // It can handle the order of drawing, as well
        // as do isolated drawing (stroke, or fill only)
        void onDrawShape(const BLPath& aPath) override
        {
            // Get the paint order from the context
            uint32_t porder = getPaintOrder();

            onBeginDrawShape(aPath);

            for (int slot = 0; slot < 3; slot++)
            {
                uint32_t ins = porder & 0x03;	// get two lowest bits, which are a single instruction

                switch (ins)
                {
                case PaintOrderKind::SVG_PAINT_ORDER_FILL:
                    printf("ctx.fillPath(apath);\n");
                    break;

                case PaintOrderKind::SVG_PAINT_ORDER_STROKE:
                    printf("ctx.strokePath(apath);\n");
                    break;

                case PaintOrderKind::SVG_PAINT_ORDER_MARKERS:
                {
                    // We don't do markers at this level
                }
                break;
                }

                // move past current instruction, 
                // shift down to get the next one ready
                porder = porder >> 2;
            }

            onEndDrawShape();


        }


        // Bitmap drawing
        void onImage(const BLImage& img, double x, double y) override
        {
            // BUGBUG
            // should turn the image into a base64 embedded image
			printf("ctx.blitImage(BLPoint(%f, %f), img);", x, y);
        }

        void onScaleImage(const BLImage& src,
            int srcX, int srcY, int srcWidth, int srcHeight,
            double dstX, double dstY, double dstWidth, double dstHeight) override
        {

            BLRect dst{ dstX,dstY,dstWidth,dstHeight };
            BLRectI srcArea{ srcX,srcY,srcWidth,srcHeight };

            printf("ctx.blitImage(dst, src, srcArea);\n");
        }





        // Text Drawing
        void onStrokeText(const ByteSpan& txt, double x, double y) override
        {
            printf("ctx.strokeUtf8Text(BLPoint(x, y), getFont(), (char*)txt.data(), txt.size());\n");
        }

        void onFillText(const ByteSpan& txt, double x, double y) override
        {
            printf("ctx.fillUtf8Text(BLPoint(x, y), getFont(), (char*)txt.data(), txt.size());\n");
        }

        void onDrawText(const ByteSpan& txt, double x, double y) override
        {
            // use paintOrder to stroke and/or fill text
        }
    };

}

