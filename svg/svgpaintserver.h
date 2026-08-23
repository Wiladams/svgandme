#pragma once


#include "svgstructuretypes.h"
#include "svgcolors.h"

namespace waavs
{


    //=====================================================
    // SVG Paint
    // 
    // General base class for paint.  
    // This is essentially the "paint server".  We parse
    // what the 'color' is, and if it's a reference to something
    // that reference's 'getVariant' is called at the right time
    // otherwise, we return the appropriate color value.
    // The primary usage here is 'fill' and 'stroke' attributes.
    // 'stop-color' and filter primitives handle their own colors
    // as they just use SVGColor instead, as there is no allowance
    // for 'paint server' style

    // There are a couple of cases when using 'currentColor' as the
    // color value.
    // 1) It is being used on an element, where there is also a 'color' attribute
    //   In this case, whatever is in the 'color' attribute needs to become our
    //   BLVar value, and set on the context at drawing time.
    // 2) It is being used on an element, where there is no 'color' attribute
    //   In this case, it is inheritance.  The color value is determined by what is
    //   currently on the 'currentColor()' property of the context, so we should use
    //   that when it comes time to draw.

    //=====================================================
    struct SVGPaint : public SVGVisualProperty, public IServePaint
    {
        ByteSpan fPaintReference{};
        SVGColor fColor;
        InternedKey fColorKey;
        InternedKey fOpacityKey;


        SVGPaint(InternedKey colorKey, InternedKey opacityKey)
            : SVGVisualProperty(nullptr)
            , fColor(colorKey, opacityKey)
        {
        }

        SVGPaint(const SVGPaint& other) = delete;

        const BLVar getVariant(IDrawGraphics* ctx, IAmGroot* groot) noexcept override
        {
            // if it's simply a color, then do a quick convert and return
            if (fColor.isColor())
            {
                BLRgba32 bColor{};
                bColor.value = Pixel_ARGB32_from_ColorSRGB(fColor.value());
                BLVar tmpVar{};
                tmpVar = bColor;

                return tmpVar;
            }
            else if (fColor.isNone())
            {
                return BLVar::null();
            }
            else if (fColor.isCurrent())
            {
                // if it's 'currentColor', then we need to look up 
                // the current color from the context.
                // we still need to apply our specific opacity if
                // it exists
                return ctx->state().getDefaultColor();
            }
            else if (fColor.isInherit())
            {
                // if it's 'inherit' treat it the same
                // as 'currentColor'
                // we'll keep it separate for now until we
                // confirm the semantic differences
                // this is a little more tricky, we want to retrieve
                // the inherited color, depends on whether we are
                // stroke or fill
                return ctx->state().getDefaultColor();
            }
            else if (fColor.isReference())
            {
                // we must have groot to do a lookup
                if (!groot)
                    return BLVar::null();

                auto node = groot->findNodeByUrl(fColor.rawValue());

                if (nullptr == node)
                    return BLVar::null();

                // dynamic cast to IServePaint, if it fails, then we can't use it as a paint server
                auto paintServer = dynamic_cast<IServePaint*>(node.get());
                if (paintServer == nullptr)
                    return BLVar::null();

                // assume calling getVariant on the referant node will
                // cause itself to do binding
                BLVar tmpVar = paintServer->getVariant(ctx, groot);
                return tmpVar;
            }

            // otherwise, use the variant we have calculated
            return BLVar::null();
        }


        // paint usually comes in color/opacity pairs
        // so use SVGColor to read it, if it's not a URL 
        // reference
        bool loadFromAttributes(const XmlAttributeCollection& attrs, IAmGroot* groot) override
        {
            if (!fColor.loadFromAttributes(attrs, groot))
                return false;

            set(true);

            return true;
        }



        void update(IAmGroot* groot) override
        {
            ByteSpan ref = rawValue();

            if (bspan_starts_with(ref, "url("))
            {
                if (groot != nullptr) {
                    auto node = groot->findNodeByUrl(ref);
                    if (nullptr != node)
                    {
                        node->update(groot);
                    }
                }
            }
        }


    };

    //
    //
    struct SVGFillPaint : public SVGPaint
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::fill(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGFillPaint>();
                node->loadFromAttributes(attrs, groot);
                return node;
                });
        }

        SVGFillPaint()
            : SVGPaint(svgattr::fill(), svgattr::fill_opacity())
        {
            setName(svgattr::fill());
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            ctx->fillPaintServer(this);
        }

    };

    struct SVGStrokePaint : public SVGPaint
    {
        static void registerFactory() {
            registerSVGAttributeByName(svgattr::stroke(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGStrokePaint>();
                node->loadFromAttributes(attrs, groot);
                return node;
                });
        }

        SVGStrokePaint()
            : SVGPaint(svgattr::stroke(), svgattr::stroke_opacity())
        {
            setName(svgattr::stroke());
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            ctx->strokePaintServer(this);
        }

    };


    struct SVGColorPaint : public SVGPaint
    {
        static void registerFactory() {
            registerSVGAttributeByName("color", [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGColorPaint>();
                node->loadFromAttributes(attrs, groot);
                return node;
                });
        }


        SVGColorPaint()
            : SVGPaint(svgattr::color(), nullptr) {
            setName(svgattr::color());
        }


        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            ctx->state().setDefaultColor(getVariant(ctx, groot));
        }

    };

}
