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

        const std::vector<std::string>& familyNames() const { return fFamilyNames; }


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





        // create a single font face by filename
        // Put it into the font manager
        // return it to the user
        uint32_t loadFontFace(const char* filename, BLFontFace &ff)
        {
            //printf("FontHandler.loadFontFace(%-40s) \n", filename);

            //BLResult err = ff.createFromFile(filename, BL_FILE_READ_MMAP_AVOID_SMALL);
            BLResult err = ff.createFromFile(filename, BL_FILE_READ_MMAP_ENABLED);

            if (err == BL_SUCCESS)
            {
                //printf("adding: '%-25s'  LastResort: %d\n", ff.familyName().data(), ff.isLastResortFont());

                fFontManager.addFace(ff);
                std::string familyName = std::string(ff.familyName().data());
                fFamilyNames.push_back(familyName);

                return BL_SUCCESS;
            }
            else  {
                //printf(" ERROR loading: %s, 0x%x\n", filename, err);
            }

            return err;
        }

        // Load the list of font files into 
        // the font manager
        bool loadFonts(std::vector<const char*> fontNames)
        {
            
            for (const auto& filename : fontNames)
            {
                BLFontFace ff{};
                uint32_t err = loadFontFace(filename, ff);

                // If we fail loading a single font, move on to the next one
                if (err != BL_SUCCESS)
				{
                    //printf("FontHandler::loadFonts Error: %s (0x%x)\n", filename, err);
                    //printf("loadFonts: %s, 0x%x\n", face.familyName().data(), face.isValid());
					continue;
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
            // BUGBUG - restructure this so that actual name
            // is looked up first
            while (!s.empty())
            {
                s = chunk_ltrim(s, fontWsp);
                ByteSpan name = chunk_token(s, delims);
                name = chunk_trim(name, quoteChars);

                if (name.empty())
                    break;

                BLStringView familyNameView{};
                bool success{ false };

                //familyNameView.reset((char*)name.fStart, name.size());
                //success = (BL_SUCCESS == fFontManager.queryFace(familyNameView, qprops, face));

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
                return false;

                //success = selectFontFamily("Arial", face, style, weight, stretch);
                //if (!success)
                //    return false;
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

        // Get a singleton font handler
        // We use a singleton for the entire app, so we don't
        // have to load fonts multiple times
		static FontHandler* getFontHandler()
		{
            static std::unique_ptr<FontHandler> sFontHandler = std::make_unique<FontHandler>();
            return sFontHandler.get();
		}

        // setDefaultFont
        // Set the font that will be used in cases where no font is specified
        // or to be the fallback font, when a font is not found
		static void setDefaultFont(const char* fontname)
		{
			//BLFontManager::setDefaultFont(fontname);
		}

        // Add a directory to be searched for fonts
		static void addFontDirectory(const char* dir)
		{
			//BLFontManager::addFontDirectory(dir);
		}

        static double ascent(const BLFont& font) noexcept
        {
            return font.metrics().ascent;
        }

        static double descent(const BLFont& font) noexcept
        {
            return font.metrics().descent;
        }

        static double capHeight(const BLFont& font) noexcept
        {
            return font.metrics().capHeight;
        }

        static double emHeight(const BLFont& font) noexcept
        {
            auto h = font.metrics().ascent + font.metrics().descent;
            return h;
        }

        static double exHeight(const BLFont& font) noexcept
        {
            return font.metrics().xHeight;
        }
    };

}

#endif // fonthandler_h
