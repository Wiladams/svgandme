#pragma once

#include <map>
#include <unordered_map>
#include <string>

#include "blend2d.h"
#include "bspan.h"


namespace waavs
{

    
    // Database of SVG colors
    // BUGBUG - it might be better if these used float instead of byte values
    // Then they can be converted to various forms as needed.
    // Note:  Everything is in lowercase.  So, when looking up a key
    // the caller should ensure their key is lowercase first
    // https://www.w3.org/TR/SVG11/types.html#ColorKeywords
    //
    // 
    
    // getSVGColorByName
    //
    // Returns a color, based on a name
    static BLRgba32 getSVGColorByName(const ByteSpan &colorName) noexcept
    {

        static std::unordered_map<ByteSpan, BLRgba32, ByteSpanInsensitiveHash, ByteSpanCaseInsensitive> svgcolors =
        {
            {("white"),  BLRgba32(255, 255, 255)},
            {("ivory"), BLRgba32(255, 255, 240)},
            {("lightyellow"), BLRgba32(255, 255, 224)},
            {("mintcream"), BLRgba32(245, 255, 250)},
            {("azure"), BLRgba32(240, 255, 255)},
            {("snow"), BLRgba32(255, 250, 250)},
            {("honeydew"), BLRgba32(240, 255, 240)},
            {("floralwhite"), BLRgba32(255, 250, 240)},
            {("ghostwhite"), BLRgba32(248, 248, 255)},
            {("lightcyan"), BLRgba32(224, 255, 255)},
            {("lemonchiffon"), BLRgba32(255, 250, 205)},
            {("cornsilk"), BLRgba32(255, 248, 220)},
            {("lightgoldenrodyellow"), BLRgba32(250, 250, 210)},
            {("aliceblue"), BLRgba32(240, 248, 255)},
            {("seashell"), BLRgba32(255, 245, 238)},
            {("oldlace"), BLRgba32(253, 245, 230)},
            {("whitesmoke"), BLRgba32(245, 245, 245)},
            {("lavenderblush"), BLRgba32(255, 240, 245)},
            {("beige"), BLRgba32(245, 245, 220)},
            {("linen"), BLRgba32(250, 240, 230)},
            {("papayawhip"), BLRgba32(255, 239, 213)},
            {("blanchedalmond"), BLRgba32(255, 235, 205)},
            {("antiquewhite"), BLRgba32(250, 235, 215)},
            {("yellow"), BLRgba32(255, 255, 0)},
            {("mistyrose"), BLRgba32(255, 228, 225)},
            {("lavender"), BLRgba32(230, 230, 250)},
            {("bisque"), BLRgba32(255, 228, 196)},
            {("moccasin"), BLRgba32(255, 228, 181)},
            {("palegoldenrod"), BLRgba32(238, 232, 170)},
            {("khaki"), BLRgba32(240, 230, 140)},
            {("navajowhite"), BLRgba32(255, 222, 173)},
            {("aquamarine"), BLRgba32(127, 255, 212)},
            {("paleturquoise"), BLRgba32(175, 238, 238)},
            {("wheat"), BLRgba32(245, 222, 179)},
            {("peachpuff"), BLRgba32(255, 218, 185)},
            {("palegreen"), BLRgba32(152, 251, 152)},
            {("greenyellow"), BLRgba32(173, 255, 47)},
            {("gainsboro"), BLRgba32(220, 220, 220)},
            {("powderblue"), BLRgba32(176, 224, 230)},
            {("lightgreen"), BLRgba32(144, 238, 144)},
            {("lightgray"), BLRgba32(211, 211, 211)},
            {("chartreuse"), BLRgba32(127, 255, 0)},
            {("gold"), BLRgba32(255, 215, 0)},
            {("lightblue"), BLRgba32(173, 216, 230)},
            {("lawngreen"), BLRgba32(124, 252, 0)},
            {("pink"), BLRgba32(255, 192, 203)},
            {("aqua"), BLRgba32(0, 255, 255)},
            {("cyan"), BLRgba32(0, 255, 255)},
            {("lightpink"), BLRgba32(255, 182, 193)},
            {("thistle"), BLRgba32(216, 191, 216)},
            {("lightskyblue"), BLRgba32(135, 206, 250)},
            {("lightsteelblue"), BLRgba32(176, 196, 222)},
            {("skyblue"), BLRgba32(135, 206, 235)},
            {("silver"), BLRgba32(192, 192, 192)},
            {("springgreen"), BLRgba32(0, 255, 127)},
            {("mediumspringgreen"), BLRgba32(0, 250, 154)},
            {("turquoise"), BLRgba32(64, 224, 208)},
            {("burlywood"), BLRgba32(222, 184, 135)},
            {("tan"), BLRgba32(210, 180, 140)},
            {("yellowgreen"), BLRgba32(154, 205, 50)},
            {("lime"), BLRgba32(0, 255, 0)},
            {("mediumaquamarine"), BLRgba32(102, 205, 170)},
            {("mediumturquoise"), BLRgba32(72, 209, 204)},
            {("darkkhaki"), BLRgba32(189, 183, 107)},
            {("lightsalmon"), BLRgba32(255, 160, 122)},
            {("plum"), BLRgba32(221, 160, 221)},
            {("sandybrown"), BLRgba32(244, 164, 96)},
            {("darkseagreen"), BLRgba32(143, 188, 143)},
            {("orange"), BLRgba32(255, 165, 0)},
            {("darkgray"), BLRgba32(169, 169, 169)},
            {("goldenrod"), BLRgba32(218, 165, 32)},
            {("darksalmon"), BLRgba32(233, 150, 122)},
            {("darkturquoise"), BLRgba32(0, 206, 209)},
            {("limegreen"), BLRgba32(50, 205, 50)},
            {("violet"), BLRgba32(238, 130, 238)},
            {("deepskyblue"), BLRgba32(0, 191, 255)},
            {("darkorange"), BLRgba32(255, 140, 0)},
            {("salmon"), BLRgba32(250, 128, 114)},
            {("rosybrown"), BLRgba32(188, 143, 143)},
            {("lightcoral"), BLRgba32(240, 128, 128)},
            {("coral"), BLRgba32(255, 127, 80)},
            {("mediumseagreen"), BLRgba32(60, 179, 113)},
            {("lightseagreen"), BLRgba32(32, 178, 170)},
            {("cornflowerblue"), BLRgba32(100, 149, 237)},
            {("cadetblue"), BLRgba32(95, 158, 160)},
            {("peru"), BLRgba32(205, 133, 63)},
            {("hotpink"), BLRgba32(255, 105, 180)},
            {("orchid"), BLRgba32(218, 112, 214)},
            {("palevioletred"), BLRgba32(219, 112, 147)},
            {("darkgoldenrod"), BLRgba32(184, 134, 11)},
            {("lightslategray"), BLRgba32(119, 136, 153)},
            {("tomato"), BLRgba32(255, 99, 71)},
            {("gray"), BLRgba32(128, 128, 128)},
            {("dodgerblue"), BLRgba32(30, 144, 255)},
            {("mediumpurple"), BLRgba32(147, 112, 219)},
            {("olivedrab"), BLRgba32(107, 142, 35)},
            {("slategray"), BLRgba32(112, 128, 144)},
            {("chocolate"), BLRgba32(210, 105, 30)},
            {("steelblue"), BLRgba32(70, 130, 180)},
            {("olive"), BLRgba32(128, 128, 0)},
            {("mediumslateblue"), BLRgba32(123, 104, 238)},
            {("indianred"), BLRgba32(205, 92, 92)},
            {("mediumorchid"), BLRgba32(186, 85, 211)},
            {("seagreen"), BLRgba32(46, 139, 87)},
            {("darkcyan"), BLRgba32(0, 139, 139)},
            {("forestgreen"), BLRgba32(34, 139, 34)},
            {("royalblue"), BLRgba32(65, 105, 225)},
            {("dimgray"), BLRgba32(105, 105, 105)},
            {("orangered"), BLRgba32(255, 69, 0)},
            {("slateblue"), BLRgba32(106, 90, 205)},
            {("teal"), BLRgba32(0, 128, 128)},
            {("darkolivegreen"), BLRgba32(85, 107, 47)},
            {("sienna"), BLRgba32(160, 82, 45)},
            {("green"), BLRgba32(0, 128, 0)},
            {("darkorchid"), BLRgba32(153, 50, 204)},
            {("saddlebrown"), BLRgba32(139, 69, 19)},
            {("deeppink"), BLRgba32(255, 20, 147)},
            {("blueviolet"), BLRgba32(138, 43, 226)},
            {("magenta"), BLRgba32(255, 0, 255)},
            {("fuchsia"), BLRgba32(255, 0, 255)},
            {("darkslategray"), BLRgba32(47, 79, 79)},
            {("darkgreen"), BLRgba32(0, 100, 0)},
            {("darkslateblue"), BLRgba32(72, 61, 139)},
            {("brown"), BLRgba32(165, 42, 42)},
            {("mediumvioletred"), BLRgba32(199, 21, 133)},
            {("crimson"), BLRgba32(220, 20, 60)},
            {("firebrick"), BLRgba32(178, 34, 34)},
            {("red"), BLRgba32(255, 0, 0)},
            {("darkviolet"), BLRgba32(148, 0, 211)},
            {("darkmagenta"), BLRgba32(139, 0, 139)},
            {("purple"), BLRgba32(128, 0, 128)},
            {("rebeccapurple"),BLRgba32(102,51,153)},
            {("midnightblue"), BLRgba32(25, 25, 112)},
            {("darkred"), BLRgba32(139, 0, 0)},
            {("maroon"), BLRgba32(128, 0, 0)},
            {("indigo"), BLRgba32(75, 0, 130)},
            {("blue"), BLRgba32(0, 0, 255)},
            {("mediumblue"), BLRgba32(0, 0, 205)},
            {("darkblue"), BLRgba32(0, 0, 139)},
            {("navy"), BLRgba32(0, 0, 128)},
            {("black"), BLRgba32(0, 0, 0)},
            {("transparent"), BLRgba32(0, 0, 0, 0) },

            // Deprecated system colors
            {"activeborder", BLRgba32(0xffb4b4b4)},
            {"activecaption", BLRgba32(0xff000080)},
            { "appworkspace", BLRgba32(0xffc0c0c0) },
            { "background", BLRgba32(0xff000000) },
            { "buttonface", BLRgba32(0xfff0f0f0) },
            { "buttonhighlight", BLRgba32(0xffffffff) },
            { "buttonshadow", BLRgba32(0xffa0a0a0) },
            { "buttontext", BLRgba32(0xff000000) },
            { "captiontext", BLRgba32(0xff000000) },
            { "graytext", BLRgba32(0xff808080) },
            { "highlight", BLRgba32(0xff3399ff) },
            { "highlighttext", BLRgba32(0xffffffff) },
            { "inactiveborder", BLRgba32(0xfff4f7fc) },
            { "inactivecaption", BLRgba32(0xff7a96df) },
            { "inactivecaptiontext", BLRgba32(0xffd2b4de) },
            { "infobackground", BLRgba32(0xffffffe1) },
            { "infotext", BLRgba32(0xff000000) },
            { "menu", BLRgba32(0xfff0f0f0) },
            { "menutext", BLRgba32(0xff000000) },
            { "scrollbar", BLRgba32(0xffd4d0c8) },
            { "threeddarkshadow", BLRgba32(0xff696969) },
            { "threedface", BLRgba32(0xffc0c0c0) },
            { "threedhighlight", BLRgba32(0xffffffff) },
            { "threedlightshadow", BLRgba32(0xffd3d3d3) },
            { "threedshadow", BLRgba32(0xffa0a0a0) },
            { "window", BLRgba32(0xffffffff) },
            { "windowframe", BLRgba32(0xff646464) },
            { "windowtext", BLRgba32(0xff000000) },
        };

        
        auto it = svgcolors.find(colorName);
		if (it != svgcolors.end())
		{
			return it->second;
		}
        //printf("UNKNOWN COLOR: ");
		//printChunk(colorName);
        
        return BLRgba32(128, 128, 128);
    }
}
