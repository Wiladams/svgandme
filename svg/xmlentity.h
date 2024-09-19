#pragma once

#include "bspan.h"
#include "utf8.h"

namespace waavs {
	// expandCharacterEntities()
	// 
	// Expand the 5 basic entities
	// &lt; &gt; &amp; &apos; &quot;
	// This routine does not handle any other kinds
	// of entities, such as the numeric kind.
	// The outputSpan is assumed to be big enough to contain
	// the expanded output will contain all the characters of the input
	//
	static size_t expandCharacterEntities(const ByteSpan& inSpan, ByteSpan& outSpan) noexcept
	{
		ByteSpan s = inSpan;
		ByteSpan outCursor = outSpan;

		while (s)
		{
			if (*s == '&')
			{
				// spip past the initial ampersand, ready to decode the rest
				s++;
				if (!s)
					break;

				// By taking the token up to the ';', we have already moved
				// the input cursor to the next character
				// so we're free to decode the content in isolation
				auto ent = chunk_token_char(s, ';');


				if (ent == "lt") {
					*outCursor = '<';
					++outCursor;
				}
				else if (ent == "gt") {
					*outCursor = '>';
					++outCursor;
				}
				else if (ent == "amp") {
					*outCursor = '&';
					++outCursor;
				}
				else if (ent == "apos") {
					*outCursor = '\'';
					++outCursor;
				}
				else if (ent == "quot") {
					*outCursor = '"';
					++outCursor;
				}

			}
			else
			{
				*outCursor = *s;
				outCursor++;
				s++;
			}


		}
		outSpan.fEnd = outCursor.fStart;

		return outSpan.size();
	}

}

namespace waavs {

	static size_t expandXmlEntities(const ByteSpan& inSpan, ByteSpan& outSpan) noexcept
	{
		ByteSpan s = inSpan;
		ByteSpan outCursor = outSpan;

		while (s)
		{
			if (*s == '&')
			{
				// spip past the initial ampersand, ready to decode the rest
				s++;
				if (!s)
					break;

				// By taking the token up to the ';', we have already moved
				// the input cursor to the next character
				// so we're free to decode the content in isolation
				auto ent = chunk_token_char(s, ';');

				if (*ent == '#') {
					char outBuff[10] = { 0 };
					size_t outLen{ 0 };
					uint64_t value = 0;
					
					// should be a numeric entity
					ent++;
					
					if (*ent == 'x') {
						// hex entity
						ent++;

						if (parseHex64u(ent, value)) {
							// convert the codepoint into a utf8 sequence
							// and insert that sequence into the outCursor
							if (convertUTF32ToUTF8(value, outBuff, outLen)) {
								outCursor.copyFrom(outBuff, outLen);
								outCursor += outLen;
							}
						}
					}
					else if (isdigit(*ent)) {
						// decimal entity

						if (parse64u(ent, value)) {
							// convert the codepoint into a utf8 sequence
							// and insert that sequence into the outCursor
							if (convertUTF32ToUTF8(value, outBuff, outLen)) {
								outCursor.copyFrom(outBuff, outLen);
								outCursor += outLen;
							}
						}

					} 

				
				}
				else {
					// Otherwise, process a standard character entity
					if (ent == "lt") {
						*outCursor = '<';
						++outCursor;
					}
					else if (ent == "gt") {
						*outCursor = '>';
						++outCursor;
					}
					else if (ent == "amp") {
						*outCursor = '&';
						++outCursor;
					}
					else if (ent == "apos") {
						*outCursor = '\'';
						++outCursor;
					}
					else if (ent == "quot") {
						*outCursor = '"';
						++outCursor;
					}
				}
			}
			else
			{
				*outCursor = *s;
				outCursor++;
				s++;
			}


		}
		outSpan.fEnd = outCursor.fStart;

		return outSpan.size();
	}
	/*
//
// expandStandardEntities()
// Do basic XML entity expansion
// BUGBUG - It should use a ByteSpan for output instead of
// a std::string.  Return value can be the size of the output
static std::string expandStandardEntities(const ByteSpan &inSpan) noexcept
{
	std::ostringstream oss;

	for (auto it = inSpan.begin(); it != inSpan.end(); ++it)
	{
		if (*it == '&')
		{
			auto next = it + 1;
			if (next == inSpan.end())
				break;

			if (*next == '#')
			{
				auto next2 = next + 1;
				if (next2 == inSpan.end())
					break;

				if (*next2 == 'x')
				{
					auto next3 = next2 + 1;
					if (next3 == inSpan.end())
						break;

					auto end = next3;
					while (end != inSpan.end() && isxdigit(*end))
						++end;

					if (end == inSpan.end())
						break;

					if (*end != ';')
						break;

					std::string hexStr(next3, end);
					int hexVal = std::stoi(hexStr, nullptr, 16);
					oss << (char)hexVal;

					it = end;
				}
				else
				{
					auto end = next2;
					while (end != inSpan.end() && isdigit(*end))
						++end;

					if (end == inSpan.end())
						break;

					if (*end != ';')
						break;

					std::string decStr(next2, end);
					int decVal = std::stoi(decStr, nullptr, 10);
					oss << (char)decVal;

					it = end;
				}
			}
			else
			{
				auto end = next;
				while (end != inSpan.end() && isalnum(*end))
					++end;

				if (end == inSpan.end())
					break;

				if (*end != ';')
					break;

				std::string entity(next, end);
				if (entity == "lt")
					oss << '<';
				else if (entity == "gt")
					oss << '>';
				else if (entity == "amp")
					oss << '&';
				else if (entity == "apos")
					oss << '\'';
				else if (entity == "quot")
					oss << '"';
				else
					break;

				it = end;
			}
		}
		else
		{
			if (*it != '\r' && *it != '\n') {
				oss << *it;
			}
		}
	}

	return oss.str();
}
*/
}

