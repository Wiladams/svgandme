#pragma once

#ifndef fonthandler_h
#define fonthandler_h


#include <algorithm>
#include <vector>
#include <memory>
#include <map>
#include <string>




#include "blend2d.h"
#include "maths.h"
#include "bspan.h"




namespace waavs {
    /*
	static bool endsWith(const std::string& src, const std::string &suffix) 
    {
        if (suffix.size() > src.size())
            return false;
        
        return src.substr(src.size() - suffix.size()) == suffix;
	}
    */
    
    class FontHandler
    {
    public:
        // Typography
        BLFontManager fFontManager{};
        std::vector<std::string> fFamilyNames{};

        // For size helper
        int fDotsPerInch = 1;
        float fUnitsPerInch = 1;

        FontHandler()
        {
            fFontManager.create();
        }

        /*
            setDpiUnits makes it possible to let the FontHandler to
            understand the DPI we're rendering to, as well as
            the intended units we're using to specify font sizes.

            This combination allows you to say; "my display is 192 dpi,
            and I want to use 'inches' to specify my font sizes"

            setDpiUnits(192, 1);

            Or, if you want to work in millimeters

            setDpiUnits(192, 25.4);

            If you don't know your displays dpi, and you just
            want to work in pixels, then you can use the default
            which is;

            setDpiUnits(1,1);

        // Some typical units
            local unitFactors = {
            ["in"] = 1;
            ["mm"] = 25.4;
            ["px"] = 96;
            ["pt"] = 72;
        }
        */
        void setDpiUnits(int dpi, float unitsPerInch)
        {
            fDotsPerInch = dpi;
            fUnitsPerInch = unitsPerInch;
        }

        const std::vector<std::string>& familyNames() const { return fFamilyNames; }




        // create a single font face by filename
        // Put it into the font manager
        // return it to the user
        bool loadFontFace(const char* filename, BLFontFace &ff)
        {
            //BLFontFace ff;
            //BLResult err = ff.createFromFile(filename, BL_FILE_READ_MMAP_AVOID_SMALL);
            BLResult err = ff.createFromFile(filename);

            if (!err)
            {
                //printf("FontHandler.loadFont() adding: %s\n", ff.familyName().data());


                
                fFontManager.addFace(ff);
                fFamilyNames.push_back(std::string(ff.familyName().data()));
                return true;
            }
            else {
                ;
                printf("FontHandler::loadFont Error: %s (0x%x)\n", filename, err);
            }

            return false;
        }

        // Load the list of font files into 
        // the font manager
        bool loadFonts(std::vector<const char*> fontNames)
        {
            bool success{ false };
            
            for (const auto& filename : fontNames)
            {
                BLFontFace ff{};
                bool success = loadFontFace(filename, ff);
                if (!success)
				{
                    //printf("loadFonts: %s, 0x%x\n", face.familyName().data(), face.isValid());
					return false;
				}

            }

            return true;
        }



        float getAdjustedFontSize(float sz) const
        {
            float fsize = sz * ((float)fDotsPerInch / fUnitsPerInch);
            return fsize;
        }
        
        
        // selectFontFamily
        // Select a specific family, where a list of possibilities have been supplied
        // The query properties of style, weight, and stretch can also be supplied
        // with defaults of 'normal'
		bool selectFontFamily(const ByteSpan& names, BLFontFace& face, uint32_t style= BL_FONT_STYLE_NORMAL, uint32_t weight= BL_FONT_WEIGHT_NORMAL, uint32_t stretch= BL_FONT_STRETCH_NORMAL) const
		{
            static charset fontWsp = chrWspChars + ',';
            
            charset delims(",");
            charset quoteChars("'\"");

            ByteSpan s = names;
            
            BLFontQueryProperties qprops{};
            qprops.stretch = stretch;
            qprops.style = style;
            qprops.weight = weight;
            
            // nammes are quoted, or separated by commas
            while (s.size() > 0)
            {
                s = chunk_ltrim(s, fontWsp);
                ByteSpan name = chunk_token(s, delims);
                name = chunk_trim(name, quoteChars);

                if (!name)
                    break;

                BLStringView familyNameView{};
                bool success{ false };

                if ((name == "Sans") ||
                    (name == "sans") ||
                    (name == "Helvetica") ||
                    (name == "sans-serif")) 
                {
                    success = (BL_SUCCESS == fFontManager.queryFace("Arial", qprops, face));
                }
				else if ((name == "Serif") ||
					(name == "serif")) {
					success = (BL_SUCCESS == fFontManager.queryFace("Georgia", qprops, face));  // Times New Roman, Garamond, Georgia
				}
				else if ((name == "Mono") ||
					(name == "mono") ||
					(name == "monospace")) {
					success = (BL_SUCCESS == fFontManager.queryFace("Consolas", qprops, face));
				}
                else {
                    familyNameView.reset((char*)name.fStart, name.size());
                    success = (BL_SUCCESS == fFontManager.queryFace(familyNameView, qprops, face));
                }

				if (success)
					return true;

				// Didn't find it, try the next one
                //printf("== FontHandler::selectFontFamily, NOT FOUND: ");
                //printChunk(name);
            }

            // last chance, nothing else worked, so try loading our default, Arial
            bool success = (BL_SUCCESS == fFontManager.queryFace("Arial", qprops, face));
            
			return success;
		}
        
        
        // Select a font with given criteria
		// If the font is not found, then return the default font
        // which should be Arial
        bool selectFont(const ByteSpan& names, BLFont& font, float sz, uint32_t style = BL_FONT_STYLE_NORMAL, uint32_t weight = BL_FONT_WEIGHT_NORMAL, uint32_t stretch = BL_FONT_STRETCH_NORMAL) const
        {
            BLFontFace face;

            // Try to select an appropriate font family
            bool success = selectFontFamily(names, face, style, weight, stretch);
            
            // If none found, try our default font
            if (!success)
            {
                success = selectFontFamily("Arial", face, style, weight, stretch);
                
                if (!success)
                    return false;
            }
            
            // Now that we've gotten a face, we need to fill in the font
            // object to be the size we want
            float fsize = getAdjustedFontSize(sz);
			blFontReset(&font);
            
            success = (BL_SUCCESS == blFontCreateFromFace(&font, &face, fsize));
            
			return success;
        }
        

        
        // This is fairly expensive, and should live with a font object
        // instead of on this interface
        BLPoint textMeasure(const ByteSpan & txt, const char* familyname, float sz) const
        {
            BLFont afont{};
            auto success = selectFont(familyname, afont, sz);
            
			if (!success)
                return BLPoint( 0, 0 );
            
            BLTextMetrics tm{};
            BLGlyphBuffer gb;
            gb.setUtf8Text(txt.fStart, txt.size());
            afont.shape(gb);
            afont.getTextMetrics(gb, tm);

            float cx = (float)(tm.boundingBox.x1 - tm.boundingBox.x0);
            float cy = afont.size();

            return BLPoint( cx, cy );
        }
    };

}

#endif // fonthandler_h
