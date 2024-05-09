#pragma once

#include "bspan.h"
#include "bspanutil.h"

#include <map>
#include <unordered_map>
#include <vector>
#include <string>

// Core data structures and types to support CSS parsing

namespace waavs
{
    static charset cssdigit("0123456789");
    static charset csswsp(" \t\r\n\f\v");
	static charset cssalpha("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
	static charset cssstartnamechar = cssalpha + "_";
    static charset cssnamechar = cssstartnamechar + cssdigit + '-';
    

    // CSS Syntax
    // selector {property:value; property:value; ...}
    // 
    enum class CSSSelectorKind
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
    
    static std::string cssSelectorKindToString(CSSSelectorKind kind)
    {
		switch (kind)
		{
		case CSSSelectorKind::CSS_SELECTOR_ELEMENT: return "ELEMENT";
		case CSSSelectorKind::CSS_SELECTOR_ID: return "ID";
		case CSSSelectorKind::CSS_SELECTOR_CLASS: return "CLASS";
		case CSSSelectorKind::CSS_SELECTOR_ATRULE: return "ATRULE";
		case CSSSelectorKind::CSS_SELECTOR_ATTRIBUTE: return "ATTRIBUTE";
		case CSSSelectorKind::CSS_SELECTOR_PSEUDO_CLASS: return "PSEUDO_CLASS";
		case CSSSelectorKind::CSS_SELECTOR_PSEUDO_ELEMENT: return "PSEUDO_ELEMENT";
		case CSSSelectorKind::CSS_SELECTOR_COMBINATOR: return "COMBINATOR";
		case CSSSelectorKind::CSS_SELECTOR_UNIVERSAL: return "UNIVERSAL";
		default: return "INVALID";
		}
    }

    
    // Look at the beginning of the selector name and determine
    // what kind of simple selector it is.
    static CSSSelectorKind parseSimpleSelectorKind(const waavs::ByteSpan& inChunk)
    {
        if (*inChunk == '.')
            return CSSSelectorKind::CSS_SELECTOR_CLASS;
        else if (*inChunk == '#')
            return CSSSelectorKind::CSS_SELECTOR_ID;
        else if (*inChunk == '@')
            return CSSSelectorKind::CSS_SELECTOR_ATRULE;
        else if (*inChunk == '[')
            return CSSSelectorKind::CSS_SELECTOR_ATTRIBUTE;
        else if (*inChunk == ':')
            return CSSSelectorKind::CSS_SELECTOR_PSEUDO_CLASS;
        else if (*inChunk == '*')
            return CSSSelectorKind::CSS_SELECTOR_UNIVERSAL;
        else if (*inChunk == ',')
            return CSSSelectorKind::CSS_SELECTOR_COMBINATOR;
        else if (cssalpha[*inChunk])
            return CSSSelectorKind::CSS_SELECTOR_ELEMENT;
        else
            return CSSSelectorKind::CSS_SELECTOR_INVALID;
    }
    
    static bool gatherCssAttributes(const ByteSpan& inChunk, XmlAttributeCollection &attributes)
    {
        waavs::ByteSpan s = inChunk;
        while (s.size() > 0)
        {
            // get the name of the attribute
            auto prop = waavs::nextToken(s, charset(':'));
            prop = chunk_trim(prop, csswsp);

            // get the value of the attribute
            auto value = waavs::nextToken(s, charset(';'));
            value = chunk_trim(value, csswsp);

            // add the attribute to the map
			auto name = std::string(prop.begin(), prop.end());
            attributes.addAttribute(name, value);
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
        CSSSelectorKind fKind{ CSSSelectorKind::CSS_SELECTOR_INVALID };
        ByteSpan fData{};
        XmlAttributeCollection fAttributes{};
        std::string fName{};
        
        
        CSSSelector() = default;
        CSSSelector(const CSSSelector &other)
            :fKind(other.fKind)
			, fName(other.fName)
            , fData(other.fData)
			, fAttributes(other.fAttributes)
		{
		}
        
        CSSSelector(CSSSelectorKind kind, std::string& name, const waavs::ByteSpan& inChunk)
            :fKind(kind),
            fName(name),
            fData(inChunk)
        {
			//fKind = kind;
            //fName = name;
            //fData = inChunk;
            
            loadFromChunk(inChunk);
        }

        CSSSelectorKind kind() const { return fKind; }
        const std::string& name() const { return fName; }
		ByteSpan data() const { return fData; }
        
        const XmlAttributeCollection& attributes() const { return fAttributes; }


        explicit operator bool() const { return fKind!= CSSSelectorKind::CSS_SELECTOR_INVALID && !fName.empty() && fAttributes.size() > 0; }
        
        
		CSSSelector& operator=(const CSSSelector& other)
		{
			fKind = other.fKind;
			fName = other.fName;
			fAttributes = other.fAttributes;
			fData = other.fData;
            
			return *this;
		}
        
        
        // When adding, we don't care whether the kinds match
        // we just need to copy over the attributes from the other
        // to ours, replacing any that are already there
		CSSSelector& mergeProperties(const CSSSelector& other)
		{
			fAttributes.mergeProperties(other.fAttributes);

			return *this;
		}
        
		//CSSSelector& operator+= (const CSSSelector& other)
		//{
        //    return mergeProperties(other);
        //}
         
        //CSSSelector operator+(const CSSSelector &rhs)
		//{
		//	CSSSelector result(*this);
        //    result.appendSelector(rhs);
		//}
        
		// Load the attributes from the chunk
		void loadFromChunk(const waavs::ByteSpan& inChunk)
		{
            fData = inChunk;
			gatherCssAttributes(inChunk, fAttributes);
		}
        
        waavs::ByteSpan getAttribute(const std::string &&name)
        {
            return fAttributes.getAttribute(name);
            
        }
    };

    //======================================================
    // CSSInlineStyleIterator
    // 
    // This iterator is used to iterate over the inline style attributes of an element
    // or the content body of a selector
    // Each iteration returns a CSS property/value pair as a std::pair
    //======================================================
    struct CSSInlineStyleIterator {
        waavs::ByteSpan fSource{};
        waavs::ByteSpan fCurrentName{};
        waavs::ByteSpan fCurrentValue{};

		CSSInlineStyleIterator() = default;
        CSSInlineStyleIterator(const CSSInlineStyleIterator& other)
            :fSource(other.fSource)
            , fCurrentName(other.fCurrentName)
            , fCurrentValue(other.fCurrentValue)
        {}
        CSSInlineStyleIterator(const waavs::ByteSpan& inChunk) 
            : fSource(inChunk)
        {
            next();
        }

		CSSInlineStyleIterator& operator=(const CSSInlineStyleIterator& other)
		{
            fSource = other.fSource;
			fCurrentName = other.fCurrentName;
			fCurrentValue = other.fCurrentValue;
			return *this;
		}
        
        bool next()
        {
            fSource = chunk_trim(fSource, csswsp);
            fCurrentName = {};
            fCurrentValue = {};

            if (fSource)
            {
				fCurrentName = nextToken(fSource, charset(":"));
				fCurrentName = chunk_trim(fCurrentName, csswsp);
                
				fCurrentValue = nextToken(fSource, charset(";"));
                fCurrentValue = chunk_trim(fCurrentValue, csswsp);

                return (bool)fCurrentName && (bool)fCurrentValue;
            }
            else
                return false;
        }

        explicit operator bool() const { return fCurrentName && fCurrentValue; }
        
        auto operator*() { return std::make_pair(fCurrentName, fCurrentValue); }
        CSSInlineStyleIterator& operator++() { next(); return *this; }
		CSSInlineStyleIterator& operator++(int i) { next(); return *this; }
    };



	// Given a whole style sheet, iterate over the selectors
	// individual selectors are indicated by <selector> { <properties> }
    struct CSSSelectorIterator
    {
        ByteSpan fSource{};
        ByteSpan fMark{};
        CSSSelector fCurrentItem{};

        CSSSelectorIterator(const ByteSpan& inChunk)
            :fSource(inChunk),
			fMark(inChunk)
        {
            next();
        }

        explicit operator bool() const { return (bool)fCurrentItem; }

        bool next()
        {
            fSource = chunk_ltrim(fSource, csswsp);

            fCurrentItem = CSSSelector();

			// BUGBUG - somewhere in here we need to remove the comments
            while (fSource)
            {
                // Look for the next selector, which should be a string
                // followed by a '{', with optional whitespace in between
                // terminated with a '}'
                ByteSpan selectorChunk = chunk_token(fSource, "{");
                selectorChunk = chunk_trim(selectorChunk, csswsp);

                if (selectorChunk)
                {
                    // fSource is positioned right after the opening '{', so we can
                    // look for the closing '}', and then trim the whitespace
                    // BUGBUG - need to do a lot more work to create selector list
                    auto selectorKind = parseSimpleSelectorKind(selectorChunk);

                    // If it's not element, trim the first character to get the raw name
                    if (selectorKind != CSSSelectorKind::CSS_SELECTOR_ELEMENT)
                    {
                        // skip the selector kind
                        selectorChunk = chunk_skip(selectorChunk, 1);
                    }
                    std::string selectorName(selectorChunk.begin(), selectorChunk.end());

                    ByteSpan content = nextToken(fSource, "}");
                    content = chunk_trim(content, csswsp);

                    if (selectorKind != CSSSelectorKind::CSS_SELECTOR_INVALID) {
                        fCurrentItem = CSSSelector(selectorKind, selectorName, content);
                        return true;
                    }
                }
            }

            return false;
        }

        // For iteration convenience
        const CSSSelector& operator*()  { return fCurrentItem; }
        CSSSelectorIterator& operator++() {  next(); return *this; }
        CSSSelectorIterator& operator++(int)  {  next();  return *this;  }
    };


	//======================================================
	// CSSStyleSheet
	//
	// This class represents a CSS style sheet
    //======================================================
    struct CSSStyleSheet
    {
        waavs::ByteSpan fSource{};

        std::map<std::string, std::shared_ptr<CSSSelector>> fIDSelectors{};
        std::map<std::string, std::shared_ptr<CSSSelector>> fClassSelectors{};
        std::map<std::string, std::shared_ptr<CSSSelector>> fElementSelectors{};
        //std::vector<std::shared_ptr<CSSSelector>> fUniversalSelectors{};

        CSSStyleSheet() = default;

        CSSStyleSheet(const waavs::ByteSpan& inSpan)
        {
            loadFromSpan(inSpan);
        }
        
        std::shared_ptr<CSSSelector> getSelector(const std::string& name, CSSSelectorKind kind)
        {
            
			switch (kind)
			{
			case CSSSelectorKind::CSS_SELECTOR_ID:
                if (fIDSelectors.find(name) != fIDSelectors.end())
                    return fIDSelectors.at(name);
                else
                    return nullptr;
			case CSSSelectorKind::CSS_SELECTOR_CLASS:
				if (fClassSelectors.find(name) != fClassSelectors.end())
					return fClassSelectors.at(name);
				else
					return nullptr;
			case CSSSelectorKind::CSS_SELECTOR_ELEMENT:
                if (fElementSelectors.find(name) != fElementSelectors.end())
                    return fElementSelectors.at(name);
                else
                    return nullptr;
                
			default:
				return nullptr;
			}
        }


		std::shared_ptr<CSSSelector> getIDSelector(const std::string& name)
        { 
            return getSelector(name, CSSSelectorKind::CSS_SELECTOR_ID);
        }
        
        std::shared_ptr<CSSSelector> getElementSelector(const std::string& name)
		{
            return getSelector(name, CSSSelectorKind::CSS_SELECTOR_ELEMENT);
		}
        
        std::shared_ptr<CSSSelector> getClassSelector(const std::string& name)
		{
            return getSelector(name, CSSSelectorKind::CSS_SELECTOR_CLASS);
		}
        
        
		void addSelectorToMap(std::map<std::string, std::shared_ptr<CSSSelector>> &amap, std::shared_ptr<CSSSelector> selector)
		{
			if (amap.contains(selector->name()))
			{
				// If the selector already exists, we need to merge the properties
				// into the existing selector
				auto & existing = amap[selector->name()];
				existing->mergeProperties(*selector);
			}
			else
			{
				amap[selector->name()] = selector;
			}
            
		}
        
        void addSelector(std::shared_ptr<CSSSelector> sel)
        {
            // first add to universal selectors
            //fUniversalSelectors.push_back(sel);

			// Now, add to the appropriate map
            switch (sel->fKind)
            {
            case CSSSelectorKind::CSS_SELECTOR_ID:
				addSelectorToMap(fIDSelectors, sel);
                break;
            case CSSSelectorKind::CSS_SELECTOR_CLASS:
                addSelectorToMap(fClassSelectors, sel);
                break;
            case CSSSelectorKind::CSS_SELECTOR_ELEMENT:
                addSelectorToMap(fElementSelectors, sel);
                break;
            }
        }

        bool loadFromSpan(const ByteSpan& inSpan)
        {
            fSource = inSpan;

            // Iterate over the selectors
            CSSSelectorIterator iter(fSource);
            while (iter)
            {
				auto sel = std::make_shared<CSSSelector>(*iter);
				addSelector(sel);
                
                ++iter;
            }

            return true;
        }
    };
}
