#pragma once

#include "bspan.h"

namespace waavs {
	
	static inline ByteSpan chunk_subchunk(const ByteSpan& a, const size_t start, const size_t sz) noexcept;
	static inline ByteSpan chunk_take(const ByteSpan& dc, size_t n) noexcept;

	
// Create a bytespan that is a subspan of another bytespan
	static inline ByteSpan chunk_subchunk(const ByteSpan& a, const size_t startAt, const size_t sz) noexcept
	{
		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;
		if (startAt < chunk_size(a))
		{
			start += startAt;
			if (start + sz < end)
				end = start + sz;
			else
				end = a.fEnd;
		}
		else
		{
			start = end;
		}
		return { start, end };
	}

	
static inline ByteSpan chunk_take(const ByteSpan& dc, size_t n) noexcept
{
	return chunk_subchunk(dc, 0, n);
}
}