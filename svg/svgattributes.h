// svgattributes.h
#pragma once

#include <memory>
#include <vector>
//#include <cstdint>  // uint8_t, etc
//#include <cstddef>  // nullptr_t, ptrdiff_t, size_t
//#include <list>

#include "maths.h"

#include "svgstructuretypes.h"
#include "svgcolors.h"

#include "svg_attribute_fill.h"
#include "svg_attribute_font.h"
#include "svg_attribute_marker.h"
#include "svg_attribute_paintorder.h"
#include "svg_attribute_stroke.h"
#include "svg_attribute_text.h"
#include "svg_attribute_transform.h"
#include "svg_attribute_viewport.h"

namespace waavs 
{
    //
    // SVGPatternExtendMode
    // 
    // A structure that represents the extend mode of a pattern.
    struct SVGPatternExtendMode : public SVGVisualProperty 
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::extendMode(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGPatternExtendMode>(nullptr);
                node->loadFromAttributes(attrs);
                return node;
                });
        }
        
        uint32_t fExtendMode{ BL_EXTEND_MODE_REPEAT };      // repeat by default

        SVGPatternExtendMode(IAmGroot* groot) : SVGVisualProperty(groot) 
        {
            setName(svgattr::extendMode());
            //setAutoDraw(false);
        }

        BLExtendMode value() const { return static_cast<BLExtendMode>(fExtendMode); }
        
        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {   
            bool success = getEnumValue(SVGExtendMode, inChunk, fExtendMode);
            set(success);
            setNeedsBinding(false);
            
            return success;
        }

    };
}




// Specific types of attributes
namespace waavs 
{

    // SVGOpacity
    // 
    // https://svgwg.org/svg2-draft/render.html#ObjectAndGroupOpacityProperties
    // Opacity is an attribute that causes a sub-tree to be rendered into
    // a buffer, then a global opacity is applied to that buffer as it's being
    // composited into the final output.
    // So, this property triggers the offscreen drawing.  We retain the actual
    // opacity value so that when the compositing occurs.
    // 
    //
    struct SVGOpacity : public SVGVisualProperty 
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::opacity(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGOpacity>();
            node->loadFromAttributes(attrs);
            return node;
                });
        }
        
        double fValue{1};
        
        SVGOpacity() 
            : SVGVisualProperty(nullptr) 
        { 
            setName(svgattr::opacity()); 
        }        
        

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!inChunk)
                return false;
            
            SVGNumberOrPercent op{};
            ByteSpan s = inChunk;

            if (!numberOrPercent_read(s, op))
                return false;

            fValue = waavs::clamp(op.normalizedValue(), 0.0, 1.0);

            set(true);
            setNeedsBinding(false);
            
            return true;
        }
        

    };
    


    struct SVGStrokeOpacity : public SVGOpacity
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::stroke_opacity(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGStrokeOpacity>(); 
                node->loadFromAttributes(attrs);
                return node; 
                });

        }

        SVGStrokeOpacity() :SVGOpacity() { setName(svgattr::stroke_opacity()); }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->strokeOpacity(fValue);
        }

    };

}


namespace waavs {
    //======================================================
    // SVGClipAttribute
    // This is the attribute that can be connected to some
    // shape that's being drawn.  
    //======================================================
    struct SVGClipPathAttribute : public SVGVisualProperty
    {
        static void registerFactory()
        {
            registerSVGAttribute(svgattr::clip_path(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGClipPathAttribute>(nullptr);
                node->loadFromAttributes(attrs);

                return node;
                });

        }


        std::shared_ptr<IViewable> fClipNode{ nullptr };
        BLVar fClipVar;
        
        
        SVGClipPathAttribute(IAmGroot* groot) 
            : SVGVisualProperty(groot) 
        { setName(svgattr::clip_path()); }

        
        bool loadFromUrl(IRenderSVG *ctx, IAmGroot* groot, const ByteSpan& inChunk)
        {
            if (nullptr == groot)
                return false;

            fClipNode = groot->findNodeByUrl(inChunk);

            if (fClipNode == nullptr) {
                set(false);
                return false;
            }

            // pull out the color value
            // BUGBUG - this will not always be the case
            // as what we point to might be a gradient or pattern
            if (fClipNode->needsBinding())
                fClipNode->bindToContext(ctx, groot);

            set(true);

            return true;
        }

        
        // Let's get a connection to our referenced thing
        void bindToContext(IRenderSVG *ctx, IAmGroot* groot) noexcept override
        {
            ByteSpan str = rawValue();

            // Just load the referenced node at this point
            //if (bspan_starts_with(str, "url("))
            Invocation invoc{};
            bool success = readInvocation(str, invoc);
            if (success && invoc.nameKey == svgfunc::url())
            {
                loadFromUrl(ctx, groot, str);
            }

            setNeedsBinding(false);
        }
        
        bool loadSelfFromChunk(const ByteSpan& ) override
        {
            setNeedsBinding(true);
            set(true);
            
            return true;
        }
    };
}
    
namespace waavs {
    
    struct SVGVectorEffectAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::vector_effect(), [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGVectorEffectAttribute>(nullptr); node->loadFromAttributes(attrs);  return node; });
        }

        VectorEffectKind fEffectKind{ VECTOR_EFFECT_NONE };

        
        SVGVectorEffectAttribute(IAmGroot* groot) 
            : SVGVisualProperty(groot) 
        { 
            setName(svgattr::vector_effect()); 
        }
        
        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            bool success = getEnumValue(SVGVectorEffect, inChunk, (uint32_t &)fEffectKind);
            set(success);
            setNeedsBinding(false);
            
            return success;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            if (VECTOR_EFFECT_NON_SCALING_STROKE == fEffectKind)
            {
                ctx->strokeBeforeTransform(true);
            }
        }
    };
}


// Dash array/offset attributes
namespace waavs 
{
    //=========================================================
     // SVGStrokeDashArray
     // stroke-dasharray: none | <length-percentage>#
     //=========================================================
    struct SVGStrokeDashArray : public SVGVisualProperty
    {
        static void registerFactory()
        {
            registerSVGAttribute(svgattr::stroke_dasharray(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGStrokeDashArray>();
                node->loadFromAttributes(attrs);
                return node;
                });
        }

        std::vector<float> fArray{};
        double fOffset{ 0.0 };
        bool fIsNone{ true };

        SVGStrokeDashArray() : SVGVisualProperty(nullptr)
        {
            setName(svgattr::stroke_dasharray());
        }

        bool loadFromAttributes(const XmlAttributeCollection& attrs) override
        {
            // Get the dash array and offset if they exist
            ByteSpan dashArrayAttr{};
            attrs.getValue(svgattr::stroke_dasharray(), dashArrayAttr);
            
            // Get the dash-offset if it exists
            ByteSpan dashOffsetAttr{};
            attrs.getValue(svgattr::stroke_dashoffset(), dashOffsetAttr);
            
            // if the two attributes are empty, then no dash pattern
            // was specified.  So we can just keep the attribute
            // not set and return.
            // It's not exactly the same as 'none', because it won't alter
            // the state of the drawing context, so whatever state dashing was
            // in previously will remain
            if (!dashArrayAttr && !dashOffsetAttr)
                return set(false);
            

            // parse the dash array value
            if (!parseStrokeDashArray(dashArrayAttr, fArray, fIsNone))
                return set(false);

            // get the offset value, if it exists, otherwise default to 0
            if (!parseNumber(dashOffsetAttr, fOffset))
                fOffset = 0.0;

            set(true);
            setNeedsBinding(false);

            return true;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            (void)groot;

            if (!ctx)
                return;

            // If we're not set, don't do anything
            if (!isSet())
                return;

            // If it was explicitly set to 'none', then we need
            // to clear any dash pattern on the context, 
            // which is different than just not setting it at all
            if (fIsNone)
            {
                // No dash pattern.
                ctx->state().clearStrokeDashArray();
                ctx->state().clearStrokeDashOffset();

                return;
            }

            // Store raw dash segments in state.
            ctx->dashArray(fArray);
            ctx->dashOffset(fOffset);
        }
    };

/*
//=========================================================
// SVGStrokeDashOffset
// stroke-dashoffset: <length-percentage>
//=========================================================
    struct SVGStrokeDashOffset : public SVGVisualProperty
    {
        static void registerFactory()
        {
            registerSVGAttribute(svgattr::stroke_dashoffset(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGStrokeDashOffset>(nullptr);
                node->loadFromAttributes(attrs);
                return node;
                });
        }

        float fOffset;
        bool fHasOffset{ false };

        SVGStrokeDashOffset(IAmGroot* groot) : SVGVisualProperty(groot)
        {
            setName(svgattr::stroke_dashoffset());
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            double offset = 0.0;
            if (parseNumber(inChunk, offset))
            {
                fHasOffset = true;
                fOffset = static_cast<float>(offset);
            }

            // Even if empty/not set, we still consider the property "set"
            // if the attribute existed; drawSelf() will clear the state.
            set(true);
            setNeedsBinding(false);

            return true;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            if (ctx == nullptr)
                return;

            if (!fHasOffset)
            {
                ctx->clearStrokeDashOffset();
                return;
            }

            ctx->dashOffset(fOffset);
        }
    };
    */
} // namespace waavs

