#pragma once

#include <string>

#include "bspan.h"

namespace waavs {

	// return 1 if the chunk is "true" or "1" or "t" or "T" or "y" or "Y" or "yes" or "Yes" or "YES"
	// return 0 if the chunk is "false" or "0" or "f" or "F" or "n" or "N" or "no" or "No" or "NO"
	// return 0 otherwise
	static INLINE int toBoolInt(const ByteSpan& inChunk) noexcept
	{
		ByteSpan s = inChunk;

		if (s == "true" || s == "1" || s == "t" || s == "T" || s == "y" || s == "Y" || s == "yes" || s == "Yes" || s == "YES")
			return 1;
		else if (s == "false" || s == "0" || s == "f" || s == "F" || s == "n" || s == "N" || s == "no" || s == "No" || s == "NO")
			return 0;
		else
			return 0;
	}

	static INLINE std::string toString(const ByteSpan& inChunk) noexcept
	{
		if (!inChunk)
			return std::string();

		return std::string(inChunk.fStart, inChunk.fEnd);
	}

}