#pragma once

#include <memory>
#include <unordered_map>
#include <functional>

#include "xmlscan.h"
#include "svgcss.h"
#include "membuff.h"



namespace waavs {
    //============================================================
    // SvgAttributeCollection
    // A collection of the attibutes found on an SvgElement
    //============================================================

    struct SvgAttributeCollection
    {
        std::unordered_map<ByteSpan, ByteSpan> fAttributes{};

        // Constructors
        SvgAttributeCollection() = default;
        SvgAttributeCollection(const SvgAttributeCollection& other)
            :fAttributes(other.fAttributes)
        {}

        SvgAttributeCollection(const ByteSpan& inChunk)
        {
            addAttributes(inChunk);
        }

        // IMPLEMENTATION
        
        // Return a const attribute collection
        const std::unordered_map<ByteSpan, ByteSpan>& attributes() const { return fAttributes; }

        // size()
        // Return number of attributes in collection
        size_t size() const { return fAttributes.size(); }

        // clear out the attribute collection
        virtual void clear() { fAttributes.clear(); }
        

        // setAttribute()
        // Set the named attribute to the given value
        // This is virtual in case whomever chooses to sub-class this
        // collection can intercept the setting of the attribute
        virtual void setAttribute(const ByteSpan& name, const ByteSpan& value)
        {
            fAttributes[name] = value;
        }

        
        //
        // addAttributes()
        // 
        // Given a chunk containing all the attribute values, 
        // separate them into key/value pairs, 
		// and add them to the collection.
        //
        bool addAttributes(const ByteSpan& inChunk)
        {
            ByteSpan src = inChunk;
            ByteSpan key{};
            ByteSpan value{};

            while (readNextKeyAttribute(src, key, value))
            {
                setAttribute(key, value);
            }

            return true;
        }

        // getAttribute()
        //
        // Fill in the value of the named span if it's found in the collection
        // return a value of 'true'
		// If it's not found, return a value of 'false', and leave the value unchanged
        //
        bool getAttribute(const ByteSpan& name, ByteSpan& out) const
        {
            auto it = fAttributes.find(name);
			if (it != fAttributes.end())
			{
				out = it->second;
				return true;
			}
            
			return false;
        }
        
        // getAttribute()
        // Return the named span.  If not found, return 
        // an empty span.
        // This is not the best, because you can't differentiate
        // between an attribute not found, and one that does exist
        // but it's value is actually blank
        ByteSpan getAttribute(const ByteSpan& name) const
        {
            ByteSpan aspan{};
            getAttribute(name, aspan);

			return aspan;
        }



    };
}


namespace waavs {
	struct IAmSvgNode : public XmlElement, public SvgAttributeCollection
	{
        IAmSvgNode() = default;
        IAmSvgNode(const IAmSvgNode& other)
			:XmlElement(other), SvgAttributeCollection(other)
		{}

		// IMPLEMENTATION
		virtual void clear() override
		{
			XmlElement::clear();
			SvgAttributeCollection::clear();
		}
	};
    
    struct IAmDocument
    {
        virtual std::shared_ptr<CSSStyleSheet> styleSheet() const = 0;
        virtual void styleSheet(std::shared_ptr<CSSStyleSheet> sheet) = 0;

        virtual void addIdNode(const ByteSpan& name, std::shared_ptr<IAmSvgNode> obj) = 0;
    };

    struct SVGNode : public IAmSvgNode
    {
        std::vector<std::shared_ptr<SVGNode>> fNodes{};
        
        // Default constructor
        SVGNode() = default;


        //
        // IMPLEMENTATION
        //
        
        bool addNode(std::shared_ptr < SVGNode > node, IAmDocument& doc)
        {
            if (node == nullptr)
                return false;
            
            fNodes.push_back(node);

            // If the node has an 'id' field, then add the node to our
            // definitions dictionary
            ByteSpan id{};
            if (node->getAttribute("id", id))
                doc.addIdNode(id, node);

            return true;
        }


        virtual void loadSelfClosingNode(XmlElementIterator& iter, IAmDocument& doc)
        {
            //printf("SVGNode::loadSelfClosingNode()[%s]\n", iter->name().c_str());
            printXmlElement(*iter);

			auto node = std::make_shared<SVGNode>();
			node->loadFromXmlElement(*iter, doc);
			addNode(node, doc);
        }

        virtual void loadContentNode(XmlElementIterator& iter, IAmDocument& doc)
        {
            //printf("SVGNode::loadContentNode\n");
            printXmlElement(*iter);
        }

        virtual void loadCDataNode(XmlElementIterator& iter, IAmDocument& doc)
        {
            //printf("SVGNode::loadCDataNode\n");
            printXmlElement(*iter);
        }

        virtual void loadComment(XmlElementIterator& iter, IAmDocument& doc)
        {
            //printf("SVGNode::loadComment\n");
            printXmlElement(*iter);
        }

        virtual void loadDocType(XmlElementIterator& iter, IAmDocument& doc)
        {
            //printf("SVGNode::loadDocType()");
            printXmlElement(*iter);
        }
        
        virtual void loadEndTag(XmlElementIterator& iter, IAmDocument& doc)
        {
            //printf("SVGNode::loadEndTag()[/%s]\n", iter->name().c_str());
			printXmlElement(*iter);
            
        }
        
        void loadProcessingInstruction(XmlElementIterator& iter, IAmDocument& doc)
        {
            //printf("loadProcessingInstruction()\n");
            printXmlElement(*iter);
        }
        
        virtual  void loadStartTag(XmlElementIterator& iter, IAmDocument& doc)
        {
			//printf("SVGNode::loadCompoundNode()[%s]\n", iter->name().c_str());
            printXmlElement(*iter);
            
            auto anode = std::make_shared<SVGNode>();
            anode->loadFromXmlIterator(iter, doc);
            addNode(anode, doc);
        }

        virtual void loadXmlDecl(XmlElementIterator& iter, IAmDocument& doc)
        {
            //printf("SVGNode::loadXmlDecl()[%s]\n", toString(iter->name()).c_str());
            //printXmlElement(*iter);
        }


        //
        // loadFromXmlElement
        // This is meant to be called to load the basic stuff, like
		// the attributes and the name of the element, as raw data
		// We then call addAttributes to parse the attributes
        // and make them accessible in a collection
        virtual void loadFromXmlElement(const XmlElement& elem, IAmDocument &doc)
        {
            // First assign
            XmlElement::operator=(elem);

            addAttributes(elem.data());
        }
        
        virtual void loadFromXmlIterator(XmlElementIterator& iter, IAmDocument &doc)
        {
            loadFromXmlElement(*iter, doc);


            while (iter.next())
            {
                //printf("===============================\n");
                //printXmlElement(*iter);
                //printf("-------------------------------\n");

                switch (iter->kind())
                {
                case XML_ELEMENT_TYPE_XMLDECL:
                {
                    loadXmlDecl(iter, doc);
                } break;

                case XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION:
                {
                    loadProcessingInstruction(iter, doc);
                } break;

                case XML_ELEMENT_TYPE_START_TAG:
                {
                    loadStartTag(iter, doc);
                } break;

                case XML_ELEMENT_TYPE_END_TAG:
                {
                    loadEndTag(iter, doc);
                } break;

                case XML_ELEMENT_TYPE_CONTENT:
                {
                    loadContentNode(iter, doc);
                } break;

                case XML_ELEMENT_TYPE_CDATA:
                {
                    loadCDataNode(iter, doc);
                } break;

                case XML_ELEMENT_TYPE_COMMENT:
                {
                    loadComment(iter, doc);
                } break;

                case XML_ELEMENT_TYPE_DOCTYPE:
                {
                    loadDocType(iter, doc);
                } break;

                case XML_ELEMENT_TYPE_SELF_CLOSING:
				{
					loadSelfClosingNode(iter, doc);
				} break;
                
                default:
                {
                    printf("SVGDOMDocument::loadFromXmlIterator: UNKNOWN Type: [%d]\n", iter->kind());
                }
                }
            }
        }

      

    };


    //
    // SVGDOMDocument
    // 
    // A simple DOM representation of the parsed XML.
    // This document contains the various referred objects
    // and other definitions, that might be needed to resolve for rendering
    //
    // Note:  The following are things used for rendering
    //
    // IAmGroot - Implementation
    //
    //virtual FontHandler* fontHandler() const = 0;
    //virtual double canvasWidth() const = 0;
    //virtual double canvasHeight() const = 0;
    //virtual double dpi() const = 0;
    
    struct SVGDOMDocument : public SVGNode, public IAmDocument
    {
        // Memory to hold a copy of the source chunk
        MemBuff fDocMem{};
        
        // Style sheet to be used by entire document
        std::shared_ptr<CSSStyleSheet> fStyleSheet = nullptr;
        
        // Things to be looked up by ID
        std::unordered_map<ByteSpan, std::shared_ptr<IAmSvgNode>> fDefinitions{};

        
        SVGDOMDocument() 
        {
            fStyleSheet = std::make_shared<CSSStyleSheet>();
        }


        virtual std::string systemLanguage() { return "en"; } // BUGBUG - What a big cheat!!

        std::shared_ptr<CSSStyleSheet> styleSheet() const override { return fStyleSheet; }
        void styleSheet(std::shared_ptr<CSSStyleSheet> sheet) override { fStyleSheet = sheet; }

        
        void addIdNode(const ByteSpan& id, std::shared_ptr<IAmSvgNode> node)
        {
            fDefinitions[id] = node;
        }
        
        std::shared_ptr<IAmSvgNode> getElementById(const ByteSpan& name)
        {
            if (fDefinitions.find(name) != fDefinitions.end())
                return fDefinitions[name];

            //printf("SVGDocument::getElementById, FAIL: %s\n", name.c_str());

            return nullptr;
        }

        // Load a URL Reference
        std::shared_ptr<IAmSvgNode> getElementByHref(const ByteSpan& inChunk)
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

        // Load a URL reference, including the 'url(' function indicator
        std::shared_ptr<IAmSvgNode> getElementByUrl(const ByteSpan& inChunk)
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

            return getElementByHref(id);
        }

        

        // Overrides of node construction
        void loadXmlDecl(XmlElementIterator& iter, IAmDocument& doc) override
        {
            printf("SVGDOMDocument::loadXmlDecl()[%s]\n", (iter->name());
            printXmlElement(*iter);
            addAttributes(iter->data());
        }


        bool loadFromChunk(const ByteSpan& srcSpan)
        {
            // We make a copy here, because we're going 
            // to be handing out various ByteSpans, so we need
            // to ensure the lifetime of the span.
            fDocMem.initFromSpan(srcSpan);

            // create an xmliterator on the fDocMem span
			XmlElementIterator iter(fDocMem.span());

			// Load the document
			loadFromXmlIterator(iter, *this);

            return true;
        }

        // Factory constructor
        static std::shared_ptr<SVGDOMDocument> createFromChunk(const ByteSpan& inChunk)
        {
			std::shared_ptr<SVGDOMDocument> doc = std::make_shared<SVGDOMDocument>();
			doc->loadFromChunk(inChunk);
			return doc;
        }
    };
}
