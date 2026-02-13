#pragma once

#include <functional>

#include "svgstructuretypes.h"
#include "viewport.h"

namespace waavs
{
    //====================================
    // SVGUseNode
    // https://www.w3.org/TR/SVG11/struct.html#UseElement
    // <use>
    //====================================
    struct SVGUseElement : public SVGGraphicsElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("use", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGUseElement>(groot);
                node->loadFromXmlElement(elem, groot);
                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("use",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGUseElement>(groot);
                    node->loadFromXmlPull(iter, groot);
                    return node;
                });


            registerSingularNode();
        }


        // Resolved state
        std::shared_ptr<IViewable> fTarget{ nullptr };
        BLRect fPlacedRect{};     // {x,y,w,h} in parent user space
        bool   fHasPlacedRect{ false };

        // Document state
        ByteSpan fHref{};

        SVGLengthValue fX{};
        SVGLengthValue fY{};
        SVGLengthValue fW{};
        SVGLengthValue fH{};



        SVGUseElement(IAmGroot*)
            : SVGGraphicsElement() 
        {
        }
        SVGUseElement(const SVGUseElement& other) = delete;

        void fixupSelfStyleAttributes(IAmGroot*) override
        {
            fX = parseLengthAttr(getAttributeByName("x"));
            fY = parseLengthAttr(getAttributeByName("y"));
            fW = parseLengthAttr(getAttributeByName("width"));
            fH = parseLengthAttr(getAttributeByName("height"));

            ByteSpan href = getAttribute(svgattr::xlink_href());
            if (href.empty())
                href = getAttributeByName(svgattr::href());

            fHref = chunk_trim(href, chrWspChars);
        }




        void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            fTarget.reset();
            fHasPlacedRect = false;
            fPlacedRect = BLRect{};

            if (fHref && groot)
                fTarget = groot->findNodeByHref(fHref);

            if (!fTarget)
                return;

            const double dpi = groot ? groot->dpi() : 96.0;
            const BLFont* fontOpt = &ctx->getFont();

            const BLRect vp = ctx->viewport();
            if (vp.w <= 0.0 || vp.h <= 0.0)
                return;

            // Resolve x/y relative to viewport, then offset by vp.x/y.
            LengthResolveCtx cx = makeLengthCtxUser(vp.w, 0.0, dpi, fontOpt);
            LengthResolveCtx cy = makeLengthCtxUser(vp.h, 0.0, dpi, fontOpt);

            // Defaults: x/y=0. width/height are "not set" unless provided.
            const double x = resolveLengthOr(fX, cx, 0.0);
            const double y = resolveLengthOr(fY, cy, 0.0);

            // For width/height:
            // - for symbol instantiation: needed to form instance viewport
            // - for non-symbol targets: often ignored, but we can still use it as a viewport if set.
            double w = 0.0;
            double h = 0.0;
            bool hasWH = false;

            if (fW.isSet() && fH.isSet())
            {
                w = resolveLengthUserUnits(fW, cx);
                h = resolveLengthUserUnits(fH, cy);
                if (w < 0.0) w = 0.0;
                if (h < 0.0) h = 0.0;
                hasWH = (w > 0.0 && h > 0.0);
            }

            fPlacedRect = BLRect{ x, y, hasWH ? w : 0.0, hasWH ? h : 0.0 };
            fHasPlacedRect = true;
        }

        void update(IAmGroot* groot) override
        {
            if (!fTarget)
                return;

            fTarget->update(groot);
        }


        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            if (!fTarget)
                return;

            const BLRect vp = ctx->viewport();
            if (vp.w <= 0.0 || vp.h <= 0.0)
                return;

            // If we never bound (or reflow), resolve now in a minimal way.
            if (!fHasPlacedRect)
            {
                const double dpi = groot ? groot->dpi() : 96.0;
                const BLFont* fontOpt = &ctx->getFont();
                LengthResolveCtx cx = makeLengthCtxUser(vp.w, 0.0, dpi, fontOpt);
                LengthResolveCtx cy = makeLengthCtxUser(vp.h, 0.0, dpi, fontOpt);

                const double x = resolveLengthOr(fX, cx, 0.0);
                const double y = resolveLengthOr(fY, cy, 0.0);

                double w = 0.0, h = 0.0;
                bool hasWH = false;
                if (fW.isSet() && fH.isSet())
                {
                    w = resolveLengthUserUnits(fW, cx);
                    h = resolveLengthUserUnits(fH, cy);
                    if (w < 0.0) w = 0.0;
                    if (h < 0.0) h = 0.0;
                    hasWH = (w > 0.0 && h > 0.0);
                }

                fPlacedRect = BLRect{ x, y, hasWH ? w : 0.0, hasWH ? h : 0.0 };
                fHasPlacedRect = true;
            }

            ctx->push();

            // Move to x/y in parent user space.
            ctx->translate(fPlacedRect.x, fPlacedRect.y);

            // Establish instance viewport for the referenced content (local coords).
            // This is CRUCIAL for <symbol>. For other targets it’s usually harmless.
            if (fPlacedRect.w > 0.0 && fPlacedRect.h > 0.0)
                ctx->setViewport(BLRect{ 0.0, 0.0, fPlacedRect.w, fPlacedRect.h });

            fTarget->draw(ctx, groot);

            ctx->pop();
        }

        /*
        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            if (fWrappedNode == nullptr)
                return;

            // set local size, if width and height were set
            // we don't want to do scaling here, because the
            // wrapped graphic might want to do something different
            // really it applies to symbols, and they're do their
            // own scaling.
            // width and height only apply when the wrapped graphic
            // is a symbol.  So, we should do that when we lookup the node
            //if (fDimWidth.isSet() && fDimHeight.isSet())
            //{
            //	ctx->localFrame(BLRect{ x,y,width,height });
            //}


            ctx->push();
            ctx->translate(fBoundingBox.x, fBoundingBox.y);

            // Draw the wrapped graphic
            if (fBoundingBox.w > 0 && fBoundingBox.h > 0) {
                ctx->setObjectFrame(fBoundingBox);
                ctx->setViewport(BLRect{ 0,0,fBoundingBox.w, fBoundingBox.h });
            }

            fWrappedNode->draw(ctx, groot);

            ctx->pop();
        }
        */
    };
}

