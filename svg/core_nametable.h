#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <string_view>
#include <utility>

#include "lang_span.h"
#include "core_hash.h"


namespace waavs
{
    // ========================================================================
    // InternedKey
    //
    // Canonical string identity.
    //
    // Once a string has been interned, pointer equality is string equality.
    // ========================================================================

    using InternedKey = const char*;


    struct InternedKeyHash
    {
        size_t operator()(InternedKey key) const noexcept
        {
            return key ? ws_table_ptr_hash(reinterpret_cast<const void*>(key)) : 0;
        }
    };


    struct InternedKeyEquivalent
    {
        bool operator()(InternedKey a, InternedKey b) const noexcept
        {
            return a == b;
        }
    };


    // ========================================================================
    // WSNameSet
    //
    // Owns permanent copies of interned strings.
    //
    // IMPORTANT ARCHITECTURAL RULE:
    //
    //      THE HASH TABLE IS ALWAYS POINTER HASHED.
    //
    // The canonical copied string pointer determines:
    //
    //      - initial slot
    //      - probe sequence
    //      - rehash placement
    //      - removal cluster repair
    //
    // contentHash and length are metadata used only to accelerate discovery
    // of an existing name from raw string contents.
    //
    //
    // Raw string:
    //
    //      content scan
    //          ->
    //      existing canonical pointer
    //
    // or:
    //
    //      copy string
    //          ->
    //      pointer hash
    //          ->
    //      table slot
    //          ->
    //      canonical InternedKey
    //
    // ========================================================================

    struct WSNameSet
    {
    private:
        struct Entry
        {
            const char* str{ nullptr };
            uint32_t contentHash{ 0 };
            uint32_t length{ 0 };
        };


        Entry* fEntries{ nullptr };
        size_t fCapacity{ 0 };
        size_t fSize{ 0 };

        static constexpr size_t kInitialCapacity = 16;


        // ====================================================================
        // Load factor
        // ====================================================================

        static constexpr size_t loadThreshold(size_t cap) noexcept
        {
            return cap - (cap >> 2);
        }


        // ====================================================================
        // Pointer hash
        //
        // THE ONLY hash used for table placement.
        // ====================================================================

        static size_t hashStringPtr(const char* str) noexcept
        {
            return str ? ws_table_ptr_hash(reinterpret_cast<const void*>(str)) : 0;
        }


        // ====================================================================
        // Content hash
        //
        // Used only as a cheap rejection test when comparing incoming bytes
        // against existing names.
        //
        // It never determines table placement.
        // ====================================================================

        static uint32_t hashStringContent(const char* str, size_t len) noexcept
        {
            uint32_t hash = 2166136261u;

            for (size_t i = 0; i < len; ++i)
            {
                hash ^= static_cast<uint8_t>(str[i]);
                hash *= 16777619u;
            }

            return fmix32(hash);
        }


        static uint32_t hashStringContent(const char* str) noexcept
        {
            if (!str)
                return 0;

            return hashStringContent(str, std::strlen(str));
        }


        // ====================================================================
        // Initialization
        // ====================================================================

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

            for (size_t i = 0; i < fCapacity; ++i)
            {
                if (fEntries[i].str)
                    delete[] fEntries[i].str;
            }

            delete[] fEntries;

            fEntries = nullptr;
            fCapacity = 0;
            fSize = 0;
        }


        // ====================================================================
        // Growth
        // ====================================================================

        bool maybeGrow() noexcept
        {
            if (!fEntries)
                return init(kInitialCapacity);

            if (fSize < loadThreshold(fCapacity))
                return true;

            if (fCapacity > (std::numeric_limits<size_t>::max() >> 1))
                return false;

            return rehash(fCapacity << 1);
        }


        bool rehash(size_t newCapacity) noexcept
        {
            newCapacity = ws_table_next_pow2(newCapacity);

            if (newCapacity < 2)
                newCapacity = 2;

            Entry* newEntries = new (std::nothrow) Entry[newCapacity];

            if (!newEntries)
                return false;

            const size_t mask = newCapacity - 1;

            for (size_t i = 0; i < fCapacity; ++i)
            {
                const Entry& old = fEntries[i];

                if (!old.str)
                    continue;

                // -----------------------------------------------------------
                // Placement is ALWAYS derived from the canonical pointer.
                // -----------------------------------------------------------

                size_t idx = hashStringPtr(old.str) & mask;

                while (newEntries[idx].str)
                    idx = (idx + 1) & mask;

                newEntries[idx] = old;
            }

            delete[] fEntries;

            fEntries = newEntries;
            fCapacity = newCapacity;

            return true;
        }


        // ====================================================================
        // Content lookup
        //
        // Because the table is pointer-hashed, arbitrary incoming string
        // contents do not define a valid probe sequence.
        //
        // We therefore scan the occupied entries, using contentHash and
        // length as cheap filters before memcmp().
        //
        // This operation is performed at the interning boundary.
        // ====================================================================

        const Entry* findEntryByContent(const ByteSpan& span) const noexcept
        {
            if (span.empty() || !fEntries)
                return nullptr;

            if (span.size() > std::numeric_limits<uint32_t>::max())
                return nullptr;

            const char* bytes = reinterpret_cast<const char*>(span.data());
            const uint32_t length = static_cast<uint32_t>(span.size());
            const uint32_t contentHash = hashStringContent(bytes, span.size());

            size_t seen = 0;

            for (size_t i = 0; i < fCapacity && seen < fSize; ++i)
            {
                const Entry& entry = fEntries[i];

                if (!entry.str)
                    continue;

                ++seen;

                if (entry.contentHash != contentHash)
                    continue;

                if (entry.length != length)
                    continue;

                if (std::memcmp(entry.str, span.data(), span.size()) == 0)
                    return &entry;
            }

            return nullptr;
        }


        const Entry* findEntryByContent(const char* str) const noexcept
        {
            if (!str)
                return nullptr;

            return findEntryByContent(ByteSpan(str));
        }


        // ====================================================================
        // Pointer lookup
        //
        // Fast canonical InternedKey lookup.
        //
        // This is the operation the physical table organization is optimized
        // for.
        // ====================================================================

        const Entry* findEntryByPtr(InternedKey key) const noexcept
        {
            if (!key || !fEntries || !fCapacity)
                return nullptr;

            const size_t mask = fCapacity - 1;
            size_t idx = hashStringPtr(key) & mask;
            size_t start = idx;

            for (;;)
            {
                const Entry& entry = fEntries[idx];

                if (!entry.str)
                    return nullptr;

                if (entry.str == key)
                    return &entry;

                idx = (idx + 1) & mask;

                if (idx == start)
                    return nullptr;
            }
        }


        Entry* findEntryByPtr(InternedKey key) noexcept
        {
            return const_cast<Entry*>(
                static_cast<const WSNameSet*>(this)->findEntryByPtr(key));
        }


        // ====================================================================
        // Insert an already-copied canonical string.
        // ====================================================================

        bool insertCopy(char* copy, uint32_t contentHash, uint32_t length) noexcept
        {
            if (!copy || !fEntries)
                return false;

            const size_t mask = fCapacity - 1;
            size_t idx = hashStringPtr(copy) & mask;

            while (fEntries[idx].str)
                idx = (idx + 1) & mask;

            fEntries[idx].str = copy;
            fEntries[idx].contentHash = contentHash;
            fEntries[idx].length = length;

            ++fSize;

            return true;
        }


    public:
        // ====================================================================
        // Construction
        // ====================================================================

        WSNameSet() noexcept
        {
            init(kInitialCapacity);
        }


        explicit WSNameSet(size_t cap) noexcept
        {
            init(cap);
        }


        WSNameSet(const WSNameSet&) = delete;
        WSNameSet& operator=(const WSNameSet&) = delete;


        WSNameSet(WSNameSet&& other) noexcept
        {
            moveFrom(other);
        }


        WSNameSet& operator=(WSNameSet&& other) noexcept
        {
            if (this != &other)
            {
                destroy();
                moveFrom(other);
            }

            return *this;
        }


        ~WSNameSet() noexcept
        {
            destroy();
        }


        // ====================================================================
        // reserve
        // ====================================================================

        bool reserve(size_t expectedCount) noexcept
        {
            size_t cap = fCapacity ? fCapacity : kInitialCapacity;

            while (loadThreshold(cap) < expectedCount)
            {
                if (cap > (std::numeric_limits<size_t>::max() >> 1))
                    return false;

                cap <<= 1;
            }

            if (!fEntries)
                return init(cap);

            if (cap == fCapacity)
                return true;

            return rehash(cap);
        }


        // ====================================================================
        // intern
        //
        // Content discovery occurs first.
        //
        // If no canonical copy exists:
        //
        //      allocate permanent copy
        //      hash COPY POINTER
        //      insert into pointer-hashed table
        //
        // ====================================================================

        InternedKey intern(const ByteSpan& span) noexcept
        {
            if (span.empty())
                return nullptr;

            if (span.size() > std::numeric_limits<uint32_t>::max())
                return nullptr;

            const Entry* existing = findEntryByContent(span);

            if (existing)
                return existing->str;

            if (!maybeGrow())
                return nullptr;

            const uint32_t length = static_cast<uint32_t>(span.size());
            const char* bytes = reinterpret_cast<const char*>(span.data());
            const uint32_t contentHash = hashStringContent(bytes, span.size());

            char* copy = new (std::nothrow) char[span.size() + 1];

            if (!copy)
                return nullptr;

            std::memcpy(copy, span.data(), span.size());
            copy[span.size()] = '\0';

            if (!insertCopy(copy, contentHash, length))
            {
                delete[] copy;
                return nullptr;
            }

            return copy;
        }


        InternedKey intern(const char* str) noexcept
        {
            return str ? intern(ByteSpan(str)) : nullptr;
        }


        InternedKey intern(std::string_view value) noexcept
        {
            return intern(ByteSpan(
                reinterpret_cast<const uint8_t*>(value.data()),
                value.size()));
        }


        // ====================================================================
        // Content queries
        // ====================================================================

        [[nodiscard]]
        bool hasName(const ByteSpan& span) const noexcept
        {
            return findEntryByContent(span) != nullptr;
        }


        [[nodiscard]]
        bool hasName(const char* str) const noexcept
        {
            return findEntryByContent(str) != nullptr;
        }


        [[nodiscard]]
        InternedKey findInterned(const ByteSpan& span) const noexcept
        {
            const Entry* entry = findEntryByContent(span);
            return entry ? entry->str : nullptr;
        }


        [[nodiscard]]
        InternedKey findInterned(const char* str) const noexcept
        {
            const Entry* entry = findEntryByContent(str);
            return entry ? entry->str : nullptr;
        }


        [[nodiscard]]
        InternedKey findInterned(std::string_view value) const noexcept
        {
            return findInterned(ByteSpan(
                reinterpret_cast<const uint8_t*>(value.data()),
                value.size()));
        }


        // ====================================================================
        // Pointer query
        // ====================================================================

        [[nodiscard]]
        InternedKey findInternedByPtr(InternedKey key) const noexcept
        {
            const Entry* entry = findEntryByPtr(key);
            return entry ? entry->str : nullptr;
        }


        // ====================================================================
        // Removal by canonical pointer
        //
        // For ordinary per-instance WSNameSet use.
        //
        // Do NOT remove names from the global singleton while other objects
        // retain InternedKey values.
        // ====================================================================

        bool removeInterned(InternedKey key) noexcept
        {
            Entry* victim = findEntryByPtr(key);

            if (!victim)
                return false;

            const size_t mask = fCapacity - 1;
            size_t hole = static_cast<size_t>(victim - fEntries);

            delete[] victim->str;

            victim->str = nullptr;
            victim->contentHash = 0;
            victim->length = 0;

            --fSize;

            size_t scan = hole;

            for (;;)
            {
                scan = (scan + 1) & mask;

                Entry& entry = fEntries[scan];

                if (!entry.str)
                    return true;

                const size_t ideal = hashStringPtr(entry.str) & mask;

                const bool mustMove =
                    (hole <= scan)
                    ? (ideal <= hole || ideal > scan)
                    : (ideal <= hole && ideal > scan);

                if (!mustMove)
                    continue;

                fEntries[hole] = entry;

                entry.str = nullptr;
                entry.contentHash = 0;
                entry.length = 0;

                hole = scan;
            }
        }


        // ====================================================================
        // Removal by contents
        // ====================================================================

        bool removeName(const ByteSpan& span) noexcept
        {
            const Entry* entry = findEntryByContent(span);

            if (!entry)
                return false;

            InternedKey key = entry->str;

            return removeInterned(key);
        }


        bool removeName(const char* str) noexcept
        {
            return str ? removeName(ByteSpan(str)) : false;
        }


        // ====================================================================
        // Statistics
        // ====================================================================

        [[nodiscard]]
        size_t size() const noexcept
        {
            return fSize;
        }


        [[nodiscard]]
        size_t capacity() const noexcept
        {
            return fCapacity;
        }


        [[nodiscard]]
        bool empty() const noexcept
        {
            return fSize == 0;
        }


        // ====================================================================
        // clear
        // ====================================================================

        void clear() noexcept
        {
            if (!fEntries)
                return;

            for (size_t i = 0; i < fCapacity; ++i)
            {
                Entry& entry = fEntries[i];

                if (!entry.str)
                    continue;

                delete[] entry.str;

                entry.str = nullptr;
                entry.contentHash = 0;
                entry.length = 0;
            }

            fSize = 0;
        }


        bool reset(size_t cap = kInitialCapacity) noexcept
        {
            return init(cap);
        }


        // ====================================================================
        // Iteration
        // ====================================================================

        template <typename Fn>
        void forEach(Fn&& fn) const noexcept
        {
            if (!fEntries)
                return;

            for (size_t i = 0; i < fCapacity; ++i)
            {
                if (fEntries[i].str)
                    fn(fEntries[i].str);
            }
        }


        template <typename Fn>
        bool forEachWhile(Fn&& fn) const noexcept
        {
            if (!fEntries)
                return true;

            for (size_t i = 0; i < fCapacity; ++i)
            {
                if (fEntries[i].str && !fn(fEntries[i].str))
                    return false;
            }

            return true;
        }


        // ====================================================================
        // Singleton
        //
        // Header-only C++17 safe.
        //
        // Every static convenience function funnels through this single
        // instance.
        // ====================================================================

        static WSNameSet* getSingletonTable() noexcept
        {
            static WSNameSet table;
            return &table;
        }


        // ====================================================================
        // Static canonicalization API
        // ====================================================================

        static InternedKey INTERN(const ByteSpan& span) noexcept
        {
            return getSingletonTable()->intern(span);
        }


        static InternedKey INTERN(const char* str) noexcept
        {
            return str ? getSingletonTable()->intern(str) : nullptr;
        }


        static InternedKey INTERN(std::string_view value) noexcept
        {
            return getSingletonTable()->intern(value);
        }


        static InternedKey FIND(const ByteSpan& span) noexcept
        {
            return getSingletonTable()->findInterned(span);
        }


        static InternedKey FIND(const char* str) noexcept
        {
            return str ? getSingletonTable()->findInterned(str) : nullptr;
        }


        static bool HAS(const ByteSpan& span) noexcept
        {
            return getSingletonTable()->hasName(span);
        }


        static bool HAS(const char* str) noexcept
        {
            return str ? getSingletonTable()->hasName(str) : false;
        }


        // ====================================================================
        // Case-insensitive interning
        //
        // Temporary storage is explicitly nothrow.
        // ====================================================================

        static InternedKey INTERN_CI(const ByteSpan& span) noexcept
        {
            if (span.empty())
                return nullptr;

            char* temp = new (std::nothrow) char[span.size()];

            if (!temp)
                return nullptr;

            for (size_t i = 0; i < span.size(); ++i)
            {
                uint8_t ch = span[i];

                if (ch >= 'A' && ch <= 'Z')
                    ch = static_cast<uint8_t>(ch - 'A' + 'a');

                temp[i] = static_cast<char>(ch);
            }

            ByteSpan lowered(
                reinterpret_cast<const uint8_t*>(temp),
                span.size());

            InternedKey result = INTERN(lowered);

            delete[] temp;

            return result;
        }


    private:
        void moveFrom(WSNameSet& other) noexcept
        {
            fEntries = other.fEntries;
            fCapacity = other.fCapacity;
            fSize = other.fSize;

            other.fEntries = nullptr;
            other.fCapacity = 0;
            other.fSize = 0;
        }
    };

} // namespace waavs
