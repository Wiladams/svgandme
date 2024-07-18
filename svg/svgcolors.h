#pragma once

#include <map>
#include <string>

#include "blend2d.h"
#include "bspan.h"


namespace waavs
{
    // Database of SVG colors
    // BUGBUG - it might be better if these used float instead of byte values
    // Then they can be converted to various forms as needed.
    // It might also be beneficial to have a key that is a ByteSpan instead
    // of a std::string
    // Note:  Everything is in lowercase.  So, when looking up a key
    // the caller should ensure their key is lowercase first
    // https://www.w3.org/TR/SVG11/types.html#ColorKeywords
    //
    // 

    using bytesHash = std::hash<ByteSpan>;

    static std::map<size_t, BLRgba32> svgcolors =
    {
        {bytesHash{}("white"),  BLRgba32(255, 255, 255)},
        {bytesHash{}("ivory"), BLRgba32(255, 255, 240)},
        {bytesHash{}("lightyellow"), BLRgba32(255, 255, 224)},
        {bytesHash{}("mintcream"), BLRgba32(245, 255, 250)},
        {bytesHash{}("azure"), BLRgba32(240, 255, 255)},
        {bytesHash{}("snow"), BLRgba32(255, 250, 250)},
        {bytesHash{}("honeydew"), BLRgba32(240, 255, 240)},
        {bytesHash{}("floralwhite"), BLRgba32(255, 250, 240)},
        {bytesHash{}("ghostwhite"), BLRgba32(248, 248, 255)},
        {bytesHash{}("lightcyan"), BLRgba32(224, 255, 255)},
        {bytesHash{}("lemonchiffon"), BLRgba32(255, 250, 205)},
        {bytesHash{}("cornsilk"), BLRgba32(255, 248, 220)},
        {bytesHash{}("lightgoldenrodyellow"), BLRgba32(250, 250, 210)},
        {bytesHash{}("aliceblue"), BLRgba32(240, 248, 255)},
        {bytesHash{}("seashell"), BLRgba32(255, 245, 238)},
        {bytesHash{}("oldlace"), BLRgba32(253, 245, 230)},
        {bytesHash{}("whitesmoke"), BLRgba32(245, 245, 245)},
        {bytesHash{}("lavenderblush"), BLRgba32(255, 240, 245)},
        {bytesHash{}("beige"), BLRgba32(245, 245, 220)},
        {bytesHash{}("linen"), BLRgba32(250, 240, 230)},
        {bytesHash{}("papayawhip"), BLRgba32(255, 239, 213)},
        {bytesHash{}("blanchedalmond"), BLRgba32(255, 235, 205)},
        {bytesHash{}("antiquewhite"), BLRgba32(250, 235, 215)},
        {bytesHash{}("yellow"), BLRgba32(255, 255, 0)},
        {bytesHash{}("mistyrose"), BLRgba32(255, 228, 225)},
        {bytesHash{}("lavender"), BLRgba32(230, 230, 250)},
        {bytesHash{}("bisque"), BLRgba32(255, 228, 196)},
        {bytesHash{}("moccasin"), BLRgba32(255, 228, 181)},
        {bytesHash{}("palegoldenrod"), BLRgba32(238, 232, 170)},
        {bytesHash{}("khaki"), BLRgba32(240, 230, 140)},
        {bytesHash{}("navajowhite"), BLRgba32(255, 222, 173)},
        {bytesHash{}("aquamarine"), BLRgba32(127, 255, 212)},
        {bytesHash{}("paleturquoise"), BLRgba32(175, 238, 238)},
        {bytesHash{}("wheat"), BLRgba32(245, 222, 179)},
        {bytesHash{}("peachpuff"), BLRgba32(255, 218, 185)},
        {bytesHash{}("palegreen"), BLRgba32(152, 251, 152)},
        {bytesHash{}("greenyellow"), BLRgba32(173, 255, 47)},
        {bytesHash{}("gainsboro"), BLRgba32(220, 220, 220)},
        {bytesHash{}("powderblue"), BLRgba32(176, 224, 230)},
        {bytesHash{}("lightgreen"), BLRgba32(144, 238, 144)},
        {bytesHash{}("lightgray"), BLRgba32(211, 211, 211)},
        {bytesHash{}("chartreuse"), BLRgba32(127, 255, 0)},
        {bytesHash{}("gold"), BLRgba32(255, 215, 0)},
        {bytesHash{}("lightblue"), BLRgba32(173, 216, 230)},
        {bytesHash{}("lawngreen"), BLRgba32(124, 252, 0)},
        {bytesHash{}("pink"), BLRgba32(255, 192, 203)},
        {bytesHash{}("aqua"), BLRgba32(0, 255, 255)},
        {bytesHash{}("cyan"), BLRgba32(0, 255, 255)},
        {bytesHash{}("lightpink"), BLRgba32(255, 182, 193)},
        {bytesHash{}("thistle"), BLRgba32(216, 191, 216)},
        {bytesHash{}("lightskyblue"), BLRgba32(135, 206, 250)},
        {bytesHash{}("lightsteelblue"), BLRgba32(176, 196, 222)},
        {bytesHash{}("skyblue"), BLRgba32(135, 206, 235)},
        {bytesHash{}("silver"), BLRgba32(192, 192, 192)},
        {bytesHash{}("springgreen"), BLRgba32(0, 255, 127)},
        {bytesHash{}("mediumspringgreen"), BLRgba32(0, 250, 154)},
        {bytesHash{}("turquoise"), BLRgba32(64, 224, 208)},
        {bytesHash{}("burlywood"), BLRgba32(222, 184, 135)},
        {bytesHash{}("tan"), BLRgba32(210, 180, 140)},
        {bytesHash{}("yellowgreen"), BLRgba32(154, 205, 50)},
        {bytesHash{}("lime"), BLRgba32(0, 255, 0)},
        {bytesHash{}("mediumaquamarine"), BLRgba32(102, 205, 170)},
        {bytesHash{}("mediumturquoise"), BLRgba32(72, 209, 204)},
        {bytesHash{}("darkkhaki"), BLRgba32(189, 183, 107)},
        {bytesHash{}("lightsalmon"), BLRgba32(255, 160, 122)},
        {bytesHash{}("plum"), BLRgba32(221, 160, 221)},
        {bytesHash{}("sandybrown"), BLRgba32(244, 164, 96)},
        {bytesHash{}("darkseagreen"), BLRgba32(143, 188, 143)},
        {bytesHash{}("orange"), BLRgba32(255, 165, 0)},
        {bytesHash{}("darkgray"), BLRgba32(169, 169, 169)},
        {bytesHash{}("goldenrod"), BLRgba32(218, 165, 32)},
        {bytesHash{}("darksalmon"), BLRgba32(233, 150, 122)},
        {bytesHash{}("darkturquoise"), BLRgba32(0, 206, 209)},
        {bytesHash{}("limegreen"), BLRgba32(50, 205, 50)},
        {bytesHash{}("violet"), BLRgba32(238, 130, 238)},
        {bytesHash{}("deepskyblue"), BLRgba32(0, 191, 255)},
        {bytesHash{}("darkorange"), BLRgba32(255, 140, 0)},
        {bytesHash{}("salmon"), BLRgba32(250, 128, 114)},
        {bytesHash{}("rosybrown"), BLRgba32(188, 143, 143)},
        {bytesHash{}("lightcoral"), BLRgba32(240, 128, 128)},
        {bytesHash{}("coral"), BLRgba32(255, 127, 80)},
        {bytesHash{}("mediumseagreen"), BLRgba32(60, 179, 113)},
        {bytesHash{}("lightseagreen"), BLRgba32(32, 178, 170)},
        {bytesHash{}("cornflowerblue"), BLRgba32(100, 149, 237)},
        {bytesHash{}("cadetblue"), BLRgba32(95, 158, 160)},
        {bytesHash{}("peru"), BLRgba32(205, 133, 63)},
        {bytesHash{}("hotpink"), BLRgba32(255, 105, 180)},
        {bytesHash{}("orchid"), BLRgba32(218, 112, 214)},
        {bytesHash{}("palevioletred"), BLRgba32(219, 112, 147)},
        {bytesHash{}("darkgoldenrod"), BLRgba32(184, 134, 11)},
        {bytesHash{}("lightslategray"), BLRgba32(119, 136, 153)},
        {bytesHash{}("tomato"), BLRgba32(255, 99, 71)},
        {bytesHash{}("gray"), BLRgba32(128, 128, 128)},
        {bytesHash{}("dodgerblue"), BLRgba32(30, 144, 255)},
        {bytesHash{}("mediumpurple"), BLRgba32(147, 112, 219)},
        {bytesHash{}("olivedrab"), BLRgba32(107, 142, 35)},
        {bytesHash{}("slategray"), BLRgba32(112, 128, 144)},
        {bytesHash{}("chocolate"), BLRgba32(210, 105, 30)},
        {bytesHash{}("steelblue"), BLRgba32(70, 130, 180)},
        {bytesHash{}("olive"), BLRgba32(128, 128, 0)},
        {bytesHash{}("mediumslateblue"), BLRgba32(123, 104, 238)},
        {bytesHash{}("indianred"), BLRgba32(205, 92, 92)},
        {bytesHash{}("mediumorchid"), BLRgba32(186, 85, 211)},
        {bytesHash{}("seagreen"), BLRgba32(46, 139, 87)},
        {bytesHash{}("darkcyan"), BLRgba32(0, 139, 139)},
        {bytesHash{}("forestgreen"), BLRgba32(34, 139, 34)},
        {bytesHash{}("royalblue"), BLRgba32(65, 105, 225)},
        {bytesHash{}("dimgray"), BLRgba32(105, 105, 105)},
        {bytesHash{}("orangered"), BLRgba32(255, 69, 0)},
        {bytesHash{}("slateblue"), BLRgba32(106, 90, 205)},
        {bytesHash{}("teal"), BLRgba32(0, 128, 128)},
        {bytesHash{}("darkolivegreen"), BLRgba32(85, 107, 47)},
        {bytesHash{}("sienna"), BLRgba32(160, 82, 45)},
        {bytesHash{}("green"), BLRgba32(0, 128, 0)},
        {bytesHash{}("darkorchid"), BLRgba32(153, 50, 204)},
        {bytesHash{}("saddlebrown"), BLRgba32(139, 69, 19)},
        {bytesHash{}("deeppink"), BLRgba32(255, 20, 147)},
        {bytesHash{}("blueviolet"), BLRgba32(138, 43, 226)},
        {bytesHash{}("magenta"), BLRgba32(255, 0, 255)},
        {bytesHash{}("fuchsia"), BLRgba32(255, 0, 255)},
        {bytesHash{}("darkslategray"), BLRgba32(47, 79, 79)},
        {bytesHash{}("darkgreen"), BLRgba32(0, 100, 0)},
        {bytesHash{}("darkslateblue"), BLRgba32(72, 61, 139)},
        {bytesHash{}("brown"), BLRgba32(165, 42, 42)},
        {bytesHash{}("mediumvioletred"), BLRgba32(199, 21, 133)},
        {bytesHash{}("crimson"), BLRgba32(220, 20, 60)},
        {bytesHash{}("firebrick"), BLRgba32(178, 34, 34)},
        {bytesHash{}("red"), BLRgba32(255, 0, 0)},
        {bytesHash{}("darkviolet"), BLRgba32(148, 0, 211)},
        {bytesHash{}("darkmagenta"), BLRgba32(139, 0, 139)},
        {bytesHash{}("purple"), BLRgba32(128, 0, 128)},
        {bytesHash{}("rebeccapurple"),BLRgba32(102,51,153)},
        {bytesHash{}("midnightblue"), BLRgba32(25, 25, 112)},
        {bytesHash{}("darkred"), BLRgba32(139, 0, 0)},
        {bytesHash{}("maroon"), BLRgba32(128, 0, 0)},
        {bytesHash{}("indigo"), BLRgba32(75, 0, 130)},
        {bytesHash{}("blue"), BLRgba32(0, 0, 255)},
        {bytesHash{}("mediumblue"), BLRgba32(0, 0, 205)},
        {bytesHash{}("darkblue"), BLRgba32(0, 0, 139)},
        {bytesHash{}("navy"), BLRgba32(0, 0, 128)},
        {bytesHash{}("black"), BLRgba32(0, 0, 0)},
        {bytesHash{}("transparent"), BLRgba32(0, 0, 0) },
    };
    

    
    static BLRgba32 getSVGColorByName(const ByteSpan &colorName) noexcept
    {
        BLRgba32 aColor(128,128,128);

		auto it = svgcolors.find(bytesHash{}(colorName));
		if (it != svgcolors.end())
		{
			aColor = it->second;
		}
        
        return aColor;
    }
}
