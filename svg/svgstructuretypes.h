#pragma once


#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstdint>		// uint8_t, etc
#include <cstddef>		// nullptr_t, ptrdiff_t, size_t

#include "maths.h"

#include "svgdatatypes.h"

#include "irendersvg.h"
#include "uievent.h"
#include "collections.h"





namespace waavs {
    struct IAmGroot;        // forward declaration
    struct IManageReferences;  // forward declaration
    struct SVGViewable;         // forward declaration

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


        virtual void bindToGroot(IAmGroot* groot, SVGViewable* container) noexcept {  }

        // sub-classes should return something interesting as BLVar
        // This can be used for styling, so images, colors, patterns, gradients, etc
        virtual const BLVar getVariant() noexcept { return BLVar::null(); }


    };
}

    
    namespace waavs {


        // SVGVisualProperty
        // This is meant to be the base class for things that are optionally
        // used to alter the graphics context.
        // If isSet() is true, then the drawSelf() is called.
        // sub-classes should override drawSelf() to do the actual drawing
        //
        // This is used for things like; Paint, Transform, Miter, etc.
        //
        struct SVGVisualProperty : public SVGObject
        {
            bool fAutoDraw{ true };
            bool fIsSet{ false };
            ByteSpan fRawValue;


            SVGVisualProperty(IAmGroot* groot) :SVGObject(), fIsSet(false) { needsBinding(false); }

            SVGVisualProperty(const SVGVisualProperty& other) = delete;
            SVGVisualProperty& operator=(const SVGVisualProperty& rhs) = delete;


            bool set(const bool value) { fIsSet = value; return value; }
            bool isSet() const { return fIsSet; }

            void autoDraw(bool value) { fAutoDraw = value; }
            bool autoDraw() const { return fAutoDraw; }

            const ByteSpan& rawValue() const { return fRawValue; }

            virtual bool loadSelfFromChunk(const ByteSpan& chunk)
            {
                return false;
            }

            bool loadFromChunk(const ByteSpan& inChunk)
            {
                auto s = chunk_trim(inChunk, xmlwsp);

                if (!s)
                    return false;

                fRawValue = s;

                return loadSelfFromChunk(fRawValue);
            }

            virtual bool loadFromAttributes(const XmlAttributeCollection& attrs)
            {
                return loadFromChunk(attrs.getAttribute(id()));
            }

            
            virtual void update(IAmGroot* groot) { ; }

            // Apply propert to the context conditionally
            virtual void drawSelf(IRenderSVG* ctx, IAmGroot* groot)
            {
                ;
            }

            virtual void draw(IRenderSVG* ctx, IAmGroot* groot)
            {
                //printf("SVGVisualProperty::draw == ");
                //printChunk(fRawValue);

                if (isSet())
                    drawSelf(ctx, groot);
            }

        };

        // Collection of property constructors
        static std::unordered_map<ByteSpan, std::function<std::shared_ptr<SVGVisualProperty>(const XmlAttributeCollection& attrs)>, ByteSpanHash> gSVGAttributeCreation;

        // Convenient function to register property constructors
        static void registerSVGAttribute(const ByteSpan& name, std::function<std::shared_ptr<SVGVisualProperty>(const XmlAttributeCollection &attrs)> func)
        {
            gSVGAttributeCreation[name] = func;
        }

    }
    
    namespace waavs {
    struct SVGViewable : public SVGObject //, public XmlAttributeCollection
    {
    protected:
        XmlAttributeCollection fPresentationAttributes{};
        XmlAttributeCollection fAttributes;
        bool fIsVisible{ true };
        ByteSpan fName{};
        ByteSpan fAttributeSpan{};
        ByteSpan fStyleAttribute{};
		ByteSpan fClassAttribute{};

        
    public:
        SVGViewable(IAmGroot* groot) :SVGObject() {}


        // IViewable
        void name(const ByteSpan& aname) { fName = aname; }
        const ByteSpan& name() const { return fName; }

		ByteSpan getAttribute(const ByteSpan& key) const noexcept
		{
			return fAttributes.getAttribute(key);
		}
        
        bool hasAttribute(const ByteSpan& key) const noexcept {return fAttributes.hasAttribute(key); }
        
        // IPlaceable
        virtual BLRect frame() const { return BLRect(); }
        virtual BLRect getBBox() const { return BLRect{}; }
        
        const bool visible() const { return fIsVisible; }
        void visible(bool visible) { fIsVisible = visible; }

        
        virtual void resolveProperties(IAmGroot* groot, SVGViewable* container) {;}
        virtual void resolvePosition(IAmGroot* groot, SVGViewable *container) { ; }
        
        virtual void update(IAmGroot* groot) { ; }
        
        virtual void draw(IRenderSVG* ctx, IAmGroot* groot) {  return; }


        // loadFromXmlElement
        // Here is our first chance at constructing SVG specifics
        // from the raw XmlElement
        // At this stage, we're interested in preserving the data associated
        // with the attributes, but not necessarily parsing all the attributes.
        // we need to capture the element name, and the ID field if it exists
        // and the attribute span for later processing
        virtual void loadFromXmlElement(const XmlElement& elem, IAmGroot* groot)
        {
            // Save the name if we've got one
            name(elem.name());

            // Gather all the presentation attributes to start
            fAttributeSpan = elem.data();
            
            // Scan the attributes, saving off the ones we think are interesting
			ByteSpan src = elem.data();
            ByteSpan attrName{};
            ByteSpan attrValue{};
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
        
        virtual void loadSelfFromXmlIterator(XmlElementIterator& iter, IAmGroot* groot)
        {
            loadFromXmlElement(*iter, groot);
        }
        
        virtual void loadFromXmlIterator(XmlElementIterator& iter, IAmGroot* groot)
        {
            loadSelfFromXmlIterator(iter, groot);

            iter++;
        }
    };

    // Interface Am Graphics Root (IAmGroot) 
    // Core interface to hold document level state, primarily
    // for the purpose of looking up nodes, but also for style sheets
    struct IManageReferences
    {
        std::unordered_map<ByteSpan, std::shared_ptr<SVGViewable>, ByteSpanHash> fDefinitions{};
        std::unordered_map<ByteSpan, ByteSpan, ByteSpanHash> fEntities{};
        
        virtual void addElementReference(const ByteSpan& name, std::shared_ptr<SVGViewable> obj)
        {
            fDefinitions[name] = obj;
        }

        
        virtual std::shared_ptr<SVGViewable> getElementById(const ByteSpan& name)
        {
            auto it = fDefinitions.find(name);
            if (it != fDefinitions.end())
                return it->second;

            printf("SVGDocument::getElementById, FAIL: ");
            printChunk(name);

            return {};
        }
        
        //virtual std::shared_ptr<SVGViewable> findNodeByHref(const ByteSpan& href) = 0;
        // Load a URL Reference
        virtual std::shared_ptr<SVGViewable> findNodeByHref(const ByteSpan& inChunk)
        {
            ByteSpan str = inChunk;

            auto id = chunk_trim(str, xmlwsp);

            // The first character could be '.' or '#'
            // so we need to skip past that
            if (*id == '.' || *id == '#')
                id++;

            if (!id)
                return nullptr;

            // lookup the thing we're referencing
            return getElementById(id);
        }
        
        //virtual std::shared_ptr<SVGViewable> findNodeByUrl(const ByteSpan& inChunk) = 0;
        // Load a URL reference, including the 'url(' function indicator
        virtual std::shared_ptr<SVGViewable> findNodeByUrl(const ByteSpan& inChunk)
        {
            ByteSpan str = inChunk;

            // the id we want should look like this
            // url(#id)
            // so we need to skip past the 'url(#'
            // and then find the closing ')'
            // and then we have the id
            auto url = chunk_token(str, "(");
            auto id = chunk_trim(chunk_token(str, ")"), xmlwsp);

            // sometimes the id is quoted
            // so trim that as well
            id = chunk_trim(id, "\"");
            id = chunk_trim(id, "'");

            return findNodeByHref(id);
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

            printf("SVGDocument::findEntity(), FAIL: ");
            printChunk(name);

            return ByteSpan{};
        }

    };
    
    
    struct IAmGroot : public IManageReferences
    {   
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
    //
    // SVGVisualNode
    // This is any object that will change the state of the rendering context
    // that's everything from paint that needs to be applied, to geometries
    // that need to be drawn, to line widths, text alignment, and the like.
    // Most things, other than basic attribute type, will be a sub-class of this
    struct SVGVisualNode : public SVGViewable
    {
        // Xml Node stuff
        std::unordered_map<ByteSpan, std::shared_ptr<SVGVisualProperty>, ByteSpanHash> fVisualProperties{};

        bool fIsStructural{ true };
        bool fHasTransform{ false };
        BLMatrix2D fTransform{};

        

        SVGVisualNode(IAmGroot* aroot)
            : SVGViewable(aroot)
        {
            needsBinding(true);
        }

        SVGVisualNode(const SVGVisualNode& other) = delete;
        SVGVisualNode& operator=(const SVGVisualNode& rhs) = delete;

        bool isStructural() const { return fIsStructural; }
        void isStructural(bool aStructural) { fIsStructural = aStructural; }

        
        // signal all properties they should
        // update themselves
        virtual void updateProperties(IAmGroot* groot)
        {
            for (auto& prop : fVisualProperties)
            {
                prop.second->update(groot);
            }
        }
        
        virtual void updateSelf(IAmGroot* groot)
        {
            ;
        }
        
        void update(IAmGroot* groot) override
        {
            updateProperties(groot);
            updateSelf(groot);
        }

        
        virtual void bindPropertiesToGroot(IAmGroot* groot, SVGViewable* container)
        {
            // This requires lookups, so if we don't have a root()
            // we need to return immediately
            if (nullptr == groot) {
                printf("SVGVisualNode::bindPropertiesToGroot, ERROR - NO ROOT()\n");
                return;
            }
            
            // Bind all the accumulated visual properties
			for (auto& prop : fVisualProperties)
			{
				prop.second->bindToGroot(groot, container);
			}
        }

		void resolveProperties(IAmGroot* groot, SVGViewable* container)
		{
            
            // First, lookup CSS based on tagname
            // See if there's an element selector for the current element
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
                auto classId = chunk_token(classChunk, xmlwsp);

                auto csel = groot->styleSheet()->getClassSelector(classId);
                if (csel != nullptr)
                {
                    fAttributes.mergeAttributes(csel->attributes());
                }
                else {
                    //printf("SVGVisualNode::bindPropertiesToGroot, ERROR - NO CLASS SELECTOR FOR %s\n", toString(classId).c_str());
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
            auto strokeColor = fAttributes.getAttribute("stroke");

            if (strokeColor && (strokeColor == "currentColor"))
            {
                auto colorValue = fAttributes.getAttribute("color");
                if (colorValue)
                {
                    fAttributes.addAttribute("stroke", colorValue);
                }
            }

            

            // Use up some of the attributes
            ByteSpan display = fAttributes.getAttribute("display");
            if (display)
            {
                display = chunk_trim(display, xmlwsp);

                if (display == "none")
                    visible(false);
            }


            fHasTransform = parseTransform(fAttributes.getAttribute("transform"), fTransform);


            
            //
            for (auto& attr : fAttributes.attributes())
            {
                // Next, see if there is a property registered for the attribute
                auto it = gSVGAttributeCreation.find(attr.first);
                if (it != gSVGAttributeCreation.end())
                {
                    // BUGBUG - we should pass in both the current attribute chunk
                    // and the collection of attributes
                    // That way, those places that only need the chunk can be quick
                    // and save another lookup.
                    // Those that need more attributes from the collection can have them
                    //auto prop = it->second(attr.second);
                    auto prop = it->second(fAttributes);
                    if (prop != nullptr)
                    {
                        fVisualProperties[attr.first] = prop;
                    }
                }
            }
		}
        
        void bindToGroot(IAmGroot* groot, SVGViewable *container) noexcept override
        {
			if (!needsBinding())
				return;
            
            resolveProperties(groot, container);
            resolvePosition(groot, container);
            
            bindPropertiesToGroot(groot, this);
            
            
            needsBinding(false);
        }

        std::shared_ptr<SVGVisualProperty> getVisualProperty(const ByteSpan& name)
        {
            auto it = fVisualProperties.find(name);
            if (it != fVisualProperties.end())
                return it->second;

            return nullptr;
        }


        // Contains styling attributes
        virtual void applyAttributes(IRenderSVG* ctx, IAmGroot* groot)
        {
            // Apply transform if it's not the identity transform
            if (fHasTransform)  //fTransform.type() != BL_MATRIX2D_TYPE_IDENTITY)
                ctx->applyTransform(fTransform);

            // BUGBUG - It might be useful to pass in the visual object
            // as additional context for attributes such as gradients
            // that might need that
            for (auto& prop : fVisualProperties) {
				if (prop.second->autoDraw() && prop.second->isSet())
                    prop.second->draw(ctx, groot);
            }
        }




        virtual void drawSelf(IRenderSVG* ctx, IAmGroot* groot)
        {
            ;
        }

        void draw(IRenderSVG* ctx, IAmGroot* groot) override
        {
            //printf("SVGVisualNode::draw(%s)\n", id().c_str());
            
            if (!visible())
                return;

            ctx->push();

            
            // Do the actual drawing
            applyAttributes(ctx, groot);
            drawSelf(ctx, groot);
            
            ctx->pop();
        }


    };
}



namespace waavs {
    // compound node creation dispatch - 'g', 'symbol', 'pattern', 'linearGradient', 'radialGradient', 'conicGradient', 'image', 'style', 'text', 'tspan', 'use'
    using SVGContainerCreationMap = std::unordered_map<ByteSpan, std::function<std::shared_ptr<SVGVisualNode>(IAmGroot* aroot, XmlElementIterator& iter)>, ByteSpanHash>;
    static SVGContainerCreationMap gSVGGraphicsElementCreation{};
    
	static void registerContainerNode(const ByteSpan& name, std::function<std::shared_ptr<SVGVisualNode>(IAmGroot* aroot, XmlElementIterator& iter)> creator)
	{
		gSVGGraphicsElementCreation[name] = creator;
	}
    
    // Geometry node creation dispatch
    // Creating from a singular element
    using ShapeCreationMap = std::unordered_map<ByteSpan, std::function<std::shared_ptr<SVGVisualNode>(IAmGroot* root, const XmlElement& elem)>, ByteSpanHash> ;
    static ShapeCreationMap gShapeCreationMap{};

    static void registerSVGSingularNode(const ByteSpan& name, std::function<std::shared_ptr<SVGVisualNode>(IAmGroot* root, const XmlElement& elem)> func)
    {
        gShapeCreationMap[name] = func;
        //printf("gShapeCreationMap.size(%d)\n", gShapeCreationMap.size());
    }
    
    // Convenience way to create an element
	static std::shared_ptr<SVGVisualNode> createSingularNode(const XmlElement& elem, IAmGroot* root)
	{
        ByteSpan aname = elem.name();
		auto it = gShapeCreationMap.find(aname);
		if (it != gShapeCreationMap.end())
		{
			return it->second(root, elem);
		}
		return nullptr;
	}
}


namespace waavs {

    //================================================
    // SVGGraphicsElement
    //================================================
    struct SVGGraphicsElement : public SVGVisualNode
    {
        static constexpr int BUILD_STATE_INITIAL = 0;
        static constexpr int BUILD_STATE_OPEN = 1;
        static constexpr int BUILD_STATE_CLOSE = 2;


        std::vector<std::shared_ptr<SVGVisualNode>> fNodes{};

        int buildState = BUILD_STATE_OPEN;
        

        BLVar fVar{};
        
        // Dealing with a cached image
        //bool fUseCacheIsolation{ false };
        bool fImageIsCached{ false };
        BLImage fCachedImage{};
        //double fOpacity{ 1.0 };

        
        SVGGraphicsElement(IAmGroot* aroot)
            :SVGVisualNode(aroot) {}

        /*
		// Return a reference to the cached image if it exists
        // Function returns 'true' when the image cache is active
        // and returns 'false' when the image cache is not used
        virtual bool getCachedImage(BLImage& img) const
        {
			if (!fImageIsCached)
				return false;
            
			img = fCachedImage;
			return true;
        }
        */
        
        virtual void bindChildrenToGroot(IAmGroot* groot, SVGViewable* container)
        {
			for (auto& node : fNodes)
			{
				node->bindToGroot(groot, container);
			}
        }
        
        
        // For compound nodes (which have children) we want to 
        // do the base stuff (binding properties) then bind the children
		// If you sub-class this, you should call this first
        // then do your own thing.  We don't want to call a 'bindSelfToGroot'
        // here, because that complicates the interactions and sequences of things
        // so just override bindToGroot
        void bindToGroot(IAmGroot* groot, SVGViewable* container) noexcept override
        {
			SVGVisualNode::bindToGroot(groot, container);
            
            bindChildrenToGroot(groot, this);

        }


        const BLVar getVariant() noexcept override
        {
            // if our variant is null
            // traverse down our fNodes, until we find
            // something that reports other than null
            // and return that.
            if (fVar.isNull())
            {
                for (auto& node : fNodes)
                {
                    auto& var = node->getVariant();
                    if (!var.isNull())
                    {
                        return node->getVariant();
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


        void updateSelf(IAmGroot* groot) override
        {
			for (auto& node : fNodes)
			{
				node->update(groot);
			}
        }
        
        virtual void drawChildren(IRenderSVG* ctx, IAmGroot* groot)
        {
			for (auto& node : fNodes) {
				node->draw(ctx, groot);
			}
        }

        
        void draw(IRenderSVG *ctx, IAmGroot* groot) override
        {
            if (!visible())
                return;

            ctx->push();
            
            applyAttributes(ctx, groot);
            drawSelf(ctx, groot);

            drawChildren(ctx, groot);
            
            ctx->pop();
        }
    
        virtual bool addNode(std::shared_ptr < SVGVisualNode > node, IAmGroot *groot)
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

        virtual void loadSelfClosingNode(const XmlElement& elem, IAmGroot *groot)
        {
            //printf("SVGGraphicsElement::loadSelfClosingNode: \n");

            auto anode = createSingularNode(elem, groot);
            if (anode != nullptr) {
                addNode(anode, groot);
            }
            else {
                //printf("SVGGraphicsElement::loadSelfClosingNode UNKNOWN[%s]\n", toString(elem.name()).c_str());
                //printXmlElement(elem);
            }
        }

        virtual void loadEndTag(const XmlElement& elem, IAmGroot* groot)
        {
			//printf("SVGGraphicsElement::loadEndTag [%s]\n", toString(elem.name()).c_str());
        }
        
        virtual void loadContentNode(const XmlElement& elem, IAmGroot* groot)
        {
            //printf("SVGCompountNode::loadContentNode\n");
            //printXmlElement(elem);
            // Do something with content nodes	
        }

        virtual void loadCDataNode(const XmlElement& elem, IAmGroot* groot)
        {
            //printf("SVGGraphicsElement::loadCDataNode\n");
            //printXmlElement(elem);
            // Do something with CDATA nodes
        }

        virtual void loadComment(const XmlElement& elem, IAmGroot* groot)
        {
			//printf("SVGGraphicsElement::loadComment\n");
			//printXmlElement(elem);
			// Do something with comments
        }
        
        virtual  void loadCompoundNode(XmlElementIterator& iter, IAmGroot* groot)
        {
            const XmlElement& elem = *iter;

            // Add a child, and call loadIterator
            // If the name of the element is found in the map,
            // then create a new node of that type and add it
            // to the list of nodes.
			auto aname = elem.name();
			auto it = gSVGGraphicsElementCreation.find(aname);
            if (it != gSVGGraphicsElementCreation.end())
            {
                auto node = it->second(groot, iter);
                addNode(node, groot);
            }
            else {
                //printf("SVGGraphicsElement::loadCompoundNode == UNKNOWN ==> '%s'\n", toString(aname).c_str());
                //printXmlElement(elem);
                auto node = gSVGGraphicsElementCreation["g"](groot, iter);
                // don't add the node to the tree as we don't
                // know what it is, so ignore it
                //addNode(node);
            }
        }


            
        ///*
        virtual void loadFromXmlIterator(XmlElementIterator& iter, IAmGroot* groot) override
        {
            // First, loadFromXmlElement because we're sitting on our opening element
            // and we need to gather our own attributes
            loadSelfFromXmlIterator(iter, groot);

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
                        loadSelfClosingNode(elem, groot);
                    }
                    else if (elem.isStart())
                    {
                        loadCompoundNode(iter, groot);
                    }
                    else if (elem.isEnd())
                    {
                        // Close the current element
                        buildState = BUILD_STATE_CLOSE;
                        loadEndTag(elem, groot);
                    }
                    else if (elem.isContent())
                    {
                        loadContentNode(elem, groot);
                    }
                    else if (elem.isCData())
                    {
                        loadCDataNode(elem, groot);
                    }
					else if (elem.isComment())
					{
						loadComment(elem, groot);
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
        //*/

    };
}
