#pragma once

#include <unordered_map>

#include "bspan.h"

namespace waavs {
    template <typename T>
    struct WSListNode {
        T value;
        WSListNode<T>* left;
        WSListNode<T>* right;

        WSListNode(T value, WSListNode<T>* left, WSListNode<T>* right)
            : left(left), right(right), value(value)
        {
        }
    };

    template <typename T>
    struct WSList {
        int count;
        WSListNode<T>* farLeft;
        WSListNode<T>* farRight;

        WSList()
            : count(0), farLeft(nullptr), farRight(nullptr) {}

        int size() const {
            return count;
        }

        bool empty() const {
            return count <= 0;
        }

        void pushRight(T value) {
            WSListNode<T>* newNode = new WSListNode<T>(value, farRight, nullptr);

            if (farRight != nullptr) {
                farRight->right = newNode;
            }

            farRight = newNode;

            if (farLeft == nullptr) {
                farLeft = newNode;
            }

            count = count + 1;
        }

        T popRight()
        {
            WSListNode<T>* valueNode = farRight;
            if (valueNode == nullptr) {
                return T();
            }

            T retValue = valueNode->value;
            if (valueNode->left != nullptr) {
                farRight = valueNode->left;
                farRight->right = nullptr;
            }
            else {
                farRight = nullptr;
            }


            if (farLeft == valueNode) {
                farLeft = nullptr;
            }
            count = count - 1;

            // Make sure we don't have a memory leak
            delete valueNode;

            return retValue;
        }



        T popLeft()
        {
            WSListNode<T>* valueNode = farLeft;
            if (valueNode == nullptr) {
                return T();
            }

            T retValue = valueNode->value;
            if (valueNode->right != nullptr) {
                farLeft = valueNode->right;
                farLeft->left = nullptr;
            }
            else {
                farLeft = nullptr;
            }

            if (farRight == valueNode) {
                farRight = nullptr;
            }

            count = count - 1;

            // Make sure we don't have a memory leak
            delete valueNode;

            return retValue;
        }

        T peekRight() const {
            if (farRight == nullptr) {
                // BUGBUG - maybe throw exception
                return T();
            }

            return farRight->value;
        }

        T peekLeft() const
        {
            if (farLeft == nullptr) {
                // BUGBUG - maybe throw exception
                return T();
            }

            return farLeft->value;
        }

        T nthFromLeft(const int n) const
        {
            int count = 0;
            WSListNode<T> sentinel = farLeft;
            if (sentinel == nullptr) {
                return T();
            }

            while (count < n) {
                sentinel = sentinel->right;
                if (sentinel == nullptr) {
                    return T();
                }
                count = count + 1;
            }
            return sentinel->value;
        }

        T nthFromRight(const int n) const
        {
            int count = n;
            WSListNode<T>* sentinel = farRight;
            // If the list is empty
            // return nothing
            // BUGBUG - should throw (index out of range) exception
            if (sentinel == nullptr) {
                return T();
            }

            while (count > 0) {
                sentinel = sentinel->left;
                // BUGBUG - should throw out of range exception
                if (sentinel == nullptr) {
                    return T();
                }
                count = count - 1;
            }

            return sentinel->value;
        }
    };
    

    // Implementation of a Stack
    // first in last out
    // enqueue(val)
    // val dequeue()
    template<typename T>
    class WSStack
    {
        WSList<T> fList;

    public:
        bool push(T value) { fList.pushRight(value); return true; }
        T pop() { return fList.popRight(); }
        int size() const { return fList.size(); }
        T top() { return fList.peekRight(); }
		bool empty() const { return fList.empty(); }
        
        // remove all the entries from the stack
        // BUGBUG - this will not call the destructors of the
        // objects being removed, so it must be used
        // with caution.
        WSStack& clear()
        {
            while (size() > 0) {
                pop();
            }

            return *this;
        }

        // duplicate the entry on the top of the stack
        WSStack& dup()
        {
            if (size() > 0) {
                push(top());
            }
            return *this;
        }

        // Exchange the two items on the top of the stack
        // same as: 2 1 roll
        WSStack& exch()
        {
            // Only do the exchange if there are two
            // or more items already on the stack
            if (size() >= 2)
            {
                T a = pop();
                T b = pop();
                push(a);
                push(b);
            }
            return *this;
        }

        // Return the nth item on the stack
        // the top of the stack is item '0'
        // the second item down the stack is item '1'
        // and so on.
        T nth(const int n) const { return fList.nthFromRight(n); }

        // Copy the top n items on the stack
        WSStack& copy(const int n)
        {
            for (int i = 0; i < n; i++) {
                push(nth(n - 1));
            }
            return *this;
        }

        // Convenience operator so we can use
        // [] array indexing notation to get at items
        T operator[](const int idx) const { return nth(idx); }
    };
    
}


namespace waavs {
    //============================================================
    // XmlAttributeCollection
    // A collection of the attibutes found on an XmlElement
    //============================================================

    struct XmlAttributeCollection
    {
        std::unordered_map<ByteSpan, ByteSpan, ByteSpanHash, ByteSpanEquivalent> fAttributes{};

        XmlAttributeCollection() = default;
        XmlAttributeCollection(const XmlAttributeCollection& other) noexcept
            :fAttributes(other.fAttributes)
        {}

        XmlAttributeCollection(const ByteSpan& inChunk) noexcept
        {
            scanAttributes(inChunk);
        }

        // Return a const attribute collection
        const std::unordered_map<ByteSpan, ByteSpan, ByteSpanHash, ByteSpanEquivalent>& attributes() const noexcept { return fAttributes; }

        size_t size() const noexcept { return fAttributes.size(); }

        void clear() noexcept { fAttributes.clear(); }

        // scanAttributes()
        // Given a chunk that contains attribute key value pairs
        // separated by whitespace, parse them, and store the key/value pairs 
        // in the fAttributes map
        bool scanAttributes(const ByteSpan& inChunk) noexcept
        {
            ByteSpan src = inChunk;
            ByteSpan key;
            ByteSpan value;

            while (readNextKeyAttribute(src, key, value))
            {
                addAttribute(key, value);
            }

            return true;
        }

        //bool hasAttribute(const std::string& inName) const
        bool hasAttribute(const ByteSpan& inName) const noexcept
        {
            return fAttributes.find(inName) != fAttributes.end();
        }


        // Add a single attribute to our collection of attributes
        // if the attribute already exists, replace its value
        // with the new value
        //void addAttribute(const std::string& name, const ByteSpan& valueChunk)
        void addAttribute(const ByteSpan& name, const ByteSpan& valueChunk) noexcept
        {
            fAttributes[name] = valueChunk;
        }


        //ByteSpan getAttribute(const std::string& name) const
        ByteSpan getAttribute(const ByteSpan& name) const noexcept
        {
            auto it = fAttributes.find(name);
            if (it != fAttributes.end())
                return it->second;
            return {};
        }


        XmlAttributeCollection& mergeAttributes(const XmlAttributeCollection& other) noexcept
        {
            for (auto& attr : other.fAttributes)
            {
                fAttributes[attr.first] = attr.second;
            }
            return *this;
        }

		// Given a name, find the attribute and return its value
        // return false if the name is not found
        static bool getValue(const ByteSpan &inChunk, const ByteSpan& key, ByteSpan& value) noexcept
        {
            ByteSpan src = inChunk;
            ByteSpan name{};

            while (readNextKeyAttribute(src, name, value))
            {
                if (name == key)
                    return true;
            }

            return false;
        }
    };
}

