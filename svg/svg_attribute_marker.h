#pragma once

#include "svgstructuretypes.h"

//
// Markers
//
namespace waavs {
    struct SVGMarkerAttribute : public SVGVisualProperty
    {

        static void registerMarkerFactory() {
            registerSVGAttribute(svgattr::marker(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {auto node = std::make_shared<SVGMarkerAttribute>(svgattr::marker()); node->loadFromAttributes(attrs, groot);  return node; });
            registerSVGAttribute(svgattr::marker_start(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {auto node = std::make_shared<SVGMarkerAttribute>(svgattr::marker_start()); node->loadFromAttributes(attrs, groot);  return node; });
            registerSVGAttribute(svgattr::marker_mid(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {auto node = std::make_shared<SVGMarkerAttribute>(svgattr::marker_mid()); node->loadFromAttributes(attrs, groot);  return node; });
            registerSVGAttribute(svgattr::marker_end(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {auto node = std::make_shared<SVGMarkerAttribute>(svgattr::marker_end()); node->loadFromAttributes(attrs, groot);  return node; });
        }

        std::shared_ptr<IViewable> fWrappedNode = nullptr;


        SVGMarkerAttribute(const InternedKey key) : SVGVisualProperty(nullptr)
        {
            setName(key);
        }

        std::shared_ptr<IViewable> markerNode(IDrawGraphics* ctx, IAmGroot* groot)
        {
            if (fWrappedNode == nullptr)
            {
                bindToContext(ctx, groot);
            }
            return fWrappedNode;
        }

        void bindToContext(IDrawGraphics* ctx, IAmGroot* groot) noexcept override
        {

            if (bspan_starts_with(rawValue(), "url("))
            {
                fWrappedNode = groot->findNodeByUrl(rawValue());

                if (fWrappedNode != nullptr)
                {
                    fWrappedNode->bindToContext(ctx, groot);
                    set(true);
                }
            }
            setNeedsBinding(false);
        }

        bool loadSelfFromChunk(const ByteSpan&) override
        {
            setAutoDraw(false); // we mark it as invisible, because we don't want it drawing when attributes are drawn
            // we only want it to draw when we're drawing during polyline/polygon drawing
            setNeedsBinding(true);

            return true;
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            if (fWrappedNode)
                fWrappedNode->draw(ctx, groot);
        }
    };
}

