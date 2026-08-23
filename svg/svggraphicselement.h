#pragma once

#include "svgstructuretypes.h"
//#include "filter_program_exec_b2d.h"
//#include "svg_attribute_clip.h"


namespace waavs {

    //================================================
    // SVGGraphicsElement
    //================================================
    struct SVGGraphicsElement : public ISVGElement
    {
        static constexpr uint32_t kMaxHrefDepth = 32;

        BLVar fVar{};

        XmlAttributeCollection fAttributes{};

        ByteSpan fAttributeSpan{};
        bool fStyleResolved{ false };


        // The resolved properties of this node
        std::unordered_map<InternedKey, std::shared_ptr<SVGVisualProperty>, InternedKeyHash, InternedKeyEquivalent> fVisualProperties{};
        
        // The child nodes of this element, in tree order.
        // This is all the nodes in the subtree, not just the
        // ones that are used for rendering.
        std::vector<std::shared_ptr<IViewable>> fChildren{};

        // All the nodes in the subtree which participate in drawing directly.
        std::vector<std::shared_ptr<IViewable>> fRenderNodes{};


        SVGGraphicsElement()
        {
            setNeedsBinding(true);
        }

        // return the collections of nodes that we are renderable
        const std::vector<std::shared_ptr<IViewable>> &
        renderNodes() const noexcept { return fRenderNodes; }

        // We have this separate for those cases where you want to traverse
        // the tree asking for bounding boxes, but you don't want to do it all the 
        // time.
        const WGRectD getObjectBoundingBox(IDrawGraphics* ctx, IAmGroot* groot) noexcept override
        {
            WGRectD bbox{};
            for (auto& node : fRenderNodes)
            {
                if (!node || !node->isVisible()) continue;

                WGRectD nodeBox{};
                nodeBox = node->getObjectBoundingBox(ctx, groot);
                wg_rectD_union(bbox, nodeBox);
            }

            return bbox;
        }




        // Deal with masks


        const BLVar getVariant(IDrawGraphics* ctx, IAmGroot* groot) noexcept override
        {
            // if our variant is null
            // traverse down our fNodes, until we find
            // something that reports other than null
            // and return that.
            if (fVar.is_null())
            {
                for (auto& node : fRenderNodes)
                {
                    // cast to IServePaint, if it fails, then we can't use it as a paint server
                    //auto paintServer = std::dynamic_pointer_cast<IServePaint *>(node);
                    //if (paintServer == nullptr)
                    //    continue;

                    BLVar tmpVar = node->getVariant(ctx, groot);
                    if (!tmpVar.is_null())
                    {
                        return tmpVar;
                    }
                }
            }

            return fVar;
        }


        bool hasAttribute(InternedKey key) const noexcept
        {
            return fAttributes.hasValue(key);
        }

        bool getAttribute(InternedKey key, ByteSpan& value) const noexcept
        {
            return fAttributes.getValue(key, value);
        }

        ByteSpan getAttribute(InternedKey key) const noexcept
        {
            ByteSpan value{};
            fAttributes.getValue(key, value);
            return value;
        }

        ByteSpan getAttributeByName(const char* name) const noexcept
        {
            return getAttribute(WSNameSet::INTERN(name));
        }

        // setting attributes
        void setAttribute(InternedKey name, const ByteSpan& value)  noexcept
        {
            fAttributes.addValue(name, value);
        }

        void setAttributeByName(const char* name, const ByteSpan& value) noexcept
        {
            InternedKey key = WSNameSet::INTERN(name);
            setAttribute(key, value);
        }

        // The way the inheritance works is, if we don't currently have
        // a value for a particular attribute, but the referred to element does
        // then we should take that value from the referred to gradient.
        void setAttributeIfAbsent(const SVGGraphicsElement* elem, InternedKey key)
        {
            if (!elem)
                return;

            if (!hasAttribute(key))
            {
                ByteSpan candidateAttr{};
                if (elem->getAttribute(key, candidateAttr))
                    setAttribute(key, candidateAttr);
            }
        }

        // Property management
        void addVisualProperty(InternedKey key, std::shared_ptr<SVGVisualProperty> prop)
        {
            fVisualProperties[key] = prop;
        }

        std::shared_ptr<SVGVisualProperty> 
        getVisualProperty(InternedKey key) const override
        {
            auto it = fVisualProperties.find(key);
            if (it != fVisualProperties.end())
                return it->second;

            return nullptr;
        }

        std::shared_ptr<SVGVisualProperty>
        getVisualProperty(InternedKey key) 
        {
            auto it = fVisualProperties.find(key);
            if (it != fVisualProperties.end())
                return it->second;

            return nullptr;
        }

        // To avoid spreading dynamic_pointer_cast all over the place, 
        // we have this templated version of getVisualProperty
        template<typename PropertyT>
        std::shared_ptr<PropertyT> getVisualPropertyAs(InternedKey key)
        {
            return std::dynamic_pointer_cast<PropertyT>(
                getVisualProperty(key));
        }

        template<typename PropertyT>
        std::shared_ptr<PropertyT> getVisualPropertyAs(InternedKey key) const
        {
            return std::dynamic_pointer_cast<PropertyT>(getVisualProperty(key));
        }


        // Adding nodes to our tree

        // We want to give all nodes a chance to be added to 
        // the index of whatever 'groot' is passed in.  That way
        // they can be looked up by their ID later if they have one.
        virtual bool addNodeToIndex(std::shared_ptr < IViewable > node, IAmGroot* groot)
        {
            if (node == nullptr || groot == nullptr)
                return false;

            // Get an attribute 'id' if it exists
            ByteSpan nodeId = node->id();
            if (!nodeId.empty())
                groot->addElementReference(nodeId, node);
            return true;
        }

        virtual bool addNodeToChildren(std::shared_ptr < IViewable > node, IAmGroot* groot)
        {
            if (!node || !groot)
                return false;

            fChildren.push_back(node);

            return true;
        }

        virtual bool addNodeToRenderTree(std::shared_ptr < IViewable > node, IAmGroot* groot)
        {
            if (!node)
                return false;

            if (node->isStructural()) {
                fRenderNodes.push_back(node);
            }
            return true;
        }

        virtual bool addNode(std::shared_ptr < IViewable > node, IAmGroot* groot)
        {
            if (!node || !groot)
                return false;

            addNodeToIndex(node, groot);
            addNodeToChildren(node, groot);
            addNodeToRenderTree(node, groot);

            return true;
        }

        virtual void loadSelfClosingNode(const XmlElement& elem, IAmGroot* groot)
        {

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

        // Sometimes we come across an element tag name that we 
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
        // It's in 'fixupSelfStyleAttributes()', that we actually bind attributes
        // to base types.
        //
        // Note:  We could just use scanAttributes(), and stuff them all into 
        // fPresentationAttributes collection, then pull out the id, class, and style
        // from there.  No need for all this special work
        //
        virtual void loadFromXmlElement(const XmlElement& elem, IAmGroot* groot)
        {
            fSourceElement = elem;

            // Save the name if we've got one
            setName(elem.nameAtom());

            // Get the span for all the presentation attributes on the element to start
            fAttributeSpan = elem.data();
            
            // Get id attribute if it exists, and save it for later
            ByteSpan idValue{};
            if (elem.getElementAttribute(svgattr::id(), idValue))
                setId(idValue);

        }

        virtual void onEndTag(IAmGroot* groot)
        {
            // This is called after we've loaded the element and all its children from the XML
            // so, at this point, we should have a full tree of nodes built out, and we can do any 
            // post processing that requires having the full tree available.
        }

        virtual void loadFromXmlPull(XmlPull& iter, IAmGroot* groot, bool isRoot = false)
        {
            if (!isRoot)
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
                    onEndTag(groot);
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

        virtual void fixupSelfStyleAttributes(IAmGroot*)
        {
            // printf("fixupSelfStyleAttributes\n");
        }

        void fixupStyleAttributes(IAmGroot* groot) override
        {
            fAttributes.clear();

            ByteSpan classAttribute{};
            ByteSpan styleAttribute{};

            // Bring in presentation attributes first, since they
            // have the lowest precedence
            // skip the id field, and hold out the style and class attributes
            // for later processing, since they have special handling.
    
            // since the xmlattribute_read_next() function is destructive of the span
            // we need to make a copy of the span to work with
            ByteSpan src = fAttributeSpan;

            // Create a couple of spans to hold the name and value
            // pairs.  These will be reused on each iteration
            ByteSpan attrName{};
            ByteSpan attrValue{};

            while (xmlattribute_read_next(src, attrName, attrValue))
            {
                InternedKey attrKey = WSNameSet::INTERN(attrName);

                if (attrKey == svgattr::id())
                {
                    //setId(attrValue);
                    continue;
                }
                else if (attrKey == svgattr::style() && !attrValue.empty())
                {
                    ByteSpan styleValue = attrValue;
                    bspan_trim_spaces(styleValue);

                    if (styleValue && *styleValue == '&')
                    {
                        // advance past the '&' character
                        styleValue.advance(1);

                        // remove the trailing ';' if it exists
                        bspan_rtrim(styleValue,";");
                        
                        // check to see if there's an entity reference at the beginning
                        styleValue = groot->findXmlEntity(styleValue);
                        if (styleValue.empty())
                        {
                            // if we can't resolve the entity reference, then just use the original value
                            styleAttribute = attrValue;
                        }
                        else
                            styleAttribute = styleValue;
                    }
                    else
                    {
                        styleAttribute = styleValue;
                    }
                }
                else if (attrKey == svgattr::klass())
                {
                    classAttribute = attrValue;
                }
                else {
                    // Add directly to attributes collection
                    fAttributes.addValue(attrKey, attrValue);
                }
            }

            // Next in precedence are the CSS based attributes, which can 
            // come from multiple selectors, so we need to loop through 
            // all of them and merge them in order of increasing precedence
            if (groot != nullptr)
            {
                // CSS based on tagname
                if (nameAtom())
                {
                    auto esel = groot->styleSheet().getSelector(CSS_SELECTOR_ELEMENT, nameAtom());
                    if (esel != nullptr)
                    {
                        fAttributes.mergeAttributes(esel->attributes());
                    }
                }

                // CSS class list specifier
                // An element can belong to multiple classes, which
                // are specified in the 'class' attribute as a whitespace 
                // delimited list of class names, so we need to loop through 
                // all of them and merge in any attributes from any matching 
                // class selectors.
                ByteSpan classChunk = classAttribute;
                while (classChunk)
                {
                    // peel a word off the front
                    auto classId = bspan_read_until(classChunk, chrWspChars);
                    auto csel = groot->styleSheet().getSelector(CSS_SELECTOR_CLASS, classId);
                    if (csel != nullptr)
                    {
                        fAttributes.mergeAttributes(csel->attributes());
                    }
                    else {
                        //printf("SVGVisualNode::bindPropertiesToGroot, ERROR - NO CLASS SELECTOR FOR %s\n", toString(classId).c_str());
                    }
                }

                // CSS ID based selector if we have one
                if (id())
                {
                    auto idsel = groot->styleSheet().getSelector(CSS_SELECTOR_ID, id());
                    if (idsel != nullptr)
                    {
                        fAttributes.mergeAttributes(idsel->attributes());
                    }
                }
            }


            // Highest presedence is the inline 'style' attribute, 
            // which can contain multiple attributes in it, so we need to parse it
            // Upsert any of the attributes associated with 'style' attribute if they exist
            if (styleAttribute) {
                parseStyleAttribute(styleAttribute, fAttributes);
            }

            // Give a chance for a sub-class to do any additional processing 
            // of the attributes, or to set up any properties based on the attributes
            this->fixupSelfStyleAttributes(groot);

            // Use up some of the attributes
            ByteSpan displayAttr{};
            if (fAttributes.getValue(svgattr::display(), displayAttr))
            {
                bspan_trim_spaces(displayAttr);
                InternedKey dv = WSNameSet::INTERN(displayAttr);

                if (dv == svgval::none())
                    setIsVisible(false);
            }

        }

        // resolveStyleAttributes
        //
        // At this point, the document has been fully loaded
        // so all the styles can be fixed up, and the ones
        // that have property mappers can be converted 
        // into properties for use during drawing.
        // 
        void resolveStyleAttributes(IAmGroot* groot)
        {
            fixupStyleAttributes(groot);
            fStyleResolved = true;

            // Convert the attributes that have a property registration 
            // into VisualProperty objects
            convertAttributesToProperties(groot);
        }

        // This is called after the document is fully loaded
        // and nodes available through groot, but before any
        // drawing has occured.

        void resolveStyleSubtree(IAmGroot* groot)
        {
            // First resolve our own style attributes, if we haven't already
            if (!fStyleResolved)
                resolveStyleAttributes(groot);

            // Then resolve the subtree
            //for (auto& node : fRenderNodes)
            for (auto& node : fChildren)
            {
                if (!node)
                    continue;

                // Only GraphicsElements have a style attributes and fNodes
                auto ge = std::dynamic_pointer_cast<SVGGraphicsElement>(node);
                if (ge) {
                    ge->resolveStyleSubtree(groot);
                }
                else {

                }
            }
        }



        //========================================
        // Animation support
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
            for (auto& node : fChildren)
            {
                node->update(groot);
            }
        }

        virtual void updateSelf(IAmGroot* groot) {}

        void update(IAmGroot* groot) override
        {
            //this->updateProperties(groot);
            this->updateSelf(groot);
            this->updateChildren(groot);
        }
        //========================================



        // convertAttributesToProperties
        // Take the step of converting a raw attribute
        // value into a specific display property if a routine
        // exists for it.
        void convertAttributesToProperties(IAmGroot* groot)
        {
            fAttributes.values().forEach([this, &groot](InternedKey key, const ByteSpan& value) {
                auto propertyMapper = getAttributeConverter(key);
                if (!propertyMapper)
                    return;

                    auto prop = propertyMapper(fAttributes, groot);
                    if (prop)
                        addVisualProperty(key, std::move(prop));

                });

        }

        virtual void bindSelfToContext(IDrawGraphics*, IAmGroot*) { ; }
        virtual void bindGeometryToContext(IDrawGraphics*, IAmGroot*) { ; }
        virtual void bindPaintToContext(IDrawGraphics*, IAmGroot*) { ; }


        // For compound nodes (which have children) we want to 
        // do the base stuff (binding properties) then bind the children
        // If you sub-class this, you should call this first
        // then do your own thing.  We don't want to call a 'bindSelfToGroot'
        // here, because that complicates the interactions and sequences of things
        // so just override bindToGroot
        void bindToContext(IDrawGraphics* ctx, IAmGroot* groot) noexcept override
        {
            // First, find any style attributes that might apply
            // to this element
            if (!fStyleResolved)
                this->resolveStyleAttributes(groot);



            // Tell the structure to bind the rest of its stuff
            bindSelfToContext(ctx, groot);

            setNeedsBinding(false);
        }

        // applyProperty
        // 
        // Apply a single property to the context
        // if it exists, is set, and is autoDraw
        void applyProperty(IDrawGraphics *ctx, IAmGroot* groot, InternedKey key)
        {
            auto prop = getVisualProperty(key);
            if (prop && prop->autoDraw() && prop->isSet())
            {
                prop->applyToContext(ctx, groot);
            }
        }

        // applyProperties
        // 
        // Given a comma separated list of property names,
        // apply each of those properties to the context if they exist.

        void applyProperties(IDrawGraphics* ctx, IAmGroot* groot, ByteSpan names)
        {
            ByteSpan s = names;

            while (s)
            {
                ByteSpan name = bspan_read_until(s, ",");
                bspan_trim_spaces(name);

                if (!name)
                    continue;

                applyProperty(ctx, groot, WSNameSet::INTERN(name));
            }
        }

        virtual void resolvePaintServers(IDrawGraphics* ctx, IAmGroot* groot)
        {
            if (!ctx)
                return;

            IServePaint* fillServer = ctx->state().getFillPaintServer();
            IServePaint* strokeServer = ctx->state().getStrokePaintServer();
        
            if (fillServer)
            {
                BLVar aVar = fillServer->getVariant(ctx, groot);
                if (aVar.is_null())
                    ctx->noFill();
                else
                    ctx->applyFillPaint(aVar);
            }

            if (strokeServer)
            {
                BLVar aVar = strokeServer->getVariant(ctx, groot);
                if (aVar.is_null())
                    ctx->noStroke();
                else
                    ctx->applyStrokePaint(aVar);
            }
        }



        virtual void applyTransform(IDrawGraphics* ctx, IAmGroot* groot)
        {
            auto tform = getVisualProperty(svgattr::transform());
            if (tform)
                tform->applyToContext(ctx, groot);
        }


        void applyPaintSelectionProperties(IDrawGraphics* ctx, IAmGroot* groot)
        {
            applyProperty(ctx, groot, svgattr::color());
            applyProperty(ctx, groot, svgattr::fill());
            applyProperty(ctx, groot, svgattr::stroke());
            applyProperty(ctx, groot, svgattr::fill_rule());
            //applyProperty(ctx, groot, svgattr::clip_rule());

            applyProperty(ctx, groot, svgattr::stroke_width());
            applyProperty(ctx, groot, svgattr::stroke_linecap());
            applyProperty(ctx, groot, svgattr::stroke_linejoin());
            applyProperty(ctx, groot, svgattr::stroke_miterlimit());
            applyProperty(ctx, groot, svgattr::stroke_dasharray());
            applyProperty(ctx, groot, svgattr::stroke_dashoffset());

            applyProperty(ctx, groot, svgattr::marker());
            applyProperty(ctx, groot, svgattr::marker_start());
            applyProperty(ctx, groot, svgattr::marker_mid());
            applyProperty(ctx, groot, svgattr::marker_end());
        }

        void applyTextProperties(IDrawGraphics* ctx, IAmGroot* groot)
        {
            static ByteSpan kTextProperties =
                "font-family,"
                "font-size,"
                "font-size-adjust,"
                "font-stretch,"
                "font-style,"
                "font-variant,"
                "font-weight,"
                "direction,"
                "unicode-bidi,"
                "writing-mode,"
                "glyph-orientation-horizontal,"
                "glyph-orientation-vertical,"
                "text-anchor,"
                "dominant-baseline,"
                "alignment-baseline,"
                "baseline-shift,"
                "kerning,"
                "letter-spacing,"
                "word-spacing,"
                "text-rendering";

            applyProperties(ctx, groot, kTextProperties);
        }


        virtual void drawSelf(IDrawGraphics*, IAmGroot*)
        {
            ;
        }



        WGRectD drawBegin( IDrawGraphics* ctx, IAmGroot* groot)
        {
            ctx->push();

            if (needsBinding())
                bindToContext(ctx, groot);

            applyTextProperties(ctx, groot);
            applyPaintSelectionProperties(ctx, groot);
            applyTransform(ctx, groot);

            bindGeometryToContext(ctx, groot);

            WGRectD bbox =
                getObjectBoundingBox(ctx, groot);

            ctx->state().setObjectFrame(bbox);

            resolvePaintServers(ctx, groot);

            return bbox;
        }

        void drawEnd(IDrawGraphics* ctx)
        {
            ctx->pop();
        }

        // Compatibility path only.
        //
        // Full document rendering, including effects and renderer-owned
        // child traversal, should enter through SVGRenderer.
        void draw(
            IDrawGraphics* ctx,
            IAmGroot* groot,
            RenderFlags flags = RF_All) override
        {
            (void)flags;

            if (!ctx || !groot)
                return;

            drawBegin(ctx, groot);

            drawSelf(ctx, groot);

            for (const auto& child : fRenderNodes)
            {
                if (child && child->isVisible())
                    child->draw(ctx, groot, flags);
            }

            drawEnd(ctx);
        }



    };
}

