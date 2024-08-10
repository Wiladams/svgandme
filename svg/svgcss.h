#pragma once



#include <map>
#include <unordered_map>
#include <vector>
#include <string>

#include "xmlscan.h"
#include "collections.h"

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
        else if (cssalpha[*inChunk])
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
            prop = chunk_trim(prop, csswsp);

            // get the value of the attribute
            auto value = chunk_token(s, charset(';'));
            value = chunk_trim(value, csswsp);

            // add the attribute to the map
			//auto name = std::string(prop.begin(), prop.end());
            attributes.addAttribute(prop, value);
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
        bool fIsNull{ true };
        CSSSelectorKind fKind{ CSSSelectorKind::CSS_SELECTOR_INVALID };
        ByteSpan fData{};
        XmlAttributeCollection fAttributes{};
        ByteSpan fName{};
        
        
        CSSSelector() = default;
        CSSSelector(const CSSSelector &other)
            :fIsNull(other.fIsNull),
            fKind(other.fKind)
			, fName(other.fName)
            , fData(other.fData)
			, fAttributes(other.fAttributes)
		{
		}
        
        CSSSelector(CSSSelectorKind kind, ByteSpan& name, const waavs::ByteSpan& inChunk)
            :fKind(kind),
            fName(name),
            fData(inChunk)
        {
            if (!name.empty())
			    fIsNull = false;
            
            loadFromChunk(inChunk);
        }

        CSSSelectorKind kind() const { return fKind; }
        const ByteSpan& name() const { return fName; }
		ByteSpan data() const { return fData; }
        
        const XmlAttributeCollection& attributes() const { return fAttributes; }


        //explicit operator bool() const { return fKind!= CSSSelectorKind::CSS_SELECTOR_INVALID && !fName.empty() && fAttributes.size() > 0; }
        explicit operator bool() const { return !fIsNull; }

        
		CSSSelector& operator=(const CSSSelector& other)
		{
            fIsNull = other.fIsNull;
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
			fAttributes.mergeAttributes(other.fAttributes);

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
        
        //waavs::ByteSpan getAttribute(const std::string &&name) const
        waavs::ByteSpan getAttribute(const ByteSpan &name) const
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
            //next();
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
				fCurrentName = chunk_token(fSource, charset(":"));
				fCurrentName = chunk_trim(fCurrentName, csswsp);
                
				fCurrentValue = chunk_token(fSource, charset(";"));
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


    // CSSSelectorIterator
    // 
	// Given a whole style sheet, iterate over the selectors
	// individual selectors are indicated by <selector>[, <selector>]* { <properties> }
    // Since there can be multiple selectors, we need to come back again and again
    //
    struct CSSSelectorIterator
    {
        ByteSpan fSource{};
        ByteSpan fMark{};

        
        ByteSpan fSelectorNames{};
        ByteSpan fSelectorContent{};

        CSSSelector fCurrentItem{};

        CSSSelectorIterator(const ByteSpan& inChunk)
            :fSource(inChunk),
			fMark(inChunk)
        {
            // don't call next() in here.  Allow the app to 
            // decide when they want to move the iterator
            //next();
        }

        explicit operator bool() const { return (bool)fCurrentItem; }

        // Queue up the next selection, skipping past comments
        // and whatnot
        bool advanceSelection()
        {
            // Skip whitespace
            fSource = chunk_ltrim(fSource, csswsp);
            if (fSource.size() == 0)
                return false;
            
            // skip 'C' style single line, and multi-line comments
            // we do this in a loop, because there can be multiple
            // comment blocks before we get to actual content
            while (fSource)
            {

                if (fSource.size() > 2 && fSource[0] == '/' && fSource[1] == '*')
                {
                    // skip past the /* asterisk */ style comment
                    fSource += 2;
                    while (fSource.size() > 1 && !(fSource[0] == '*' && fSource[1] == '/'))
                        fSource += 1;
                    if (fSource.size() > 1)
                        fSource += 2;

                    // start from the top again, if there are consecutive comments
                    continue;
                }
                else if (fSource.size() > 1 && fSource[0] == '/' && fSource[1] == '/')
                {
                    // skip past the // double slash style of comment
                    fSource += 2;
                    fSource = chunk_skip_until_char(fSource, '\n');

                    // start from the top again, if there are consecutive comments
                    continue;
                }

                // separate out the select name list from the content
				fSelectorNames = chunk_token_char(fSource, '{');
                fSelectorNames = chunk_trim(fSelectorNames, csswsp);

                if (!fSelectorNames)
                    return false;

                // Isolate the content portion
                fSelectorContent = chunk_token_char(fSource, '}');
                fSelectorContent = chunk_trim(fSelectorContent, csswsp);

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
            
            fCurrentItem = CSSSelector();

            // pull off the next name delimeted by a comma
            ByteSpan selectorName = chunk_token(fSelectorNames, ",");
			fSelectorNames = chunk_trim(fSelectorNames, csswsp);
            
            // determine what kind of selector we have
            auto selectorKind = parseSimpleSelectorKind(selectorName);
            
            if (selectorKind != CSSSelectorKind::CSS_SELECTOR_INVALID) {
                selectorName++; // skip the first character of the name
                
                fCurrentItem = CSSSelector(selectorKind, selectorName, fSelectorContent);
                return true;
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

        std::unordered_map<ByteSpan, std::shared_ptr<CSSSelector>, ByteSpanHash> fIDSelectors{};
        std::unordered_map<ByteSpan, std::shared_ptr<CSSSelector>, ByteSpanHash> fClassSelectors{};
        std::unordered_map<ByteSpan, std::shared_ptr<CSSSelector>, ByteSpanHash> fElementSelectors{};
        std::unordered_map<ByteSpan, std::shared_ptr<CSSSelector>, ByteSpanHash> fAnimationSelectors{};
        //std::vector<std::shared_ptr<CSSSelector>> fUniversalSelectors{};

        CSSStyleSheet() = default;

        CSSStyleSheet(const waavs::ByteSpan& inSpan)
        {
            loadFromSpan(inSpan);
        }
        
        std::shared_ptr<CSSSelector> getSelector(const ByteSpan& name, CSSSelectorKind kind)
        {
     
            
            switch (kind)
            {
            case CSSSelectorKind::CSS_SELECTOR_ID:
            {
                auto it = fIDSelectors.find(name);
                if (it != fIDSelectors.end())
                    return it->second;
                break;
            }
            case CSSSelectorKind::CSS_SELECTOR_CLASS:
            {
                auto it = fClassSelectors.find(name);
                if (it != fClassSelectors.end())
                    return it->second;
                break;
            }
            case CSSSelectorKind::CSS_SELECTOR_ATRULE:
            {
                auto it = fAnimationSelectors.find(name);
                if (it != fAnimationSelectors.end())
                    return it->second;
                break;
            }
            case CSSSelectorKind::CSS_SELECTOR_ELEMENT:
            {
                auto it = fElementSelectors.find(name);
                if (it != fElementSelectors.end())
                    return it->second;
                break;
            }
            default:
                break;
            }

            
            return nullptr;
        }


		std::shared_ptr<CSSSelector> getIDSelector(const ByteSpan& name)
        { 
            return getSelector(name, CSSSelectorKind::CSS_SELECTOR_ID);
        }
        
        std::shared_ptr<CSSSelector> getElementSelector(const ByteSpan& name)
		{
            return getSelector(name, CSSSelectorKind::CSS_SELECTOR_ELEMENT);
		}
        
        std::shared_ptr<CSSSelector> getClassSelector(const ByteSpan& name)
		{
            return getSelector(name, CSSSelectorKind::CSS_SELECTOR_CLASS);
		}
        
        std::shared_ptr<CSSSelector> getAnimationSelector(const ByteSpan& name)
        {
            return getSelector(name, CSSSelectorKind::CSS_SELECTOR_ATRULE);
        }
        
        
		void addSelectorToMap(std::unordered_map<ByteSpan, std::shared_ptr<CSSSelector>, ByteSpanHash> &amap, std::shared_ptr<CSSSelector> selector)
		{
            ByteSpan aname = selector->name();
            
			auto it = amap.find(aname);
			if (it != amap.end())
			{
				// merge the two selectors
                // newer stuff replaces older stuff
				it->second->mergeProperties(*selector);
			}
			else
			{
				amap[aname] = selector;
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
            case CSSSelectorKind::CSS_SELECTOR_ATRULE:
                addSelectorToMap(fAnimationSelectors, sel);
                break;
            }
        }

        bool loadFromSpan(const ByteSpan& inSpan)
        {
            fSource = inSpan;

            // Iterate over the selectors
            CSSSelectorIterator iter(fSource);
            while (iter.next())
            {
				auto sel = std::make_shared<CSSSelector>(*iter);
				addSelector(sel);
            }

            return true;
        }
    };
}
