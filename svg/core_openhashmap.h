#pragma once

#include <utility>
#include <new>

#include "definitions.h"
//#include "core_nametable.h"
#include "core_hash.h"


namespace waavs
{
    struct WSHash32
    {
        size_t operator()(uint32_t value) const noexcept
        {
            return static_cast<size_t>(fmix32(value));
        }
    };

    struct WSHash64
    {
        size_t operator()(uint64_t value) const noexcept
        {
            return static_cast<size_t>(fmix64(value));
        }
    };

    template <typename KeyT>
    struct WSPointerHash
    {
        size_t operator()(KeyT key) const noexcept
        {
            return ws_table_ptr_hash(reinterpret_cast<const void*>(key));
        }
    };

    template <typename KeyT>
    struct WSPointerEqual
    {
        bool operator()(KeyT a, KeyT b) const noexcept
        {
            return a == b;
        }
    };

    template <
        typename KeyT,
        typename ValueT,
        typename HashT = WSPointerHash<KeyT>,
        typename EqualT = WSPointerEqual<KeyT>>
    struct WSOpenHashMap
    {
    private:
        struct Entry
        {
            KeyT key{};
            ValueT value{};
            size_t hash{};
            bool occupied{ false };
        };
        //ASSERT_MEMCPY_SAFE(Entry);

        Entry* fEntries{ nullptr };
        size_t fCapacity{ 0 };
        size_t fSize{ 0 };

        HashT fHash{};
        EqualT fEqual{};

        static constexpr size_t kInitialCapacity = 8;

    public:

        WSOpenHashMap() noexcept
        {
            init(kInitialCapacity);
        }

        explicit WSOpenHashMap(size_t cap) noexcept
        {
            init(cap);
        }

        WSOpenHashMap(const WSOpenHashMap&) = delete;
        WSOpenHashMap& operator=(const WSOpenHashMap&) = delete;

        WSOpenHashMap(WSOpenHashMap&& other) noexcept
        {
            moveFrom(other);
        }

        WSOpenHashMap& operator=(WSOpenHashMap&& other) noexcept
        {
            if (this != &other)
            {
                destroy();
                moveFrom(other);
            }

            return *this;
        }

        ~WSOpenHashMap() noexcept
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

            //fEntries = static_cast<Entry*>(std::calloc(fCapacity, sizeof(Entry)));
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
                Entry& e = fEntries[i];

                if (!e.occupied)
                    continue;

                e.key = KeyT{};
                e.value = ValueT{};
                e.hash = 0;
                e.occupied = false;
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

        //const Entry* entries() const noexcept { return fEntries; }
        //Entry* entries() noexcept { return fEntries; }

        bool reserve(size_t expectedCount) noexcept
        {
            size_t cap = fCapacity ? fCapacity : kInitialCapacity;

            while (loadThreshold(cap) < expectedCount)
                cap <<= 1;

            if (!fEntries)
                return init(cap);

            if (cap == fCapacity)
                return true;

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




        bool put(KeyT key, ValueT value) noexcept
        {
            if (!maybeGrow())
                return false;

            size_t h = fHash(key);
            size_t idx = h & (fCapacity - 1);

            for (;;)
            {
                Entry& e = fEntries[idx];

                if (!e.occupied)
                {
                    e.key = key;
                    e.hash = h;
                    e.value = std::move(value);
                    e.occupied = true;
                    ++fSize;
                    return true;
                }

                if (e.hash == h && fEqual(e.key, key))
                {
                    e.value = std::move(value);
                    return true;
                }

                idx = (idx + 1) & (fCapacity - 1);
            }
        }

        /*
        bool put(KeyT key, ValueT&& value) noexcept
        {
            if (!maybeGrow())
                return false;

            size_t h = fHash(key);
            size_t idx = h & (fCapacity - 1);

            for (;;)
            {
                Entry& e = fEntries[idx];

                if (!e.occupied)
                {
                    e.key = key;
                    e.hash = h;
                    e.value = std::move(value);
                    e.occupied = true;
                    ++fSize;
                    return true;
                }

                if (fEqual(e.key, key))
                {
                    e.value = std::move(value);
                    return true;
                }

                idx = (idx + 1) & (fCapacity - 1);
            }
        }
        */

        //bool get(KeyT key, ValueT& outValue) const noexcept
        //{
        //    const Entry* e = findEntry(key);
        //    if (!e)
        //        return false;

        //    outValue = e->value;
        //    return true;
        //}

        const ValueT* getRef(KeyT key) const noexcept
        {
            const Entry* e = findEntry(key);
            return e ? &e->value : nullptr;
        }

        ValueT* getRef(KeyT key) noexcept
        {
            Entry* e = findEntry(key);
            return e ? &e->value : nullptr;
        }

        bool contains(KeyT key) const noexcept
        {
            return findEntry(key) != nullptr;
        }

        bool remove(KeyT key) noexcept
        {
            Entry* victim = findEntry(key);
            if (!victim)
                return false;

            const size_t mask = fCapacity - 1;
            size_t hole = static_cast<size_t>(victim - fEntries);

            victim->key = KeyT{};
            victim->value = ValueT{};
            victim->hash = 0;
            victim->occupied = false;

            --fSize;

            size_t scan = hole;

            for (;;)
            {
                scan = (scan + 1) & mask;

                Entry& e = fEntries[scan];

                if (!e.occupied)
                    return true;

                const size_t ideal = e.hash & mask;

                const bool mustMove =
                    (hole <= scan)
                    ? (ideal <= hole || ideal > scan)
                    : (ideal <= hole && ideal > scan);

                if (!mustMove)
                    continue;

                fEntries[hole].key = e.key;
                fEntries[hole].value = std::move(e.value);
                fEntries[hole].hash = e.hash;
                fEntries[hole].occupied = true;

                e.key = KeyT{};
                e.value = ValueT{};
                e.hash = 0;
                e.occupied = false;

                hole = scan;
            }
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
        void forEach(Fn&& fn) const noexcept
        {
            if (!fEntries)
                return;

            for (size_t i = 0; i < fCapacity; ++i)
            {
                const Entry& e = fEntries[i];
                if (e.occupied)
                    fn(e.key, e.value);
            }
        }

        
        // forEachWhile
        //
        // Calls the provided function for each valid entry in the table.
        // If the function returns false, the iteration stops and forEachWhile
        // returns false.
        template <typename Fn>
        bool forEachWhile(Fn&& fn) const noexcept
        {
            if (!fEntries)
                return true;

            for (size_t i = 0; i < fCapacity; ++i)
            {
                const Entry& e = fEntries[i];
                if (e.occupied && !fn(e.key, e.value))
                    return false;
            }

            return true;
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

                if (!old.occupied)
                    continue;

                size_t idx = old.hash & (newCapacity - 1);

                while (newEntries[idx].occupied)
                    idx = (idx + 1) & (newCapacity - 1);

                newEntries[idx].key = old.key;
                newEntries[idx].hash = old.hash;
                newEntries[idx].value = std::move(old.value);
                newEntries[idx].occupied = true;
            }

            delete[] fEntries;

            fEntries = newEntries;
            fCapacity = newCapacity;

            return true;
        }

        const Entry* findEntry(KeyT key) const noexcept
        {
            if (!fEntries || !fCapacity)
                return nullptr;

            size_t h = fHash(key);
            size_t idx = h & (fCapacity - 1);
            size_t start = idx;

            for (;;)
            {
                const Entry& e = fEntries[idx];

                if (!e.occupied)
                    return nullptr;

                if (e.hash == h && fEqual(e.key, key))
                    return &e;

                idx = (idx + 1) & (fCapacity - 1);

                if (idx == start)
                    return nullptr;
            }
        }

        Entry* findEntry(KeyT key) noexcept
        {
            return const_cast<Entry*>(
                static_cast<const WSOpenHashMap*>(this)->findEntry(key));
        }

        void moveFrom(WSOpenHashMap& other) noexcept
        {
            fEntries = other.fEntries;
            fCapacity = other.fCapacity;
            fSize = other.fSize;
            fHash = other.fHash;
            fEqual = other.fEqual;

            other.fEntries = nullptr;
            other.fCapacity = 0;
            other.fSize = 0;
        }
    };
}

