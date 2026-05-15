#pragma once

#include "svgstructuretypes.h"
#include "filter_program_exec_b2d.h"

namespace waavs
{

    template<class SubtreeT>
    static bool resolveIsolatedRenderPlan(
        IRenderSVG* ctx,
        IAmGroot* groot,
        SubtreeT* elem,
        const WGRectD& objectBBoxUS,
        RenderFlags rFlags,
        IsolatedRenderPlan& plan) noexcept
    {
        if (!ctx || !groot || !elem)
            return false;

        plan = {};
        plan.objectBBoxUS = objectBBoxUS;
        plan.ctm = ctx->getTransform();
        plan.invCtm = plan.ctm;
        (void)plan.invCtm.invert();

        plan.hasFilter =
            rFlags.has(RF_Filter) &&
            elem->hasFeature(svgattr::filter());

        plan.hasMask =
            rFlags.has(RF_Mask) &&
            elem->hasFeature(svgattr::mask());

        plan.hasClip =
            rFlags.has(RF_Clip) &&
            elem->hasFeature(svgattr::clip_path());

        plan.hasOpacity =
            rFlags.has(RF_Opacity) &&
            elem->hasFeature(svgattr::opacity());

        plan.needsIsolation =
            plan.hasFilter ||
            plan.hasMask ||
            plan.hasClip ||
            plan.hasOpacity;

        if (!plan.needsIsolation)
            return true;

        WGRectD effectRectUS = objectBBoxUS;

        if (plan.hasFilter) {
            auto filterNode = elem->getReferencedFeatureNode(groot, svgattr::filter());
            if (!filterNode)
                plan.hasFilter = false;
            else
                effectRectUS = filterNode->resolveFilterRegion(ctx, groot, objectBBoxUS);
        }

        // check whether isolation is still needed if the filter
        // doesn't actually exist.
        plan.needsIsolation =
            plan.hasFilter ||
            plan.hasMask ||
            plan.hasClip ||
            plan.hasOpacity;

        if (!plan.needsIsolation)
            return true;

        if (!(effectRectUS.w > 0.0) || !(effectRectUS.h > 0.0))
            return false;

        plan.effectRectUS = effectRectUS;
        plan.nominalRectPX = mapRectAABB(plan.ctm, effectRectUS);

        // Later this should intersect against viewport/device clip.
        plan.allocRectPX = plan.nominalRectPX;

        if (!(plan.allocRectPX.w > 0.0) || !(plan.allocRectPX.h > 0.0))
            return false;

        const int x0 = (int)std::floor(plan.allocRectPX.x);
        const int y0 = (int)std::floor(plan.allocRectPX.y);
        const int x1 = (int)std::ceil(plan.allocRectPX.x + plan.allocRectPX.w);
        const int y1 = (int)std::ceil(plan.allocRectPX.y + plan.allocRectPX.h);

        if (x1 <= x0 || y1 <= y0)
            return false;

        plan.pixelRect = WGRectI{ x0, y0, x1 - x0, y1 - y0 };
        return true;
    }


    template<class SubtreeT>
    bool renderSubtreeToSurface(
        IAmGroot* groot,
        SubtreeT* subtree,
        const IsolatedSubtreeRequest& req,
        Surface& outSurface) noexcept
    {
        if (!groot || !subtree)
            return false;

        if (req.pixelRect.w <= 0 || req.pixelRect.h <= 0)
            return true;

        // Resize the output surface if needed.
        //
        if (outSurface.width() != uint32_t(req.pixelRect.w) || outSurface.height() != uint32_t(req.pixelRect.h))
        {
            outSurface.reset(
                uint32_t(req.pixelRect.w),
                uint32_t(req.pixelRect.h));
        }

        if (outSurface.empty())
            return false;

        WGMatrix3x3 off = req.ctm;
        off.postTranslate(-double(req.pixelRect.x), -double(req.pixelRect.y));

        SVGB2DDriver tmp{};
        tmp.attach(outSurface, 1, &req.drawingState);
        //tmp.renew();

        // We have to pass along the drawing state
        // as the item in question will need context
        tmp.copyDrawingState(req.drawingState);


        if (req.clear)
            tmp.clear();

        tmp.transform(off);
        tmp.setObjectFrame(req.objectBBoxUS);
        tmp.setViewport(req.objectBBoxUS);

        if (req.renderMode.has(RenderFeature::RF_Content))
        {
            subtree->drawContent(&tmp, groot);
        }

        tmp.detach();

        return true;
    }


}

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


        std::unordered_map<InternedKey, std::shared_ptr<SVGVisualProperty>, InternedKeyHash, InternedKeyEquivalent> fVisualProperties{};
        std::vector<std::shared_ptr<IViewable>> fNodes{};

        SVGGraphicsElement()
        {
            setNeedsBinding(true);
        }

        // We have this separate for those cases where you want to traverse
        // the tree asking for bounding boxes, but you don't want to do it all the 
        // time.
        const WGRectD getObjectBoundingBox(IRenderSVG* ctx, IAmGroot* groot) noexcept
        {
            WGRectD bbox{};
            for (auto& node : fNodes)
            {
                if (!node || !node->isVisible()) continue;

                WGRectD nodeBox{};
                nodeBox = node->getObjectBoundingBox(ctx, groot);
                wg_rectD_union(bbox, nodeBox);
            }

            return bbox;
        }

        // hasFeature
        //
        // Indicates whether the node has a particular feature
        // based on the presence of a given attribute.
        // Used to detect; filter, mask, clip, opacity
        virtual bool hasFeature(InternedKey featureKey)
        {
            ByteSpan nodeRef{};
            if (!getAttribute(featureKey, nodeRef))
                return false;

            return !nodeRef.empty();
        }

        std::shared_ptr< SVGGraphicsElement> getReferencedFeatureNode(IAmGroot* groot, InternedKey featureKey) const noexcept
        {
            if (!groot)
                return {};

            // Get the filter reference from our 'filter' attribute
            ByteSpan nodeRef{};
            if (!getAttribute(featureKey, nodeRef))
                return {};

            // Look up the filter element by id reference
            // (filterRef is expected to be in the form "url(#id)")
            auto featureNode = groot->findNodeByUrl(nodeRef);
            if (!featureNode)
                return {};

            // try to dynamic cast the feature 
            auto featureElem = std::dynamic_pointer_cast<SVGGraphicsElement>(featureNode);
            if (!featureElem)
                return {};

            return featureElem;
        }

        // Deal with masks


        // Deal with filters
        // Retrieve a filter program stream for this element
        // Any SVGGraphicsElement can have a filter, but only SVGFilter elements 
        // will actually have a program stream to return.
        virtual std::shared_ptr<FilterProgramStream> getFilterProgramStream(IAmGroot* groot) noexcept
        {
            return nullptr;
        }

        // deal with masks
        virtual bool applyMaskToSurface(IAmGroot* groot,
            const IsolatedRenderPlan& plan,
            Surface& result) noexcept
        {
            return true;
        }

        // Deal with clipping
        virtual bool applyClipToSurface(
            IRenderSVG *ctx,
            IAmGroot* groot,
            const IsolatedRenderPlan& plan,
            Surface& result) noexcept
        {
            return true;
        }



        const BLVar getVariant(IRenderSVG* ctx, IAmGroot* groot) noexcept override
        {
            // if our variant is null
            // traverse down our fNodes, until we find
            // something that reports other than null
            // and return that.
            if (fVar.is_null())
            {
                for (auto& node : fNodes)
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
            return getAttribute(PSNameTable::INTERN(name));
        }

        // setting attributes
        void setAttribute(InternedKey name, const ByteSpan& value)  noexcept
        {
            fAttributes.addValue(name, value);
        }

        void setAttributeByName(const char* name, const ByteSpan& value) noexcept
        {
            InternedKey key = PSNameTable::INTERN(name);
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

        std::shared_ptr<SVGVisualProperty> getVisualProperty(InternedKey key) override
        {
            auto it = fVisualProperties.find(key);
            if (it != fVisualProperties.end())
                return it->second;

            return nullptr;
        }


        // Adding nodes to our tree
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

        virtual bool addNodeToSubtree(std::shared_ptr < IViewable > node, IAmGroot* groot)
        {
            if (node == nullptr || groot == nullptr)
                return false;
            if (node->isStructural()) {
                fNodes.push_back(node);
            }
            return true;
        }

        virtual bool addNode(std::shared_ptr < IViewable > node, IAmGroot* groot)
        {
            if (node == nullptr || groot == nullptr)
                return false;

            addNodeToIndex(node, groot);
            addNodeToSubtree(node, groot);

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

            /*
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
            // BUGBUG: This might be better done by just getting the specific
            // attributes out of the attributeSpan, but not building up
            // the presentation attributes collection.  That collection is
            // largely temporary, as it's used in fixup, and just bloats the size of the structure
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
            */

        }

        virtual void onEndTag(IAmGroot* groot)
        {
            // This is called after we've loaded the element and all its children from the XML
            // so, at this point, we should have a full tree of nodes built out, and we can do any 
            // post processing that requires having the full tree available.
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

            // If we're here, we've run off the end of the document without finding a matching end tag
            // Maybe an assert would be appropriate here, but for now we'll just call the 'onLoadedFromXmlPull' 
            // to allow any post processing to happen
            //onEndTag(groot);
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
    
            // since the readNextKeyAttribute() function is destructive of the span
            // we need to make a copy of the span to work with
            ByteSpan src = fAttributeSpan;

            // Create a couple of spans to hold the name and value
            // pairs.  These will be reused on each iteration
            ByteSpan attrName{};
            ByteSpan attrValue{};

            while (readNextKeyAttribute(src, attrName, attrValue))
            {
                InternedKey attrKey = PSNameTable::INTERN(attrName);

                if (attrKey == svgattr::id())
                {
                    //setId(attrValue);
                    continue;
                }
                else if (attrKey == svgattr::style() && !attrValue.empty())
                {
                    styleAttribute = attrValue;
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
                    auto classId = chunk_token(classChunk, chrWspChars);
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
                displayAttr = chunk_trim(displayAttr, chrWspChars);
                InternedKey dv = PSNameTable::INTERN(displayAttr);

                if (dv == svgval::none())
                    setIsVisible(false);
            }

        }

        void resolveStyleAttributes(IAmGroot* groot)
        {
            fixupStyleAttributes(groot);
            fStyleResolved = true;
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
            for (auto& node : fNodes)
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
            for (auto& node : fNodes)
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
            if (!fStyleResolved)
                this->resolveStyleAttributes(groot);


            // Convert the attributes that have a property registration 
            // into VisualProperty objects
            convertAttributesToProperties(ctx, groot);

            // Tell the structure to bind the rest of its stuff
            bindSelfToContext(ctx, groot);

            setNeedsBinding(false);
        }

        virtual void applyProperties(IRenderSVG* ctx, IAmGroot* groot)
        {
            for (auto& prop : fVisualProperties)
            {
                if (prop.second->autoDraw() && prop.second->isSet())
                {
                    prop.second->applyToContext(ctx, groot);
                }
            }
        }

        virtual void drawChildren(IRenderSVG* ctx, IAmGroot* groot)
        {
            for (auto& node : fNodes)
            {
                // should we check to see if the node
                // is visible before drawing it?
                bool nodeVisible = node->isVisible();
                if (nodeVisible)
                {
                    node->draw(ctx, groot);
                }
            }
        }


        virtual void drawSelf(IRenderSVG*, IAmGroot*)
        {
            ;
        }

        // This is the core of drawing the content for this node
        // All environment preperation occurs outside this call.
        void drawContent(IRenderSVG* ctx, IAmGroot* groot)
        {
            applyProperties(ctx, groot);
            drawSelf(ctx, groot);
            drawChildren(ctx, groot);
        }

        WGRectD drawBegin(IRenderSVG* ctx, IAmGroot* groot) 
        {
            if (needsBinding())
                this->bindToContext(ctx, groot);

            ctx->push();

            // Apply transform first since it affects bbox space
            auto tform = getVisualProperty(svgattr::transform());
            if (tform)
                tform->applyToContext(ctx, groot);

            // Compute bbox in current user space
            WGRectD bbox = getObjectBoundingBox(ctx, groot);

            ctx->setObjectFrame(bbox);

            return bbox;
        }

        void drawEnd(IRenderSVG* ctx, IAmGroot* groot)
        {
            ctx->pop();
        }

        void draw(IRenderSVG* ctx, IAmGroot* groot, RenderFlags rFlags = RF_All) override
        {
            if (!ctx || !groot)
                return;

            WGRectD bbox = drawBegin(ctx, groot);

            IsolatedRenderPlan plan{};
            if (!resolveIsolatedRenderPlan(ctx, groot, this, bbox, rFlags, plan)) {
                drawEnd(ctx, groot);
                return;
            }

            // BUGBUG - just for debugging
            //plan.needsIsolation = false;

            // If we don't need any isolation, then just draw the content
            // as per usual
            if (!plan.needsIsolation) {
                drawContent(ctx, groot);
                drawEnd(ctx, groot);
                return;
            }

            Surface result{};

            if (plan.hasFilter) {
                auto filterNode = getReferencedFeatureNode(groot, svgattr::filter());
                auto program = filterNode ? filterNode->getFilterProgramStream(groot) : nullptr;

                if (program) {
                    RenderFlags sourceFlags = rFlags;
                    sourceFlags.remove(RF_Filter);

                    B2DFilterExecutor exec;
                    WGResult err = exec.applyFilterToSurface(
                        ctx,
                        groot,
                        this,
                        plan.objectBBoxUS,
                        plan.effectRectUS,
                        plan.pixelRect,
                        *program,
                        result,
                        sourceFlags);
                }
            }
            else {
                IsolatedSubtreeRequest req{};
                SVGDrawingState* ds = ctx->getDrawingState();
                if (ds)
                    req.drawingState = *ds;
                req.userRect = plan.effectRectUS;
                req.pixelRect = plan.pixelRect;
                req.ctm = plan.ctm;
                req.objectBBoxUS = plan.objectBBoxUS;
                req.renderMode = RF_Content;

                renderSubtreeToSurface(groot, this, req, result);
            }

            if (!result.empty()) {
                if (plan.hasMask)
                {
                    //printf("Applying mask to surface...\n");
                    auto featureNode = getReferencedFeatureNode(groot, svgattr::mask());
                    if (featureNode)
                        featureNode->applyMaskToSurface(groot, plan, result);
                }

                if (plan.hasClip)
                {
                    auto featureNode = getReferencedFeatureNode(groot, svgattr::clip_path());
                    if (featureNode)
                        featureNode->applyClipToSurface(ctx,groot, plan, result);
                }

                if (plan.hasOpacity)
                {
                    //printf("Applying opacity to surface...\n");
                    ByteSpan opacityAttr{};
                    if (getAttribute(svgattr::opacity(), opacityAttr))
                    {
                        SVGNumberOrPercent op{};
                        if (readSVGNumberOrPercent(opacityAttr, op))
                        {
                            double opacityValue = op.calculatedValue();
                            ctx->globalOpacity(opacityValue);
                        }
                    }
                }

                ctx->push();
                ctx->transform(WGMatrix3x3::makeIdentity());
                ctx->image(result, double(plan.pixelRect.x), double(plan.pixelRect.y));
                ctx->flush();
                ctx->pop();
            }

            drawEnd(ctx, groot);
        }



    };
}

