#pragma once

#include <cmath>
#include <cstdint>

#include "svgstructuretypes.h"
#include "wsenum.h"

namespace waavs {

	/*
		Clock-value         ::= ( Full-clock-value | Partial-clock-value
						  | Timecount-value )
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


	// parse SMIL animation clock value
	// returns 'true' if successful, and outSecs contains
	// the clock value converted to seconds

	// readTwoDigits()
	// Reads exactly two digits.  
	// Will only return 'true' if at least two digits are present
	// if there are fewer, it will return 'false'
	// A higher level scanner will need to deal with the fact that
	// there are more digits after the two digits have been read
	static inline bool readTwoDigits(ByteSpan& b, double& val)
	{
		uint64_t digits{ 0 };
		if (!read_required_digits(b, digits, 2))
			return false;

		val = digits;

		return true;
	}
	
	// Enumeration of valid metric types for SMIL animation clock durations
	enum class AnimMetricType {
		HOURS,
		MINUTES,
		SECONDS,
		MILLISECONDS,
		NONE
	};

	// parseMetric - Extracts the metric type but does not apply scaling
	static bool parseMetric(ByteSpan& span, AnimMetricType& metricType)
	{
		metricType = AnimMetricType::NONE;

		if (!span) {
			return true;  // **Blank input is valid, meaning no metric**
		}

		// ms, min
		if (span.size() >= 2 && span[0] == 'm') {
			if (span[1] == 's' && span.size() == 2) {  // Must be exactly "ms"
				metricType = AnimMetricType::MILLISECONDS;
				span += 2;  // Consume "ms"
			}
			else if (span.size() == 3 && span[1] == 'i' && span[2] == 'n') {  // Must be exactly "min"
				metricType = AnimMetricType::MINUTES;
				span += 3;  // Consume "min"
			}
			else {
				return false;  // **Invalid metric (e.g., "mss", "max", "mins")**
			}
		}
		else if (span.size() == 1) {
			if (span[0] == 'h') {
				metricType = AnimMetricType::HOURS;
				span++;  // Consume 'h'
			}
			else if (span[0] == 's') {
				metricType = AnimMetricType::SECONDS;
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
	// scaledSeconds - Convert a value to seconds, scaling as needed
	// 
	static double scaledSeconds(double inValue, AnimMetricType metricType)
	{
		switch (metricType) {
		case AnimMetricType::HOURS: return inValue * 3600.0;
		case AnimMetricType::MINUTES: return inValue * 60.0;
		case AnimMetricType::MILLISECONDS: return inValue * 0.001;
		case AnimMetricType::SECONDS:
		case AnimMetricType::NONE: return inValue;  // No scaling needed for seconds or missing metric
		}
		return inValue;
	}


	static bool readTimeComponent(ByteSpan& bs, double& outSeconds, AnimMetricType metricType)
	{
		ByteSpan s = bs;
		double value = 0;
		bool allowFraction = false;
		bool requireTwoDigits = false;
		int minVal = 0, maxVal = -1; // -1 means unrestricted

		// Set constraints based on the metric type
		switch (metricType) {
		case AnimMetricType::HOURS:
			allowFraction = false;
			requireTwoDigits = false;
			break;
		case AnimMetricType::MINUTES:
			allowFraction = false;
			requireTwoDigits = true;
			minVal = 0; maxVal = 59;
			break;
		case AnimMetricType::SECONDS:
			allowFraction = true;
			requireTwoDigits = true;
			minVal = 0; maxVal = 59;
			break;
		default:
			return false; // Invalid type
		}

		// Use `readTwoDigits()` for MM and SS, otherwise `read_u64()`
		if (requireTwoDigits) {
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

		// Allow fraction **only for seconds**
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
		if (!readTimeComponent(part1, hours, AnimMetricType::HOURS)) return false;
		if (!readTimeComponent(part2, minutes, AnimMetricType::MINUTES)) return false;
		if (!readTimeComponent(part3, seconds, AnimMetricType::SECONDS)) return false;

		outSecs = hours + minutes + seconds;
		return true;
	}

	static bool parsePartialClockValue(ByteSpan& part1, ByteSpan& part2, double& outSecs) 
	{
		double minutes, seconds;
		if (!readTimeComponent(part1, minutes, AnimMetricType::MINUTES)) return false;
		if (!readTimeComponent(part2, seconds, AnimMetricType::SECONDS)) return false;

		outSecs = minutes + seconds;
		return true;
	}

	static bool parseTimecountValue(ByteSpan& part1, double& outSecs) 
	{
		double value = 0;

		// Read the full number (integer + optional fraction)
		if (!readNumber(part1, value)) return false;

		// Try parsing a metric (optional)
		AnimMetricType metricType = AnimMetricType::NONE;
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


	// parseClockValue()
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

		if (part2) colonCount++;  // If part2 is non-empty, at least one `:`
		if (part3) colonCount++;  // If part3 is non-empty, two `:`

		switch (colonCount) {
			case 2: return parseFullClockValue(part1, part2, part3, outSecs);
			case 1: return parsePartialClockValue(part1, part2, outSecs);
			case 0: return parseTimecountValue(part1, outSecs);
			default: return false; // Should never happen
		}

		return true;
	}




}


namespace waavs {
	enum AnimRestartKind : uint32_t {
		SVG_ANIM_RESTART_ALWAYS,
		SVG_ANIM_RESTART_NEVER,
		SVG_ANIM_RESTART_WHEN_NOT_ACTIVE
	};

	static WSEnum SVGAnimRestart = {
		{ "always", AnimRestartKind::SVG_ANIM_RESTART_ALWAYS },
		{ "never", AnimRestartKind::SVG_ANIM_RESTART_NEVER },
		{ "whenNotActive", AnimRestartKind::SVG_ANIM_RESTART_WHEN_NOT_ACTIVE }
	};

	enum AnimFillKind : uint32_t {
		SVG_ANIM_FILL_REMOVE,
		SVG_ANIM_FILL_FREEZE
	};

	static WSEnum SVGAnimFill = {
		{ "remove", AnimFillKind::SVG_ANIM_FILL_REMOVE },
		{ "freeze", AnimFillKind::SVG_ANIM_FILL_FREEZE }
	};

	enum AnimAdditiveKind : uint32_t {
		SVG_ANIM_ADD_REPLACE,
		SVG_ANIM_ADD_SUM
	};

	static WSEnum SVGAnimAdditive = {
		{ "replace", AnimAdditiveKind::SVG_ANIM_ADD_REPLACE },
		{ "sum", AnimAdditiveKind::SVG_ANIM_ADD_SUM }
	};

	enum AnimAccumulateKind : uint32_t {
		SVG_ANIM_ACCUM_NONE,
		SVG_ANIM_ACCUM_SUM
	};

	static WSEnum SVGAnimAccumulate = {
		{ "none", AnimAccumulateKind::SVG_ANIM_ACCUM_NONE },
		{ "sum", AnimAccumulateKind::SVG_ANIM_ACCUM_SUM }
	};

	enum AnimCalcModeKind : uint32_t {
		SVG_ANIM_CALC_DISCRETE,
		SVG_ANIM_CALC_LINEAR,
		SVG_ANIM_CALC_PACED,
		SVG_ANIM_CALC_SPLINE
	};

	static WSEnum SVGAnimCalcMode = {
		{ "discrete", AnimCalcModeKind::SVG_ANIM_CALC_DISCRETE },
		{ "linear", AnimCalcModeKind::SVG_ANIM_CALC_LINEAR },
		{ "paced", AnimCalcModeKind::SVG_ANIM_CALC_PACED },
		{ "spline", AnimCalcModeKind::SVG_ANIM_CALC_SPLINE }
	};

	enum AnimAttributeTypeKind : uint32_t {
		SVG_ANIM_ATTR_TYPE_AUTO,
		SVG_ANIM_ATTR_TYPE_CSS,
		SVG_ANIM_ATTR_TYPE_XML
	};

	static WSEnum SVGAnimAttributeType = {
		{ "auto", AnimAttributeTypeKind::SVG_ANIM_ATTR_TYPE_AUTO },
		{ "css", AnimAttributeTypeKind::SVG_ANIM_ATTR_TYPE_CSS },
		{ "xml", AnimAttributeTypeKind::SVG_ANIM_ATTR_TYPE_XML }
	};




	struct SVGAnimateElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			getSVGSingularCreationMap()["animate"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGAnimateElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			getSVGContainerCreationMap()["animate"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGAnimateElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				node->visible(false);

				return node;
				};

			registerSingularNode();
		}

		AnimFillKind fAnimFill{ AnimFillKind::SVG_ANIM_FILL_REMOVE };
		AnimRestartKind fAnimRestart{ AnimRestartKind::SVG_ANIM_RESTART_ALWAYS };
		AnimAdditiveKind fAnimAdditive{ AnimAdditiveKind::SVG_ANIM_ADD_REPLACE };
		AnimAccumulateKind fAnimAccumulate{ AnimAccumulateKind::SVG_ANIM_ACCUM_NONE };
		AnimCalcModeKind fAnimCalcMode{ AnimCalcModeKind::SVG_ANIM_CALC_LINEAR };
		AnimAttributeTypeKind fAnimAttributeType{ AnimAttributeTypeKind::SVG_ANIM_ATTR_TYPE_AUTO };


		SVGAnimateElement(IAmGroot*)
			: SVGGraphicsElement()
		{
			isStructural(true);
		}

		// Attributes
		// %timingAttrs
		//	begin
		//	dur
		//	end
		//	restart (always|never|whenNotActive) "always"
		//	repeatCount
		//	repeatDur
		//	fill (remove|freeze) "remove"
		//
		// %animAttrs
		//	attributeName
		//	attributeType
		//	additive (replace|sum) "replace"
		//	accumulate (none|sum) "none"
		//
		//	calcMode (discrete|linear|paced|spline) "linear"
		//	values
		//	keyTimes
		//	keySplines
		//	from
		//	to
		//	by
		//
		// %animTargetAttrs
		//	targetElement	IDREF	#IMPLIED

		// Read in all the attributes for the animation
		void fixupSelfStyleAttributes(IRenderSVG*, IAmGroot*) override
		{
			getEnumValue(SVGAnimFill, getAttribute("fill"), (uint32_t&)fAnimFill);
			getEnumValue(SVGAnimRestart, getAttribute("restart"), (uint32_t&)fAnimRestart);
			getEnumValue(SVGAnimAdditive, getAttribute("additive"), (uint32_t&)fAnimAdditive);
			getEnumValue(SVGAnimAccumulate, getAttribute("accumulate"), (uint32_t&)fAnimAccumulate);
			getEnumValue(SVGAnimCalcMode, getAttribute("calcMode"), (uint32_t&)fAnimCalcMode);
			getEnumValue(SVGAnimAttributeType, getAttribute("attributeType"), (uint32_t&)fAnimAttributeType);

		}
	};
}
