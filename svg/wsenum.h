#pragma once

#include <unordered_map>
#include "bspan.h"

namespace waavs {
	using WSEnum = std::unordered_map<ByteSpan, uint32_t, ByteSpanHash, ByteSpanEquivalent>;

	static INLINE bool getEnumValue(WSEnum enumMap, const ByteSpan& key, uint32_t& value) noexcept
	{
		auto it = enumMap.find(key);
		if (it == enumMap.end())
			return false;
		value = it->second;
		return true;
	}

	// Return the key that corresponds to the specified value
	static INLINE bool getEnumKey(WSEnum enumMap, uint32_t value, ByteSpan& key) noexcept
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