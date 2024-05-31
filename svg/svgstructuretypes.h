#pragma once


#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstdint>		// uint8_t, etc
#include <cstddef>		// nullptr_t, ptrdiff_t, size_t

#include "xmlscan.h"

#include "svgcolors.h"
#include "svgdatatypes.h"
#include "svgcss.h"

#include "irendersvg.h"
#include "placeable.h"

#include "base64.h"




namespace waavs {
    struct IAmGroot;    // forward declaration

    struct SVGObject
    {
    protected:
        IAmGroot* fRoot{ nullptr };

    public:
        BLVar fVar{};

        bool fNeedsBinding{ false };

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
        std::string fId{};      // The id of the element

        
        
        SVGViewable(IAmGroot* groot) :SVGObject(groot) {}

        const std::string& id() const { return fId; }
        void id(const std::string& aid) { fId = aid; }
        
        
        const bool visible() const { return fIsVisible; }
        void visible(bool visible) { fIsVisible = visible; }

        // ISVGDrawable
        void draw(IRenderSVG* ctx) override {  return; }

        // IPlaceable
        BLRect frame() const override { return BLRect(); }

        void moveTo(double x, double y) override { ; }
        //void mouseEvent(const MouseEvent& e) override { return; }
        

        virtual void loadSelfFromXmlElement(const XmlElement& elem)
        {
            ;
        }

        virtual void loadFromXmlElement(const XmlElement& elem)
        {
            scanAttributes(elem.data());
            name(elem.name());
            
            auto idchunk = getAttribute("id");
            if (idchunk)
                id(std::string(idchunk.fStart, idchunk.fEnd));

            loadSelfFromXmlElement(elem);
        }
        
        virtual void loadFromXmlIterator(XmlElementIterator& iter)
        {
            loadFromXmlElement(*iter);
            iter++;
        }
    };

    // Interface Am Graphics Root (IAmGroot) 
    // Core interface to hold document level state, primarily
    // for the purpose of looking up nodes, but also for style sheets
    struct IAmGroot
    {
        virtual std::shared_ptr<SVGViewable> findNodeById(const std::string& name) = 0;
        virtual std::shared_ptr<SVGViewable> findNodeByHref(const ByteSpan& href) = 0;
        virtual std::shared_ptr<SVGViewable> findNodeByUrl(const ByteSpan& inChunk) = 0;
        virtual ByteSpan findEntity(const std::string& name) = 0;

        virtual FontHandler* fontHandler() const = 0;

        virtual std::shared_ptr<CSSStyleSheet> styleSheet() = 0;
        virtual void styleSheet(std::shared_ptr<CSSStyleSheet> sheet) = 0;

        virtual void addDefinition(const std::string& name, std::shared_ptr<SVGViewable> obj) = 0;
        virtual void addEntity(const std::string& name, ByteSpan expansion) = 0;

        virtual std::string systemLanguage() { return "en"; } // BUGBUG - What a big cheat!!

        virtual double canvasWidth() const = 0;
        virtual double canvasHeight() const = 0;
        virtual double dpi() const = 0;
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
    struct SVGVisualProperty : public SVGObject  // public SVGViewable
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
            return true;
        }

        bool loadFromChunk(const ByteSpan& inChunk)
        {
            fRawValue = chunk_trim(inChunk, xmlwsp);
            if (!fRawValue)
            {
                set(false);
                return false;
            }

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

    static std::map<std::string, std::function<std::shared_ptr<SVGVisualProperty>(const ByteSpan&)>> gSVGAttributeCreation;
    static std::map<std::string, std::function<std::shared_ptr<SVGVisualProperty>(IAmGroot* aroot, const XmlAttributeCollection&)>> gSVGPropertyCreation;
    
	static void registerSVGAttribute(const std::string& name, std::function<std::shared_ptr<SVGVisualProperty>(const ByteSpan&)> func)
	{
		gSVGAttributeCreation[name] = func;
	}

    static void registerSVGProperty(const std::string& name, std::function < std::shared_ptr<SVGVisualProperty>(IAmGroot* aroot, const XmlAttributeCollection&)> func)
    {
        gSVGPropertyCreation[name] = func;
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
        std::unordered_map<std::string, std::shared_ptr<SVGVisualProperty>> fVisualProperties{};

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
                auto aWord = chunk_token(classChunk, xmlwsp);
                auto classId = toString(aWord);

                auto csel = root()->styleSheet()->getClassSelector(classId);
                if (csel != nullptr)
                {
                    loadVisualProperties(csel->attributes());
				}
                else {
                    printf("SVGVisualNode::bindPropertiesToGroot, ERROR - NO CLASS SELECTOR FOR %s\n", classId.c_str());
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
				auto idsel = root()->styleSheet()->getIDSelector(toString(id));
				if (idsel != nullptr)
				{
					loadVisualProperties(idsel->attributes());
				}
			}
            
            
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

        std::shared_ptr<SVGVisualProperty> getVisualProperty(const std::string& name)
        {
            auto it = fVisualProperties.find(name);
            if (it != fVisualProperties.end())
                return it->second;

            return nullptr;
        }


        virtual void setAttribute(const std::string& name, const ByteSpan& value)
        {
            // If the value is nothing, then don't set it
            if (!value)
                return;
            
            if (gSVGAttributeCreation.contains(name))
            {
                auto prop = gSVGAttributeCreation[name](value);
                if (prop)
                {
                    fVisualProperties[name] = prop;
                }
            }
        }

        // This might be called multiple times on an element
        // once for the regular attributes
        // a second time for attributes hidden in a 'style' attribute
        // Run through the attributes of the element
        // looking for field constructors 
        virtual void loadVisualProperties(const XmlAttributeCollection & attrCollection)
        {
            ByteSpan display = attrCollection.getAttribute("display");
            if (display)
            {
                display = chunk_trim(display, xmlwsp);

                //auto disp = std::string(display.fStart, display.fEnd);
                if (display == "none")
                    visible(false);
            }

            if (attrCollection.getAttribute("transform"))
            {
                fHasTransform = parseTransform(attrCollection.getAttribute("transform"), fTransform);
                if (fHasTransform)
                {
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
                // BUGBUG - drawing visual property
                if (prop.second->autoDraw())
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
        


        //void mouseEvent(const MouseEvent& e) override
        //{
        //  ; // do nothing by default
        //}

    };
}



namespace waavs {
    // compound node creation dispatch - 'g', 'symbol', 'pattern', 'linearGradient', 'radialGradient', 'conicGradient', 'image', 'style', 'text', 'tspan', 'use'
    static std::map<std::string, std::function<std::shared_ptr<SVGVisualNode>(IAmGroot* aroot, XmlElementIterator& iter)>> gSVGGraphicsElementCreation{};

    // Geometry node creation dispatch
    static std::map<std::string, std::function<std::shared_ptr<SVGVisualNode>(IAmGroot* root, const XmlElement& elem)>> gShapeCreationMap{};
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

        SVGGraphicsElement(IAmGroot* aroot)
            :SVGVisualNode(aroot) {}

        virtual void bindChildrenToGroot(IAmGroot* groot)
        {
			for (auto& node : fNodes)
			{
				node->bindToGroot(groot);
			}
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
        
        void draw(IRenderSVG *ctx) override
        {
            if (!visible())
                return;

            ctx->push();

            applyAttributes(ctx);
            drawSelf(ctx);
            
            drawChildren(ctx);


            ctx->pop();
        }
    
        virtual void addNode(std::shared_ptr < SVGVisualNode > node)
        {
            if (node == nullptr)
                return;

            if (!node->id().empty())
                root()->addDefinition(node->id(), node);

            if (node->isStructural()) {
                fNodes.push_back(node);
            }
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
                //printf("SVGGraphicsElement::loadSelfClosingNode UNKNOWN[%s]\n", elem.name().c_str());
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
            if (gSVGGraphicsElementCreation.contains(aname))
            {
                auto& func = gSVGGraphicsElementCreation[aname];
                auto node = func(root(), iter);
                addNode(node);
            }
            else {
                printf("SVGGraphicsElement::loadCompoundNode == UNKNOWN ==> '%s'\n", aname.c_str());
                //printXmlElement(elem);
                auto node = gSVGGraphicsElementCreation["g"](root(), iter);
                // don't add the node to the tree as we don't
                // know what it is, so ignore it
                //addNode(node);
            }
        }



        virtual void loadFromXmlIterator(XmlElementIterator& iter) override
        {
            // First, loadFromXmlElement because we're sitting on our opening element
            // and we need to gather our own attributes
            loadFromXmlElement(*iter);


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
                        printf("SVGGraphicsElement::loadFromXmlIterator ==> IGNORING: %s\n", elem.name().c_str());
                        //printXmlElement(elem);
                    }
                }
                break;

                }	// end of switch


            }
        }

    };
}
