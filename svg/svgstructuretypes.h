// svgstructuretypes.h

#ifndef SVGSTRUCTURETYPES_H
#define SVGSTRUCTURETYPES_H

#pragma once

#include "maths.h"


#include "core_nametable.h"
#include "xml_pull.h"
#include "graphics_draw.h"
#include "stopwatch.h"

#include "svgatoms.h"
#include "svg_interface.h"
#include "svgdatatypes.h"
#include "svgcss.h"
#include "svgscan.h"


namespace waavs {

    // RenderFeature
    // 
    // Gives the graphic that might be responding to a 
    // draw() call some intention information
    enum RenderFeature : uint32_t
    {
        RF_Content = 1u << 0,
        RF_Filter = 1u << 1,
        RF_Mask = 1u << 2,
        RF_Clip = 1u << 3,
        RF_Opacity = 1u << 4,

        RF_All = RF_Content | RF_Filter | RF_Mask | RF_Clip | RF_Opacity
    };

    using RenderFlags = BitFlags<RenderFeature>;

    struct IsolatedRenderPlan
    {
        WGRectD objectBBoxUS{};
        WGRectD effectRectUS{};
        WGRectD nominalRectPX{};
        WGRectD allocRectPX{};
        WGRectI pixelRect{};

        WGMatrix3x3 ctm{};
        WGMatrix3x3 invCtm{};

        bool needsIsolation = false;
        bool hasFilter = false;
        bool hasMask = false;
        bool hasClip = false;
        bool hasOpacity = false;
    };


    struct IsolatedSubtreeRequest
    {
        SVGDrawingState drawingState;
        WGRectD userRect;
        WGRectI pixelRect;

        WGMatrix3x3 ctm;
        WGRectD objectBBoxUS;

        RenderFlags renderMode = RenderFeature::RF_All;

        bool clear = true;
    };

    // -----------------------------------------
    //

    struct IViewable : public SVGObject, public IServePaint
    {
        bool fIsVisible{ true };
        bool fIsStructural{ true };

        InternedKey fName{};
        ByteSpan fId{};      // The id of the element

        virtual ~IViewable() = default;

        virtual const BLVar getVariant(IDrawGraphics*, IAmGroot*) noexcept { return BLVar::null(); }


        const ByteSpan& id() const noexcept { return fId; }
        void setId(const ByteSpan& aid) noexcept { fId = aid; }

        bool isStructural() const { return fIsStructural; }
        void setIsStructural(bool aStructural) { fIsStructural = aStructural; }
        
        bool isVisible() const { return fIsVisible; }
        void setIsVisible(bool visible) { fIsVisible = visible; }

        virtual const WGRectD resolveFilterRegion(IDrawGraphics* ctx, IAmGroot* groot, const WGRectD& bbox)  noexcept
        {
            (void)ctx;
            (void)groot;

            return bbox;
        }

        virtual const WGRectD getObjectBoundingBox(IDrawGraphics* ctx, IAmGroot* groot)  noexcept
        {
            return {};
        }

        virtual const WGRectD objectBoundingBox() const noexcept { 
            return {}; 
        }

        virtual bool contains(double x, double y) { return false; }
        
        void setName(InternedKey aname) { fName = aname; }
        InternedKey nameAtom() const { return fName; }

        virtual void fixupStyleAttributes(IAmGroot* groot) = 0;

        virtual void update(IAmGroot*) = 0;
        virtual void draw(IDrawGraphics*, IAmGroot*, 
            RenderFlags featureSet = RenderFeature::RF_All) = 0;

    };

}


namespace waavs 
{
    // SVGVisualProperty
    // 
    // This is the base class for things that alter the graphics context while drawing
    // If isSet() is true, then the drawSelf() is called.
    // sub-classes should override drawSelf() to do the actual drawing.
    //
    // These properties are independent like; Paint, Transform, Miter, etc.
    // and they usually make a state altering call on the drawing context
    //
    struct SVGVisualProperty : public SVGObject
    {
        InternedKey fName{};    // The type name of the property
                                // used to lookup type converter

        bool fAutoDraw{ true };
        bool fIsSet{ false };
        ByteSpan fRawValue;


        SVGVisualProperty(IAmGroot*) :SVGObject(), fIsSet(false) { setNeedsBinding(false); }

        SVGVisualProperty(const SVGVisualProperty& other) = delete;
        SVGVisualProperty& operator=(const SVGVisualProperty& rhs) = delete;

        const InternedKey& name() const { return fName; }
        void setName(const InternedKey name) { fName = name; }

        bool set(const bool value) { fIsSet = value; return value; }
        bool isSet() const { return fIsSet; }

        void setAutoDraw(bool value) { fAutoDraw = value; }
        bool autoDraw() const { return fAutoDraw; }

        void setRawValue(const ByteSpan& value) { fRawValue = value; }
        const ByteSpan& rawValue() const { return fRawValue; }

        virtual bool loadSelfFromChunk(const ByteSpan&)
        {
            return false;
        }

        bool loadFromChunk(const ByteSpan& inChunk)
        {
            auto s = inChunk;
            bspan_trim_spaces(s);

            if (!s)
                return false;

            setRawValue(s);

            return this->loadSelfFromChunk(fRawValue);
        }

        virtual bool loadFromAttributes(const XmlAttributeCollection& attrs, IAmGroot *groot)
        {
            ByteSpan attr{};

            if (!attrs.getValue(name(), attr))
                return false;
            
            return loadFromChunk(attr);
        }

		void bindToContext(IDrawGraphics*, IAmGroot*) noexcept override
		{
            // do nothing by default
            setNeedsBinding(false);
		}

        // Give an attribute a chance to update itself
        virtual void update(IAmGroot*) { ; }

        // Apply property to the context

        virtual void applySelfToContext(IDrawGraphics*, IAmGroot*) { ; }
        
        virtual void applyToContext(IDrawGraphics* ctx, IAmGroot* groot)
        {
            if (needsBinding())
                this->bindToContext(ctx, groot);
            
            if (isSet())
                this->applySelfToContext(ctx, groot);
        }

    };


    //===================================================
    // Handling attribute conversion to properties
    // ================================================
    // Collection of property constructors
    using SVGAttributeToPropertyConverter = std::function<std::shared_ptr<SVGVisualProperty>(const XmlAttributeCollection& attrs, IAmGroot *groot)>;    
    using SVGPropertyConstructorMap = WSNameMap<SVGAttributeToPropertyConverter>;


    static SVGPropertyConstructorMap & getPropertyConstructionMap()
    {
        static SVGPropertyConstructorMap gSVGAttributeCreation{};
        
        return gSVGAttributeCreation;
    }

    static bool registerSVGAttribute(InternedKey key, SVGAttributeToPropertyConverter func)
    {
        if (!key)
            return false;

        return getPropertyConstructionMap().put(key, std::move(func));
    }
    
    static bool registerSVGAttributeByName(const char* name, SVGAttributeToPropertyConverter func)
    {
        InternedKey k = WSNameSet::INTERN(name);
        return registerSVGAttribute(k, std::move(func));
    }

    static SVGAttributeToPropertyConverter getAttributeConverter(InternedKey k)
    {
        if (!k)
            return nullptr;

        auto& mapper = getPropertyConstructionMap();

        if (auto converter = mapper.getRef(k))
            return *converter;

        return nullptr;
    }

}





namespace waavs 
{
    
    // I Am Graphics Root (IAmGroot) 
    // Core interface to hold document level state, primarily
    // for the purpose of looking up nodes, but also for style sheets
    // and animation program



    struct IAmGroot
    {
        WSNameSet fIdNameTable{};
        WSNameMap<std::shared_ptr<IViewable>> fDefinitions{};

        //std::unordered_map<ByteSpan, std::shared_ptr<IViewable>, ByteSpanHash, ByteSpanEquivalent> fDefinitions{};
        std::unordered_map<ByteSpan, ByteSpan, ByteSpanHash, ByteSpanEquivalent> fEntities{};
        StopWatch fDocClock{};

        virtual void addElementReference(const ByteSpan& name, std::shared_ptr<IViewable> obj)
        {
            ByteSpan keySpan = name;
            bspan_trim_spaces(keySpan);

            if (!keySpan || !obj)
                return;

            InternedKey key = fIdNameTable.intern(keySpan);
            fDefinitions.put(key, std::move(obj));
        }

        //virtual std::shared_ptr<IViewable> getElementById(const ByteSpan& name)


        virtual std::shared_ptr<IViewable> getElementById(const ByteSpan& name)
        {
            ByteSpan keySpan = name;
            bspan_trim_spaces(keySpan);

            if (!keySpan)
                return {};

            InternedKey key = fIdNameTable.intern(keySpan);

            if (auto obj = fDefinitions.getRef(key))
                return *obj;

            return {};
        }

        // Load a URL Reference
        // BUGBUG - This needs to be more sophisticated.  Right now
        // it only finds an already existent element with the given ID
        // But, the URL could be pointing to an external file, in which
        // case we need to actually load that file.
        // the Image element does this, either loading from inline base64
        // or loading from a file.
        // We need to generalize that here as well.  Filtering also has this
        // proision implemented for image references
        virtual std::shared_ptr<IViewable> findNodeByHref(const ByteSpan& inChunk)
        {
            ByteSpan id = inChunk;

            bspan_trim_spaces(id);

            // early return if we don't have a valid id
            if (!id)
                return nullptr;

            // The first character could be '.' or '#'
            // so we need to skip past that
            if (*id == '.' || *id == '#')
                ++id;

            if (!id)
                return nullptr;

            // lookup the thing we're referencing
            return this->getElementById(id);
        }
        
        // Load a URL reference, including the 'url(' function indicator
        virtual std::shared_ptr<IViewable> findNodeByUrl(const ByteSpan& inChunk)
        {
            ByteSpan str = inChunk;

            // do a 'parse invocation' of the url() function, 
            // and extract the id from it
            // the id we want should look like this
            // url(#id)
            // so we need to skip past the 'url(#'
            // and then find the closing ')'
            // and then we have the id
            Invocation invoc{};
            bool success = readInvocation(str, invoc);
            if (success && invoc.nameKey == svgfunc::url())
            {
                bspan_trim_spaces(invoc.payload);
                bspan_trim_matching_quotes(invoc.payload);

                return this->findNodeByHref(invoc.payload);
            }

            return nullptr;
            
        }
        

        // Entities for entity expansion
        virtual void addXmlEntity(const ByteSpan& name, ByteSpan expansion)
        {
            ByteSpan key = name;
            bspan_trim_spaces(key);
            fEntities[key] = expansion;
        }
        
        virtual ByteSpan findXmlEntity(const ByteSpan& name)
        {
            auto it = fEntities.find(name);
            if (it != fEntities.end())
                return it->second;

            return ByteSpan{};
        }

        // IAmGroot

        virtual const CSSStyleSheet& styleSheet() const = 0;
        virtual CSSStyleSheet& styleSheet() = 0;

        // Animation support
        // Clock that maintains 'document time' for the purposes of animations
        StopWatch& documentClock() noexcept { return fDocClock; }

        //virtual AnimationProgram& animationProgram() noexcept = 0;
        //virtual const AnimationProgram& animationProgram() const noexcept = 0;

        //virtual AnimationValueContext& animationValueContext() noexcept = 0;
        //virtual const AnimationValueContext& animationValueContext() const noexcept = 0;


        virtual ByteSpan systemLanguage() { return "en"; } // BUGBUG - What a big cheat!!

        virtual double canvasWidth() const = 0;
        virtual double canvasHeight() const = 0;
        
        virtual double dpi() const = 0;
        virtual void setDpi(const double d) = 0;
    };
}



namespace waavs {
    struct ISVGElement : public IViewable 
    {
		XmlElement fSourceElement{};
        
       virtual  std::shared_ptr<SVGVisualProperty> getVisualProperty(InternedKey key) const = 0;

       bool getElementAttribute(const ByteSpan & name, ByteSpan &value) const
       {
           return fSourceElement.getElementAttribute(name, value);
       }
    };
}

namespace waavs 
{

    // node creation dispatch
    // Creating from a singular element, typically a self-closing tag
    using SVGSingularCreator = std::function<std::shared_ptr<ISVGElement>(IAmGroot*, const XmlElement&)>;
    using ShapeCreationMap = WSNameMap<SVGSingularCreator>;


    // compound node creation dispatch - 
    // 'g', 'symbol', 'pattern', 'linearGradient', 'radialGradient', 'conicGradient', 
    // 'image', 
    // 'style', 
    // 'text', 
    // 'tspan', 
    // 'use'
    //
    // And probably some others.  Basically, anything that has a start tag should 
    // register a routine here.
    using SVGContainerCreator = std::function<std::shared_ptr<ISVGElement>(IAmGroot*, XmlPull&)>;
    using SVGContainerCreationMap = WSNameMap<SVGContainerCreator>;


    //using SVGContainerCreationMap =
    //    std::unordered_map<InternedKey,
    //    std::function<std::shared_ptr<ISVGElement>(IAmGroot*, XmlPull&)>,
    //    InternedKeyHash, InternedKeyEquivalent>;



    static ShapeCreationMap& getSVGSingularCreationMap()
    {
        static ShapeCreationMap gShapeCreationMap{};

        return gShapeCreationMap;
    }

    static SVGContainerCreationMap& getSVGContainerCreationMap()
    {
        static SVGContainerCreationMap gSVGGraphicsElementCreation{};

        return gSVGGraphicsElementCreation;
    }


    // Register named creation routines for singular nodes
    static void registerSVGSingularNode(InternedKey key, SVGSingularCreator func)
    {
        getSVGSingularCreationMap().put(key, std::move(func));
    }

    static void registerSVGSingularNodeByName(const char* name, SVGSingularCreator func)
    {
        InternedKey k = WSNameSet::INTERN(name);
        registerSVGSingularNode(k, std::move(func));
    }

    // Register named creation routines for container nodes
    static void registerContainerNode(InternedKey key, std::function<std::shared_ptr<ISVGElement>(IAmGroot*, XmlPull&)> creator)
    {
        getSVGContainerCreationMap().put(key, std::move(creator));
    }

    static void registerContainerNodeByName(const char* name, std::function<std::shared_ptr<ISVGElement>(IAmGroot*, XmlPull&)> creator)
    {
        InternedKey key = WSNameSet::INTERN(name);
        registerContainerNode(key, std::move(creator));
    }



    // Convenience way to create an element
    static std::shared_ptr<ISVGElement> createSingularNode(const XmlElement& elem, IAmGroot* root)
    {
        InternedKey k = elem.nameAtom(); // nameAtom -> interned const char*
        if (!k)
            return nullptr;

        auto& m = getSVGSingularCreationMap();
        
        if (auto creator = m.getRef(k))
            return (*creator)(root, elem);

        return nullptr;
    }


    static std::shared_ptr<ISVGElement> createContainerNode(XmlPull& iter, IAmGroot* root)
    {
        InternedKey k = iter->nameAtom();
        if (!k)
            return nullptr;
        
        auto& m = getSVGContainerCreationMap();

        if (auto creator = m.getRef(k))
            return (*creator)(root, iter);

        return nullptr;
    }
}



#endif
