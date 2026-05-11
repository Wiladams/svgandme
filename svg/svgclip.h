#pragma once


#include <functional>
#include <unordered_map>

#include "svgb2ddriver.h"
#include "svgattributes.h"
#include "svggraphicselement.h"
#include "pixeling_clip.h"

namespace waavs 
{

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


        SpaceUnitsKind fClipPathUnits{ SpaceUnitsKind::SVG_SPACE_USER };

        // Instance Constructor
        SVGClipPathElement(IAmGroot* )
            : SVGGraphicsElement()
        {
            setIsVisible(false);
        }

        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            ByteSpan clipUnitsAttr{};
            fAttributes.getValue(svgattr::clipPathUnits(), clipUnitsAttr);
            if (clipUnitsAttr)
            {
                InternedKey clipUnitsKey = PSNameTable::INTERN(clipUnitsAttr);
                if (clipUnitsKey == svgval::userSpaceOnUse())
                    fClipPathUnits = SpaceUnitsKind::SVG_SPACE_USER;
                else if (clipUnitsKey == svgval::objectBoundingBox())
                    fClipPathUnits = SpaceUnitsKind::SVG_SPACE_OBJECT;
            }
        }

        bool renderClipSurface(IRenderSVG *ctx, IAmGroot* groot,
            const IsolatedRenderPlan& plan,
            Surface& outMask) noexcept
        {
            if (!groot)
                return false;

            WGMatrix3x3 maskCtm = plan.ctm;

            if (fClipPathUnits == SpaceUnitsKind::SVG_SPACE_OBJECT)
            {
                WGMatrix3x3 obb{};
                obb.translate(plan.objectBBoxUS.x, plan.objectBBoxUS.y);
                obb.scale(plan.objectBBoxUS.w, plan.objectBBoxUS.h);

                maskCtm.postTransform(obb);
            }

            IsolatedSubtreeRequest req{};
            req.userRect = plan.effectRectUS;
            req.pixelRect = plan.pixelRect;
            req.ctm = maskCtm;
            req.objectBBoxUS = plan.objectBBoxUS;
            req.renderMode = RF_Content;
            req.clear = true;

            return renderSubtreeToSurface(groot, this, req, outMask);
        }


        bool applyClipToSurface(
            IRenderSVG *ctx,
            IAmGroot* groot,
            const IsolatedRenderPlan& plan,
            Surface& result) noexcept override
        {
            if (!groot || result.empty())
                return false;

            Surface clipSurface{};
            if (!renderClipSurface(ctx, groot, plan, clipSurface)) {
                //result.clear();
                return false;
            }

            Surface_ARGB32 maskView = clipSurface.info();
            Surface_ARGB32 resultView = result.info();
            wg_surface_clip(resultView, maskView);

            return true;
        }
    };
}