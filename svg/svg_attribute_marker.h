#pragma once

#include "svgstructuretypes.h"

//
// Markers
//
namespace waavs {
    struct SVGMarkerAttribute : public SVGVisualProperty
    {

        static void registerMarkerFactory() {
            registerSVGAttribute(svgattr::marker(), [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGMarkerAttribute>(svgattr::marker()); node->loadFromAttributes(attrs);  return node; });
            registerSVGAttribute(svgattr::marker_start(), [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGMarkerAttribute>(svgattr::marker_start()); node->loadFromAttributes(attrs);  return node; });
            registerSVGAttribute(svgattr::marker_mid(), [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGMarkerAttribute>(svgattr::marker_mid()); node->loadFromAttributes(attrs);  return node; });
            registerSVGAttribute(svgattr::marker_end(), [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGMarkerAttribute>(svgattr::marker_end()); node->loadFromAttributes(attrs);  return node; });
        }

        std::shared_ptr<IViewable> fWrappedNode = nullptr;


        SVGMarkerAttribute(const InternedKey key) : SVGVisualProperty(nullptr)
        {
            setName(key);
        }

        std::shared_ptr<IViewable> markerNode(IRenderSVG* ctx, IAmGroot* groot)
        {
            if (fWrappedNode == nullptr)
            {
                bindToContext(ctx, groot);
            }
            return fWrappedNode;
        }

        void bindToContext(IRenderSVG* ctx, IAmGroot* groot) noexcept override
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

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            if (fWrappedNode)
                fWrappedNode->draw(ctx, groot);
        }
    };
}

