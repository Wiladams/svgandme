#pragma once

#include <memory>
#include <vector>


#include "svgstructuretypes.h"

#include "screensnapshot.h"
#include "gmonitor.h"
#include "converters.h"


namespace waavs {
    //
    // DisplayCaptureElement
    // This is a source of paint, just like a SVGImage
    // It captures from the user's screen
    // Return value as variant(), and it can be used in all 
    // the same places a SVGImage can be used 
    //
    struct DisplayCaptureElement : public SVGGraphicsElement
    {
        static void registerFactory() {
            registerSVGSingularNodeByName("displayCapture",
                [](IAmGroot* groot, const XmlElement& elem) {
                    auto node = std::make_shared<DisplayCaptureElement>(groot);
                    node->loadFromXmlElement(elem, groot);
                    return node;
                });
        }


        // Stuff that must be resolved at bindToContext time
        // 
        // The intended destination
        SVGLengthValue fDimX{};
        SVGLengthValue fDimY{};
        SVGLengthValue fDimWidth{};
        SVGLengthValue fDimHeight{};

        // Stuff that can be resolved at fixupStyle time
        ScreenSnapper fSnapper{};
        ByteSpan fSrcSpan{};

        // The parameters for the capture area
        // if the width or height are zero, then it
        // will capture nothing, and not draw
        int64_t fCapX = 0;
        int64_t fCapY = 0;
        int64_t fCapWidth = 0;
        int64_t fCapHeight = 0;
        bool fHasCapture = false;

        // Resolved location where we are drawing
        double fX{ 0 };
        double fY{ 0 };
        double fWidth{ 0 };
        double fHeight{ 0 };


        BLPattern fPatternForVariant{};



        DisplayCaptureElement(IAmGroot*)
            :SVGGraphicsElement() 
        {
        }

        int64_t capX() const { return fCapX; }
        int64_t capY() const { return fCapY; }
        int64_t capWidth() const { return fCapWidth; }
        int64_t capHeight() const { return fCapHeight; }


        const BLVar getVariant(IRenderSVG* ctx, IAmGroot* groot) noexcept override
        {
            if (fVar.isNull())
            {
                bindToContext(ctx, groot);

                fSnapper.update();
                fPatternForVariant.setImage(fSnapper.image());
                fVar = fPatternForVariant;
            }

            return fVar;
        }




        double getNumberOrPercent(InternedKey key, const double range, const double fallback) const noexcept
        {
            ByteSpan attr = getAttribute(key);
            if (attr)
            {
                SVGNumberOrPercent norp{};
                if (readSVGNumberOrPercent(attr, norp))
                {
                    if (norp.isPercent())
                        return norp.calculatedValue() * range;

                    return norp.calculatedValue();
                }
            }
            return fallback;
        }


        void fixupSelfStyleAttributes(IAmGroot*) override
        {
            fSrcSpan = getAttribute(svgattr::src());
            
            // If no 'src' then don't do any setup
            if (fSrcSpan.empty())
                return;

            // The capture area MUST be a number or percent.
            // if it's a percent, it's of the available capture area 
            // (e.g. the screen size, or the size of the display specified in src)
            // captureUnits - default to physical pixels
            // displayUnits - Object bounding box, user space, or physical pixels (default)
            // 
            // Setup a GraphicsDeviceContext so we can get the size
            // of the intended device.
            GraphicsDeviceContext device{};
            InternedKey deviceName = PSNameTable::INTERN(fSrcSpan);

            // If we could not find the device, so we can't capture
            // so just return so we won't draw anything.
            if (!device.resetByName(deviceName))
            {
                return;
            }

            
            fCapX = (int64_t)getNumberOrPercent(svgattr::capX(), device.pixelWidth(), 0);
            fCapY = (int64_t)getNumberOrPercent(svgattr::capY(), device.pixelHeight(), 0);
            fCapWidth = (int64_t)getNumberOrPercent(svgattr::capWidth(), device.pixelWidth(), 0);
            fCapHeight = (int64_t)getNumberOrPercent(svgattr::capHeight(), device.pixelHeight(), 0);

        }

        void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            // We need to resolve the size of the user space
            // start out with some information from groot
            double dpi = groot ? groot->dpi() : 96.0;
            const BLFont* fontOpt = groot ? &ctx->getFont() : nullptr;



            // For now, we're just going to assume captureUnits == CaptureSpace
            //BLRect cFrame = ctx->viewport();
            double w = fCapWidth;
            double h = fCapHeight;

            // BUGBUG - need to get the dpi and canvas size to calculate these properly
            LengthResolveCtx wCtx = makeLengthCtxUser(w, 0, dpi, fontOpt);
            LengthResolveCtx hCtx = makeLengthCtxUser(h, 0, dpi, fontOpt);

            parseLengthValue(getAttribute(svgattr::x()), fDimX);
            parseLengthValue(getAttribute(svgattr::y()), fDimY);
            parseLengthValue(getAttribute(svgattr::width()), fDimWidth);
            parseLengthValue(getAttribute(svgattr::height()), fDimHeight);


            fX = resolveLengthOr(fDimX, wCtx, 0);
            fY = resolveLengthOr(fDimY, hCtx, 0);
            fWidth = resolveLengthOr(fDimWidth, wCtx, w);
            fHeight = resolveLengthOr(fDimHeight, hCtx, h);


            if (fSrcSpan && fCapWidth > 0.0 && fCapHeight > 0.0)
            {
                InternedKey displayKey = PSNameTable::INTERN(fSrcSpan);

                // If we can't reset the snapper, then we can't capture, 
                // so just return and don't draw anything
                // The reset could also fail if the device does not have
                // enough size the accomodate the request.
                if (!fSnapper.reset((int)fCapX, (int)fCapY, (int)fCapWidth, (int)fCapHeight, displayKey))
                    return ;

                fHasCapture = true;
            }

            if (!fDimWidth.isSet())
                fWidth = (double)fSnapper.width();
            if (!fDimHeight.isSet())
                fHeight = (double)fSnapper.height();

            fSnapper.update();
        }


        void updateSelf(IAmGroot*) override
        {
            fSnapper.update();
        }

        void drawSelf(IRenderSVG* ctx, IAmGroot*) override
        {
            if (!fHasCapture)
                return;

            int lWidth = (int)fSnapper.width();
            int lHeight = (int)fSnapper.height();

            ctx->scaleImage(this->fSnapper.image(), 0, 0, lWidth, lHeight, fX, fY, fWidth, fHeight);
            ctx->flush();
        }

    };
}
