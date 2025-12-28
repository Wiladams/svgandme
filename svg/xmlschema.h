#pragma once

//#include "charset.h"
//#include "bspan.h"
#include "xmltoken.h"

// Forward declarations for convenience
namespace waavs 
{
	static bool isXsdName(const ByteSpan& inChunk) noexcept;
	static bool isXsdNCName(const ByteSpan& inChunk) noexcept;
	static bool isXsdNMToken(const ByteSpan& inChunk) noexcept;

	static bool parseXsdNmtoken(const ByteSpan& inChunk, ByteSpan& outValue) noexcept;
	static bool parseXsdName(const ByteSpan& inChunk, ByteSpan& outValue) noexcept;
	static bool parseXsdNCName(const ByteSpan& inChunk, ByteSpan& outValue) noexcept;

	static bool readXsdName(ByteSpan& src, ByteSpan& name) noexcept;

}

// These routines validate xml names
// They only support an ASCII subset, and not the full unicode
//
namespace waavs
{

	// Check if input is a valid ASCII xsd:Name (input already trimmed)
	static bool isXsdName(const ByteSpan& inChunk) noexcept
	{
		if (inChunk.size() == 0)
			return false;

		const uint8_t* c = inChunk.begin();
		const uint8_t* end = inChunk.end();

		if (!xmlNameStartChars(*c))
			return false;
		++c;

		while (c < end)
		{
			if (!xmlNameChars(*c))
				return false;
			++c;
		}

		return true;
	}




	// Check if input is a valid ASCII xsd:NCName (trimmed input)
	static bool isXsdNCName(const ByteSpan& inChunk) noexcept
	{
		if (inChunk.size() == 0)
			return false;

		const uint8_t* c = inChunk.begin();
		const uint8_t* end = inChunk.end();

		if (!xmlNameStartChars(*c))
			return false;
		++c;

		while (c < end)
		{
			if (!xmlNcnameChars(*c))
				return false;
			++c;
		}

		return true;
	}

	/*
The type xsd:NMTOKEN represents a single string token.
xsd:NMTOKEN values may consist of
	letters,
	digits,
	periods (.),
	hyphens (-),
	underscores (_),
	and colons (:).

They may start with any of these characters.
xsd:NMTOKEN has a whiteSpace facet value of collapse, so any leading or trailing
whitespace will be removed. However, no whitespace may appear within the value
itself.

*/

	// A NMTOKEN differs from Xsd:name in that the xsd:name has a restriction
	// on which characters can start the token.
	// Xsd:NMTOKEN is more generic as any of the namechars can be at the
	// beginning of the token as well.

	static bool isXsdNMToken(const ByteSpan& inChunk) noexcept
	{
		const uint8_t* c = inChunk.begin();
		const uint8_t* end = inChunk.end();

		for (; c < end; ++c)
		{
			if (!xmlNameChars(*c))
				return false;
		}

		return true;
	}
}


// Some things from XSD
namespace waavs 
{
	static bool parseXsdNmtoken(const ByteSpan& inChunk, ByteSpan& outValue) noexcept
	{
		outValue = chunk_trim(inChunk, chrWspChars);
		if (outValue.size() == 0)
			return false;

		return isXsdNMToken(outValue);
	}

	// Main entry for xsd:Name parser
	static bool parseXsdName(const ByteSpan& inChunk, ByteSpan& outValue) noexcept
	{
		outValue = chunk_trim(inChunk, chrWspChars);
		if (outValue.size() == 0)
			return false;

		return isXsdName(outValue);
	}

	// Main entry for xsd:NCName parser
	static bool parseXsdNCName(const ByteSpan& inChunk, ByteSpan& outValue) noexcept
	{
		outValue = chunk_trim(inChunk, chrWspChars);
		if (outValue.size() == 0)
			return false;

		return isXsdNCName(outValue);
	}

}

namespace waavs {

	// readXsdName()
	//
	// Read an Xsd name from a source, altering the source
	// to start right after the name.
	// Return the name
	// Return false, if there's an error
	//
	static bool readXsdName(ByteSpan& src, ByteSpan& name) noexcept
	{
		if (src.empty())
			return false;

		const uint8_t* c = src.begin();
		const uint8_t* end = src.end();

		if (!xmlNameStartChars(*c))
			return false;

		name.fStart = c;
		++c;

		while (c < end)
		{
			if (!xmlNameChars(*c))
				break;
			++c;
		}
		name.fEnd = c;
		src.fStart = c;

		return true;
	}
}