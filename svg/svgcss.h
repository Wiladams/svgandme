#pragma once



#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <iterator>


#include "svgatoms.h"
#include "xml_attribute_collection.h"

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
        if (!inChunk)
            return CSSSelectorKind::CSS_SELECTOR_INVALID;

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
    
    static INLINE bool css_read_property_value(ByteSpan& src, ByteSpan& out) noexcept
    {
        out.reset();

        bspan_skip_spaces(src);

        const uint8_t* start = src.begin();
        const uint8_t* p = start;
        const uint8_t* end = src.end();

        int parenDepth = 0;
        int bracketDepth = 0;
        int braceDepth = 0;

        while (p < end)
        {
            uint8_t ch = *p;

            if (ch == '"' || ch == '\'')
            {
                uint8_t quote = ch;
                ++p;

                while (p < end)
                {
                    if (*p == '\\')
                    {
                        ++p;
                        if (p < end)
                            ++p;
                        continue;
                    }

                    if (*p == quote)
                    {
                        ++p;
                        break;
                    }

                    ++p;
                }

                continue;
            }

            if (ch == '(')
            {
                ++parenDepth;
                ++p;
                continue;
            }

            if (ch == ')' && parenDepth > 0)
            {
                --parenDepth;
                ++p;
                continue;
            }

            if (ch == '[')
            {
                ++bracketDepth;
                ++p;
                continue;
            }

            if (ch == ']' && bracketDepth > 0)
            {
                --bracketDepth;
                ++p;
                continue;
            }

            if (ch == '{')
            {
                ++braceDepth;
                ++p;
                continue;
            }

            if (ch == '}' && braceDepth > 0)
            {
                --braceDepth;
                ++p;
                continue;
            }

            if (ch == ';' &&
                parenDepth == 0 &&
                bracketDepth == 0 &&
                braceDepth == 0)
            {
                out.resetPointers(start, p);
                bspan_trim(out, chrWspChars);
                src.resetStart(p + 1);
                return true;
            }

            ++p;
        }

        out.resetPointers(start, end);
        bspan_trim(out, chrWspChars);
        src.resetStart(end);

        return out.size() > 0;
    }


    static bool gatherCssAttributes(const ByteSpan& inChunk, XmlAttributeCollection& attributes)
    {
        ByteSpan s = inChunk;

        while (s)
        {
            bspan_skip_spaces(s);

            if (!s)
                break;

            ByteSpan prop{};
            if (!identifier_read(s, prop, cssstartnamechar, cssnamechar))
                return false;

            bspan_skip_spaces(s);

            if (!s || *s != ':')
                return false;

            ++s;

            ByteSpan value{};
            if (!css_read_property_value(s, value))
                return false;

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

        CSSSelector& mergePropertiesFromChunk(const ByteSpan& data)
        {
            XmlAttributeCollection attrs;
            gatherCssAttributes(data, attrs);
            fAttributes.mergeAttributes(attrs);
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
        /*
        static INLINE ByteSpan css_skip_block_comment(ByteSpan src) noexcept
        {
            // src is positioned after the initial /*
            const uint8_t* cur = src.begin();
            const uint8_t* end = src.end();

            while (cur < end)
            {
                cur = static_cast<const uint8_t*>(
                    std::memchr(cur, '*', static_cast<size_t>(end - cur)));

                if (!cur)
                    return ByteSpan::fromPointers(end, end);

                if ((cur + 1) < end && cur[1] == '/')
                    return ByteSpan::fromPointers(cur + 2, end);

                ++cur;
            }

            return ByteSpan::fromPointers(end, end);
        }
        */

        bool advanceSelection()
        {
            // Skip whitespace
            bspan_ltrim_spaces(fSource);
            fSentinel = fSource;
            
            // skip 'C' style single line, and multi-line comments
            // we do this in a loop, because there can be multiple
            // comment blocks before we get to actual content
            while (fSource)
            {
                bspan_ltrim_spaces(fSource);

                if (!fSource)
                    return false;

                if (bspan_starts_with(fSource, "/*"))
                {
                    // Skip past /* comment */
                    fSource.advance(2);
                    ByteSpan found;
                    if (bspan_find_span(fSource, "*/", found))
                    {
                        fSource.resetStart(found.end());
                    }
                    else
                    {
                        fSource.advanceToEnd();
                    }
                    continue;

                }
                
                if (bspan_starts_with(fSource, "//"))
                {
                    fSource.advance(2);
                    fSelectorContent = {};
                    ByteSpan comment = bspan_read_until(fSource, '\n');
                    (void)comment;  // don't care about output

                    // Skip past // comment
                    //fSource = chunk_find_char(fSource, '\n');
                    continue;
                }

                // separate out the select name list from the content
				fSelectorNames = bspan_read_until(fSource, '{');
                bspan_trim_spaces(fSelectorNames);

                if (!fSelectorNames)
                    return false;

                // Isolate the content portion
                fSelectorContent = bspan_read_until(fSource, '}');
                bspan_trim_spaces(fSelectorContent);

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
            ByteSpan selectorName = bspan_read_until(fSelectorNames, ",");
			bspan_trim(fSelectorNames, chrWspChars);
            
            // determine what kind of selector we have
            auto selectorKind = parseSimpleSelectorKind(selectorName);
            
            if (selectorKind != CSSSelectorKind::CSS_SELECTOR_INVALID) {
                if (selectorKind != CSSSelectorKind::CSS_SELECTOR_ELEMENT)
                {
                    ++selectorName; // skip the first character of the name
                }
                
                fCurrentItem.reset(selectorKind, selectorName, fSelectorContent);
                return true;
            }

            return false;
        }

        bool operator==(const CSSSelectorIterator& other) const noexcept 
        { 
            return fSentinel.begin() == other.fSource.begin();
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
            return CSSSelectorIterator({ fSource.end(), fSource.end() });
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
			//auto it = selectors.find(kind);
            //if (it != selectors.end())
            //{
            //    return it->second;
            //}
            //selectors[kind] = {};
            //return selectors[kind];

            return selectors.try_emplace(kind).first->second;
		}

        CSSSelector* getSelector(CSSSelectorKind kind, const ByteSpan& name)
        {
            auto& aMap = getSelectorMap(kind);

            auto selIt = aMap.find(name);
            if (selIt != aMap.end())
                return &selIt->second;

            return nullptr;
        }

 
        static INLINE bool css_class_list_contains(ByteSpan classList, const ByteSpan& name) noexcept
        {
            while (classList)
            {
                bspan_skip_spaces(classList);

                if (!classList)
                    break;

                ByteSpan item = bspan_read_while(classList, charset(" \t\r\n"));

                if (item == name)
                    return true;
            }

            return false;
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
                    return elem.getElementAttribute("id", idValue) && idValue == info.name();
                    };
                break;

            case CSS_SELECTOR_CLASS:
                predicate = [info](const XmlElement& elem) {
                    ByteSpan classValue{};
                    return elem.getElementAttribute("class", classValue) &&
                        css_class_list_contains(classValue, info.name());
                    };
                break;

            case CSS_SELECTOR_ELEMENT:
                predicate = [info](const XmlElement& elem) {
                    return info.name() == elem.nameAtom();
                    };
                break;

            case CSS_SELECTOR_ATTRIBUTE:
                predicate = [info](const XmlElement& elem) {
                    ByteSpan attrValue;
                    return elem.getElementAttribute(info.name(), attrValue);
                    };
                break;

            default:
                return;
            }

            // Construct selector with the predicate
            //CSSSelector newSelector(info.kind(), info.name(), info.data(), predicate);


            // Get correct selector map and add/merge
            auto& selectorMap = getSelectorMap(info.kind());
            auto it = selectorMap.find(info.name());
            if (it != selectorMap.end())
            {
                it->second.mergePropertiesFromChunk(info.data());
                //it->second.mergeProperties(newSelector);
            }
            else
            {
                selectorMap.try_emplace(
                    info.name(),
                    info.kind(),
                    info.name(),
                    info.data(),
                    std::move(predicate));

                //selectorMap.emplace(info.name(), std::move(newSelector));
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
