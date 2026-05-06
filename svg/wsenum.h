#pragma once

#include <unordered_map>
#include "bspan.h"

namespace waavs {
    template<typename EnumT>
    struct BitFlags
    {
        using Underlying = std::underlying_type_t<EnumT>;

        Underlying value = 0;

        constexpr BitFlags() = default;
        constexpr BitFlags(EnumT e) : value((Underlying)e) {}

        constexpr bool has(EnumT e) const noexcept
        {
            return (value & (Underlying)e) != 0;
        }

        constexpr void add(EnumT e) noexcept
        {
            value |= (Underlying)e;
        }

        constexpr void remove(EnumT e) noexcept
        {
            value &= ~(Underlying)e;
        }

        constexpr BitFlags operator|(EnumT e) const noexcept
        {
            BitFlags out = *this;
            out.add(e);
            return out;
        }

        constexpr BitFlags operator&(EnumT e) const noexcept
        {
            BitFlags out;
            out.value = value & (Underlying)e;
            return out;
        }

        constexpr BitFlags operator~() const noexcept
        {
            BitFlags out;
            out.value = ~value;
            return out;
        }
    };

    // support for EnumT | EnumT to produce a BitFlags<EnumT>
	template<typename EnumT>
	constexpr BitFlags<EnumT> operator|(EnumT a, EnumT b) noexcept
	{
		return BitFlags<EnumT>(a) | b;
	}

    // ------------------------------
    //
	using WSEnum = std::unordered_map<ByteSpan, uint32_t, ByteSpanHash, ByteSpanEquivalent>;

	static INLINE bool getEnumValue(const WSEnum &enumMap, const ByteSpan& key, uint32_t& value) noexcept
	{
		auto it = enumMap.find(key);
		if (it == enumMap.end())
			return false;
		value = it->second;
		return true;
	}

	// Return the key that corresponds to the specified value
	static INLINE bool getEnumKey(const WSEnum &enumMap, uint32_t value, ByteSpan& key) noexcept
	{
		for (auto it = enumMap.begin(); it != enumMap.end(); ++it)
		{
			if (it->second == value)
			{
				key = it->first;
				return true;
			}
		}
		return false;
	}
}