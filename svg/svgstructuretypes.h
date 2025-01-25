#pragma once

#ifndef SVGSTRUCTURETYPES_H
#define SVGSTRUCTURETYPES_H

#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstdint>		// uint8_t, etc
#include <cstddef>		// nullptr_t, ptrdiff_t, size_t

#include "maths.h"

#include "svgdatatypes.h"
#include "svgcss.h"
#include "irendersvg.h"
#include "uievent.h"
#include "collections.h"
#include "xmlscan.h"




namespace waavs {
    struct IAmGroot;        // forward declaration
    struct IManageReferences;  // forward declaration

    struct SVGObject
    {
    protected:
        ByteSpan fId{};      // The id of the element

        bool fNeedsBinding{ false };

    public:

        // default and copy constructor not allowed, let's see what breaks
        SVGObject() = default;

        // want to know when a copy or assignment is happening
        // so mark these as 'delete' for now so we can catch it
        SVGObject(const SVGObject& other) = delete;
        SVGObject& operator=(const SVGObject& other) = delete;

        virtual ~SVGObject() = default;


        const ByteSpan& id() const noexcept { return fId; }
        void id(const ByteSpan& aid) noexcept { fId = aid; }


        bool needsBinding() const noexcept { return fNeedsBinding; }
        void needsBinding(bool needsIt) noexcept { fNeedsBinding = needsIt; }


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


        virtual BLRect frame() const = 0;
        virtual BLRect getBBox() const = 0;
        virtual bool contains(double x, double y) { return false; }
        
        void name(const ByteSpan& aname) { fName = aname; }
        const ByteSpan& name() const { return fName; }

        const bool visible() const { return fIsVisible; }
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
        bool fAutoDraw{ true };
        bool fIsSet{ false };
        ByteSpan fRawValue;


        SVGVisualProperty(IAmGroot*) :SVGObject(), fIsSet(false) { needsBinding(false); }

        SVGVisualProperty(const SVGVisualProperty& other) = delete;
        SVGVisualProperty& operator=(const SVGVisualProperty& rhs) = delete;


        bool set(const bool value) { fIsSet = value; return value; }
        bool isSet() const { return fIsSet; }

        void autoDraw(bool value) { fAutoDraw = value; }
        bool autoDraw() const { return fAutoDraw; }

        const ByteSpan& rawValue() const { return fRawValue; }

        virtual bool loadSelfFromChunk(const ByteSpan&)
        {
            return true;
        }

        bool loadFromChunk(const ByteSpan& inChunk)
        {
            auto s = chunk_trim(inChunk, chrWspChars);

            if (!s)
                return false;

            fRawValue = s;

            return this->loadSelfFromChunk(fRawValue);
        }

        virtual bool loadFromAttributes(const XmlAttributeCollection& attrs)
        {
            auto attr = attrs.getAttribute(id());
            if (!attr)
                return false;
            
            return loadFromChunk(attr);
        }

		void bindToContext(IRenderSVG*, IAmGroot*) noexcept override
		{
            needsBinding(false);
			// do nothing
		}

        
        virtual void update(IAmGroot*) { ; }

        // Apply propert to the context conditionally
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
    using SVGPropertyConstructorMap = std::unordered_map<ByteSpan, SVGAttributeToPropertyConverter, ByteSpanHash, ByteSpanEquivalent>;


    static SVGPropertyConstructorMap & getPropertyConstructionMap()
    {
        static SVGPropertyConstructorMap gSVGAttributeCreation{};
        
        return gSVGAttributeCreation;
    }

    // Convenient function to register property constructors
    static void registerSVGAttribute(const ByteSpan& name, SVGAttributeToPropertyConverter func)
    {
        getPropertyConstructionMap()[name] = func;
    }

    static SVGAttributeToPropertyConverter getAttributeConverter(const ByteSpan &name)
    {
        // Next, see if there is a property registered for the attribute
        auto & mapper = getPropertyConstructionMap();
        auto it = mapper.find(name);
        if (it != mapper.end())
        {
            return it->second;
        }

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
        
        //virtual std::shared_ptr<SVGViewable> findNodeByUrl(const ByteSpan& inChunk) = 0;
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
            auto id = chunk_trim(chunk_token(str, ")"), chrWspChars);

            // sometimes the id is quoted
            // so trim that as well
            id = chunk_trim(id, "\"");
            id = chunk_trim(id, "'");

            return this->findNodeByHref(id);
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
        virtual FontHandler* fontHandler() const = 0;

        virtual std::shared_ptr<CSSStyleSheet> styleSheet() = 0;
        virtual void styleSheet(std::shared_ptr<CSSStyleSheet> sheet) = 0;



        virtual ByteSpan systemLanguage() { return "en"; } // BUGBUG - What a big cheat!!

        virtual double canvasWidth() const = 0;
        virtual double canvasHeight() const = 0;
        
        virtual double dpi() const = 0;
        virtual void dpi(const double d) = 0;
    };

}

namespace waavs {
    struct ISVGElement : public IViewable 
    {
        bool fIsStructural{ true };
        
        
       virtual  std::shared_ptr<SVGVisualProperty> getVisualProperty(const ByteSpan& name) const = 0;

       bool isStructural() const { return fIsStructural; }
       void isStructural(bool aStructural) { fIsStructural = aStructural; }
       
    };
}

namespace waavs {
    // Geometry node creation dispatch
    // Creating from a singular element
    using ShapeCreationMap = std::unordered_map<ByteSpan, std::function<std::shared_ptr<ISVGElement>(IAmGroot* root, const XmlElement& elem)>, ByteSpanHash, ByteSpanEquivalent>;

    // compound node creation dispatch - 'g', 'symbol', 'pattern', 'linearGradient', 'radialGradient', 'conicGradient', 'image', 'style', 'text', 'tspan', 'use'
    using SVGContainerCreationMap = std::unordered_map<ByteSpan, std::function<std::shared_ptr<ISVGElement>(IAmGroot* aroot, XmlElementIterator& iter)>, ByteSpanHash, ByteSpanEquivalent>;






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
    static void registerSVGSingularNode(const ByteSpan& name, std::function<std::shared_ptr<ISVGElement>(IAmGroot* root, const XmlElement& elem)> func)
    {
        getSVGSingularCreationMap()[name] = func;
    }

    static void registerContainerNode(const ByteSpan& name, std::function<std::shared_ptr<ISVGElement>(IAmGroot* aroot, XmlElementIterator& iter)> creator)
    {
        getSVGContainerCreationMap()[name] = creator;
    }



    // Convenience way to create an element
    static std::shared_ptr<ISVGElement> createSingularNode(const XmlElement& elem, IAmGroot* root)
    {
        ByteSpan aname = elem.name();
        auto it = getSVGSingularCreationMap().find(aname);
        if (it != getSVGSingularCreationMap().end())
        {
            return it->second(root, elem);
        }
        return nullptr;
    }

    static std::shared_ptr<ISVGElement> createContainerNode(XmlElementIterator& iter, IAmGroot* root)
    {
        ByteSpan aname = iter->name();
        auto it = getSVGContainerCreationMap().find(aname);
        if (it != getSVGContainerCreationMap().end())
        {
            return it->second(root, iter);
        }
        return nullptr;
    }
}




namespace waavs {

    //================================================
    // SVGGraphicsElement
    //================================================
    struct SVGGraphicsElement : public ISVGElement
    {
        static constexpr int BUILD_STATE_INITIAL = 0;
        static constexpr int BUILD_STATE_OPEN = 1;
        static constexpr int BUILD_STATE_CLOSE = 2;

        int buildState = BUILD_STATE_OPEN;
        BLVar fVar{};

        XmlAttributeCollection fPresentationAttributes{};
        XmlAttributeCollection fAttributes;

        ByteSpan fAttributeSpan{};
        ByteSpan fStyleAttribute{};
        ByteSpan fClassAttribute{};

		BLMatrix2D fTransform{};
		bool fHasTransform{ false };
        
        std::unordered_map<ByteSpan, std::shared_ptr<SVGVisualProperty>, ByteSpanHash, ByteSpanEquivalent> fVisualProperties{};
        std::vector<std::shared_ptr<IViewable>> fNodes{};




        SVGGraphicsElement()
        {
            needsBinding(true);
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

        BLRect frame() const override
        {
            BLRect extent{};
            bool firstOne = true;

            // traverse the graphics
            // expand bounding box to include their frames, without alternation
            for (auto& g : fNodes)	// std::shared_ptr<IGraphic> g
            {
                if (firstOne) {
                    extent = g->frame();
                    firstOne = false;
                }
                else {
                    expandRect(extent, g->frame());
                }
            }

            return extent;
        }
        BLRect getBBox() const override { return BLRect{}; }
        



        bool hasAttribute(const ByteSpan& key) const noexcept { return fAttributes.hasAttribute(key); }

        ByteSpan getAttribute(const ByteSpan& key) const noexcept
        {
            return fAttributes.getAttribute(key);
        }
        
		void setAttribute(const ByteSpan& key, const ByteSpan& value) noexcept
		{
			fAttributes.addAttribute(key,value);
		}

        std::shared_ptr<SVGVisualProperty> getVisualProperty(const ByteSpan& name) const noexcept
        {
            auto it = fVisualProperties.find(name);
            if (it != fVisualProperties.end())
                return it->second;

            return nullptr;
        }

        
        virtual bool addNode(std::shared_ptr < ISVGElement > node, IAmGroot* groot)
        {
            if (node == nullptr || groot == nullptr)
                return false;

            if (!node->id().empty())
                groot->addElementReference(node->id(), node);

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

        virtual  void loadCompoundNode(XmlElementIterator& iter, IAmGroot* groot)
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
                auto & mapper = getSVGContainerCreationMap();
                auto & mapperfunc = mapper["g"];
                if (mapperfunc)
                    node = mapperfunc(groot, iter);
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
            while (readNextKeyAttribute(src, attrName, attrValue))
            {
                if (attrName == "id")
                {
                    id(attrValue);
                }
                else if (attrName == "style")
                {
                    fStyleAttribute = attrValue;
                }
                else if (attrName == "class")
                {
                    fClassAttribute = attrValue;
                }
                else {
                    fPresentationAttributes.addAttribute(attrName, attrValue);
                }
            }

        }
        

        virtual void loadFromXmlIterator(XmlElementIterator& iter, IAmGroot* groot)
        {
            // First, loadFromXmlElement because we're sitting on our opening element
            // and we need to gather our own attributes
            this->loadFromXmlElement(*iter, groot);
            
            buildState = BUILD_STATE_OPEN;

            while (iter && (buildState != BUILD_STATE_CLOSE))
            {
                // move iterator forward to get to next item
                iter++;

                const XmlElement& elem = *iter;

                // BUGBUG - debug
                //printXmlElement(elem);

                if (!elem)
                    break;

                // Skip over these node types we don't know how to process


                switch (buildState)
                {
                    // We are in an open element.  There are a couple of actions
                    // that are valid from here
                    // 1) We get another open - add a child, call loadIterator
                    // 2) We get a self closing - add a child, call loadXmlElement
                    // 3) We get a close - close the current element
                    // 4) anything else, just ignore it
                case BUILD_STATE_OPEN:
                {
                    if (elem.isSelfClosing()) {
                        this->loadSelfClosingNode(elem, groot);
                    }
                    else if (elem.isStart())
                    {
                        this->loadCompoundNode(iter, groot);
                    }
                    else if (elem.isEnd())
                    {
                        // Close the current element
                        buildState = BUILD_STATE_CLOSE;
                        this->loadEndTag(elem, groot);
                    }
                    else if (elem.isContent())
                    {
                        this->loadContentNode(elem, groot);
                    }
                    else if (elem.isCData())
                    {
                        this->loadCDataNode(elem, groot);
                    }
                    else if (elem.isComment())
                    {
                        this->loadComment(elem, groot);
                    }
                    else
                    {
                        // Ignore anything else
                        //printf("SVGGraphicsElement::loadFromXmlIterator ==> IGNORING kind(%d) name:", elem.kind());
                        //printChunk(elem.nameSpan());
                        //printChunk(elem.data());

                        //printf("SVGGraphicsElement::loadFromXmlIterator ==> IGNORING: %s\n", elem.name().c_str());
                        //printXmlElement(elem);
                    }
                }
                break;

                }	// end of switch


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
                    auto esel = groot->styleSheet()->getElementSelector(name());
                    if (esel != nullptr)
                    {
                        fAttributes.mergeAttributes(esel->attributes());
                    }
                }

                // Check for id selector if we have one
                if (id())
                {
                    auto idsel = groot->styleSheet()->getIDSelector(id());
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


                    auto csel = groot->styleSheet()->getClassSelector(classId);
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

            // BUGBUG - this is a hack to get the fill and stroke
            // It needs to go deeper and get the color attribute that's from the tree
            // where the current node is just the latest.
            // if there is a stroke attribute with a value of 'currentColor', then look
            // for a 'color' attribute, and set the stroke color to that value
            //auto strokeColor = fAttributes.getAttribute("stroke");

            //if (strokeColor && (strokeColor == "currentColor"))
            //{
            //    auto colorValue = fAttributes.getAttribute("color");
            //    if (colorValue)
            //    {
            //        fAttributes.addAttribute("stroke", colorValue);
            //    }
            //}
            this->fixupSelfStyleAttributes(ctx, groot);

            // Use up some of the attributes
            ByteSpan display = fAttributes.getAttribute("display");
            if (display)
            {
                display = chunk_trim(display, chrWspChars);

                if (display == "none")
                    visible(false);
            }

            // Get transformation matrix if it exists as early as possible
            // but after attributes have been set.
            fHasTransform = parseTransform(getAttribute("transform"), fTransform);

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
            for (auto& attr : fAttributes.attributes())
            {
                // Find an attribute to property converter, if it exists
                auto propertyMapper = getAttributeConverter(attr.first);
                if (propertyMapper)
                {
                    auto prop = propertyMapper(fAttributes);
                    if (prop != nullptr)
                        fVisualProperties[attr.first] = prop;
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
            this->fixupStyleAttributes(ctx, groot);
            convertAttributesToProperties(ctx, groot);

            this->bindSelfToContext(ctx, groot);

            needsBinding(false);
        }

        virtual void applyProperties(IRenderSVG* ctx, IAmGroot* groot)
        {
            // Apply transform if it's not the identity transform
            if (fHasTransform)  //fTransform.type() != BL_MATRIX2D_TYPE_IDENTITY)
                ctx->applyTransform(fTransform);

            for (auto& prop : fVisualProperties) {
                if (prop.second->autoDraw() && prop.second->isSet())
                    prop.second->draw(ctx, groot);
            }
        }

        virtual void drawChildren(IRenderSVG* ctx, IAmGroot* groot)
        {
            for (auto& node : fNodes) {
                // Restore our context before drawing each child
                ctx->objectFrame(getBBox());

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

            if (needsBinding())
                this->bindToContext(ctx, groot);
            
            // Should have valid bounding box by now
            // so set objectFrame on the context
			ctx->objectFrame(getBBox());
            
            this->applyProperties(ctx, groot);
            this->drawSelf(ctx, groot);

            this->drawChildren(ctx, groot);

            ctx->pop();
        }
    };
}









#endif
