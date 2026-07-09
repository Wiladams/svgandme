#pragma once

#include "xml_scan_element.h"

namespace waavs 
{


    // A simple pull model forward XML element iterator
    struct XmlPull
    {
        ByteSpan fInput{};
        XmlIteratorParams fParams{};
        XmlElement fCurrentElement{};

        explicit XmlPull(const ByteSpan& src)
            : fInput{ src }
        {
        }

        // Convenience methods for dealing with current element
        const XmlElement& operator*() const { return fCurrentElement; }
        const XmlElement* operator->() const { return &fCurrentElement; }

        bool next()
        {
            const unsigned char* before = fInput.begin();

            bool success = read_next_xml_element_direct(fParams, fInput, fCurrentElement);
            const unsigned char* after = fInput.begin();

            if (success && (before == after))
            {
                // We made no progress; to avoid infinite loop, we must fail
                fCurrentElement.reset();
                return false;
            }

            return success;
        }
    };
}
