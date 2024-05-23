#pragma once

#include "irendersvg.h"


namespace waavs
{
    struct SvgDrawingContext : public IRenderSVG
    {
        bool fOwnsImage;
        BLImage fImage;

    public:
        SvgDrawingContext(const size_t width, const size_t height, FontHandler *fh)
            : IRenderSVG(fh)
        {
            // create a BLImage to match the width and height
			fImage.create(width, height, BL_FORMAT_PRGB32);
            fOwnsImage = true;
            
			// initialize BLContext to draw into the image
			BLContext::begin(fImage);
        }
        

        
        // Construct from a passed in image
        SvgDrawingContext(BLImage& img, FontHandler* fh)
            : IRenderSVG(fh)
        {
            fImage.reset();
			fImage.assign(img);
            //fImage = img;
            fOwnsImage = false;

            // initialize BLContext to draw into the image
            BLContext::begin(img);
        }
        
        SvgDrawingContext(FontHandler* fh)
            : IRenderSVG(fh)
        {
        }

        
        virtual ~SvgDrawingContext()
        {
			// If we own the image, destroy it
            BLContext::end();
            
			if (fOwnsImage)
			{
				fImage.reset();
			}
        }


        void saveToFile(const char* filename) const
        {
            // save the fImage as a .png file
            fImage.writeToFile(filename);
        }
        
        // A couple of factory methods to create the context
        // with a BLImage backing store
        // Returns a shared pointer to the context
        static std::shared_ptr<SvgDrawingContext> create(const size_t width, const size_t height, FontHandler *fh)
        {
            return std::make_shared<SvgDrawingContext>(width, height, fh);
        }

    };
}
