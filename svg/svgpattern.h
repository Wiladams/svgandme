#pragma once

// Support for SVGPatternElement
// http://www.w3.org/TR/SVG11/feature#Pattern
// pattern
//

#include <memory>
#include <vector>
#include <cmath>
#include <cstdint>

#include "svgstructuretypes.h"
#include "viewport.h"
#include "svgb2ddriver.h"



namespace waavs
{
    //============================================================
    // SVGPatternElement
    // https://www.svgbackgrounds.com/svg-pattern-guide/#:~:text=1%20Background-size%3A%20cover%3B%20This%20declaration%20is%20good%20when,5%20Background-repeat%3A%20repeat%3B%206%20Background-attachment%3A%20scroll%20%7C%20fixed%3B
    // https://www.svgbackgrounds.com/category/pattern/
    // https://www.visiwig.com/patterns/
    //============================================================

    struct SVGPatternElement : public SVGGraphicsElement
    {
        static constexpr size_t kMaxHrefDepth = 16; // guard against cycles; arbitrary limit

        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("pattern", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGPatternElement>(groot);
                node->loadFromXmlElement(elem, groot);
                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("pattern",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGPatternElement>(groot);
                    node->loadFromXmlPull(iter, groot);
                    return node;
                });
            registerSingularNode();
        }

        // Document authored State
        ByteSpan fHref{};

        // Geometry attributes (lengths, with units)
        SVGLengthValue fX{};         // default 0
        SVGLengthValue fY{};         // default 0
        SVGLengthValue fWidth{};     // required, but we allow unset to mean 0 (so default 0)
        SVGLengthValue fHeight{};    // required, but we allow unset to mean 0 (so default 0)

        // Enums
        SpaceUnitsKind fPatternUnits{ SpaceUnitsKind::SVG_SPACE_OBJECT };        // objectBoundingBox default
        SpaceUnitsKind fPatternContentUnits{ SpaceUnitsKind::SVG_SPACE_USER };   // userSpaceOnUse default
        BLExtendMode fExtendMode{ BL_EXTEND_MODE_REPEAT };

        // Optional transforms
        bool fHasPatternTransform{ false };
        BLMatrix2D fPatternTransform { BLMatrix2D::makeIdentity() };

        // viewBox/par
        bool fHasViewBox{ false };
        BLRect fViewBox;
        PreserveAspectRatio fPar;


        // resolved
        std::shared_ptr<SVGPatternElement> fContentSource;
        BLRect fTileRect{};
        int fTilePxW{ 0 };
        int fTilePxH{ 0 };
        BLImage  fTileImage{};
        BLPattern fPattern{};



        SVGPatternElement(IAmGroot*) : SVGGraphicsElement()
        {
            setIsStructural(true);
            setIsVisible(false); // never directly visible; only via paint server

            fPattern.setExtendMode(BL_EXTEND_MODE_REPEAT);
        }


        bool hasHref() const { return !fHref.empty(); }
        ByteSpan href() const { return fHref; }

        // Pattern is used as a paint server: return variant
        const BLVar getVariant(IRenderSVG* ctx, IAmGroot* groot) noexcept override
        {
            // Should be bindToContext(), to be more normal
            // Resolve + update cache if needed
            if (needsBinding())
                (void)bindToContext(ctx, groot);

            BLVar v{};
            v = fPattern;

            // BUGBUG, cheat check
            //ctx->image(fTileImage, 0, 0);

            return v;
        }

        // Not a geometry bbox; return something stable
        BLRect objectBoundingBox() const override
        {
            return fTileRect;
        }



        virtual void inheritProperties(const SVGPatternElement* elem)
        {
            if (!elem) return;


            // 1) All the standard properties that can be inherited
            // Geometry
            setAttributeIfAbsent(elem, svgattr::x());
            setAttributeIfAbsent(elem, svgattr::y());
            setAttributeIfAbsent(elem, svgattr::width());
            setAttributeIfAbsent(elem, svgattr::height());

            // Coordinate system controls
            setAttributeIfAbsent(elem, svgattr::patternUnits());
            setAttributeIfAbsent(elem, svgattr::patternContentUnits());

            // Transform
            setAttributeIfAbsent(elem, svgattr::patternTransform());

            // ViewBox system
            setAttributeIfAbsent(elem, svgattr::viewBox());
            setAttributeIfAbsent(elem, svgattr::preserveAspectRatio());




        }

        // If we have an href, followed the chain of referred to
        // gradients, inheriting raw attributes that are missing 
        // along the way.
        void resolveReferenceChain(IAmGroot* groot)
        {
            if (!groot) return;         // if no groot, can't do lookups
            if (!hasHref()) return;     // if no href, nothing to do

            const SVGPatternElement * cur = this;
            ByteSpan hrefSpan = fHref;

            // Keep a simple visited list to detect cycles
            const SVGPatternElement* visited[kMaxHrefDepth]{};
            uint32_t visitedCount = 0;

            // Traverse the chain of references, inheriting attributes
            // and holding onto it as fContentSource if it has any child nodes
            // and we don't.
            for (uint32_t depth = 0; depth < kMaxHrefDepth; ++depth)
            {
                if (!hrefSpan) break;

                // Make sure we actually find a node associated with the href
                auto node = groot->findNodeByHref(hrefSpan);
                if (!node) break;

                // Make sure that node is a pattern
                auto gnode = std::dynamic_pointer_cast<SVGPatternElement>(node);
                if (!gnode) break;

                const SVGPatternElement* ref = gnode.get();

                // cycle detection (including self)
                bool seen = (ref == this);
                for (uint32_t i = 0; i < visitedCount && !seen; ++i)
                    if (visited[i] == ref) seen = true;

                if (seen)
                {
                    WAAVS_ASSERT(false && "SVGPatternElement href cycle detected");
                    break;
                }

                if (visitedCount < kMaxHrefDepth)
                    visited[visitedCount++] = ref;

                // Merge from nearest first: direct reference wins.
                inheritProperties(ref);

                // point to element as source of content if we don't
                // have any child nodes of our own
                // Inherit child content pointer
                if (fNodes.empty() && !gnode->fNodes.empty() && !fContentSource)
                {
                    fContentSource = gnode;

                    // Do NOT do this simple assignment because it copies
                    // node references into our own node tree
                    //fNodes = elem->fNodes;
                }

                // follow next link in the chain
                hrefSpan = ref->href();   // requires ref to have captured its href in its own fixup/load
            }

        }

        // Fixup attributes that were authored on this node, before we do any inheritance.
        // We want to convert to a representation that is ready for binding later
        // 
        void fixupCommonAttributes(IAmGroot* groot)
        {
            // x,y,w,h
            ByteSpan xA{}, yA{}, wA{}, hA{};
            fAttributes.getValue(svgattr::x(), xA);
            fAttributes.getValue(svgattr::y(), yA);
            fAttributes.getValue(svgattr::width(), wA);
            fAttributes.getValue(svgattr::height(), hA);

            fX = parseLengthAttr(xA);
            fY = parseLengthAttr(yA);
            fWidth = parseLengthAttr(wA);
            fHeight = parseLengthAttr(hA);

            // patternUnits / patternContentUnits (only if present; otherwise keep defaults)
            ByteSpan puA{}, pcuA{};
            if (fAttributes.getValue(svgattr::patternUnits(), puA)) {
                uint32_t v{};
                if (getEnumValue(SVGSpaceUnits, puA, v)) fPatternUnits = (SpaceUnitsKind)v;
            }
            if (fAttributes.getValue(svgattr::patternContentUnits(), pcuA)) {
                uint32_t v{};
                if (getEnumValue(SVGSpaceUnits, pcuA, v)) fPatternContentUnits = (SpaceUnitsKind)v;
            }

            // extendMode (your custom)
            ByteSpan emA{};
            if (fAttributes.getValue(svgattr::extendMode(), emA)) {
                uint32_t v{};
                if (getEnumValue(SVGExtendMode, emA, v)) fExtendMode = (BLExtendMode)v;
            }

            // patternTransform
            ByteSpan ptA{};
            if (fAttributes.getValue(svgattr::patternTransform(), ptA)) {
                fHasPatternTransform = parseTransform(ptA, fPatternTransform);
            }

            // preserveAspectRatio
            ByteSpan parA{};
            if (fAttributes.getValue(svgattr::preserveAspectRatio(), parA)) {
                fPar.loadFromChunk(parA);
            }

            // viewBox
            ByteSpan vbA{};
            BLRect vb{};
            if (fAttributes.getValue(svgattr::viewBox(), vbA) && parseViewBox(vbA, vb)) {
                fHasViewBox = true;
                fViewBox = vb;
            }
        }


        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            fContentSource.reset();

            // We already have our attributes (unresolved) sitting on our
            // element.  Since resolving references depends on attributes
            // it's safe to do that first.
            resolveReferenceChain(groot);

            // After all attributes are inherited, we can now convert them
            // to the intermediary values that will later be bound.
            fixupCommonAttributes(groot);

            setNeedsBinding(true);
        }


        void bindToContext(IRenderSVG* ctx, IAmGroot* groot) noexcept override
        {
            if (!ctx) {
                printf("SVGPatternElement::bindToContext: no context\n");
                return;
            }


            const BLRect paintVP = ctx->viewport();
            const BLRect objBBox = ctx->getObjectFrame();
            const double dpi = groot ? groot->dpi() : 96.0;
            const BLFont* fontOpt = &ctx->getFont();


            const bool puObject = (fPatternUnits == SpaceUnitsKind::SVG_SPACE_OBJECT);

            // Resolve tile x/y/w/h
            // depending on which patternUnits specified
            LengthResolveCtx cx{}, cy{}, cw{}, ch{};
            if (fPatternUnits == SpaceUnitsKind::SVG_SPACE_OBJECT) {
                cx = makeLengthCtxUser(objBBox.w, 0.0, dpi, fontOpt, SpaceUnitsKind::SVG_SPACE_OBJECT);
                cy = makeLengthCtxUser(objBBox.h, 0.0, dpi, fontOpt, SpaceUnitsKind::SVG_SPACE_OBJECT); 
                cw = cx; ch = cy;
            }
            else {
                cx = makeLengthCtxUser(paintVP.w, 0.0, dpi, fontOpt, SpaceUnitsKind::SVG_SPACE_USER);
                cy = makeLengthCtxUser(paintVP.h, 0.0, dpi, fontOpt, SpaceUnitsKind::SVG_SPACE_USER);
                cw = cx; ch = cy;
            }

            const double x = resolveLengthOr(fX, cx, 0.0) ;
            const double y = resolveLengthOr(fY, cy, 0.0) ;
            const double w = resolveLengthOr(fWidth, cw, 0.0);
            const double h = resolveLengthOr(fHeight, ch, 0.0);

            // If the width or height are zero, we will not render
            if (w <= 0.0 || h <= 0.0)
                return ;

            fTileRect = BLRect{ x, y, w, h };

            // Decide tile pixel size
            fTilePxW = std::max(1, int(std::ceil(fTileRect.w)));
            fTilePxH = std::max(1, int(std::ceil(fTileRect.h)));


            // We now have the desired tile size, so we need
            // to create the transform that gets us from the drawing's 
            // space, to the BLImage space
            BLMatrix2D T = BLMatrix2D::makeIdentity();
            T.postScale((double)fTilePxW / w, (double)fTilePxH / h);
            if (fHasPatternTransform) T.postTransform(fPatternTransform);

            if (puObject)
                T.postTranslate(-(objBBox.x + x), -(objBBox.y + y));
            else
                T.postTranslate(-x, -y);


            // Now, ensure the bitmap is the size we specified
            fTileImage.create(fTilePxW, fTilePxH, BL_FORMAT_PRGB32);
            if (!renderTile(groot, objBBox))
                return;

            fPattern.create(fTileImage, fExtendMode, T);

            //BLMatrix2D tform = fPattern.transform();
            //printf("TFORM: %d\n", tform.type());

            setNeedsBinding(false);
        }



        bool renderTile(IAmGroot* groot, const BLRect &objBBox) noexcept
        {
            // Create a drawing context and attach it to our image
            // Clear out the image
            SVGB2DDriver ictx;
            ictx.attach(fTileImage);
            ictx.renew();
            //ictx.background(BLVar::null());
            ictx.clear();

            // Nearest viewport for pattern children is the TILE USER RECT.
            ictx.setViewport(BLRect{ 0.0, 0.0, fTileRect.w, fTileRect.h });

            // Object frame for children:
            // - If patternContentUnits==objectBoundingBox and no viewBox: children content space is 0..1
            // - Otherwise: treat like tile user space
            if (fPatternContentUnits == SpaceUnitsKind::SVG_SPACE_OBJECT && !fHasViewBox)
                ictx.setObjectFrame(BLRect{ 0.0, 0.0, 1.0, 1.0 });
            else
                ictx.setObjectFrame(BLRect{ 0.0, 0.0, fTileRect.w, fTileRect.h });

            // Need to construct the matrix that maps from patternContentUnits
            // to the pixel units of the tile bitmap
            const double sx = double(fTilePxW) / fTileRect.w;
            const double sy = double(fTilePxH) / fTileRect.h;

            BLMatrix2D C = BLMatrix2D::makeIdentity();
            C.postScale(sx, sy);

            if (fHasViewBox)
            {
                BLMatrix2D vb2tile = BLMatrix2D::makeIdentity();
                // viewport is tile-local [0..w, 0..h]
                computeViewBoxToViewport(BLRect{ 0,0,fTileRect.w, fTileRect.h }, fViewBox, fPar, vb2tile);
                C.postTransform(vb2tile);
            }
            else if (fPatternContentUnits == SpaceUnitsKind::SVG_SPACE_USER)
            {
                C.postTranslate(-fTileRect.x, -fTileRect.y);
            }
            else {  // objectBoundingBox
                C.postTranslate(objBBox.x, objBBox.y);
                C.postScale(objBBox.w, objBBox.h);
                C.postTranslate(-fTileRect.x, -fTileRect.y);
            }

            // Map pattern-content user coords -> tile pixels.
            // IMPORTANT: setTransform, not applyTransform, so we start clean.
            ictx.transform(C);

            // Draw children into the tile image.
            if (fContentSource)
                fContentSource->drawChildren(&ictx, groot);
            else
                drawChildren(&ictx, groot);

            ictx.flush();
            ictx.detach();


            return true;
        }

        void update(IAmGroot* groot) override
        {
            updateChildren(groot);
            setNeedsBinding(true);
        }
    };

}


