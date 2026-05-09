
#ifndef SVGSTRUCTURETYPES_H
#define SVGSTRUCTURETYPES_H

#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstdint>		// uint8_t, etc
#include <cstddef>		// nullptr_t, ptrdiff_t, size_t

#include "maths.h"

#include "xmlscan.h"

#include "svgobject.h"
#include "svgdatatypes.h"
#include "svgcss.h"
#include "irendersvg.h"
#include "uievent.h"
#include "stopwatch.h"

#include "svgatoms.h"




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

        virtual const BLVar getVariant(IRenderSVG*, IAmGroot*) noexcept { return BLVar::null(); }


        const ByteSpan& id() const noexcept { return fId; }
        void setId(const ByteSpan& aid) noexcept { fId = aid; }

        bool isStructural() const { return fIsStructural; }
        void setIsStructural(bool aStructural) { fIsStructural = aStructural; }
        
        bool isVisible() const { return fIsVisible; }
        void setIsVisible(bool visible) { fIsVisible = visible; }

        virtual const WGRectD resolveFilterRegion(IRenderSVG* ctx, IAmGroot* groot, const WGRectD& bbox)  noexcept
        {
            (void)ctx;
            (void)groot;

            return bbox;
        }

        virtual const WGRectD getObjectBoundingBox(IRenderSVG* ctx, IAmGroot* groot)  noexcept
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
        virtual void draw(IRenderSVG*, IAmGroot*, 
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
            auto s = chunk_trim(inChunk, chrWspChars);

            if (!s)
                return false;

            setRawValue(s);

            return this->loadSelfFromChunk(fRawValue);
        }

        virtual bool loadFromAttributes(const XmlAttributeCollection& attrs)
        {
            ByteSpan attr{};

            if (!attrs.getValue(name(), attr))
                return false;
            
            return loadFromChunk(attr);
        }

		void bindToContext(IRenderSVG*, IAmGroot*) noexcept override
		{
            // do nothing by default
            setNeedsBinding(false);
		}

        // Give an attribute a chance to update itself
        virtual void update(IAmGroot*) { ; }

        // Apply property to the context
        virtual void drawSelf(IRenderSVG*, IAmGroot*) = delete; // { ; }

        virtual void applySelfToContext(IRenderSVG*, IAmGroot*) { ; }
        
        virtual void applyToContext(IRenderSVG* ctx, IAmGroot* groot)
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
    using SVGAttributeToPropertyConverter = std::function<std::shared_ptr<SVGVisualProperty>(const XmlAttributeCollection& attrs)>;    
    using SVGPropertyConstructorMap = std::unordered_map<InternedKey, SVGAttributeToPropertyConverter, InternedKeyHash, InternedKeyEquivalent>;


    static SVGPropertyConstructorMap & getPropertyConstructionMap()
    {
        static SVGPropertyConstructorMap gSVGAttributeCreation{};
        
        return gSVGAttributeCreation;
    }

    static bool registerSVGAttribute(InternedKey key, SVGAttributeToPropertyConverter func)
    {
        if (!key)
            return false;

        auto& pmap = getPropertyConstructionMap();
        pmap[key] = std::move(func);

        return true;
    }
    

    static bool registerSVGAttributeByName(const char* name, SVGAttributeToPropertyConverter func)
    {
        InternedKey k = PSNameTable::INTERN(name);
        return registerSVGAttribute(k, std::move(func));
    }

    static SVGAttributeToPropertyConverter getAttributeConverter(InternedKey k)
    {
        if (!k) 
            return nullptr;

        auto& mapper = getPropertyConstructionMap();
        auto it = mapper.find(k);
        if (it != mapper.end())
            return it->second;

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
        std::unordered_map<ByteSpan, std::shared_ptr<IViewable>, ByteSpanHash, ByteSpanEquivalent> fDefinitions{};
        std::unordered_map<ByteSpan, ByteSpan, ByteSpanHash, ByteSpanEquivalent> fEntities{};
        StopWatch fDocClock{};



        virtual void addElementReference(const ByteSpan& name, std::shared_ptr<IViewable> obj)
        {
            fDefinitions[name] = obj;
        }

        virtual std::shared_ptr<IViewable> getElementById(const ByteSpan& name)
        {
            auto it = fDefinitions.find(name);
            if (it != fDefinitions.end())
                return it->second;

            //printf("SVGDocument::getElementById, FAIL: ");
            //printChunk(name);

            return {};
        }
        

        // Load a URL Reference
        virtual std::shared_ptr<IViewable> findNodeByHref(const ByteSpan& inChunk)
        {
            //ByteSpan str = inChunk;

            auto id = chunk_trim(inChunk, chrWspChars);

            // The first character could be '.' or '#'
            // so we need to skip past that
            if (*id == '.' || *id == '#')
                id++;

            if (!id)
                return nullptr;

            // lookup the thing we're referencing
            return this->getElementById(id);
        }
        
        // Load a URL reference, including the 'url(' function indicator
        virtual std::shared_ptr<IViewable> findNodeByUrl(const ByteSpan& inChunk)
        {
            ByteSpan str = inChunk;

            // the id we want should look like this
            // url(#id)
            // so we need to skip past the 'url(#'
            // and then find the closing ')'
            // and then we have the id
            auto url = chunk_token(str, "(");
            auto nodeId = chunk_trim(chunk_token(str, ")"), chrWspChars);

            // sometimes the id is quoted
            // so trim that as well
            nodeId = chunk_trim(nodeId, "\"");
            nodeId = chunk_trim(nodeId, "'");

            return this->findNodeByHref(nodeId);
        }
        

        // Entities for entity expansion
        virtual void addEntity(const ByteSpan& name, ByteSpan expansion)
        {
            fEntities[name] = expansion;
        }
        
        virtual ByteSpan findEntity(const ByteSpan& name)
        {
            auto it = fEntities.find(name);
            if (it != fEntities.end())
                return it->second;

            //printf("SVGDocument::findEntity(), FAIL: ");
            //printChunk(name);

            return ByteSpan{};
        }

        // IAmGroot
        //virtual std::shared_ptr<CSSStyleSheet> styleSheet() = 0;
        //virtual void styleSheet(std::shared_ptr<CSSStyleSheet> sheet) = 0;
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
        
       virtual  std::shared_ptr<SVGVisualProperty> getVisualProperty(InternedKey key) = 0;

       bool getElementAttribute(const ByteSpan & name, ByteSpan &value) const
       {
           return fSourceElement.getElementAttribute(name, value);
       }
    };
}

namespace waavs {


    // node creation dispatch
    // Creating from a singular element, typically a self-closing tag
    using ShapeCreationMap =
        std::unordered_map<InternedKey,
        std::function<std::shared_ptr<ISVGElement>(IAmGroot*, const XmlElement&)>,
        InternedKeyHash, InternedKeyEquivalent>;

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
    using SVGContainerCreationMap =
        std::unordered_map<InternedKey,
        std::function<std::shared_ptr<ISVGElement>(IAmGroot*, XmlPull&)>,
        InternedKeyHash, InternedKeyEquivalent>;



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


    // Register named creation routines
    static void registerSVGSingularNode(const char* name, std::function<std::shared_ptr<ISVGElement>(IAmGroot*, const XmlElement&)> func) = delete;

    static void registerSVGSingularNodeByName(const char* name,
        std::function<std::shared_ptr<ISVGElement>(IAmGroot*, const XmlElement&)> func)
    {
        InternedKey k = PSNameTable::INTERN(name);
        getSVGSingularCreationMap()[k] = std::move(func);
    }

    static void registerContainerNode(InternedKey key, std::function<std::shared_ptr<ISVGElement>(IAmGroot*, XmlPull&)> creator)
    {
        getSVGContainerCreationMap()[key] = std::move(creator);
    }

    static void registerContainerNodeByName(const char* name, std::function<std::shared_ptr<ISVGElement>(IAmGroot*, XmlPull&)> creator)
    {
        InternedKey key = PSNameTable::INTERN(name);
        return registerContainerNode(key, std::move(creator));
    }



    // Convenience way to create an element
    static std::shared_ptr<ISVGElement> createSingularNode(const XmlElement& elem, IAmGroot* root)
    {
        InternedKey k = elem.nameAtom(); // nameAtom -> interned const char*
        if (!k)
            return nullptr;

        auto& m = getSVGSingularCreationMap();
        auto it = m.find(k);
        if (it != m.end())
            return it->second(root, elem);
        return nullptr;
    }


    static std::shared_ptr<ISVGElement> createContainerNode(XmlPull& iter, IAmGroot* root)
    {
        InternedKey k = iter->nameAtom();
        if (!k)
            return nullptr;
        
        auto& m = getSVGContainerCreationMap();
        auto it = m.find(k);
        if (it != m.end())
            return it->second(root, iter);
        return nullptr;
    }
}



#endif
