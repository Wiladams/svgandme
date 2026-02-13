#pragma once

#include <string>
#include <array>
#include <functional>
#include <unordered_map>


#include "svgshapes.h"
#include "b2dpathbuilder.h"
#include "svgcss.h"


//
// SVG Supports embedded fonts.  This feature is little used, and obsolete
// as web fonts, and truetype fonts are the norm.  The support here is experimental
// as there's no real way of adding this information in this form into blend2d
//

namespace waavs {
	enum GLYPH_ORIENTATION {
		GLYPH_ORIENTATION_HORIZONTAL=1
		, GLYPH_ORIENTATION_VERTICAL=2
		, GLYPH_ORIENTATION_BOTH=3
	};
	
	enum GLYPH_ARABIC_FORM {
		GLYPH_ARABIC_FORM_ISOLATED = 0
		, GLYPH_ARABIC_FORM_INITIAL = 1
		, GLYPH_ARABIC_FORM_MEDIAL = 2
		, GLYPH_ARABIC_FORM_TERMINAL = 3
	};
	
	//====================================
	// SVGFont
	// Support the 'font' element
	//====================================
	struct SVGFontNode : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			registerContainerNodeByName("font", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFontNode>(groot);
				node->loadFromXmlPull(iter, groot);

				return node;
				});
		}

		// id
		double fHorizAdvX{ 0 };
		double fHorizOriginX{ 0 };
		double fHorizOriginY{ 0 };
		double fVertAdvY{ 0 };
		double fVertOriginX{ 0 };
		double fVertOriginY{ 0 };

		SVGFontNode(IAmGroot* )
			: SVGGraphicsElement()
			, fHorizAdvX{ 0 }
			, fHorizOriginX{0}
		{
			setIsStructural(true);
            setIsVisible(false);
		}


		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{


			parseNumber(getAttributeByName("horiz-adv-x"), fHorizAdvX);
			parseNumber(getAttributeByName("horiz-origin-x"), fHorizOriginX);
			parseNumber(getAttributeByName("horiz-origin-y"), fHorizOriginY);
			parseNumber(getAttributeByName("vert-adv-y"), fVertAdvY);
			parseNumber(getAttributeByName("vert-origin-x"), fVertOriginX);
			parseNumber(getAttributeByName("vert-origin-y"), fVertOriginY);


			setNeedsBinding(true);
		}

	};
	
	//====================================
	// SVGFontFaceNode
	// Has both singular and compound forms
	//====================================
	struct SVGFontFaceNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("font-face", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFontFaceNode>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
			});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("font-face", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFontFaceNode>(groot);
				node->loadFromXmlPull(iter, groot);

				return node;
			});

			registerSingularNode();
		}

		std::string fFontFamily;
		std::string fFontStyle;
		std::string fFontVariant;
		std::string fFontWeight;
		std::string fFontStretch;
		std::string fFontSize;
		std::string fUnicodeRange;
		std::string fUnitsPerEm;
		std::string fPanose1;
		std::string fStemV;
		std::string fStemH;
		std::string fSlope;
		std::string fCapHeight;
		std::string fXHeight;
		std::string fAccentHeight;
		std::string fAscent;
		std::string fDescent;
		std::string fWidths;
		std::string fBbox;
		std::string fIdeographic;
		std::string fAlphabetic;
		std::string fMathematical;
		std::string fHanging;
		std::string fVideographic;
		std::string fValphabetic;
		std::string fVmathematical;
		std::string fVhanging;
		std::string fUnderlinePosition;
		std::string fUnderlineThickness;
		std::string fStrikethroughPosition;
		std::string fStrikethroughThickness;
		std::string fOverlinePosition;
		std::string fOverlineThickness;

		SVGFontFaceNode(IAmGroot* iMap) 
			:SVGGraphicsElement() 
		{
            setIsVisible(false);
		}


		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			auto fontFamily = getAttributeByName("font-family");
			auto fontStyle = getAttributeByName("font-style");
			auto fontVariant = getAttributeByName("font-variant");
			auto fontWeight = getAttributeByName("font-weight");
			auto fontStretch = getAttributeByName("font-stretch");
			auto fontSize = getAttributeByName("font-size");
			auto unicodeRange = getAttributeByName("unicode-range");
			auto unitsPerEm = getAttributeByName("units-per-em");
		}
	};

	
	//====================================
	// SVGMissingGlyphNode
	// Can be a singular node, or a compound
	// node with an embedded path
	// Really glyphs act like symbols
	//====================================
	struct SVGMissingGlyphNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("missing-glyph", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGMissingGlyphNode>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
			});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("missing-glyph", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGMissingGlyphNode>(groot);
				node->loadFromXmlPull(iter, groot);

				return node;
			});
			
			registerSingularNode();
		}
		
		double fHorizAdvX{ 0 };
		double fVertAdvY{ 0 };
		double fVertOriginX{ 0 };
		double fVertOriginY{ 0 };
		BLPath fPath{};
		
		SVGMissingGlyphNode(IAmGroot* )
			: SVGGraphicsElement()
		{
		}

		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{	
			parseNumber(getAttributeByName("horiz-adv-x"), fHorizAdvX);
			parseNumber(getAttributeByName("vert-adv-y"), fVertAdvY);
			parseNumber(getAttributeByName("vert-origin-x"), fVertOriginX);
			parseNumber(getAttributeByName("vert-origin-y"), fVertOriginY);
			
			
			auto d = getAttributeByName("d");
			if (!d)
				return;

			if (parsePath(d, fPath))
				fPath.shrink();

			setNeedsBinding(false);
		}
	};
	
	//====================================
	// SVGGlyph
	// Support the 'glyph' found in the 'font' element
	//====================================
	struct SVGGlyphNode : public SVGPathBasedGeometry
	{
		static void registerFactory() {
			registerSVGSingularNodeByName("glyph", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGGlyphNode>(groot);
				node->loadFromXmlElement(elem, groot);
				
				return node;
			});
		}

		uint64_t fUnicode{ 0 };
		double fHorizAdvX{ 0 };
		double fVertAdvY{ 0 };
		double fVertOriginX{ 0 };
		double fVertOriginY{ 0 };
		
		std::string fGlyphName{};
		GLYPH_ORIENTATION fOrientation{ GLYPH_ORIENTATION_BOTH };
		GLYPH_ARABIC_FORM arabicForm{ GLYPH_ARABIC_FORM_ISOLATED };
		ByteSpan fLang{};

		SVGGlyphNode(IAmGroot* iMap) :SVGPathBasedGeometry(iMap) {}

		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			//SVGGeometryElement::loadSelfFromXmlElement(elem, groot);

			// load unicode property
			// just store the raw ByteSpan
			// and let the caller parse it
			auto uni = getAttributeByName("unicode");
			
			parseNumber(getAttributeByName("horiz-adv-x"), fHorizAdvX);
			parseNumber(getAttributeByName("vert-adv-y"), fVertAdvY);
			parseNumber(getAttributeByName("vert-origin-x"), fVertOriginX);
			parseNumber(getAttributeByName("vert-origin-y"), fVertOriginY);
			

			auto d = getAttribute(svgattr::d());
			if (d) {
				parsePathProgram(d, fProg);
				//fHasProg = true;
			}
		}

	};

	//====================================
	// SVGFontFaceNode
	// Has both singular and compound forms
	//====================================
	struct SVGFontFaceSrcNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("font-face-src", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFontFaceSrcNode>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
			});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("font-face-src", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFontFaceSrcNode>(groot);
				node->loadFromXmlPull(iter, groot);

				return node;
			});

			registerSingularNode();
		}


		SVGFontFaceSrcNode(IAmGroot* iMap) :SVGGraphicsElement() {}

	};
	
	//=========================================================
	// SVGFontFaceNameNode
	//=========================================================
	struct SVGFontFaceNameNode : public SVGGraphicsElement
	{
		static void registerFactory() {
			registerSVGSingularNodeByName("font-face-name", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFontFaceNameNode>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
			});
		}

		ByteSpan fFaceName{};

		SVGFontFaceNameNode(IAmGroot* iMap) :SVGGraphicsElement() 
		{ 
			setIsVisible(false); 
		}

		const ByteSpan& faceName() const {
			return fFaceName;
		}
		
		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* ) override
		{
			fFaceName = getAttributeByName("name");
		}


	};
	

}