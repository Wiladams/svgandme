#pragma once

#include "svgstructuretypes.h"


namespace waavs {
    enum class SVGReferenceSyntax
    {
        Url,
        Href
    };

    struct SVGReferenceAttribute : public SVGVisualProperty
    {
        std::shared_ptr<IViewable> fReferencedNode{};
        SVGReferenceSyntax fSyntax{ SVGReferenceSyntax::Url };

        SVGReferenceAttribute(
            InternedKey name,
            SVGReferenceSyntax syntax = SVGReferenceSyntax::Url)
            : SVGVisualProperty(nullptr)
            , fSyntax(syntax)
        {
            setName(name);
            setAutoDraw(false);
            setNeedsBinding(false);
        }

        const std::shared_ptr<IViewable>& referencedNode() const noexcept
        {
            return fReferencedNode;
        }

        bool isResolved() const noexcept
        {
            return bool(fReferencedNode);
        }

        virtual void onReferencedNodeResolved(const std::shared_ptr<IViewable>& node) noexcept {}

        bool loadFromAttributes(
            const XmlAttributeCollection& attrs,
            IAmGroot* groot = nullptr) override
        {
            ByteSpan value{};

            if (!attrs.getValue(name(), value))
                return false;

            setRawValue(value);
            fReferencedNode.reset();

            if (groot)
            {
                switch (fSyntax)
                {
                case SVGReferenceSyntax::Href:
                    fReferencedNode =
                        groot->findNodeByHref(value);
                    break;

                case SVGReferenceSyntax::Url:
                    fReferencedNode =
                        groot->findNodeByUrl(value);
                    break;
                }
            }

            onReferencedNodeResolved(fReferencedNode);

            set(true);
            return true;
        }
    };

    template<typename PropertyT>
    static bool registerReferenceAttribute()
    {
        return registerSVGAttribute(
            PropertyT::attributeKey(),
            [](const XmlAttributeCollection& attrs, IAmGroot* groot)
            {
                auto prop = std::make_shared<PropertyT>();
                prop->loadFromAttributes(attrs, groot);
                return prop;
            });
    }
}