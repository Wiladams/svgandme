#pragma once


#include <utility>
#include <new>

#include "definitions.h"
#include "core_nametable.h"
#include "core_hash.h"


namespace waavs
{


    template <typename ValueT>
    struct WSNameMap
    {
        using Key = InternedKey;

        struct Entry
        {
            Key key{};
            ValueT value{};
            size_t hash{};
        };
        //ASSERT_MEMCPY_SAFE(Entry);

        Entry* fEntries{ nullptr };
        size_t fCapacity{ 0 };
        size_t fSize{ 0 };

        static constexpr size_t kInitialCapacity = 8;

        WSNameMap() noexcept
        {
            init(kInitialCapacity);
        }

        explicit WSNameMap(size_t cap) noexcept
        {
            init(cap);
        }

        WSNameMap(const WSNameMap&) = delete;
        WSNameMap& operator=(const WSNameMap&) = delete;

        WSNameMap(WSNameMap&& other) noexcept
        {
            moveFrom(other);
        }

        WSNameMap& operator=(WSNameMap&& other) noexcept
        {
            if (this != &other)
            {
                destroy();
                moveFrom(other);
            }

            return *this;
        }

        ~WSNameMap() noexcept
        {
            destroy();
        }

        static constexpr size_t loadThreshold(size_t cap) noexcept
        {
            return cap - (cap >> 2); // 0.75
        }

        bool init(size_t cap = kInitialCapacity) noexcept
        {
            destroy();

            fCapacity = ws_table_next_pow2(cap);
            if (fCapacity < 2)
                fCapacity = 2;

            fEntries = new (std::nothrow) Entry[fCapacity];
            if (!fEntries)
            {
                fCapacity = 0;
                fSize = 0;
                return false;
            }

            fSize = 0;
            return true;
        }

        void destroy() noexcept
        {
            if (!fEntries)
                return;

            delete[] fEntries;

            fEntries = nullptr;
            fCapacity = 0;
            fSize = 0;
        }

        void clear() noexcept
        {
            if (!fEntries)
                return;

            for (size_t i = 0; i < fCapacity; ++i)
            {
                fEntries[i].key = nullptr;
                fEntries[i].value = ValueT{};
                fEntries[i].hash = 0;
            }

            fSize = 0;
        }

        bool reset(size_t cap = kInitialCapacity) noexcept
        {
            return init(cap);
        }

        size_t size() const noexcept
        {
            return fSize;
        }

        size_t capacity() const noexcept
        {
            return fCapacity;
        }

        bool empty() const noexcept
        {
            return fSize == 0;
        }

        const Entry* entries() const noexcept
        {
            return fEntries;
        }

        Entry* entries() noexcept
        {
            return fEntries;
        }

        bool reserve(size_t expectedCount) noexcept
        {
            size_t cap = fCapacity ? fCapacity : size_t{ 2 };

            while (loadThreshold(cap) < expectedCount)
                cap <<= 1;

            if (cap == fCapacity)
                return true;

            if (!fEntries)
                return init(cap);

            return rehash(cap);
        }

        bool maybeGrow() noexcept
        {
            if (!fEntries)
                return init(kInitialCapacity);

            if (fSize >= loadThreshold(fCapacity))
                return rehash(fCapacity << 1);

            return true;
        }



    public:
        bool put(Key key, const ValueT& value) noexcept
        {
            if (!key)
                return false;

            if (!maybeGrow())
                return false;

            size_t h = ws_table_ptr_hash(key);
            size_t idx = h & (fCapacity - 1);

            for (;;)
            {
                Entry& e = fEntries[idx];

                if (!e.key)
                {
                    e.key = key;
                    e.hash = h;
                    e.value = value;
                    ++fSize;
                    return true;
                }

                if (e.key == key)
                {
                    e.value = value;
                    return true;
                }

                idx = (idx + 1) & (fCapacity - 1);
            }
        }

        bool put(Key key, ValueT&& value) noexcept
        {
            if (!key)
                return false;

            if (!maybeGrow())
                return false;

            size_t h = ws_table_ptr_hash(key);
            size_t idx = h & (fCapacity - 1);

            for (;;)
            {
                Entry& e = fEntries[idx];

                if (!e.key)
                {
                    e.key = key;
                    e.hash = h;
                    e.value = std::move(value);
                    ++fSize;
                    return true;
                }

                if (e.key == key)
                {
                    e.value = std::move(value);
                    return true;
                }

                idx = (idx + 1) & (fCapacity - 1);
            }
        }

        bool get(Key key, ValueT& outValue) const noexcept
        {
            const Entry* e = findEntry(key);
            if (!e)
                return false;

            outValue = e->value;
            return true;
        }

        const ValueT* getRef(Key key) const noexcept
        {
            const Entry* e = findEntry(key);
            return e ? &e->value : nullptr;
        }

        ValueT* getRef(Key key) noexcept
        {
            Entry* e = findEntry(key);
            return e ? &e->value : nullptr;
        }

        bool contains(Key key) const noexcept
        {
            return findEntry(key) != nullptr;
        }

        bool remove(Key key) noexcept
        {
            if (!fEntries || !key || !fCapacity)
                return false;

            const size_t mask = fCapacity - 1;

            // Find the entry to remove.
            size_t hole = ws_table_ptr_hash(key) & mask;

            for (;;)
            {
                Entry& entry = fEntries[hole];

                if (!entry.key)
                    return false;

                if (entry.key == key)
                    break;

                hole = (hole + 1) & mask;
            }

            // Remove the target entry. This slot is now the hole.
            fEntries[hole].key = nullptr;
            fEntries[hole].value = ValueT{};
            fEntries[hole].hash = 0;
            --fSize;

            // Scan forward through the remainder of the cluster.
            size_t scan = (hole + 1) & mask;

            while (fEntries[scan].key)
            {
                Entry& candidate = fEntries[scan];

                const size_t ideal =
                    candidate.hash & mask;

                // Cyclic distances from the candidate's ideal position.
                const size_t distanceToHole =
                    (hole - ideal) & mask;

                const size_t distanceToCandidate =
                    (scan - ideal) & mask;

                // Move the candidate when the hole occurs earlier in that
                // candidate's probe sequence.
                if (distanceToHole < distanceToCandidate)
                {
                    fEntries[hole].key = candidate.key;
                    fEntries[hole].hash = candidate.hash;
                    fEntries[hole].value = std::move(candidate.value);

                    candidate.key = nullptr;
                    candidate.value = ValueT{};
                    candidate.hash = 0;

                    // The candidate's old position is now the new hole.
                    hole = scan;
                }

                // Advance independently of the hole.
                scan = (scan + 1) & mask;
            }

            return true;
        }

        // ----------------------------------
        // Operations
        // 
        // Instead of exposing the internal array, 
        // we provide a forEach method that takes a callable function
        // as the only parameter.  The function 
        // will be called for each valid entry in the table.

        // forEach
        // Calls the provided function for each valid entry in the table.
        template <typename Fn>
        void forEach(Fn&& fn) const
        {
            if (!fEntries)
                return;

            for (size_t i = 0; i < fCapacity; ++i)
            {
                const Entry& e = fEntries[i];
                if (e.key)
                    fn(e.key, e.value);
            }
        }

        // forEachWhile
        // 
        // Calls the provided function for each valid entry in the table.
        // If the function returns false, the iteration stops and forEachWhile
        // returns false.
        template <typename Fn>
        bool forEachWhile(Fn&& fn) const
        {
            if (!fEntries)
                return true;

            for (size_t i = 0; i < fCapacity; ++i)
            {
                const Entry& e = fEntries[i];
                if (e.key && !fn(e.key, e.value))
                    return false;
            }

            return true;
        }

        template <typename Fn>
        const Entry* findIf(Fn&& fn) const
        {
            if (!fEntries)
                return nullptr;

            for (size_t i = 0; i < fCapacity; ++i)
            {
                const Entry& e = fEntries[i];
                if (e.key && fn(e.key, e.value))
                    return &e;
            }

            return nullptr;
        }

    private:
        bool rehash(size_t newCapacity) noexcept
        {
            newCapacity = ws_table_next_pow2(newCapacity);
            if (newCapacity < 2)
                newCapacity = 2;

            Entry* newEntries = new (std::nothrow) Entry[newCapacity];

            if (!newEntries)
                return false;

            for (size_t i = 0; i < fCapacity; ++i)
            {
                Entry& old = fEntries[i];
                if (!old.key)
                    continue;

                size_t idx = old.hash & (newCapacity - 1);

                while (newEntries[idx].key)
                    idx = (idx + 1) & (newCapacity - 1);

                newEntries[idx].key = old.key;
                newEntries[idx].hash = old.hash;
                newEntries[idx].value = std::move(old.value);
            }

            delete[] fEntries;

            fEntries = newEntries;
            fCapacity = newCapacity;

            return true;
        }

        const Entry* findEntry(Key key) const noexcept
        {
            if (!fEntries || !key || !fCapacity)
                return nullptr;

            size_t h = ws_table_ptr_hash(key);
            size_t idx = h & (fCapacity - 1);
            size_t start = idx;

            for (;;)
            {
                const Entry& e = fEntries[idx];

                if (!e.key)
                    return nullptr;

                if (e.key == key)
                    return &e;

                idx = (idx + 1) & (fCapacity - 1);

                if (idx == start)
                    return nullptr;
            }
        }

        Entry* findEntry(Key key) noexcept
        {
            return const_cast<Entry*>(
                static_cast<const WSNameMap*>(this)->findEntry(key));
        }

        void moveFrom(WSNameMap& other) noexcept
        {
            fEntries = other.fEntries;
            fCapacity = other.fCapacity;
            fSize = other.fSize;

            other.fEntries = nullptr;
            other.fCapacity = 0;
            other.fSize = 0;
        }
    };


    template<typename ValueT>
    void dumpTableMap(const WSNameMap<ValueT>& map) {
        std::cout << "Table entries (size=" << map.size() << ", cap=" << map.capacity() << "):" << std::endl;
        for (size_t i = 0; i < map.capacity(); ++i) {
            const auto* e = &map.entries()[i];  // entries() returns const Entry*
            if (e->key) {
                std::cout << "  [" << i << "] key=" << e->key << " hash=" << e->hash << std::endl;
            }
            else {
                std::cout << "  [" << i << "] NULL" << std::endl;
            }
        }
    }
}


