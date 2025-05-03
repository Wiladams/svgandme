#pragma once

#include <functional>
#include "xmlscan.h"
#include "xpath.h"

namespace waavs {
    /*
    // XmlElementInfoIterator
    // 
	// Iterate over a container of XmlElementInfo
    // 
    struct XmlElementInfoIterator {

        using difference_type = std::ptrdiff_t;
        using value_type = XmlElementInfo;
        using pointer = const XmlElementInfo*;
        using reference = const XmlElementInfo&;
        using iterator_category = std::forward_iterator_tag;


    private:
        XmlIteratorParams fParams{};
        XmlIteratorState fState{};
        XmlElementInfo fCurrentElement{};

    public:
        XmlElementInfoIterator() = default;

        // Construct an iterator from a chunk of XML
        XmlElementInfoIterator(const ByteSpan& inChunk, bool autoScanAttributes = false)
            : fState{ XML_ITERATOR_STATE_CONTENT, inChunk, inChunk }
        {
            fParams.fAutoScanAttributes = autoScanAttributes;
        }

        // return 'true' if the node we're currently sitting on is valid
        // return 'false' if otherwise
        explicit operator bool() { return !fCurrentElement.empty(); }

        // STL-compliant equality comparison
        bool operator==(const XmlElementInfoIterator& other) const noexcept
        {
            return fState.fSource == other.fState.fSource;
        }

        bool operator!=(const XmlElementInfoIterator& other) const noexcept
        {
            return !(*this == other);
        }

        // These operators make it operate like an iterator
        const XmlElementInfo& operator*() const { return fCurrentElement; }
        const XmlElementInfo* operator->() const { return &fCurrentElement; }

        // Increment the iterator either pre, or post notation
        XmlElementInfoIterator& operator++() { next(); return *this; }
        XmlElementInfoIterator operator++(int)
        {
            XmlElementInfoIterator temp = *this;  // Copy current state
            next();
            return temp;  // Return previous state
        }


        // Return the current value of the iteration
        //const XmlElement & next(XmlElement& elem)
        bool next()
        {
            bool validElement = XmlNextElementInfo(fParams, fState, fCurrentElement);
            return validElement;
        }
    };
    */

    // XmlElementIterator
    // scans XML generating a sequence of XmlElements
    // This is a forward only non-writeable iterator
    // 
    // Usage:
    //   XmlElementIterator iter(xmlChunk, false);
    // 
    //   while(iter.next())
    //   {
    //     printXmlElement(*iter);
    //   }
    //
    //  or, you can do it this way
    //  
    //  iter++;
    //  while (iter) {
    //    printXmlElement(*iter);
    //    iter++;
    //  }
    //
    struct XmlElementIterator {

        using difference_type = std::ptrdiff_t;
        using value_type = XmlElement;
        using pointer = const XmlElement*;
        using reference = const XmlElement&;
        using iterator_category = std::forward_iterator_tag;


    protected:
        XmlIteratorParams fParams{};
        XmlIteratorState fState{};
        XmlElement fCurrentElement{};

    public:
        XmlElementIterator() = default;

        // Construct an iterator from a chunk of XML
        XmlElementIterator(const ByteSpan& inChunk, bool autoScanAttributes = false)
            : fState{ XML_ITERATOR_STATE_CONTENT, inChunk, inChunk }
        {
            fParams.fAutoScanAttributes = autoScanAttributes;
            next();
        }

        // return 'true' if the node we're currently sitting on is valid
        // return 'false' if otherwise
        explicit operator bool() { return !fCurrentElement.empty(); }

        // STL-compliant equality comparison
        bool operator==(const XmlElementIterator& other) const noexcept
        {
            return fState.fSource.data() == other.fState.fSource.data();
            //return fState.fSource == other.fState.fSource;
        }

        bool operator!=(const XmlElementIterator& other) const noexcept
        {
            return !(*this == other);
        }

        // These operators make it operate like an iterator
        const XmlElement& operator*() const { return fCurrentElement; }
        const XmlElement* operator->() const { return &fCurrentElement; }

        // Increment the iterator either pre, or post notation
        XmlElementIterator& operator++() { next(); return *this; }
        XmlElementIterator operator++(int)
        {
            XmlElementIterator temp = *this;  // Copy current state
            next();
            return temp;  // Return previous state
        }


        // Return the current value of the iteration
        //const XmlElement & next(XmlElement& elem)
        bool next()
        {
            bool validElement = nextXmlElement(fParams, fState, fCurrentElement);
            return validElement;
        }
    };


    // XmlElementContainer
    // Support for range-based iteration over XML elements
    struct XmlElementContainer
    {
    private:
        const ByteSpan fXmlData;  // Stores the XML chunk for iteration
        const bool fAutoScanAttributes{ false };

    public:
        explicit constexpr XmlElementContainer(const ByteSpan& xmlData, bool autoScanAttributes = false)
            : fXmlData(xmlData)
            , fAutoScanAttributes(autoScanAttributes)
        {
        }

        // Return an iterator starting from the beginning
        XmlElementIterator begin() const
        {
            return XmlElementIterator(fXmlData, fAutoScanAttributes);
        }

        // Return an iterator representing the end (empty iterator)
        XmlElementIterator end() const
        {
            return XmlElementIterator(ByteSpan(fXmlData.fEnd, fXmlData.fEnd), fAutoScanAttributes);
        }
    };
}


namespace waavs 
{
    template <typename Predicate>
    struct XmlElementFilteredContainer
    {
    private:
        XmlElementContainer baseContainer;
        Predicate predicate;

    public:
        explicit XmlElementFilteredContainer(XmlElementContainer container, Predicate pred)
            : baseContainer(std::move(container)), predicate(std::move(pred)) {
        }

        struct XmlFilterIter
        {
            using difference_type = std::ptrdiff_t;
            using value_type = XmlElement;
            using pointer = const XmlElement*;
            using reference = const XmlElement&;
            using iterator_category = std::forward_iterator_tag;

            XmlElementIterator current;
            XmlElementIterator end;
            const Predicate* filterPredicate;

            XmlFilterIter(XmlElementIterator start, XmlElementIterator stop, const Predicate* pred)
                : current(start), end(stop), filterPredicate(pred)
            {
                while (current != end && !(*filterPredicate)(*current))
                    ++current;
            }

            reference operator*() const { return *current; }
            pointer operator->() const { return &(*current); }

            XmlFilterIter& operator++()
            {
                do {
                    ++current;
                } while (current != end && !(*filterPredicate)(*current));
            return *this;
            }

            XmlFilterIter operator++(int)
            {
                XmlFilterIter temp = *this;
                ++(*this);
                return temp;
            }

            bool operator==(const XmlFilterIter& other) const noexcept { return current == other.current; }
            bool operator!=(const XmlFilterIter& other) const noexcept { return current != other.current; }
        };

        XmlFilterIter begin() const { return XmlFilterIter(baseContainer.begin(), baseContainer.end(), &predicate); }
        XmlFilterIter end() const { return XmlFilterIter(baseContainer.end(), baseContainer.end(), &predicate); }
    };

    // Define a generic predicate function type
    using XmlElementPredicate = std::function<bool(const XmlElement&)>;

    // Create type alias for filtered container
    using XmlFilteredContainer = XmlElementFilteredContainer<XmlElementPredicate>;


}

namespace waavs
{
    using XPathFilteredContainer = XmlElementFilteredContainer<XPathPredicate>;
}
