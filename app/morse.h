#pragma once

#include <vector>
#include <unordered_map>
#include <cctype>
#include <cstring>

namespace waavs 
{
	size_t createMorseCode(const char* src, std::vector<double>& out, double dotDuration = 1.0) {
		static const std::unordered_map<char, const char*> morseMap = {
			{'A', ".-"},    {'B', "-..."},  {'C', "-.-."},  {'D', "-.."},
			{'E', "."},     {'F', "..-."},  {'G', "--."},   {'H', "...."},
			{'I', ".."},    {'J', ".---"},  {'K', "-.-"},   {'L', ".-.."},
			{'M', "--"},    {'N', "-."},    {'O', "---"},   {'P', ".--."},
			{'Q', "--.-"},  {'R', ".-."},   {'S', "..."},   {'T', "-"},
			{'U', "..-"},   {'V', "...-"},  {'W', ".--"},   {'X', "-..-"},
			{'Y', "-.--"},  {'Z', "--.."},
			{'0', "-----"}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"},
			{'4', "....-"}, {'5', "....."}, {'6', "-...."}, {'7', "--..."},
			{'8', "---.."}, {'9', "----."}
		};

		bool atWordStart = true;
		size_t count = 0;

		for (size_t i = 0; src[i]; ++i) {
			char ch = std::toupper(src[i]);

			if (ch == ' ') {
				if (!out.empty()) {
					// Extend last 'off' to represent inter-word space (7 units total)
					out.back() += dotDuration * 7;
				}
				atWordStart = true;
				continue;
			}

			auto it = morseMap.find(ch);
			if (it == morseMap.end()) continue;

			if (!atWordStart && !out.empty()) {
				// Extend last 'off' to 3 units (inter-character space)
				out.back() += dotDuration * 3;
			}

			const char* code = it->second;
			bool firstSymbol = true;
			for (size_t j = 0; code[j]; ++j) {
				if (!firstSymbol) {
					// Intra-character space (1 unit off)
					out.push_back(dotDuration); // off
				}

				if (code[j] == '.') {
					out.push_back(dotDuration); // on
				}
				else if (code[j] == '-') {
					out.push_back(dotDuration * 3); // on
				}

				// Followed by off-time unless it's the last symbol (handled in next)
				out.push_back(0); // placeholder for next off
				firstSymbol = false;
			}

			// Pop last dummy off if at end
			if (!out.empty() && out.back() == 0.0)
				out.pop_back();

			atWordStart = false;
			++count;
		}

		// Ensure even length (end with 'off' if needed)
		if (out.size() % 2 != 0)
			out.push_back(dotDuration * 7); // trailing silence

		return count;
	}
}
