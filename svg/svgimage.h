#pragma once


//==================================================
// SVGImageNode
// https://www.w3.org/TR/SVG11/struct.html#ImageElement
// Stores embedded or referenced images
//==================================================

#include <functional>
#include <unordered_map>
#include <string>

#include "svgattributes.h"
#include "svgstructuretypes.h"
#include "converters.h"

namespace waavs {
    //
    // parseImage()
    // 
    // Turn a base64 encoded inlined image into a BLImage
    // We are handed the attribute, typically coming from a 
    // href of an <image> tag, or as a lookup for a fill, or stroke, 
    // paint attribute.
    // What we're passed are the contents of the 'url()'.  
    // 
    // Example: <image id="image_textures" x="0" y="0" width="1024" height="768" xlink:href="data:image/jpeg;base64,/9j/...
    //
    static bool parseImage(const ByteSpan& inChunk, BLImage& img)
    {
        static int n = 1;
        bool success{ false };
        ByteSpan value = inChunk;

        // figure out what kind of encoding we're dealing with
        // value starts with: 'data:image/png;base64,<base64 encoded image>
        //
        ByteSpan data = chunk_token(value, ":");
        auto mime = chunk_token(value, ";");
        auto encoding = chunk_token(value, ",");


        if (encoding == "base64")
        {
            // allocate some memory to decode into
            unsigned int outBuffSize = base64::getDecodeOutputSize(value.size());
            MemBuff outBuff(outBuffSize);

            auto decodedSize = base64::decode((const char*)value.data(), value.size(), outBuff.data());

            // BUGBUG - write chunk to file for debugging
            //ByteSpan outChunk = ByteSpan(outBuff.data(), outBuff.size());
            //char filename[256]{ 0 };
			//sprintf_s(filename, "base64_%02d.dat", n++);
            //writeChunkToFile(outChunk, filename);
            
            if (decodedSize < 1) {
                printf("parseImage: Error in base54::decode, decoded \n");
                return false;
            }
            
			// See if it's a format that blend2d can deal with using its
            // own codecs
            BLResult res = img.readFromData(outBuff.data(), outBuff.size());
            success = (res == BL_SUCCESS);
            
            // If we didn't succeed in decoding, then try any specilized methods of decoding
            // we might have.
            if (!success) {
                if (mime == "image/gif")
                {
                    printf("parseImage:: trying to decode GIF\n");
					// try to decode it as a gif
                    //BLResult res = img.readFromData(outBuff.data(), outBuff.size());
                    //success = (res == BL_SUCCESS);
                }
            }
            
        }

        return success;
    }
}

namespace waavs {
	
	static INLINE std::string toString(const ByteSpan& inChunk) noexcept
	{
		if (!inChunk)
			return std::string();

		return std::string(inChunk.fStart, inChunk.fEnd);
	}

	
	struct SVGImageElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			getSVGSingularCreationMap()["image"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGImageElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			registerContainerNode("image",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGImageElement>(groot);
					node->loadFromXmlIterator(iter, groot);

					return node;
				});

			registerSingularNode();
		}


		BLImage fImage{};
		ByteSpan fImageRef{};
		BLVar fImageVar{};

		double fX{ 0 };
		double fY{ 0 };
		double fWidth{ 0 };
		double fHeight{ 0 };




		SVGImageElement(IAmGroot* root)
			: SVGGraphicsElement() 
		{
			needsBinding(true);
		}

		BLRect frame() const override
		{

			//if (fHasTransform) {
			//	auto leftTop = fTransform.mapPoint(fX, fY);
			//	auto rightBottom = fTransform.mapPoint(fX + fWidth, fY + fHeight);
			//	return BLRect(leftTop.x, leftTop.y, rightBottom.x - leftTop.x, rightBottom.y - leftTop.y);
			//}

			return BLRect(fX, fY, fWidth, fHeight);
		}

		BLRect getBBox() const override
		{
			return BLRect(fX, fY, fWidth, fHeight);
		}
		
		const BLVar getVariant(IRenderSVG *ctx, IAmGroot *groot) noexcept override
		{
			if (fImageVar.isNull())
			{
				bindSelfToContext(ctx, groot);
			}
			
			return fImageVar;
		}

		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}
			
			auto cFrame = ctx->viewport();
			w = cFrame.w;	// groot->canvasWidth();
			h = cFrame.h;	// groot->canvasHeight();


			SVGDimension fDimX{};
			SVGDimension fDimY{};
			SVGDimension fDimWidth{};
			SVGDimension fDimHeight{};
			
			fDimX.loadFromChunk(getAttribute("x"));
			fDimY.loadFromChunk(getAttribute("y"));
			fDimWidth.loadFromChunk(getAttribute("width"));
			fDimHeight.loadFromChunk(getAttribute("height"));

			fImageRef = getAttribute("href");
			if (!fImageRef)
				fImageRef = getAttribute("xlink:href");

			
			// Parse the image so we can get its dimensions
			if (fImageRef)
			{
				// First, see if it's embedded data
				if (chunk_starts_with_cstr(fImageRef, "data:"))
				{
					bool success = parseImage(fImageRef, fImage);
				}
				else {
					// Otherwise, assume it's a file reference
					auto path = toString(fImageRef);
					if (path.size() > 0)
					{
						fImage.readFromFile(path.c_str());
					}
				}
			}


			fX = 0;
			fY = 0;
			fWidth = fImage.size().w;
			fHeight = fImage.size().h;

			if (fDimX.isSet())
				fX = fDimX.calculatePixels(w, 0, dpi);
			if (fDimY.isSet())
				fY = fDimY.calculatePixels(h, 0, dpi);
			if (fDimWidth.isSet())
				fWidth = fDimWidth.calculatePixels(w, 0, dpi);
			if (fDimHeight.isSet())
				fHeight = fDimHeight.calculatePixels(h, 0, dpi);

			fImageVar = fImage;
		}


		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			if (fImage.empty())
				return;

			BLRect dst{ fX,fY, fWidth,fHeight };
			BLRectI src{ 0,0,fImage.size().w,fImage.size().h };

			ctx->scaleImage(fImage, src.x, src.y, src.w, src.h, fX, fY, fWidth, fHeight);
		}

	};
}
