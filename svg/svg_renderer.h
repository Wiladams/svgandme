#pragma once

#include "svggraphicselement.h"
#include "pixeling_clip.h"
#include "svg_element_clip.h"
#include "render_blend2d.h"
#include "svgdocument.h"

namespace waavs
{
    struct SVGRenderer
    {
        bool draw(
            SVGDocument& document,
            IDrawGraphics& ctx,
            RenderFlags flags = RenderFeature::RF_All) noexcept
        {
            // Preserve the existing document-binding behavior.
            // This establishes the document portal and binds children that
            // still require context-dependent preparation.
            if (document.needsBinding())
                document.bindToContext(&ctx, &document);

            ctx.push();

            const WGRectD portalFrame{
                0.0,
                0.0,
                document.canvasWidth(),
                document.canvasHeight()
            };

            ctx.state().setViewport(portalFrame);
            ctx.state().setObjectFrame(portalFrame);

            // SVGDocument is a synthetic root container. Its actual renderable
            // content is contained in its render-node list, so it should not
            // enter the normal per-element drawBegin()/drawEnd() pipeline.
            const bool result = drawChildren(
                document,
                ctx,
                document,
                flags);

            ctx.pop();

            return result;
        }

        bool drawElement(
            SVGGraphicsElement& element,
            IDrawGraphics& ctx,
            IAmGroot& groot,
            RenderFlags flags) noexcept
        {
            const WGRectD bbox =
                element.drawBegin(&ctx, &groot);

            bool result = false;

            auto clipProp =
                element.getVisualPropertyAs<SVGClipPathAttribute>(
                    svgattr::clip_path());

            if (flags.has(RF_Clip) &&
                clipProp &&
                clipProp->referencedNode())
            {
                result = drawClipped(
                    element,
                    *clipProp,
                    bbox,
                    ctx,
                    groot,
                    flags);
            }
            else
            {
                result = drawDirect(
                    element,
                    ctx,
                    groot,
                    flags);
            }

            element.drawEnd(&ctx, &groot);
            return result;
        }


        bool drawChildren(
            SVGGraphicsElement& parent,
            IDrawGraphics& ctx,
            IAmGroot& groot,
            RenderFlags flags) noexcept
        {
            for (const auto& child : parent.renderNodes())
            {
                if (!child || !child->isVisible())
                    continue;

                if (auto graphics =
                    std::dynamic_pointer_cast<SVGGraphicsElement>(
                        child))
                {
                    if (!drawElement(
                        *graphics,
                        ctx,
                        groot,
                        flags))
                    {
                        return false;
                    }
                }
                else
                {
                    child->draw(
                        &ctx,
                        &groot,
                        flags);
                }
            }

            return true;
        }

        bool drawContent( SVGGraphicsElement& element,
            IDrawGraphics& ctx,
            IAmGroot& groot,
            RenderFlags flags) noexcept
        {
            element.drawSelf(&ctx, &groot);

            return drawChildren( element, ctx, groot, flags);
        }


        bool drawDirect(
            SVGGraphicsElement& element,
            IDrawGraphics& ctx,
            IAmGroot& groot,
            RenderFlags flags) noexcept
        {
            return drawContent(
                element,
                ctx,
                groot,
                flags);
        }

        bool drawIsolated(
            SVGGraphicsElement& element,
            IDrawGraphics& parentCtx,
            IAmGroot& groot,
            RenderFlags flags) noexcept
        {
            // Create temporary destination/context.

            return false;

            //return drawContent(
            //    element,
            //    temporaryCtx,
            //    groot,
            //    sourceFlags);
        }


        bool drawClipped(
            SVGGraphicsElement& element,
            const SVGClipPathAttribute& clipProperty,
            const WGRectD& objectBBoxUS,
            IDrawGraphics& parentCtx,
            IAmGroot& groot,
            RenderFlags flags) noexcept
        {
            auto clipElement =
                std::dynamic_pointer_cast<SVGClipPathElement>(
                    clipProperty.referencedNode());

            // Invalid or wrongly typed reference:
            // preserve the current forgiving behavior and draw normally.
            if (!clipElement)
            {
                return drawDirect(
                    element,
                    parentCtx,
                    groot,
                    flags);
            }

            // An empty object contributes no pixels. This is a valid no-op,
            // rather than a renderer failure.
            if (!(objectBBoxUS.w > 0.0) ||
                !(objectBBoxUS.h > 0.0))
            {
                return true;
            }

            const WGMatrix3x3 parentCtm =
                parentCtx.state().getTransform();

            const WGRectD deviceRect =
                mapRectAABB(parentCtm, objectBBoxUS);

            if (!(deviceRect.w > 0.0) ||
                !(deviceRect.h > 0.0))
            {
                return true;
            }

            const int x0 =
                static_cast<int>(std::floor(deviceRect.x));

            const int y0 =
                static_cast<int>(std::floor(deviceRect.y));

            const int x1 =
                static_cast<int>(
                    std::ceil(deviceRect.x + deviceRect.w));

            const int y1 =
                static_cast<int>(
                    std::ceil(deviceRect.y + deviceRect.h));

            if (x1 <= x0 || y1 <= y0)
                return true;

            const WGRectI pixelRect{
                x0,
                y0,
                x1 - x0,
                y1 - y0
            };

            Surface result =
                Surface::createOwned(
                    pixelRect.w,
                    pixelRect.h);

            Surface clipSurface =
                Surface::createOwned(
                    pixelRect.w,
                    pixelRect.h);

            if (result.empty() || clipSurface.empty())
                return false;

            // Convert the current user-to-parent-destination CTM into
            // a user-to-local-offscreen-surface CTM.
            WGMatrix3x3 userToSurface = parentCtm;

            userToSurface.translate(
                -double(pixelRect.x),
                -double(pixelRect.y));

            // Retain the logical SVG drawing state established by
            // element.drawBegin(). Each offscreen renderer receives its
            // own copy.
            const SVGDrawingState sourceState =
                parentCtx.state();

            // --------------------------------------------------------
            // Render the element's unclipped source content.
            // --------------------------------------------------------

            {
                SVGB2DDriver sourceCtx{};

                sourceCtx.attach(
                    result,
                    1,
                    &sourceState);

                sourceCtx.clear();
                sourceCtx.setTransform(userToSurface);

                sourceCtx.state().setObjectFrame(
                    objectBBoxUS);

                // Do not replace the inherited viewport with the object
                // bounding box. Isolation changes the destination, not
                // the logical SVG viewport.

                const bool rendered =
                    drawContent(
                        element,
                        sourceCtx,
                        groot,
                        flags);

                sourceCtx.detach();

                if (!rendered)
                    return false;
            }

            // --------------------------------------------------------
            // Render the clip-path content into an aligned surface.
            // --------------------------------------------------------

            WGMatrix3x3 clipToSurface =
                userToSurface;

            if (clipElement->clipPathUnits() ==
                SpaceUnitsKind::SVG_SPACE_OBJECT)
            {
                WGMatrix3x3 objectBBoxTransform =
                    WGMatrix3x3::makeIdentity();

                objectBBoxTransform.translate(
                    objectBBoxUS.x,
                    objectBBoxUS.y);

                objectBBoxTransform.scale(
                    objectBBoxUS.w,
                    objectBBoxUS.h);

                clipToSurface.transform(
                    objectBBoxTransform);
            }

            {
                SVGB2DDriver clipCtx{};

                clipCtx.attach(
                    clipSurface,
                    1,
                    &sourceState);

                clipCtx.clear();
                clipCtx.setTransform(clipToSurface);

                clipCtx.state().setObjectFrame(
                    objectBBoxUS);

                const bool rendered =
                    drawContent(
                        *clipElement,
                        clipCtx,
                        groot,
                        flags);

                clipCtx.detach();

                if (!rendered)
                    return false;
            }

            // Both surfaces have exactly the same size and origin.
            wg_surface_clip(
                result.info(),
                clipSurface.info());

            // --------------------------------------------------------
            // Composite the clipped result into the parent destination.
            // --------------------------------------------------------

            parentCtx.push();

            parentCtx.setTransform(
                WGMatrix3x3::makeIdentity());

            parentCtx.blendMode(
                BL_COMP_OP_SRC_OVER);

            parentCtx.image(
                result,
                double(pixelRect.x),
                double(pixelRect.y));

            parentCtx.flush();
            parentCtx.pop();

            return true;
        }
    };
}