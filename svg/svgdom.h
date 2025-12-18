#pragma once

#include <memory>

#include "xmltypes.h"
#include "xmlelementgen.h"

#include "fonthandler.h"
#include "bspan.h"
#include "membuff.h"


namespace waavs 
{
    struct SVGNode : public XmlElement
    {
        std::vector<std::shared_ptr<SVGNode>> fNodes{};

        bool isStructural() const noexcept
        {
            return true;
        }

        ByteSpan id() const noexcept
        {
            ByteSpan val{};
            if (this->getRawAttributeValue("id", val))
                return val;
         
            return {};
        }

        // addNode
        // 
        // Add a child node to this node
        //
        virtual bool addNode(std::shared_ptr < SVGNode > node)
        {
            if (node == nullptr)
                return false;

            fNodes.push_back(node);

            return true;
        }
    };


    // SVGDOM
    //
    // Data structure to hold a parsed SVG Document Object Model
    // Data structures such as referenced items, 
    // CSS Style Sheets,
    // Element tree
    // Once you have this DOM, you can then run over it and turn it
    // into something else, like binding it to a graphics rendering tree
    // Some of the core data types are converted to their native representation, where possible
    // That goes for things like fixed sizes, color values, transforms, etc.
    // Everything else is left in a raw form, ready for interpretation by a DOM walker.
    //
    // Note: This DOM is not bound to any particular rendering context
    //
    struct SVGDOM
    {
        SVGNode fRootNode;
        std::vector<std::shared_ptr<SVGNode>> fNodes{};

        MemBuff fSourceMem{};
        
        // For inquiry by href
        std::unordered_map<ByteSpan, std::shared_ptr<SVGNode>, ByteSpanHash, ByteSpanEquivalent> fDefinitions{};
        std::unordered_map<ByteSpan, ByteSpan, ByteSpanHash, ByteSpanEquivalent> fEntities{};


        virtual void addElementReference(const ByteSpan& name, std::shared_ptr<SVGNode> obj)
        {
            fDefinitions[name] = obj;
        }

        virtual std::shared_ptr<SVGNode> getElementById(const ByteSpan& name)
        {
            auto it = fDefinitions.find(name);
            if (it != fDefinitions.end())
                return it->second;

            return {};
        }


        // Load a URL Reference
        virtual std::shared_ptr<SVGNode> findNodeByHref(const ByteSpan& inChunk)
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
        virtual std::shared_ptr<SVGNode> findNodeByUrl(const ByteSpan& inChunk)
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


        //virtual std::shared_ptr<CSSStyleSheet> getStyleSheet()
        //{ 
        //    return nullptr;
        //}
        
        //virtual void setStyleSheet(std::shared_ptr<CSSStyleSheet> sheet)
        //{
        //
        //}

        // Structural construction

        virtual bool addNode(std::shared_ptr < SVGNode > node)
        {
            if (node == nullptr)
                return false;

            if (!node->id().empty())
                addElementReference(node->id(), node);

            if (node->isStructural()) {
                fNodes.push_back(node);
            }

            return true;
        }

        static std::shared_ptr<SVGNode> createContainerNode(XmlElementIterator& iter)
        {
            ByteSpan aname = iter->name();
            auto anode = std::make_shared<SVGNode>(iter.currentElement());
            return anode;
        }

        virtual void loadSelfClosingNode(const XmlElement& elem)
        {
            //printf("SVGGraphicsElement::loadSelfClosingNode: \n");

            auto anode = createSingularNode(elem);
            if (anode != nullptr) {
                this->addNode(anode);
            }
            else {
                //printf("SVGGraphicsElement::loadSelfClosingNode UNKNOWN[%s]\n", toString(elem.name()).c_str());
                //printXmlElement(elem);
            }
        }


        virtual  void loadStartTag(XmlElementIterator& iter)
        {
            // Add a child, and call loadIterator
            // If the name of the element is found in the map,
            // then create a new node of that type and add it
            // to the list of nodes.
            auto node = createContainerNode(iter);
            if (node != nullptr) {
                this->addNode(node);
            }
            else {
                // BUGBUG
                // This isn't strictly needed as we're not actually
                // adding the node to anything.
                auto& mapper = getSVGContainerCreationMap();
                auto& mapperfunc = mapper["g"];
                if (mapperfunc)
                    node = mapperfunc(groot, iter);
            }

        }

        virtual void loadEndTag(const XmlElement&)
        {
            //printf("SVGGraphicsElement::loadEndTag [%s]\n", toString(elem.name()).c_str());
        }

        virtual void loadContentNode(const XmlElement&)
        {
            //printf("SVGCompountNode::loadContentNode\n");
            //printXmlElement(elem);
            // Do something with content nodes	
        }

        virtual void loadComment(const XmlElement&)
        {
            //printf("SVGGraphicsElement::loadComment\n");
            //printXmlElement(elem);
            // Do something with comments
        }

        virtual void loadCDataNode(const XmlElement&)
        {
            //printf("SVGGraphicsElement::loadCDataNode\n");
            //printXmlElement(elem);
            // Do something with CDATA nodes
        }



        // Parsing up the whole document from an XML Iterator
        void loadFromCache()
        {
            XmlIteratorState state{ fSourceMem.span() };
            XmlIteratorParams params{};
            params.fAutoScanAttributes = false;

            XmlElement elem{};


            // By the time we get here, the iterator is already positioned on a ready
            // to use element
            // Do something with that information if we need to
            // before continuing onto other nodes
            while(nextXmlElement(params, state, elem))
            {
                // BUGBUG - debug
                //printXmlElement(*iter);

                switch (elem.kind())
                {
                case XML_ELEMENT_TYPE_START_TAG:                    // <tag>
                    this->loadStartTag(iter);
                    break;

                case XML_ELEMENT_TYPE_END_TAG:                      // </tag>
                    this->loadEndTag(elem);
                    return;

                case XML_ELEMENT_TYPE_SELF_CLOSING:                 // <tag/>
                    this->loadSelfClosingNode(elem);
                    break;

                case XML_ELEMENT_TYPE_CONTENT:                      // <tag>content</tag>
                    this->loadContentNode(elem);
                    break;

                case XML_ELEMENT_TYPE_COMMENT:                      // <!-- comment -->
                    this->loadComment(elem);
                    break;

                case XML_ELEMENT_TYPE_CDATA:                        // <![CDATA[<greeting>Hello, world!</greeting>]]>
                    this->loadCDataNode(elem);
                    break;

                case XML_ELEMENT_TYPE_DOCTYPE:                      // <!DOCTYPE greeting SYSTEM "hello.dtd">
                case XML_ELEMENT_TYPE_ENTITY:                       // <!ENTITY hello "Hello">
                case XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION:       // <?target data?>
                case XML_ELEMENT_TYPE_XMLDECL:                      // <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
                case XML_ELEMENT_TYPE_EMPTY_TAG:                    // <br>
                default:
                {
                    // Ignore anything else
                    //printf("SVGGraphicsElement::loadFromXmlIterator ==> IGNORING kind(%d) name:", elem.kind());
                    //printChunk(elem.nameSpan());
                    //printChunk(elem.data());

                    //printf("SVGGraphicsElement::loadFromXmlIterator ==> IGNORING: %s\n", elem.name().c_str());
                    //printXmlElement(elem);
                }
                break;
                }


            } while (iter.next());

        }



        // Assuming we've already got a file mapped into memory, load the document
        bool constructFromChunk(const ByteSpan& srcChunk)
        {
            // create a memBuff from srcChunk
            // since we use memory references, we need
            // to keep the memory around for the duration of the 
            // document's life
            // BUGBUG - ideally, we should take this opportunity to expand
            // basic entities, and eliminate whitespace, if that's what we're doing
            // It will cause some performance slow down here, but simplify later
            // processing.

            fSourceMem.initFromSpan(srcChunk);
            //size_t sz = expandXmlEntities(srcChunk, fSourceMem.span());

            // Create the XML Iterator we're going to use to parse the document
            //XmlElementIterator iter(fSourceMem.span(), true);

            // The first pass builds the DOM
            loadFromCache();

            return true;
        }



        // createDOM
        // Create a new SVGDocument object
        // This document is not bound to a drawing context, so a lot of things are not going
        // to be resolved, particularly relative sizing, and fonts
        // But, tree visitors can be used to turn the DOM into something useful, like 
        // a graphics rendering tree.
        std::shared_ptr<SVGDOM> createShared(const ByteSpan& srcChunk)
        {
            // this MUST be done, or node registrations will not happen
            //auto sFactory = getFactory();

            auto doc = std::make_shared<SVGDOM>();

            
            if (!doc->loadFromChunk(srcChunk))
                return {};

            return doc;
        }





    };
}