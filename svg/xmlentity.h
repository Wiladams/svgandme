#pragma once

#include "bspan.h"
#include "utf8.h"


namespace waavs {

	static size_t expandXmlEntities(const ByteSpan& inSpan, ByteSpan& outSpan) noexcept
	{
		ByteSpan s = inSpan;
		ByteSpan outCursor = outSpan;

		while (s)
		{
			if (*s == '&')
			{
				// skip past the initial ampersand, ready to decode the rest
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

}

