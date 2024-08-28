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
        SVGObject() = delete;
        SVGObject(const SVGObject& other) = delete;
        
        SVGObject(IAmGroot* root) {}

        virtual ~SVGObject() = default;

        // we don't want an implied assignment operator
        SVGObject& operator=(const SVGObject& other) = delete;

        
        const ByteSpan& id() const { return fId; }
        void id(const ByteSpan& aid) { fId = aid; }
        
       
        bool needsBinding() const { return fNeedsBinding; }
        void needsBinding(bool needsIt) { fNeedsBinding = needsIt; }


        virtual void bindToGroot(IAmGroot* groot, SVGViewable* container) {  }

        // sub-classes should return something interesting as BLVar
        // This can be used for styling, so images, colors, patterns, gradients, etc
        virtual const BLVar getVariant() { return BLVar::null();}


    };

    struct SVGViewable : public SVGObject, public XmlAttributeCollection
    {
    protected:
        bool fIsVisible{ true };
        ByteSpan fName{};
        
    public:
        SVGViewable(IAmGroot* groot) :SVGObject(groot) {}


        // IViewable
        void name(const ByteSpan& aname) { fName = aname; }
        const ByteSpan& name() const { return fName; }


        // IPlaceable
        virtual BLRect frame() const { return BLRect(); }
        virtual BLRect getBBox() const { return BLRect{}; }
        
        const bool visible() const { return fIsVisible; }
        void visible(bool visible) { fIsVisible = visible; }

        
        virtual void resolvePosition(IAmGroot* groot, SVGViewable *container) { ; }
        virtual void resolveStyle(IAmGroot* groot, SVGViewable *container) { ; }
        
        virtual void update(IAmGroot* groot) { ; }
        
        virtual void draw(IRenderSVG* ctx, IAmGroot* groot) {  return; }

        virtual void loadFromXmlElement(const XmlElement& elem, IAmGroot* groot)
        {
            scanAttributes(elem.data());
            
            // save the id if we've got an id attribute
            id(getAttribute("id"));
            
            // Save the name if we've got one
            name(elem.name());
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

            printf("SVGDocument::getElementById, FAIL: %s\n", toString(name).c_str());

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

            printf("SVGDocument::findEntity(), FAIL: %s\n", toString(name).c_str());

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

        
        SVGVisualProperty(IAmGroot* groot) :SVGObject(groot), fIsSet(false) {}

        SVGVisualProperty(const SVGVisualProperty& other) = delete;
        SVGVisualProperty& operator=(const SVGVisualProperty& rhs) = delete;


        void set(const bool value) { fIsSet = value; }
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
    static std::unordered_map<ByteSpan, std::function<std::shared_ptr<SVGVisualProperty>(const ByteSpan&)>, ByteSpanHash> gSVGAttributeCreation;

    // Convenient function to register property constructors
	static void registerSVGAttribute(const ByteSpan& name, std::function<std::shared_ptr<SVGVisualProperty>(const ByteSpan&)> func)
	{
		gSVGAttributeCreation[name] = func;
	}
    
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
        
        void bindToGroot(IAmGroot* groot, SVGViewable *container) override
        {
			if (!needsBinding())
				return;
            
            resolvePosition(groot, container);
            resolveStyle(groot, container);
            
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

        //
        // setAttribute
        // 
        // Allows setting any attribute on the element
        // The 'name', is the name of the attribute to be set
        // the 'value', is a span, that contains a raw string to be parsed to set the value
        //
        virtual void setAttribute(const ByteSpan& name, const ByteSpan& value)
        {
            // If the value is nothing, then don't set it
            if (!value)
                return;
            
			auto it = gSVGAttributeCreation.find(name);
			if (it != gSVGAttributeCreation.end())
			{
				auto prop = it->second(value);
				if (prop != nullptr)
				{
					fVisualProperties[name] = prop;
				}
			}
        }

        //
        // loadVisualProperties
        // 
        // This might be called multiple times on an element
        // once for the regular attributes
        // a second time for attributes hidden in a 'style' attribute
        //
        virtual void loadVisualProperties(const XmlAttributeCollection & attrCollection, IAmGroot* groot)
        {
            ByteSpan display = attrCollection.getAttribute("display");
            if (display)
            {
                display = chunk_trim(display, xmlwsp);

                if (display == "none")
                    visible(false);
            }

            if (attrCollection.getAttribute("transform"))
            {
                fHasTransform = parseTransform(attrCollection.getAttribute("transform"), fTransform);
                if (fHasTransform)
                {
                    // create the inverse transform for subsequent
                    // UI interaction
                    //fTransformInverse = fTransform;
					//fTransformInverse.invert();
                }
            }
            
            // Run through the attributes passed in 
            // add them into our attributes 
            // BUGBUG - not sure we actually need to do this
            // all attributes are typically loaded during
            // loadVisualProperties of the sub-class
            // so this is kind of redundant
            // Yes, we do need to do this, because we use ourself
            // on a loadVisualProperties(*this) call
            for (auto& attr : attrCollection.fAttributes)
            {
				setAttribute(attr.first, attr.second);
            }

			// BUGBUG - this is a hack to get the fill and stroke
            // It needs to go deeper and get the color attribute that's from the tree
            // where the current node is just the latest.
            // if there is a stroke attribute with a value of 'currentColor', then look
			// for a 'color' attribute, and set the stroke color to that value
			auto strokeColor = attrCollection.getAttribute("stroke");
            auto fillColor = attrCollection.getAttribute("fill");
            if (strokeColor && (strokeColor == "currentColor"))
            {
                auto colorValue = attrCollection.getAttribute("color");
                if (colorValue)
                {
                    setAttribute("stroke", colorValue);
                }
            }
        }
        


        // Assuming we've already scanned our attributes
        // do further processing with them
        void loadCommonVisualProperties(IAmGroot* groot)
        {
            // BUGBUG - need to decide order of precedence for presentation attributes
            // load the common stuff that doesn't require
            // any additional processing
            loadVisualProperties(*this, groot);

            // Handle the class attribute if there is one
            // The class attribute can be a whitespace separated list
            // of class names, so we need to lookup each selector
            // and turn whatever we find there into an attribute list
            // and load them.
            // BUGBUG - need to parse full class selector
            // Assume multiple simple words for a first go
            ByteSpan classChunk = getAttribute("class");
            while (classChunk)
            {
                // peel a word off the front
                auto classId = chunk_token(classChunk, xmlwsp);

                auto csel = groot->styleSheet()->getClassSelector(classId);
                if (csel != nullptr)
                {
                    loadVisualProperties(csel->attributes(), groot);
                }
                else {
                    printf("SVGVisualNode::bindPropertiesToGroot, ERROR - NO CLASS SELECTOR FOR %s\n", toString(classId).c_str());
                }
            }

            // See if there's an element selector for the current element
            // Deal with any more attributes that need special handling

            if (!name().empty())
            {
                auto esel = groot->styleSheet()->getElementSelector(name());
                if (esel != nullptr)
                {
                    loadVisualProperties(esel->attributes(), groot);
                }
            }

            // Check for id selector if we have one
            // These
            auto id = getAttribute("id");
            if (id)
            {
                auto idsel = groot->styleSheet()->getIDSelector(id);
                if (idsel != nullptr)
                {
                    loadVisualProperties(idsel->attributes(), groot);
                }
            }

            // Do the local style sheet properties last as they
            // override any that came from global style sheets

            ByteSpan styleChunk = getAttribute("style");

            if (styleChunk) {
                XmlAttributeCollection styleAttributes;

                parseStyleAttribute(styleChunk, styleAttributes);
                loadVisualProperties(styleAttributes, groot);
            }
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

        void loadFromXmlElement(const XmlElement & elem, IAmGroot* groot) override
        {
			SVGViewable::loadFromXmlElement(elem, groot);

			loadCommonVisualProperties(groot);
        }
        

    };
}



namespace waavs {
    // compound node creation dispatch - 'g', 'symbol', 'pattern', 'linearGradient', 'radialGradient', 'conicGradient', 'image', 'style', 'text', 'tspan', 'use'
    static std::unordered_map<ByteSpan, std::function<std::shared_ptr<SVGVisualNode>(IAmGroot* aroot, XmlElementIterator& iter)>, ByteSpanHash> gSVGGraphicsElementCreation{};

    // Geometry node creation dispatch
    // Creating from a singular element
    static std::unordered_map<ByteSpan, std::function<std::shared_ptr<SVGVisualNode>(IAmGroot* root, const XmlElement& elem)>, ByteSpanHash> gShapeCreationMap{};

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
        void bindToGroot(IAmGroot* groot, SVGViewable* container) override
        {
			SVGVisualNode::bindToGroot(groot, container);
            
            bindChildrenToGroot(groot, this);


            /*
            // Do the image cache thing if necessary
            auto opacity = getVisualProperty("opacity");
            if (opacity)
            {
                //BLResult res = opacity->getVariant().toDouble(&fOpacity);

                //if (fUseCacheIsolation)
                //{
                //    drawIntoCache(groot);
                //}

            }
            */
        }


        const BLVar getVariant() override
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
        
        virtual void drawIntoCache(IAmGroot *groot)
        {
            // get the bounding box to determine how big
            // the cache should be
            BLRect bbox = frame();
            int iWidth = (int)bbox.w;
            int iHeight = (int)bbox.h;

            // Create a new image with the same size as the bounding box
            fCachedImage.create(iWidth, iHeight, BLFormat::BL_FORMAT_PRGB32);

            // Create the drawing context to go with the cache
            IRenderSVG cachectx(groot->fontHandler());
            cachectx.begin(fCachedImage);
            cachectx.clearAll();
            //cachectx.fillAll(BLRgba32(0xFFffffffu));
            cachectx.setCompOp(BL_COMP_OP_SRC_COPY);
            cachectx.noStroke();
            cachectx.translate(-bbox.x, -bbox.y);


            // Render out content into the backing buffer
            fImageIsCached = false;
            SVGGraphicsElement::draw(&cachectx, groot);
            fImageIsCached = true;
            
            cachectx.flush();
        }
        
        void draw(IRenderSVG *ctx, IAmGroot* groot) override
        {
            if (!visible())
                return;

            ctx->push();
            
            //if (fUseCacheIsolation && fImageIsCached && !fCachedImage.empty())
            //{
            //    ctx->setGlobalAlpha(fOpacity);
            //    ctx->blitImage(getBBox(), fCachedImage);
            //}
            //else 
            {
                applyAttributes(ctx, groot);
                drawSelf(ctx, groot);

                drawChildren(ctx, groot);
            }
            
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
                        //loadEndTag(elem, groot);
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
                        printf("SVGGraphicsElement::loadFromXmlIterator ==> IGNORING kind(%d) name:", elem.kind());
                        printChunk(elem.nameSpan());
                        printChunk(elem.data());
                        
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
