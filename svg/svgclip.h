#pragma once




#include <functional>
#include <unordered_map>

#include "svgb2ddriver.h"
#include "svgattributes.h"
#include "svggraphicselement.h"

namespace waavs {

    //============================================================
    // SVGClipPath
    // Create a SVGSurface that we can draw into
    // get the size by asking for the extent of the enclosed
    // visuals.
    // 
    // At render time, use the clip path in a pattern and fill
    // based on that.
    // 
    // Reference:
    // http://www.w3.org/TR/SVG11/feature#Clip
    //
    //============================================================
    struct SVGClipPathElement : public SVGGraphicsElement
    {
        // Static constructor to register factory method in map
        static void registerFactory()
        {
            registerContainerNodeByName(svgtag::tag_clipPath(), [](IAmGroot * groot, XmlPull & iter) {
                auto node = std::make_shared<SVGClipPathElement>(groot);
                node->loadFromXmlPull(iter, groot);

                return node;
            });
            
        }

        Surface fSurface;		// Where we'll render the mask
        BLImage fMask;		// The actual mask we use for clipping

        // Instance Constructor
        SVGClipPathElement(IAmGroot* )
            : SVGGraphicsElement()
        {
            setIsVisible(false);
        }

        // 
        // BUGBUG - this needs to happen in resolvePaint, or bindToGroot()
        //
        const BLVar getVariant(IRenderSVG * ctx, IAmGroot *groot) noexcept override
        {
            if (!fVar.isNull())
                return fVar;

            // get our extent
            const WGRectD extent = getObjectBoundingBox(ctx, groot);

            // create a surface of that size
            // if it's valid
            if (extent.w > 0 && extent.h > 0)
            {
                fSurface.reset((int)floor((float)extent.w + 0.5f), (int)floor((float)extent.h + 0.5f));
                fMask = blImageFromSurface(fSurface);

                // Draw our content into the image
                {
                    SVGB2DDriver rctx;
                    rctx.attach(fSurface, 1);

                    WGRectD bbox = drawBegin(&rctx, groot);

                    rctx.blendMode(BL_COMP_OP_SRC_OVER);
                    rctx.clear();
                    rctx.fill(BLRgba32(0xffffffff));
                    rctx.translate(-extent.x, -extent.y);

                    drawContent(&rctx, groot);
                    drawEnd(&rctx, groot);

                    rctx.flush();
                    rctx.detach();
                }

                // bind our image to fVar for later retrieval
                fVar = fMask;

            }

            return fVar;
        }


    };
}