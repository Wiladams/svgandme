#pragma once


#include <utility>
#include <new>

#include "definitions.h"


namespace waavs
{
    static inline size_t ws_table_next_pow2(size_t n) noexcept
    {
        if (n < 2) return 2;

        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;

#if SIZE_MAX > 0xffffffffu
        n |= n >> 32;
#endif

        return n + 1;
    }

    static inline size_t ws_table_ptr_hash(const void* p) noexcept
    {
        uintptr_t x = reinterpret_cast<uintptr_t>(p);

#if INTPTR_MAX > 0xffffffff
        x ^= x >> 33;
        x *= uintptr_t(0xff51afd7ed558ccdULL);
        x ^= x >> 33;
        x *= uintptr_t(0xc4ceb9fe1a85ec53ULL);
        x ^= x >> 33;
#else
        x ^= x >> 16;
        x *= uintptr_t(0x7feb352dU);
        x ^= x >> 15;
        x *= uintptr_t(0x846ca68bU);
        x ^= x >> 16;
#endif

        return size_t(x);
    }

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
        struct WSOpenHashTable
    {
        static_assert(std::is_pointer_v<KeyT>, "WSOpenHashTable expects pointer keys.");

        ASSERT_MEMCPY_SAFE(KeyT);
        //ASSERT_MEMCPY_SAFE(ValueT);


        struct Entry
        {
            KeyT key{};
            ValueT value{};
            size_t hash{};
        };
        //ASSERT_MEMCPY_SAFE(Entry);

        Entry* fEntries{ nullptr };
        size_t fCapacity{ 0 };
        size_t fSize{ 0 };

        HashT fHash{};
        EqualT fEqual{};

        static constexpr size_t kInitialCapacity = 8;

        WSOpenHashTable() noexcept
        {
            init(kInitialCapacity);
        }

        explicit WSOpenHashTable(size_t cap) noexcept
        {
            init(cap);
        }

        WSOpenHashTable(const WSOpenHashTable&) = delete;
        WSOpenHashTable& operator=(const WSOpenHashTable&) = delete;

        WSOpenHashTable(WSOpenHashTable&& other) noexcept
        {
            moveFrom(other);
        }

        WSOpenHashTable& operator=(WSOpenHashTable&& other) noexcept
        {
            if (this != &other)
            {
                destroy();
                moveFrom(other);
            }

            return *this;
        }

        ~WSOpenHashTable() noexcept
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
                fEntries[i].key = KeyT{};
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
            if (!fEntries)
                return init(expectedCount);

            size_t needed = expectedCount * 4 / 3 + 1;
            size_t cap = fCapacity;

            while (loadThreshold(cap) < needed)
                cap <<= 1;

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

        bool put(KeyT key, const ValueT& value) noexcept
        {
            if (!key)
                return false;

            if (!maybeGrow())
                return false;

            size_t h = fHash(key);
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

                if (fEqual(e.key, key))
                {
                    e.value = value;
                    return true;
                }

                idx = (idx + 1) & (fCapacity - 1);
            }
        }

        bool put(KeyT key, ValueT&& value) noexcept
        {
            if (!key)
                return false;

            if (!maybeGrow())
                return false;

            size_t h = fHash(key);
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

                if (fEqual(e.key, key))
                {
                    e.value = std::move(value);
                    return true;
                }

                idx = (idx + 1) & (fCapacity - 1);
            }
        }

        bool get(KeyT key, ValueT& outValue) const noexcept
        {
            const Entry* e = findEntry(key);
            if (!e)
                return false;

            outValue = e->value;
            return true;
        }

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
            if (!fEntries || !key || !fCapacity)
                return false;

            size_t h = fHash(key);
            size_t idx = h & (fCapacity - 1);

            for (;;)
            {
                Entry& e = fEntries[idx];

                if (!e.key)
                    return false;

                if (fEqual(e.key, key))
                    break;

                idx = (idx + 1) & (fCapacity - 1);
            }

            fEntries[idx].key = KeyT{};
            fEntries[idx].value = ValueT{};
            fEntries[idx].hash = 0;
            --fSize;

            size_t hole = idx;

            for (;;)
            {
                size_t next = (hole + 1) & (fCapacity - 1);
                Entry& enext = fEntries[next];

                if (!enext.key)
                    return true;

                size_t ideal = enext.hash & (fCapacity - 1);

                bool mustMove = (hole <= next)
                    ? (ideal <= hole || ideal > next)
                    : (ideal <= hole && ideal > next);

                if (mustMove)
                {
                    fEntries[hole].key = enext.key;
                    fEntries[hole].hash = enext.hash;
                    fEntries[hole].value = std::move(enext.value);

                    enext.key = KeyT{};
                    enext.value = ValueT{};
                    enext.hash = 0;

                    hole = next;
                }
                else
                {
                    hole = next;
                }
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
        bool forEachWhile(Fn&& fn) const noexcept
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
        const Entry* findIf(Fn&& fn) const noexcept
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
        const Entry* findEntry(KeyT key) const noexcept
        {
            if (!fEntries || !key || !fCapacity)
                return nullptr;

            size_t h = fHash(key);
            size_t idx = h & (fCapacity - 1);
            size_t start = idx;

            for (;;)
            {
                const Entry& e = fEntries[idx];

                if (!e.key)
                    return nullptr;

                if (fEqual(e.key, key))
                    return &e;

                idx = (idx + 1) & (fCapacity - 1);

                if (idx == start)
                    return nullptr;
            }
        }

        Entry* findEntry(KeyT key) noexcept
        {
            return const_cast<Entry*>(
                static_cast<const WSOpenHashTable*>(this)->findEntry(key));
        }

        void moveFrom(WSOpenHashTable& other) noexcept
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
