
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

#include "svgdatatypes.h"
#include "svgcss.h"
#include "irendersvg.h"
#include "uievent.h"
#include "collections.h"
#include "svgatoms.h"

namespace waavs 
{
    struct IAmGroot;            // forward declaration

    struct SVGObject
    {
    protected:
        bool fNeedsBinding{ false };

    public:

        // default and copy constructor not allowed, let's see what breaks
        SVGObject() = default;

        // want to know when a copy or assignment is happening
        // so mark these as 'delete' for now so we can catch it
        SVGObject(const SVGObject& other) = delete;
        SVGObject& operator=(const SVGObject& other) = delete;

        virtual ~SVGObject() = default;


        bool needsBinding() const noexcept { return fNeedsBinding; }
        void setNeedsBinding(bool needsIt) noexcept { fNeedsBinding = needsIt; }


        virtual void bindToContext(IRenderSVG*, IAmGroot*) noexcept = 0;

        // sub-classes should return something interesting as BLVar
        // This can be used for styling, so images, colors, patterns, gradients, etc
        virtual const BLVar getVariant(IRenderSVG*, IAmGroot*) noexcept { return BLVar::null(); }


    };
}


namespace waavs {

    struct IViewable : public SVGObject
    {
        bool fIsVisible{ true };
        ByteSpan fName{};
        ByteSpan fId{};      // The id of the element

        
        const ByteSpan& id() const noexcept { return fId; }
        void setId(const ByteSpan& aid) noexcept { fId = aid; }

        virtual BLRect viewPort() const = 0;
        virtual BLRect getBBox() const = 0;
        virtual bool contains(double x, double y) { return false; }
        
        void name(const ByteSpan& aname) { fName = aname; }
        const ByteSpan& name() const { return fName; }

        bool visible() const { return fIsVisible; }
        void visible(bool visible) { fIsVisible = visible; }

        virtual void fixupStyleAttributes(IRenderSVG* ctx, IAmGroot* groot) = 0;

        virtual void update(IAmGroot*) = 0;
        virtual void draw(IRenderSVG*, IAmGroot*) = 0;

    };
}


namespace waavs {


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

        void autoDraw(bool value) { fAutoDraw = value; }
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
        virtual void drawSelf(IRenderSVG*, IAmGroot*) { ; }

        virtual void draw(IRenderSVG* ctx, IAmGroot* groot)
        {
            //printf("SVGVisualProperty::draw == ");
            //printChunk(fRawValue);
            if (needsBinding())
                this->bindToContext(ctx, groot);
            
            if (isSet())
                this->drawSelf(ctx, groot);
        }

    };


    //===================================================
    // Handling attribute conversion to properties
    // 
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





namespace waavs {
    // Interface Am Graphics Root (IAmGroot) 
    // Core interface to hold document level state, primarily
    // for the purpose of looking up nodes, but also for style sheets
    struct IAmGroot
    {
        std::unordered_map<ByteSpan, std::shared_ptr<IViewable>, ByteSpanHash, ByteSpanEquivalent> fDefinitions{};
        std::unordered_map<ByteSpan, ByteSpan, ByteSpanHash, ByteSpanEquivalent> fEntities{};
        
        
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
            ByteSpan str = inChunk;

            auto id = chunk_trim(str, chrWspChars);

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
        virtual std::shared_ptr<CSSStyleSheet> styleSheet() = 0;
        virtual void styleSheet(std::shared_ptr<CSSStyleSheet> sheet) = 0;



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
        bool fIsStructural{ true };
		XmlElement fSourceElement{};
        
       virtual  std::shared_ptr<SVGVisualProperty> getVisualProperty(InternedKey key) = 0;

       bool isStructural() const { return fIsStructural; }
       void setIsStructural(bool aStructural) { fIsStructural = aStructural; }
       
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
    static void registerSVGSingularNode(const char* name,
        std::function<std::shared_ptr<ISVGElement>(IAmGroot*, const XmlElement&)> func)
    {
        InternedKey k = PSNameTable::INTERN(name);
        getSVGSingularCreationMap()[k] = std::move(func);
    }

    static void registerContainerNode(InternedKey key, std::function<std::shared_ptr<ISVGElement>(IAmGroot*, XmlPull&)> creator)
    {
        getSVGContainerCreationMap()[key] = std::move(creator);
    }

    static void registerContainerNodeByName(const char* name,
        std::function<std::shared_ptr<ISVGElement>(IAmGroot*, XmlPull&)> creator)
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




namespace waavs {

    //================================================
    // SVGGraphicsElement
    //================================================
    struct SVGGraphicsElement : public ISVGElement
    {
        BLVar fVar{};

        XmlAttributeCollection fPresentationAttributes{};
        XmlAttributeCollection fAttributes;

        ByteSpan fAttributeSpan{};
        ByteSpan fStyleAttribute{};
        ByteSpan fClassAttribute{};

        
        std::unordered_map<InternedKey, std::shared_ptr<SVGVisualProperty>, InternedKeyHash, InternedKeyEquivalent> fVisualProperties{};

        std::vector<std::shared_ptr<IViewable>> fNodes{};


        SVGGraphicsElement()
        {
            setNeedsBinding(true);
        }

        const BLVar getVariant(IRenderSVG *ctx, IAmGroot *groot) noexcept override
        {
            // if our variant is null
            // traverse down our fNodes, until we find
            // something that reports other than null
            // and return that.
            if (fVar.isNull())
            {
                for (auto& node : fNodes)
                {
                    auto& var = node->getVariant(ctx, groot);
                    if (!var.isNull())
                    {
                        return node->getVariant(ctx, groot);
                    }
                }
            }

            return fVar;
        }

        // The viewport is what the graphics report as their extent
        // This is NOT the viewBox, which determines their internal coordinate system
        BLRect viewPort() const override
        {
            BLRect extent{};
            bool firstOne = true;

            // traverse the graphics
            // expand viewport to include their frames, without alteration
            for (auto& g : fNodes)	// std::shared_ptr<IGraphic> g
            {
                if (firstOne) {
                    extent = g->viewPort();
                    firstOne = false;
                }
                else {
                    expandRect(extent, g->viewPort());
                }
            }

            return extent;
        }
        BLRect getBBox() const override { return BLRect{}; }
        



        bool hasAttribute(const ByteSpan& name) const noexcept 
        { 
            InternedKey key = PSNameTable::INTERN(name);
            return fAttributes.hasValue(key); 
        }

        ByteSpan getAttribute(InternedKey key) const noexcept
        {
            ByteSpan value{};
            fAttributes.getValue(key, value);
            return value;
        }

        ByteSpan getAttributeByName(const char * name) const noexcept
        {
            return getAttribute(PSNameTable::INTERN(name));
        }

        void setAttribute(const ByteSpan& name, const ByteSpan& value) noexcept
		{
			fAttributes.addValueBySpan(name,value);
		}


        // Property management
        void addVisualProperty(InternedKey key, std::shared_ptr<SVGVisualProperty> prop)
        {
            fVisualProperties[key] = prop;
        }

        std::shared_ptr<SVGVisualProperty> getVisualProperty(InternedKey key) override
        {
            auto it = fVisualProperties.find(key);
            if (it != fVisualProperties.end())
                return it->second;

            return nullptr;
        }

        
        // Adding nodes to our tree
        virtual bool addNode(std::shared_ptr < ISVGElement > node, IAmGroot* groot)
        {
            if (node == nullptr || groot == nullptr)
                return false;

            // Get an attribute 'id' if it exists
            ByteSpan nodeId = node->id();
            if (!nodeId.empty())
                groot->addElementReference(nodeId, node);

            if (node->isStructural()) {
                fNodes.push_back(node);
            }

            return true;
        }

        virtual void loadSelfClosingNode(const XmlElement& elem, IAmGroot* groot)
        {
            //printf("SVGGraphicsElement::loadSelfClosingNode: \n");

            auto anode = createSingularNode(elem, groot);
            if (anode != nullptr) {
                this->addNode(anode, groot);
            }
            else {
                //printf("SVGGraphicsElement::loadSelfClosingNode UNKNOWN[%s]\n", toString(elem.name()).c_str());
                //printXmlElement(elem);
            }
        }

        virtual void loadEndTag(const XmlElement&, IAmGroot*)
        {
            //printf("SVGGraphicsElement::loadEndTag [%s]\n", toString(elem.name()).c_str());
        }

        virtual void loadContentNode(const XmlElement&, IAmGroot*)
        {
            //printf("SVGCompountNode::loadContentNode\n");
            //printXmlElement(elem);
            // Do something with content nodes	
        }

        virtual void loadCDataNode(const XmlElement&, IAmGroot*)
        {
            //printf("SVGGraphicsElement::loadCDataNode\n");
            //printXmlElement(elem);
            // Do something with CDATA nodes
        }

        virtual void loadComment(const XmlElement&, IAmGroot*)
        {
            //printf("SVGGraphicsElement::loadComment\n");
            //printXmlElement(elem);
            // Do something with comments
        }

        // Sometimes we come across a element tag name that we 
        // don't know anything about.  We want to just skip over 
        // those nodes without trying to do any processing.
        void skipSubtree(XmlPull& iter)
        {
            int depth = 1;
            while (depth > 0 && iter.next())
            {
                const XmlElement& elem = *iter;
                switch (elem.kind())
                {
                    case XML_ELEMENT_TYPE_START_TAG:                    // <tag>
                        depth++;
                        break;
                    case XML_ELEMENT_TYPE_END_TAG:                      // </tag>
                        depth--;
                        break;
                    case XML_ELEMENT_TYPE_SELF_CLOSING:                 // <tag/>
                    case XML_ELEMENT_TYPE_CONTENT:                      // <tag>content</tag>
                    case XML_ELEMENT_TYPE_COMMENT:                      // <!-- comment -->
                    case XML_ELEMENT_TYPE_CDATA:                        // <![CDATA[<greeting>Hello, world!</greeting>]]>
                        // do nothing
                        break;
                    case XML_ELEMENT_TYPE_DOCTYPE:                      // <!DOCTYPE greeting SYSTEM "hello.dtd">
                    case XML_ELEMENT_TYPE_ENTITY:                       // <!ENTITY hello "Hello">
                    case XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION:       // <?target data?>
                    case XML_ELEMENT_TYPE_XMLDECL:                      // <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
                    case XML_ELEMENT_TYPE_EMPTY_TAG:                    // <br>
                    default:
                        // Ignore anything else
                        break;
                }
            }
        }

        virtual  void loadStartTag(XmlPull& iter, IAmGroot* groot)
        {
            // Add a child, and call loadIterator
            // If the name of the element is found in the map,
            // then create a new node of that type and add it
            // to the list of nodes.
            auto node = createContainerNode(iter, groot);
            if (node != nullptr) {
                this->addNode(node, groot);
            }
            else {
                // If we're here, we've run across a start tag that does
                // not have a registered factory method.
                // so, we just consume its content until we find the matching end tag
                // and throw it away.
                skipSubtree(iter);
            }

        }


        // loadFromXmlElement()
        // 
        // At this stage, we're interested in preserving the data associated
        // with the attributes, but not necessarily parsing all the attributes,
        // because we don't have enough information to actually resolve them 
        // all, until the document is fully built.
        // Here, we capture the element name, and the ID field if it exists
        // and the attribute spans for later processing.
        // We explicitly separate out 'id', 'class', 'style', and the rest of the presentation attributes
        // It's in 'fixupSelfStyleAttributes()', that we actuall bind attributes
        // to actual fixed values.
        
        virtual void loadFromXmlElement(const XmlElement& elem, IAmGroot* groot)
        {
            fSourceElement = elem;

            // Save the name if we've got one
            name(elem.name());

            // Get the span for all the presentation attributes on the element to start
            fAttributeSpan = elem.data();

            // Scan the list of attributes, separating out the id, style and class attributes

            // since the readNextKeyAttribute() function is destructive of the span
            // we need to make a copy of the span to work with
            ByteSpan src = elem.data();

            // Create a couple of spans to hold the name and value
            // pairs.  These will be reused on each iteration
            ByteSpan attrName{};
            ByteSpan attrValue{};

            // Loop through the attributes
            // setting well known attributes directly
            // and presentation attributes into their own collection
            while (readNextKeyAttribute(src, attrName, attrValue))
            {
                InternedKey attrKey = PSNameTable::INTERN(attrName);

                if (attrKey == svgattr::id())
                {
                    setId(attrValue);
                }
                else if (attrKey == svgattr::style() && !attrValue.empty())
                {
                    fStyleAttribute = attrValue;
                }
                else if (attrKey == svgattr::klass())
                {
                    fClassAttribute = attrValue;
                }
                else {
                    fPresentationAttributes.addValue(attrKey, attrValue);
                }
            }

        }
        
        virtual void loadFromXmlPull(XmlPull& iter, IAmGroot* groot)
        {
            this->loadFromXmlElement(*iter, groot);

            while (iter.next())
            {
                const XmlElement& elem = *iter;
                switch (elem.kind())
                {
                    case XML_ELEMENT_TYPE_START_TAG:                    // <tag>
                        this->loadStartTag(iter, groot);
                        break;
                    case XML_ELEMENT_TYPE_END_TAG:                      // </tag>
                        this->loadEndTag(elem, groot);
                        return;
                    case XML_ELEMENT_TYPE_SELF_CLOSING:                 // <tag/>
                        this->loadSelfClosingNode(elem, groot);
                        break;
                    case XML_ELEMENT_TYPE_CONTENT:                      // <tag>content</tag>
                        this->loadContentNode(elem, groot);
                        break;
                    case XML_ELEMENT_TYPE_COMMENT:                      // <!-- comment -->
                        this->loadComment(elem, groot);
                        break;
                    case XML_ELEMENT_TYPE_CDATA:                        // <![CDATA[<greeting>Hello, world!</greeting>]]>
                        this->loadCDataNode(elem, groot);
                        break;
                    case XML_ELEMENT_TYPE_DOCTYPE:                      // <!DOCTYPE greeting SYSTEM "hello.dtd">
                    case XML_ELEMENT_TYPE_ENTITY:                       // <!ENTITY hello "Hello">
                    case XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION:       // <?target data?>
                    case XML_ELEMENT_TYPE_XMLDECL:                      // <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
                    case XML_ELEMENT_TYPE_EMPTY_TAG:                    // <br>
                    default:
                    {
                        // Ignore anything else
                    }		
                    break;
                }
            }
        }



        virtual void fixupSelfStyleAttributes(IRenderSVG*, IAmGroot*) 
        {
           // printf("fixupSelfStyleAttributes\n");
        }


        void fixupStyleAttributes(IRenderSVG* ctx, IAmGroot* groot) override
        {
            // First, lookup CSS based on tagname
            // See if there's an element selector for the current element
            if (groot != nullptr) {
                if (!name().empty())
                {
                    auto esel = groot->styleSheet()->getSelector(CSS_SELECTOR_ELEMENT, name());
                    if (esel != nullptr)
                    {
                        fAttributes.mergeAttributes(esel->attributes());
                    }
                }

                // Check for id selector if we have one
                if (id())
                {
                    auto idsel = groot->styleSheet()->getSelector(CSS_SELECTOR_ID, id());
                    if (idsel != nullptr)
                    {
                        fAttributes.mergeAttributes(idsel->attributes());
                    }
                }

                // Lookup any attributes based on the class, if specified
                ByteSpan classChunk = fClassAttribute;
                while (classChunk)
                {
                    // peel a word off the front
                    auto classId = chunk_token(classChunk, chrWspChars);


                    auto csel = groot->styleSheet()->getSelector(CSS_SELECTOR_CLASS, classId);
                    if (csel != nullptr)
                    {
                        fAttributes.mergeAttributes(csel->attributes());
                    }
                    else {
                        //printf("SVGVisualNode::bindPropertiesToGroot, ERROR - NO CLASS SELECTOR FOR %s\n", toString(classId).c_str());
                    }
                }
            }

            // Upsert any of the attributes associated with 'style' attribute if they exist
            if (fStyleAttribute) {
                parseStyleAttribute(fStyleAttribute, fAttributes);
            }

            // Finally, override any of the attributes already set with the 
            // presentation attributes of the current element, if any
            fAttributes.mergeAttributes(fPresentationAttributes);
            this->fixupSelfStyleAttributes(ctx, groot);

            // Use up some of the attributes
            ByteSpan displayAttr{};
            if (fAttributes.getValue(svgattr::display(), displayAttr))
            {
                displayAttr = chunk_trim(displayAttr, chrWspChars);
                InternedKey dv = PSNameTable::INTERN(displayAttr);

                if (displayAttr == svgval::none())
                    visible(false);
            }

            // Get transformation matrix if it exists as early as possible
            // but after attributes have been set.
            //fHasTransform = parseTransform(getAttribute("transform"), fTransform);

        }
        
        // IViewable



        


        // signal all properties they should
        // update themselves
        virtual void updateProperties(IAmGroot* groot)
        {
            for (auto& prop : fVisualProperties)
            {
                prop.second->update(groot);
            }
        }

        void updateChildren(IAmGroot* groot)
        {
            for (auto& node : fNodes)
            {
                node->update(groot);
            }
        }

        virtual void updateSelf(IAmGroot* groot) {

        }
        
        void update(IAmGroot* groot) override
        {
            this->updateProperties(groot);
            this->updateSelf(groot);
            this->updateChildren(groot);
        }
        


        
        // convertAttributesToProperties
        // Take the step of converting a raw attribute
        // value into a specific display property if a routine
        // exists for it.
        void convertAttributesToProperties(IRenderSVG* ctx, IAmGroot* groot)
        {
            //
            for (auto& attr : fAttributes.values())
            {
                // Find an attribute to property converter, if it exists
                auto propertyMapper = getAttributeConverter(attr.first);
                if (propertyMapper)
                {
                    auto prop = propertyMapper(fAttributes);
                    if (prop != nullptr)
                        addVisualProperty(attr.first, prop);
                }
            }
        }

        virtual void bindSelfToContext(IRenderSVG*, IAmGroot*) { ; }


        // For compound nodes (which have children) we want to 
        // do the base stuff (binding properties) then bind the children
        // If you sub-class this, you should call this first
        // then do your own thing.  We don't want to call a 'bindSelfToGroot'
        // here, because that complicates the interactions and sequences of things
        // so just override bindToGroot
        void bindToContext(IRenderSVG* ctx, IAmGroot* groot) noexcept override
        {
            // First, find any style attributes that might apply
            // to this element
            this->fixupStyleAttributes(ctx, groot);


            // Convert the attributes that have a property registration 
            // into VisualProperty objects
            convertAttributesToProperties(ctx, groot);

            // Tell the structure to bind the rest of its stuff
            this->bindSelfToContext(ctx, groot);

            // Set the object frame, because some attributes
            // will need that when binding
            // ctx->setObjectFrame(getBBox());

            setNeedsBinding(false);
        }

        virtual void applyProperties(IRenderSVG* ctx, IAmGroot* groot)
        {
            // BUGBUG - need to apply transform appropriately here
            //if (fHasTransform)  //fTransform.type() != BL_MATRIX2D_TYPE_IDENTITY)
            //    ctx->applyTransform(fTransform);
			auto tform = getVisualProperty(svgattr::transform());
			if (tform)
				tform->draw(ctx, groot);

            for (auto& prop : fVisualProperties) {
                // We've already applied the transform, so skip
                // applying it again.
                if (prop.first == svgattr::transform())
                    continue;

                if (prop.second->autoDraw() && prop.second->isSet())
                {
                    prop.second->draw(ctx, groot);
                }
            }
        }

        virtual void drawChildren(IRenderSVG* ctx, IAmGroot* groot)
        {
            for (auto& node : fNodes) {
                // Restore our context before drawing each child
                //ctx->setObjectFrame(getBBox());

                node->draw(ctx, groot);
            }
        }
        
        
        virtual void drawSelf(IRenderSVG*, IAmGroot*)
        {
            ;
        }
        
        
        void draw(IRenderSVG* ctx, IAmGroot* groot) override
        {
            if (!visible())
                return;

            ctx->push();

            // Should have valid bounding box by now
            // so set objectFrame on the context
            BLRect bbox = getBBox();
            if (bbox.w && bbox.h)
                ctx->setObjectFrame(bbox);

            if (needsBinding())
                this->bindToContext(ctx, groot);
            
            this->applyProperties(ctx, groot);
            this->drawSelf(ctx, groot);

            this->drawChildren(ctx, groot);

            ctx->pop();
        }
    };
}









#endif
