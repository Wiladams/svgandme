#pragma once

#include <cmath>
#include <cstdint>

#include "converters.h"
#include "xmlschema.h"



namespace waavs {

	/*
		SVG Clock-value, associated with animation timing
		Reference: https://www.w3.org/TR/SMIL/smil-timing.html#clock-value

		Clock-value         ::= ( Full-clock-value | Partial-clock-value | Timecount-value )
		Full-clock-value    ::= Hours ":" Minutes ":" Seconds ("." Fraction)?
		Partial-clock-value ::= Minutes ":" Seconds ("." Fraction)?
		Timecount-value     ::= Timecount ("." Fraction)? (Metric)?
		Metric              ::= "h" | "min" | "s" | "ms"
		Hours               ::= DIGIT+; any positive number
		Minutes             ::= 2DIGIT; range from 00 to 59
		Seconds             ::= 2DIGIT; range from 00 to 59
		Fraction            ::= DIGIT+
		Timecount           ::= DIGIT+
		2DIGIT              ::= DIGIT DIGIT
		DIGIT               ::= [0-9]
	*/

	// Enumeration of valid metric types for SMIL animation clock durations
	enum class AnimMetricType {
		TIME_METRIC_HOURS,
		TIME_METRIC_MINUTES,
		TIME_METRIC_SECONDS,
		TIME_METRIC_MILLISECONDS,
		TIME_METRIC_NONE
	};

	// parse SMIL animation clock value
	// returns 'true' if successful, and outSecs contains
	// the clock value converted to seconds

	// readTwoDigits()
	// Reads exactly two digits.  
	// Will only return 'true' if at least two digits are present
	// if there are fewer, it will return 'false'
	// A higher level scanner will need to deal with the fact that
	// there might be more digits after the two digits have been read
	static inline bool readTwoDigits(ByteSpan& b, double& val)
	{
		uint64_t digits{ 0 };
		if (!read_required_digits(b, digits, 2))
			return false;

		val = digits;

		return true;
	}



	// parseMetric - Extracts the metric type but does not apply scaling
	static bool parseMetric(ByteSpan& span, AnimMetricType& metricType)
	{
		metricType = AnimMetricType::TIME_METRIC_NONE;

		if (!span) {
			return true;  // **Blank input is valid, meaning no metric**
		}

		// ms, min
		if (span.size() >= 2 && span[0] == 'm') {
			if (span[1] == 's' && span.size() == 2) {  // Must be exactly "ms"
				metricType = AnimMetricType::TIME_METRIC_MILLISECONDS;
				span += 2;  // Consume "ms"
			}
			else if (span.size() == 3 && span[1] == 'i' && span[2] == 'n') {  // Must be exactly "min"
				metricType = AnimMetricType::TIME_METRIC_MINUTES;
				span += 3;  // Consume "min"
			}
			else {
				return false;  // **Invalid metric (e.g., "mss", "max", "mins")**
			}
		}
		else if (span.size() == 1) {
			if (span[0] == 'h') {
				metricType = AnimMetricType::TIME_METRIC_HOURS;
				span++;  // Consume 'h'
			}
			else if (span[0] == 's') {
				metricType = AnimMetricType::TIME_METRIC_SECONDS;
				span++;  // Consume 's'
			}
			else {
				return false;  // **Invalid single-character metric**
			}
		}
		else {
			return false;  // **Invalid metric**
		}

		// Ensure no extra characters after the metric
		return span.size() == 0;
	}

	//
	// scaledSeconds()
	// 
	// Convert a value to seconds, according to the kind of scale
	// the value represents.
	// 
	static double scaledSeconds(double inValue, AnimMetricType metricType)
	{
		switch (metricType) {
		case AnimMetricType::TIME_METRIC_HOURS: return inValue * 3600.0;
		case AnimMetricType::TIME_METRIC_MINUTES: return inValue * 60.0;
		case AnimMetricType::TIME_METRIC_MILLISECONDS: return inValue * 0.001;
		case AnimMetricType::TIME_METRIC_SECONDS:
		case AnimMetricType::TIME_METRIC_NONE: return inValue;  // No scaling needed for seconds or missing metric
		}
		return inValue;
	}

	// readTimeComponent()
	//
	// Reads a time component according to the specified metricType
	// advances the ByteSpan past the last character read
	//
	static bool readTimeComponent(ByteSpan& bs, double& outSeconds, AnimMetricType metricType)
	{
		ByteSpan s = bs;
		double value = 0;
		bool allowFraction = false;
		bool twoDigitsRequired = false;
		int minVal = 0, maxVal = -1; // -1 means unrestricted

		// Set constraints based on the metric type
		switch (metricType) {
		case AnimMetricType::TIME_METRIC_HOURS:
			allowFraction = false;
			twoDigitsRequired = false;
			break;
		case AnimMetricType::TIME_METRIC_MINUTES:
			allowFraction = false;
			twoDigitsRequired = true;
			minVal = 0; maxVal = 59;
			break;
		case AnimMetricType::TIME_METRIC_SECONDS:
			allowFraction = true;
			twoDigitsRequired = true;
			minVal = 0; maxVal = 59;
			break;
		default:
			return false; // Invalid type
		}

		// Use `readTwoDigits()` for MM and SS, otherwise `read_u64()`
		if (twoDigitsRequired) {
			if (!readTwoDigits(s, value))
				return false;
		}
		else {
			uint64_t myint{ 0 };
			if (!read_u64(s, myint))
				return false;
			value = myint;
		}

		// Validate range (for MM and SS)
		if (maxVal != -1 && (value < minVal || value > maxVal))
			return false;

		double fraction = 0.0;

		// Allow fraction --> only for seconds <--
		if (allowFraction && s.size() > 0 && s[0] == '.') {
			s++;  // Skip '.'
			uint64_t fracPart = 0, fracBase = 1;

			// Read fractional digits manually
			while (s.size() > 0 && is_digit(s[0])) {
				fracPart = fracPart * 10 + (s[0] - '0');
				fracBase *= 10;
				s++;
			}

			fraction = static_cast<double>(fracPart) / static_cast<double>(fracBase);
		}
		else if (!allowFraction && s.size() > 0 && s[0] == '.') {
			return false;  // **Invalid: Decimal not allowed for this component**
		}

		// Convert to seconds
		outSeconds = scaledSeconds(value + fraction, metricType);
		bs = s;  // Advance the ByteSpan

		return true;
	}


	static bool parseFullClockValue(ByteSpan& part1, ByteSpan& part2, ByteSpan& part3, double& outSecs)
	{
		double hours, minutes, seconds;
		if (!readTimeComponent(part1, hours, AnimMetricType::TIME_METRIC_HOURS)) return false;
		if (!readTimeComponent(part2, minutes, AnimMetricType::TIME_METRIC_MINUTES)) return false;
		if (!readTimeComponent(part3, seconds, AnimMetricType::TIME_METRIC_SECONDS)) return false;

		outSecs = hours + minutes + seconds;
		return true;
	}

	static bool parsePartialClockValue(ByteSpan& part1, ByteSpan& part2, double& outSecs)
	{
		double minutes, seconds;
		if (!readTimeComponent(part1, minutes, AnimMetricType::TIME_METRIC_MINUTES)) return false;
		if (!readTimeComponent(part2, seconds, AnimMetricType::TIME_METRIC_SECONDS)) return false;

		outSecs = minutes + seconds;
		return true;
	}

	static bool parseTimecountValue(ByteSpan& part1, double& outSecs)
	{
		double value = 0;

		// Read the full number (integer + optional fraction)
		if (!readNumber(part1, value)) return false;

		// Try parsing a metric (optional)
		AnimMetricType metricType = AnimMetricType::TIME_METRIC_NONE;
		if (part1.size() > 0 && !parseMetric(part1, metricType)) {
			return false;  // **Reject invalid metric**
		}

		// If there is additional input after the metric
		// then we have a formatting error
		if (part1.size() > 0) return false;

		// Apply the correct scaling based on the parsed metric
		outSecs = scaledSeconds(value, metricType);
		return true;
	}


	// parseClockDuration()
	// Parse a SMIL Animation clock value
	// Returns 'true' if successful, and 'outSecs' contains
	// the clock value converted to seconds
	//	'false' - if there are any formatting or constraint violations
	//
	static bool parseClockDuration(const ByteSpan& bs, double& outSecs)
	{
		ByteSpan s = bs;
		int colonCount = 0;
		outSecs = 0;

		// Step 1: Reject negative values
		if (s.size() > 0 && s[0] == '-') {
			return false;  // **Negative values are invalid**
		}

		// Step 2: Count colons (:) and split input into tokens
		ByteSpan part1 = chunk_token_char(s, ':');
		ByteSpan part2 = chunk_token_char(s, ':');
		ByteSpan part3 = s;  // Remaining data

		if (!part2.empty()) colonCount++;  // If part2 is non-empty, at least one `:`
		if (!part3.empty()) colonCount++;  // If part3 is non-empty, two `:`

		switch (colonCount) {
		case 2: return parseFullClockValue(part1, part2, part3, outSecs);
		case 1: return parsePartialClockValue(part1, part2, outSecs);
		case 0: return parseTimecountValue(part1, outSecs);
		default: return false; // Should never happen, so indicates invlid input
		}

		return true;
	}
}

namespace waavs {

	// Parsing animation clock values
	// Rules
	//	1) Strip any leading, trailing, or intervening white space characters.
	//	2) If the value begins with a number or numeric sign indicator(i.e. '+' or '-'), the value should be parsed as an offset value.
	//	3) Else if the value begins with the unescaped token "wallclock", it should be parsed as a Wallclock - sync - value.
	//	4) Else if the value is the unescaped token "indefinite", it should be parsed as the value "indefinite".
	//	5) Else: Build a token substring up to but not including any sign indicator(i.e.strip off any offset, parse that 
	//		separately, and add it to the result of this step).In the following, any '.' characters preceded by a reverse 
	//		solidus '\' escape character should not be treated as a separator, but as a normal token character.
	//		1) If the token contains no '.' separator character, then the value should be parsed as an Event - 
	//			value with an unspecified(i.e. default) eventbase - element.
	//		2) Else if the token ends with the unescaped string ".begin" or ".end", then the value should be parsed as a Syncbase - value.
	//		3) Else if the token contains the unescaped string ".marker(", then the value should be parsed as a Media - Marker - value.
	//		4) Else, the value should be parsed as an Event - value(with a specified eventbase - element).
	//
	enum TimeSpecifierKind
	{
		TIME_SPEC_NONE,
		TIME_SPEC_OFFSET,
		TIME_SPEC_WALL_CLOCK,
		TIME_SPEC_INDEFINITE,
		TIME_SPEC_EVENT,
	};

	// parseOffsetValue()
	// 
	// Offset-value   ::= ( S? ("+" | "-") S? )? ( Clock-value )
	// 
		// The leading whitespace has already been stripped off
	bool parseOffsetValue(const ByteSpan& bs, double& offset)
	{
		ByteSpan s = bs;
		if (s.empty())
			return false;

		bool isNegative = false;
		if (*s == '-')
		{
			isNegative = true;
			s++;
			chunk_ltrim(s, xmlwsp);
		}

		if (!parseClockDuration(s, offset))
			return false;

		if (isNegative)
			offset = -offset;

		return true;
	}

	bool parseTimingSpecifier(const ByteSpan& bs, int& kind)
	{
		ByteSpan s = bs;
		s = chunk_ltrim(s, xmlwsp);
		if (s.size() == 0)
			return false;

		if (is_digit(*s) || *s == '+' || *s == '-')
		{
			// Check for offset value
			double offset = 0;
			if (parseOffsetValue(s, offset)) {
				kind = TIME_SPEC_OFFSET; // Offset value
				return true;
			}
		}

		// Check for wallclock
		if (s.startsWith("wallclock")) {
			// parseWallClockSyncValue()
			kind = TIME_SPEC_WALL_CLOCK; // Wallclock
			return true;
		}

		// Check for indefinite
		if (s.startsWith("indefinite")) {
			kind = TIME_SPEC_INDEFINITE; // Indefinite
			return true;
		}

		// Create a 
		// Check for event value
		if (s.startsWith("event")) {
			kind = TIME_SPEC_EVENT; // Event value
			return true;
		}

		return false; // Unknown specifier
	}
}

namespace waavs {

	// eBNF for event-value
	// 
	// Event-value       ::= ( Eventbase-element "." )? Event-symbol
	//						(S ? ("+" | "-") S ? Clock-value) ?
	// Eventbase-element :: = Id-value
	// Event-symbol      :: = Nmtoken
	/*
		ByteSpan& outBase,     // optional
		ByteSpan& outSymbol,   // required
		int& sign,             // 0 = no offset, +1/-1 = present
		double& clockValue     // if sign != 0

	*/
	static bool parseEventValue(const ByteSpan& inChunk, ByteSpan& outBase,ByteSpan& outSymbol,
		int& sign, double& clockValue) noexcept
	{
		// Initialize
		outBase = ByteSpan();
		outSymbol = ByteSpan();
		sign = 0;
		clockValue = 0.0;

		ByteSpan span = chunk_trim(inChunk, chrWspChars);
		if (span.size() == 0)
			return false;

		// Try to split on the first '.'
		const uint8_t* dot = (const uint8_t*)memchr(span.begin(), '.', span.size());

		if (dot)
		{
			ByteSpan base(span.begin(), dot);
			if (!parseXsdNCName(base, outBase))
				return false;

			span = ByteSpan(dot + 1, span.end());  // remainder after '.'
		}

		// Now: symbol [ (+|-) clock ]
		span = chunk_ltrim(span, chrWspChars);

		ByteSpan symPart, rest;
		if (!chunk_token_char(span, ' ', symPart, rest)) {
			// no whitespace => entire thing is probably symbol
			symPart = span;
			rest = ByteSpan();
		}

		// Try to isolate symbol and remainder
		ByteSpan symbolCandidate, offsetCandidate;
		chunk_token_char(symPart, '+', symbolCandidate, offsetCandidate)
			|| chunk_token_char(symPart, '-', symbolCandidate, offsetCandidate);

		if (symbolCandidate.size() == 0)
			symbolCandidate = symPart;

		if (!parseXsdNmtoken(symbolCandidate, outSymbol))
			return false;

		// Process optional time offset
		ByteSpan offsetInput;

		if (offsetCandidate.size())
		{
			sign = symPart[symbolCandidate.size()] == '+' ? +1 : -1;
			offsetInput = offsetCandidate;
		}
		else
		{
			// Check if offset exists in rest (with whitespace between symbol and + / -)
			rest = chunk_ltrim(rest, chrWspChars);
			if (rest.size() && (*rest.begin() == '+' || *rest.begin() == '-'))
			{
				sign = (*rest.begin() == '+') ? +1 : -1;
				offsetInput = ByteSpan(rest.begin() + 1, rest.end());
			}
		}

		if (sign != 0)
		{
			offsetInput = chunk_ltrim(offsetInput, chrWspChars);
			if (!parseClockDuration(offsetInput, clockValue))
				return false;
		}

		return true;
	}

}
