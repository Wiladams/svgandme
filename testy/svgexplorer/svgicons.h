#pragma once

#include <unordered_map>

#include "bspan.h"


namespace waavs {

	using SVGDB = std::unordered_map<ByteSpan, ByteSpan, ByteSpanInsensitiveHash, ByteSpanCaseInsensitive>;
	
	// A little database of small svg string literals
	static ByteSpan getIconSpan(const ByteSpan& name) noexcept
	{
		static SVGDB icondb = {
			{"goDown", R"||(
				<svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 22 22">
					<path d="M14,11V5c0-1.105-0.895-2-2-2h-2C8.895,3,8,3.895,8,5v6H3l8,8l8-8H14z"/>
				</svg>
			)||"},

			{"goUp", R"||(
				<svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 22 22">
					<path d="M11,3l-8,8h5v6c0,1.105,0.895,2,2,2h2c1.105,0,2-0.895,2-2v-6h5L11,3z"/>
				</svg>
			)||"},

			{"goLeft", R"||(
				<svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 22 22">
					<path d="M17,8h-6V3l-8,8l8,8v-5h6c1.105,0,2-0.895,2-2v-2C19,8.895,18.105,8,17,8z"/>
				</svg>
			)||"},

			{"goRight", R"||(
				<svg xmlns="http://www.w3.org/2000/svg" width="100%" height="100%" viewBox="0 0 22 22">
					<path d="M5,8h6v5l8-8l-8-8v5H5c-1.105,0-2,0.895-2,2v2C3,7.105,3.895,8,5,8z"/>
				</svg>
			)||"},

			{"floppydisk", R"||(
				<svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 22 22">
					<path d="M6,21h10v-6H6V21z M16,4h-3v3h3V4z M19,1H3C1.895,1,1,1.895,1,3v16 c0,1.105,0.895,2,2,2h1v-6c0-1.105,0.895-2,2-2h10c1.105,0,2,0.895,2,2v6h1c1.105,0,2-0.895,2-2V3C21,1.895,20.105,1,19,1z M17,8 H5V3h12V8z"/>
				</svg>
			)||"},

			{"checkerboard", R"||(
	<svg width = "100%" height = "100%" xmlns = "http://www.w3.org/2000/svg">								
	<!--Define a checkerboard pattern-->															
	<defs>																							
	<pattern id = "checkerboard" width = "20" height = "20" patternUnits = "userSpaceOnUse">		
	<rect width = "10" height = "10" fill = "lightgray" />
	<rect x = "10" width = "10" height = "10" fill = "darkgray" />									
	<rect y = "10" width = "10" height = "10" fill = "darkgray" />									
	<rect x = "10" y = "10" width = "10" height = "10" fill = "lightgray" />						
	</pattern>																					
	</defs>																						
	
	<!--Background with checkerboard pattern-->														
	<rect width = "100%" height = "100%" fill = 'url(#checkerboard)' />								
	</svg>
			)||"},
		};



		auto it = icondb.find(name);
		if (it != icondb.end())
		{
			return it->second;
		}

		// Didn't find one, so return blank
		return {};

	}
}