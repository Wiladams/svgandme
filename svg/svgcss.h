#pragma once



#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <iterator>

#include "xmlscan.h"

// Core data structures and types to support CSS parsing

namespace waavs
{
	static charset cssstartnamechar = chrAlphaChars + "_";
    static charset cssnamechar = cssstartnamechar + chrDecDigits + '-';
    

    // CSS Syntax
    // selector {property:value; property:value; ...}
    // 
    enum CSSSelectorKind : uint32_t
    {
		CSS_SELECTOR_INVALID = 0,
        CSS_SELECTOR_ELEMENT,               // All elements with given name - e.g. "rect"
        CSS_SELECTOR_ID,                 // Element with given id - e.g. "#myid"
        CSS_SELECTOR_CLASS,              // Elements with given class - e.g. ".myclass"
		CSS_SELECTOR_ATRULE,             // At-rule - e.g. "@font-face"
        CSS_SELECTOR_ATTRIBUTE,          // Elements with given attribute - e.g. "[myattr]"
        CSS_SELECTOR_PSEUDO_CLASS,       // Elements with given pseudo-class - e.g. ":hover"
        CSS_SELECTOR_PSEUDO_ELEMENT,     // Elements with given pseudo-element - e.g. "::first-line"
        CSS_SELECTOR_COMBINATOR,         // Combinator - e.g. "E F"
        CSS_SELECTOR_UNIVERSAL,          // Universal selector - e.g. "*"
	};
    
    struct CSSSelectorInfo
    {
        uint32_t fKind{ CSS_SELECTOR_INVALID };
        ByteSpan fName{};
        ByteSpan fData{};

        CSSSelectorInfo() = default;
        CSSSelectorInfo(uint32_t akind, const ByteSpan& aname, const ByteSpan& adata)
            :fKind(akind), fName(aname), fData(adata)
        {
        }

        CSSSelectorInfo& operator=(const CSSSelectorInfo& other)
        {
            if (this != &other)
            {
                fKind = other.fKind;
                fName = other.fName;
                fData = other.fData;
            }
            return *this;
        }

        void reset()
        {
            fKind = CSS_SELECTOR_INVALID;
            fName = {};
            fData = {};
        }

		void reset(uint32_t kind, const ByteSpan& name, const ByteSpan& data)
		{
			fKind = kind;
			fName = name;
			fData = data;
		}

        bool empty() const noexcept { return fKind == CSS_SELECTOR_INVALID; }
        explicit operator bool() const noexcept { return !empty(); }

        constexpr uint32_t kind() const noexcept { return fKind; }
        constexpr const ByteSpan& name() const noexcept { return fName; }
        constexpr const ByteSpan& data() const noexcept { return fData; }

    };
    

    // Look at the beginning of the selector name and determine
    // what kind of simple selector it is.
    static CSSSelectorKind parseSimpleSelectorKind(const waavs::ByteSpan& inChunk)
    {
        if (*inChunk == '.')
            return CSSSelectorKind::CSS_SELECTOR_CLASS;         // Select a particular class
        else if (*inChunk == '#')
            return CSSSelectorKind::CSS_SELECTOR_ID;            // Select elements with the given ID
        else if (*inChunk == '@')
            return CSSSelectorKind::CSS_SELECTOR_ATRULE;        // Animation selector
        else if (*inChunk == '[')
			return CSSSelectorKind::CSS_SELECTOR_ATTRIBUTE;     // Select elements with the given attribute
        else if (*inChunk == ':')
			return CSSSelectorKind::CSS_SELECTOR_PSEUDO_CLASS;  // Select elements with the given pseudo-class
        else if (*inChunk == '*')
			return CSSSelectorKind::CSS_SELECTOR_UNIVERSAL;     // Select all elements
        else if (*inChunk == ',')
			return CSSSelectorKind::CSS_SELECTOR_COMBINATOR;    // Combinator
        else if (chrAlphaChars[*inChunk])
			return CSSSelectorKind::CSS_SELECTOR_ELEMENT;       // Select elements with the given name
        else
            return CSSSelectorKind::CSS_SELECTOR_INVALID;
    }
    
    static bool gatherCssAttributes(const ByteSpan& inChunk, XmlAttributeCollection &attributes)
    {
        waavs::ByteSpan s = inChunk;
        while (s.size() > 0)
        {
            // get the name of the attribute
            auto prop = chunk_token(s, charset(':'));
            prop = chunk_trim(prop, chrWspChars);

            // get the value of the attribute
            auto value = chunk_token(s, charset(';'));
            value = chunk_trim(value, chrWspChars);

            attributes.addValueBySpan(prop, value);
        }

        return true;
    }
    
    //======================================================
    // CSSSelector
    // 
	// Holds onto a single CSS selector, which has a map of
    // attribute name/value pairs.
	// This is a simple selector, not a complex selector, so
	// it can be used on its own, but also act as a building
    // block for more complex selectors, and style sheets
    //======================================================

    struct CSSSelector
    {
        using MatchFunction = std::function<bool(const XmlElement&)>;

    private:
        uint32_t fKind{ CSS_SELECTOR_INVALID };
        ByteSpan fName{};
        ByteSpan fData{};
        XmlAttributeCollection fAttributes{};
        MatchFunction fMatchFunction{};

    public:
        CSSSelector() = default;

        CSSSelector(uint32_t kind, const ByteSpan& name, const ByteSpan& data, MatchFunction matchFn)
            : fKind(kind), fName(name), fData(data), fMatchFunction(std::move(matchFn))
        {
            loadFromChunk(data);
        }

        bool matches(const XmlElement& element) const noexcept
        {
            return fMatchFunction ? fMatchFunction(element) : false;
        }

        uint32_t kind() const noexcept { return fKind; }
        const ByteSpan& name() const noexcept { return fName; }
        ByteSpan data() const noexcept { return fData; }
        const XmlAttributeCollection& attributes() const noexcept { return fAttributes; }

        CSSSelector& mergeProperties(const CSSSelector& other)
        {
            fAttributes.mergeAttributes(other.fAttributes);

            return *this;
        }

    private:
        void loadFromChunk(const ByteSpan& inChunk) noexcept
        {
            fData = inChunk;
            gatherCssAttributes(inChunk, fAttributes);
        }
    };


 

    // CSSSelectorIterator
    // 
	// Given a whole style sheet, iterate over the selectors within that sheet
	// Individual selectors are indicated by <selector>[, <selector>]* { <properties> }
    // 
    // That is, there can be multiple selector names before the property list
    // each one of them must be iterated separately. We deliver each selector name
    // with the set of properties, in the order in which they were originally listed.
    //
    // This iterator can deal with embedded style sheet comments, which are either
	// C++ single line comments '//', or C style multi-line comments '/* ... */'
    //
    struct CSSSelectorIterator
    {
        using difference_type = std::ptrdiff_t;
        using value_type = CSSSelectorInfo;
        using pointer = const CSSSelectorInfo*;
        using reference = const CSSSelectorInfo&;
        using iterator_category = std::forward_iterator_tag;


        ByteSpan fSource{};
        
        ByteSpan fSelectorNames{};
        ByteSpan fSelectorContent{};

        CSSSelectorInfo fCurrentItem{};
        ByteSpan fSentinel{};    // Marks the beginning of the current item


        CSSSelectorIterator(const ByteSpan& inChunk)
            :fSource(inChunk)
        {
            // we need to be positioned on the first item to start
            next();
        }

        //explicit operator bool() const { return (bool)fCurrentItem; }

        // Queue up the next selection, skipping past comments
        // and whatnot
        bool advanceSelection()
        {
            // Skip whitespace
            fSource = chunk_ltrim(fSource, chrWspChars);
            fSentinel = fSource;

            if (!fSource)
                return false;
            
            // skip 'C' style single line, and multi-line comments
            // we do this in a loop, because there can be multiple
            // comment blocks before we get to actual content
            while (fSource)
            {
                if (fSource.startsWith("/*"))
                {
                    // Skip past /* comment */
                    fSource += 2;
                    fSource = chunk_skip_until_cstr(fSource, "*/");
                    if (fSource.startsWith("*/"))
                        fSource += 2;
                    continue;
                }
                else if (fSource.startsWith("//"))
                {
                    // Skip past // comment
                    fSource = chunk_find_char(fSource, '\n');
                    continue;
                }

                // separate out the select name list from the content
				fSelectorNames = chunk_token_char(fSource, '{');
                fSelectorNames = chunk_trim(fSelectorNames, chrWspChars);

                if (!fSelectorNames)
                    return false;

                // Isolate the content portion
                fSelectorContent = chunk_token_char(fSource, '}');
                fSelectorContent = chunk_trim(fSelectorContent, chrWspChars);

                break;
            }

            return true;
        }
        
        bool next()
        {
			if (!fSelectorNames)
			{
				if (!advanceSelection())
					return false;
			}
            
            fCurrentItem.reset();

            // pull off the next name delimeted by a comma
            ByteSpan selectorName = chunk_token(fSelectorNames, ",");
			fSelectorNames = chunk_trim(fSelectorNames, chrWspChars);
            
            // determine what kind of selector we have
            auto selectorKind = parseSimpleSelectorKind(selectorName);
            
            if (selectorKind != CSSSelectorKind::CSS_SELECTOR_INVALID) {
                if (selectorKind != CSSSelectorKind::CSS_SELECTOR_ELEMENT)
                {
                    selectorName++; // skip the first character of the name
                }
                
                fCurrentItem.reset(selectorKind, selectorName, fSelectorContent);
                return true;
            }

            return false;
        }

        bool operator==(const CSSSelectorIterator& other) const noexcept 
        { 
            return fSentinel.fStart == other.fSource.fStart;
        }
        bool operator!=(const CSSSelectorIterator& other) const noexcept 
        { 
            return !(*this == other);
        }

        // For iteration convenience
        const CSSSelectorInfo& operator*()  { return fCurrentItem; }
		const CSSSelectorInfo* operator->() { return &fCurrentItem; }

        CSSSelectorIterator &operator++() 
        {  
            next(); 
            return *this; 
        }

        CSSSelectorIterator operator++(int)
        {
            CSSSelectorIterator temp = *this;  // Copy current state
            next();
            return temp;  // Return previous state
        }
    };

    struct CSSSelectorContainer
    {
    private:
        ByteSpan fSource;  // Stores the source CSS text for iteration

    public:
        explicit CSSSelectorContainer(const ByteSpan& cssData)
            : fSource(cssData) {
        }

        // Return an iterator to the beginning
        CSSSelectorIterator begin() const
        {
            return CSSSelectorIterator(fSource);
        }

        // Return an iterator representing the end (empty iterator)
        CSSSelectorIterator end() const
        {
            return CSSSelectorIterator(ByteSpan(fSource.fEnd, fSource.fEnd));
        }
    };


	//======================================================
	// CSSStyleSheet
	//
	// This class represents a CSS style sheet
    //======================================================
    struct CSSStyleSheet
    {
        using SelectorMap = std::unordered_map<ByteSpan, CSSSelector, ByteSpanHash, ByteSpanEquivalent>;

        std::unordered_map<uint32_t, SelectorMap> selectors;


        CSSStyleSheet()
        {
            reset();
        }

        //CSSStyleSheet(const waavs::ByteSpan& inSpan)
        //{
        //    reset();
        //    loadFromSpan(inSpan);
        //}
        

        void reset()
        {
            selectors.clear();
        }

		SelectorMap & getSelectorMap(uint32_t kind)
		{
			auto it = selectors.find(kind);
            if (it != selectors.end())
            {
                return it->second;
            }

            selectors[kind] = {};
			return selectors[kind];
		}

        CSSSelector* getSelector(CSSSelectorKind kind, const ByteSpan& name)
        {
            auto& aMap = getSelectorMap(kind);

            auto selIt = aMap.find(name);
            if (selIt != aMap.end())
                return &selIt->second;

            return nullptr;
        }

 

        // Add a selector to the style sheet
        // based on the info.name(), put the selector into
        // the proper selector category
        void addSelector(const CSSSelectorInfo& info)
        {
            if (info.empty())
                return;

            // Determine predicate dynamically
            std::function<bool(const XmlElement&)> predicate;
            switch (info.kind())
            {
            case CSS_SELECTOR_ID:
                predicate = [info](const XmlElement& elem) {
                    ByteSpan idValue;
                    return elem.getRawAttributeValue("id", idValue) && idValue == info.name();
                    };
                break;

            case CSS_SELECTOR_CLASS:
                predicate = [info](const XmlElement& elem) {
                    ByteSpan classValue{};
                    ByteSpan nameValue{};
                    return elem.getRawAttributeValue("class", classValue) && chunk_find(classValue,info.name(), nameValue);
                    };
                break;

            case CSS_SELECTOR_ELEMENT:
                predicate = [info](const XmlElement& elem) {
                    return elem.name() == info.name();
                    };
                break;

            case CSS_SELECTOR_ATTRIBUTE:
                predicate = [info](const XmlElement& elem) {
                    ByteSpan attrValue;
                    return elem.getRawAttributeValue(info.name(), attrValue);
                    };
                break;

            default:
                return;
            }

            // Construct selector with the predicate
            CSSSelector newSelector(info.kind(), info.name(), info.data(), predicate);

            //selectors[info.kind()][info.name()] = newSelector;

            // Get correct selector map and add/merge
            auto& selectorMap = getSelectorMap(info.kind());
            auto it = selectorMap.find(info.name());
            if (it != selectorMap.end())
            {
                it->second.mergeProperties(newSelector);
            }
            else
            {
                selectorMap.emplace(info.name(), std::move(newSelector));
            }
        }



        bool loadFromSpan(const ByteSpan& inSpan)
        {
			CSSSelectorContainer selContainer(inSpan);

            // Iterate over the selectors
			for (const CSSSelectorInfo & selInfo : selContainer)
			{
				addSelector(selInfo);
			}

            return true;
        }
    };
}
