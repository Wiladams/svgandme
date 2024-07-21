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



namespace waavs {
    // Experimental
    struct IPlaceable
    {
        bool fAutoMoveToFront{ false };


        void autoMoveToFront(bool b) { fAutoMoveToFront = b; }
        bool autoMoveToFront() const { return fAutoMoveToFront; }

        virtual BLRect frame() const = 0;
        virtual BLRect getBBox() const = 0;

        virtual bool contains(double x, double y)
        {
            return containsRect(frame(), x, y);
        }

        virtual void gainFocus() { ; }
        virtual void loseFocus(double x, double y) { ; }

        virtual void moveTo(double x, double y) = 0;
        virtual void moveBy(double dx, double dy) {
            moveTo(frame().x + dx, frame().y + dy);
        }

        virtual void mouseEvent(const MouseEvent& e) { return; }
        virtual void keyEvent(const KeyboardEvent& e) { return; }
    };


}

namespace waavs {
    // Something that can be drawn in the user interface
    struct IViewable : public IPlaceable, public ISVGDrawable
    {
        ByteSpan fName{};

        void name(const ByteSpan& aname) { fName = aname; }
        //void name(const char* aname) { fName = aname; }
        const ByteSpan& name() const { return fName; }
    };
}


namespace waavs {
    struct IAmGroot;    // forward declaration

    struct SVGObject
    {
    protected:
        IAmGroot* fRoot{ nullptr };
        bool fNeedsBinding{ false };

    public:
        BLVar fVar{};


        SVGObject(IAmGroot* root) :fRoot(root) {}

        SVGObject() = delete;
        SVGObject(const SVGObject& other) = delete;
        SVGObject& operator=(const SVGObject& other) = delete;


        virtual ~SVGObject() = default;


        bool needsBinding() const { return fNeedsBinding; }
        void needsBinding(bool needsIt) { fNeedsBinding = needsIt; }

        IAmGroot* root() const { return fRoot; }

        virtual void bindToGroot(IAmGroot* groot) { fRoot = groot; } // { fRoot = groot; bindSelfToGroot(groot); }

        // sub-classes should return something interesting as BLVar
        // This can be used for styling, so images, colors, patterns, gradients, etc
        virtual const BLVar& getVariant()
        {
            return fVar;
        }


    };

    struct SVGViewable : public SVGObject, public IViewable, public XmlAttributeCollection
    {
        bool fIsVisible{ true };
        ByteSpan fId{};      // The id of the element

        
        
        SVGViewable(IAmGroot* groot) :SVGObject(groot) {}

        const ByteSpan& id() const { return fId; }
        void id(const ByteSpan& aid) { fId = aid; }
        
        
        const bool visible() const { return fIsVisible; }
        void visible(bool visible) { fIsVisible = visible; }

        // ISVGDrawable
        void draw(IRenderSVG* ctx) override {  return; }

        // IPlaceable
        BLRect frame() const override { return BLRect(); }
        BLRect getBBox() const override { return BLRect{}; }
        
        void moveTo(double x, double y) override { ; }
        void mouseEvent(const MouseEvent& e) override { return; }
        

        virtual void loadSelfFromXmlElement(const XmlElement& elem)
        {
            ;
        }

        virtual void loadFromXmlElement(const XmlElement& elem)
        {
            scanAttributes(elem.data());

            // Save the name if we've got one
            name(elem.name());
            
            // save the id if we've got an id attribute
            id(getAttribute("id"));

            loadSelfFromXmlElement(elem);
        }
        
        virtual void loadSelfFromXmlIterator(XmlElementIterator& iter)
        {
            loadFromXmlElement(*iter);
        }
        
        virtual void loadFromXmlIterator(XmlElementIterator& iter)
        {
            loadSelfFromXmlIterator(iter);

            iter++;
        }
    };

    // Interface Am Graphics Root (IAmGroot) 
    // Core interface to hold document level state, primarily
    // for the purpose of looking up nodes, but also for style sheets
    struct IAmGroot
    {
        virtual std::shared_ptr<SVGViewable> getElementById(const ByteSpan& name) = 0;
        virtual std::shared_ptr<SVGViewable> findNodeByHref(const ByteSpan& href) = 0;
        virtual std::shared_ptr<SVGViewable> findNodeByUrl(const ByteSpan& inChunk) = 0;
        virtual ByteSpan findEntity(const ByteSpan& name) = 0;

        virtual FontHandler* fontHandler() const = 0;

        virtual std::shared_ptr<CSSStyleSheet> styleSheet() = 0;
        virtual void styleSheet(std::shared_ptr<CSSStyleSheet> sheet) = 0;

        virtual void addDefinition(const ByteSpan& name, std::shared_ptr<SVGViewable> obj) = 0;
        virtual void addEntity(const ByteSpan& name, ByteSpan expansion) = 0;

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

        virtual void update() { ; }
        
        // Apply propert to the context conditionally
        virtual void drawSelf(IRenderSVG* ctx)
        {
            ;
        }

        virtual void draw(IRenderSVG* ctx)
        {
            //printf("SVGVisualProperty::draw == ");
            //printChunk(fRawValue);

            if (isSet())
                drawSelf(ctx);
        }

    };

    static std::unordered_map<ByteSpan, std::function<std::shared_ptr<SVGVisualProperty>(const ByteSpan&)>> gSVGAttributeCreation;
    static std::unordered_map<ByteSpan, std::function<std::shared_ptr<SVGVisualProperty>(IAmGroot* aroot, const XmlAttributeCollection&)>> gSVGPropertyCreation;
    
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


        BLMatrix2D fTransform{};
		BLMatrix2D fTransformInverse{};
        bool fHasTransform{ false };
        

        SVGVisualNode(IAmGroot* aroot)
            : SVGViewable(aroot)
        {
            needsBinding(true);
        }

        SVGVisualNode(const SVGVisualNode& other) = delete;
        SVGVisualNode& operator=(const SVGVisualNode& rhs) = delete;

        // signal all properties they should
        // update themselves
        virtual void updateProperties()
        {
            for (auto& prop : fVisualProperties)
            {
                prop.second->update();
            }
        }
        
        virtual void updateSelf()
        {
            ;
        }
        
        void update() override
        {
            updateProperties();
            updateSelf();
        }

        bool isStructural() const { return fIsStructural; }
        void isStructural(bool aStructural) { fIsStructural = aStructural; }


        void moveTo(double x, double y) override
        {
            // BUGBUG - this moveTo should not do this transform??
            // should move the frame??
            //fTransform.reset();
            fTransform.translate(x, y);
        }

        bool contains(double x, double y) override
        {
            BLPoint localPoint(x, y);

            // check to see if we have a transform property
            // if we do, transform the points through that transform
            // then check to see if the point is inside the transformed path
            if (fHasTransform)
            {
                // get the inverse transform
                auto inverse = fTransform;
                inverse.invert();
                localPoint = inverse.mapPoint(localPoint);
            }

			return containsRect(frame(), localPoint);
        }
        
        virtual SVGVisualNode* nodeAt(double x, double y)
        {
            if (contains(x, y))
                return this;

            return nullptr;
        }

        virtual void bindPropertiesToGroot(IAmGroot* groot)
        {
            // This requires lookups, so if we don't have a root()
            // we need to return immediately
            // Maybe this should occur in bind to groot?
            if (nullptr == groot) {
                printf("SVGVisualNode::bindPropertiesToGroot, ERROR - NO ROOT()\n");
                return;
            }
            
            
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
                //auto classId = toString(aWord);

                auto csel = root()->styleSheet()->getClassSelector(classId);
                if (csel != nullptr)
                {
                    loadVisualProperties(csel->attributes());
				}
                else {
                    printf("SVGVisualNode::bindPropertiesToGroot, ERROR - NO CLASS SELECTOR FOR %s\n", toString(classId).c_str());
                }
            }
            
            
            // See if there's an element selector for the current element
            // Deal with any more attributes that need special handling
            // BUGBUG - This came from SetCommonVisualProperties, 
            // so put it back there if this doesn't work out
            if (!name().empty())
            {
                auto esel = root()->styleSheet()->getElementSelector(name());
                if (esel != nullptr)
                {
                    loadVisualProperties(esel->attributes());
                }
            }
            
            // Check for id selector if we have one
            // These
			auto id = getAttribute("id");
			if (id)
			{
				auto idsel = root()->styleSheet()->getIDSelector(id);
				if (idsel != nullptr)
				{
					loadVisualProperties(idsel->attributes());
				}
			}
            
            // Do the local style sheet properties last as they
            // override any that came from global style sheets
            // 
     
            // Bind all the accumulated visual properties
			for (auto& prop : fVisualProperties)
			{
				prop.second->bindToGroot(groot);
			}
        }
        
        void bindToGroot(IAmGroot* groot) override
        {
            fRoot = groot;
            bindPropertiesToGroot(groot);
            needsBinding(false);
        }

        //std::shared_ptr<SVGVisualProperty> getVisualProperty(const std::string& name)
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
            
            if (gSVGAttributeCreation.find(name) != gSVGAttributeCreation.end())
            {
                auto prop = gSVGAttributeCreation[name](value);
                if (prop)
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
        virtual void loadVisualProperties(const XmlAttributeCollection & attrCollection)
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
                    fTransformInverse = fTransform;
					fTransformInverse.invert();
                }
            }
            
            // Run through the attributes passed in 
            // add them into our attributes 
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
        void setCommonVisualProperties()
        {
            // BUGBUG - need to decide order of precedence for presentation attributes
            // load the common stuff that doesn't require
            // any additional processing
            loadVisualProperties(*this);

            
            // Handle the inline style attribute separately by turning
            // it into an XmlAttributeCollection, and processing the same
            // as the other attributes.
            // Anything in the 'style' attribute supersedes any values that
            // were in presentation attributes

            ByteSpan styleChunk = getAttribute("style");

            if (styleChunk) {
				XmlAttributeCollection styleAttributes;

                parseStyleAttribute(styleChunk, styleAttributes);
				loadVisualProperties(styleAttributes);
            }
            
        }


        // Contains styling attributes
        virtual void applyAttributes(IRenderSVG* ctx)
        {
            // Apply transform if it's not the identity transform
            if (fHasTransform)  //fTransform.type() != BL_MATRIX2D_TYPE_IDENTITY)
                ctx->applyTransform(fTransform);

            // BUGBUG - It might be useful to pass in the visual object
            // as additional context for attributes such as gradients
            // that might need that
            for (auto& prop : fVisualProperties) {
				if (prop.second->autoDraw() && prop.second->isSet())
                    prop.second->draw(ctx);
            }
        }




        virtual void drawSelf(IRenderSVG* ctx)
        {
            ;
        }

        void draw(IRenderSVG* ctx) override
        {
            //printf("SVGVisualNode::draw(%s)\n", id().c_str());
            
            if (!visible())
                return;

            ctx->push();

            
            // Do the actual drawing
            applyAttributes(ctx);
            drawSelf(ctx);
            
            ctx->pop();
        }

        void loadFromXmlElement(const XmlElement & elem) override
        {
			SVGViewable::loadFromXmlElement(elem);

			setCommonVisualProperties();
        }
        


        void mouseEvent(const MouseEvent& e) override
        {
          ; // do nothing by default
        }

    };
}



namespace waavs {
    // compound node creation dispatch - 'g', 'symbol', 'pattern', 'linearGradient', 'radialGradient', 'conicGradient', 'image', 'style', 'text', 'tspan', 'use'
    static std::unordered_map<ByteSpan, std::function<std::shared_ptr<SVGVisualNode>(IAmGroot* aroot, XmlElementIterator& iter)>, ByteSpanHash> gSVGGraphicsElementCreation{};

    // Geometry node creation dispatch
    // Creating from a singular element
    static std::unordered_map<ByteSpan, std::function<std::shared_ptr<SVGVisualNode>(IAmGroot* root, const XmlElement& elem)>, ByteSpanHash> gShapeCreationMap{};
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


        std::shared_ptr<SVGGraphicsElement> fParent{ nullptr };
        std::vector<std::shared_ptr<SVGVisualNode>> fNodes{};

        int buildState = BUILD_STATE_OPEN;
        
        // Dealing with a cached image
        bool fUseCacheIsolation{ false };
        bool fImageIsCached{ false };
        BLRect fBBox{};
        BLImage fCachedImage{};
        double fOpacity{ 1.0 };

        
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
        
        virtual void bindChildrenToGroot(IAmGroot* groot)
        {
			for (auto& node : fNodes)
			{
				node->bindToGroot(groot);
			}
        }
        
        virtual void bindSelfToGroot(IAmGroot* groot)
        {
            // don't do anything by default
        }
        
        // For compound nodes (which have children) we want to 
        // do the base stuff (binding properties) then bind the children
		// If you sub-class this, you should call this first
        // then do your own thing.  We don't want to call a 'bindSelfToGroot'
        // here, because that complicates the interactions and sequences of things
        // so just override bindToGroot
        void bindToGroot(IAmGroot* groot) override
        {
			SVGVisualNode::bindToGroot(groot);
            bindChildrenToGroot(groot);
            
            bindSelfToGroot(groot);

            // Do the image cache thing if necessary
            auto opacity = getVisualProperty("opacity");
            if (opacity)
            {
                BLResult res = opacity->getVariant().toDouble(&fOpacity);

                if (fUseCacheIsolation)
                {
                    drawIntoCache();
                }

            }
        }


        const BLVar& getVariant() override
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


        // Find the topmost node at a given position
        SVGVisualNode* nodeAt(double x, double y) override
        {
            // traverse through windows in reverse order
            // return when one of them contains the mouse point
            std::vector<std::shared_ptr<SVGVisualNode> >::reverse_iterator rit = fNodes.rbegin();
            for (rit = fNodes.rbegin(); rit != fNodes.rend(); ++rit)
            {
                // if the node is not visible, skip it
                if (!(*rit)->visible())
                    continue;

                auto anode = (*rit)->nodeAt(x, y);
                if (anode != nullptr)
                {
                    // If the node does not have an ID, then return the containing node
                    // instead
                    if (anode->id() == "")
                        return this;

                    return anode;
                }
            }

            // if the coordinates don't match one of our children
            // then if we're a container, return ourselves, if the
            // we contain the coordinates, otherwise return null
            if (contains(x, y))
                return this;
            else
                return nullptr;
        }

        void updateSelf() override
        {
			for (auto& node : fNodes)
			{
				node->update();
			}
        }
        
        virtual void drawChildren(IRenderSVG* ctx)
        {
			for (auto& node : fNodes) {
				node->draw(ctx);
			}
        }
        
        virtual void drawIntoCache()
        {
            // get the bounding box to determine how big
            // the cache should be
            fBBox = frame();
            int iWidth = (int)fBBox.w;
            int iHeight = (int)fBBox.h;

            // Create a new image with the same size as the bounding box
            fCachedImage.create(iWidth, iHeight, BLFormat::BL_FORMAT_PRGB32);

            // Create the drawing context to go with the cache
            IRenderSVG cachectx(root()->fontHandler());
            cachectx.begin(fCachedImage);
            cachectx.clearAll();
            //cachectx.fillAll(BLRgba32(0xFFffffffu));
            cachectx.setCompOp(BL_COMP_OP_SRC_COPY);
            cachectx.noStroke();
            cachectx.translate(-fBBox.x, -fBBox.y);


            // Render out content into the backing buffer
            fImageIsCached = false;
            SVGGraphicsElement::draw(&cachectx);
            fImageIsCached = true;
            
            cachectx.flush();
        }
        
        void draw(IRenderSVG *ctx) override
        {
            if (!visible())
                return;

            ctx->push();
            
            if (fUseCacheIsolation && fImageIsCached && !fCachedImage.empty())
            {
                ctx->setGlobalAlpha(fOpacity);
                ctx->blitImage(fBBox, fCachedImage);
            }
            else {
                applyAttributes(ctx);
                drawSelf(ctx);

                drawChildren(ctx);
            }
            
            ctx->pop();
        }
    
        virtual bool addNode(std::shared_ptr < SVGVisualNode > node)
        {
            if (node == nullptr)
                return false;

            if (!node->id().empty())
                root()->addDefinition(node->id(), node);
            
            if (node->isStructural()) {
                fNodes.push_back(node);
            }

            return true;
        }

        virtual void loadSelfClosingNode(const XmlElement& elem)
        {
            //printf("SVGGraphicsElement::loadSelfClosingNode: \n");

            auto it = gShapeCreationMap.find(elem.name());
            if (it != gShapeCreationMap.end())
            {
                auto node = it->second(root(), elem);
                addNode(node);
            }
            else {
                printf("SVGGraphicsElement::loadSelfClosingNode UNKNOWN[%s]\n", toString(elem.name()).c_str());
                //printXmlElement(elem);
            }
        }


        virtual void loadContentNode(const XmlElement& elem)
        {
            //printf("SVGCompountNode::loadContentNode\n");
            //printXmlElement(elem);
            // Do something with content nodes	
        }

        virtual void loadCDataNode(const XmlElement& elem)
        {
            //printf("SVGGraphicsElement::loadCDataNode\n");
            //printXmlElement(elem);
            // Do something with CDATA nodes
        }

        virtual void loadComment(const XmlElement& elem)
        {
			//printf("SVGGraphicsElement::loadComment\n");
			//printXmlElement(elem);
			// Do something with comments
        }
        
        virtual  void loadCompoundNode(XmlElementIterator& iter)
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
                //auto& func = gSVGGraphicsElementCreation[aname];
                //auto node = func(root(), iter);
                auto node = it->second(root(), iter);
                addNode(node);
            }
            else {
                printf("SVGGraphicsElement::loadCompoundNode == UNKNOWN ==> '%s'\n", toString(aname).c_str());
                //printXmlElement(elem);
                auto node = gSVGGraphicsElementCreation["g"](root(), iter);
                // don't add the node to the tree as we don't
                // know what it is, so ignore it
                //addNode(node);
            }
        }

        /*
        virtual void loadFromXmlIterator(XmlElementIterator& iter) override
        {

            loadSelfFromXmlIterator(iter);

            while (iter.next())
            {
                const XmlElement& elem = *iter;

                // BUGBUG - debug
                //printXmlElement(elem);

                if (!elem)
                    break;


                if (elem.isSelfClosing()) {
                    loadSelfClosingNode(elem);
                }
                else if (elem.isStart())
                {
                    loadCompoundNode(iter);
                }
                else if (elem.isEnd())
                {
                    // Close the current element
                    //buildState = BUILD_STATE_CLOSE;
                    //loadEndTag(elem);
                }
                else if (elem.isContent())
                {
                    loadContentNode(elem);
                }
                else if (elem.isCData())
                {
                    loadCDataNode(elem);
                }
                else if (elem.isComment())
                {
                    loadComment(elem);
                }
                else
                {
                    // Ignore anything else
                    printf("SVGGraphicsElement::loadFromXmlIterator ==> IGNORING kind(%d) name:", elem.kind());
                    printChunk(elem.nameSpan());
                    printChunk(elem.data());
                    //printXmlElement(elem);
                }

            }
        }
        */
            
        ///*
        virtual void loadFromXmlIterator(XmlElementIterator& iter) override
        {
            // First, loadFromXmlElement because we're sitting on our opening element
            // and we need to gather our own attributes
            loadSelfFromXmlIterator(iter);

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
                        loadSelfClosingNode(elem);
                    }
                    else if (elem.isStart())
                    {
                        loadCompoundNode(iter);
                    }
                    else if (elem.isEnd())
                    {
                        // Close the current element
                        buildState = BUILD_STATE_CLOSE;
                        //loadEndTag(elem);
                    }
                    else if (elem.isContent())
                    {
                        loadContentNode(elem);
                    }
                    else if (elem.isCData())
                    {
                        loadCDataNode(elem);
                    }
					else if (elem.isComment())
					{
						loadComment(elem);
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
