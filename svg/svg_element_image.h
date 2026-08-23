#pragma once


//==================================================
// SVGImageNode
// https://www.w3.org/TR/SVG11/struct.html#ImageElement
// Stores embedded or referenced images
//==================================================

#include <functional>


#include "converters.h"
#include "svgattributes.h"
#include "svggraphicselement.h"


namespace waavs {
    // This is an odd duck, only used for getting a filename out of a ByteSpan, 
    static INLINE std::string toString(const ByteSpan& inChunk) noexcept
    {
        if (!inChunk)
            return std::string();

        return std::string(inChunk.begin(), inChunk.end());
    }

    //
    // parseImage()
    // 
    // Turn a base64 encoded inlined image into a Surface
    // We are handed the attribute, typically coming from a 
    // href of an <image> tag, or as a lookup for a fill, or stroke, 
    // paint attribute.
    // What we're passed are the contents of the 'url()'.  
    // 
    // Example: <image id="image_textures" x="0" y="0" width="1024" height="768" xlink:href="data:image/jpeg;base64,/9j/...
    //

    static bool parseImage(const ByteSpan& inChunk, BLImage& img)
    {
        ByteSpan value = inChunk;
        bspan_trim_spaces(value);

        // data:<metadata>,<payload>
        ByteSpan scheme = bspan_read_until(value, ':');
        bspan_trim_spaces(scheme);

        if (scheme != "data")
            return false;

        ByteSpan meta = bspan_read_until(value, ',');
        ByteSpan payload = value;

        bspan_trim_spaces(meta);
        bspan_trim_spaces(payload);

        if (!meta || !payload)
            return false;

        // First metadata field is the media type.
        ByteSpan mime = bspan_read_until(meta, ';');
        bspan_trim_spaces(mime);

        if (!bspan_starts_with(mime, "image/"))
            return false;

        bool isBase64 = false;

        ByteSpan key{};
        ByteSpan val{};

        while (parameter_read_next(meta, key, val, ';', '='))
        {
            if (key == "base64")
            {
                isBase64 = true;
                break;
            }
        }

        if (!isBase64)
            return false;

        const unsigned int outBuffSize =
            base64::getDecodeOutputSize(payload.size());

        MemBuff outBuff(outBuffSize);

        const size_t decodedSize =
            base64::decode(payload.data(), payload.size(), outBuff.data(), outBuffSize);

        if (decodedSize < 1 || decodedSize > outBuffSize)
        {
            printf("parseImage: Error in base64::decode, decodedSize: %zu\n", decodedSize);
            return false;
        }

        BLResult res = img.read_from_data(outBuff.data(), decodedSize);
        if (res == BL_SUCCESS)
            return true;

        if (mime == "image/gif")
        {
            printf("parseImage: trying to decode GIF\n");
            // specialized GIF fallback here
        }

        return false;
    }

    /*
    static bool parseImage(const ByteSpan& inChunk, BLImage& img)
    {
        static int n = 1;
        bool success{ false };
        ByteSpan value = inChunk;
        bspan_trim(value, chrWspChars);

        // figure out what kind of encoding we're dealing with
        // value starts with: 'data:image/png;base64,<base64 encoded image>
        //
        ByteSpan data = bspan_read_until(value, ':');
        auto mime = bspan_read_until(value, ';');
        auto encoding = bspan_read_until(value, ',');


        if (encoding == "base64")
        {
            // allocate some memory to decode into
            unsigned int outBuffSize = base64::getDecodeOutputSize(value.size());
            MemBuff outBuff(outBuffSize);

            size_t decodedSize = base64::decode(value.data(), value.size(), outBuff.data(), outBuffSize);

            // BUGBUG - write chunk to file for debugging
            //ByteSpan outChunk = ByteSpan(outBuff.data(), outBuff.size());
            //char filename[256]{ 0 };
            //sprintf_s(filename, "base64_%02d.dat", n++);
            //FILE *s = fopen(filename, "wb");
            //fwrite(outBuff.data(), 1, outBuff.size(), s);
            //fclose(s);

            
            if (decodedSize < 1 || decodedSize > outBuffSize) {
                printf("parseImage: Error in base64::decode, decodedSize: %d \n", decodedSize);
                return false;
            }
            
            // See if it's a format that blend2d can deal with using its
            // own codecs
            BLResult res = img.read_from_data(outBuff.data(), decodedSize);

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
    */

}

namespace waavs 
{
    struct DocImageState
    {
        SVGLengthValue x{};
        SVGLengthValue y{};
        SVGLengthValue width{};
        SVGLengthValue height{};

        ByteSpan href{};
    };

    static void loadDocImageState(DocImageState &out, const XmlAttributeCollection& attrs) noexcept
    {
        ByteSpan fX{}, fY{}, fWidth{}, fHeight{};
        attrs.getValue(svgattr::x(), fX);
        attrs.getValue(svgattr::y(), fY);
        attrs.getValue(svgattr::width(), fWidth);
        attrs.getValue(svgattr::height(), fHeight);

        if (!lengthValue_parse(fX, out.x))
            out.x = SVGLengthValue{};
        if (!lengthValue_parse(fY, out.y))
            out.y = SVGLengthValue{};
        if (!lengthValue_parse(fWidth, out.width))
            out.width = SVGLengthValue{};
        if (!lengthValue_parse(fHeight, out.height))
            out.height = SVGLengthValue{};

        attrs.getValue(svgattr::href(), out.href);
        if (!out.href)
            attrs.getValue(svgattr::xlink_href(), out.href);


    }


    struct SVGImageElement : public SVGGraphicsElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("image", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGImageElement>(groot);
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("image",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGImageElement>(groot);
                    node->loadFromXmlPull(iter, groot);

                    return node;
                });

            registerSingularNode();
        }


        // Document state as authored
        DocImageState fDocState{};

        // Resolved state of the image
        Surface fSurface{};
        BLImage fImage{};
        BLVar fImageVar{};

        // Calculated values
        double fX{ 0 };
        double fY{ 0 };
        double fWidth{ 0 };
        double fHeight{ 0 };
        PreserveAspectRatio fPAR{};



        //=========================
        //  Instance Constructor
        //=========================
        SVGImageElement(IAmGroot* root)
            : SVGGraphicsElement() 
        {
            setNeedsBinding(true);
        }


        const WGRectD objectBoundingBox() const noexcept override
        {
            return { fX, fY, fWidth, fHeight };
        }
        
        const WGRectD getObjectBoundingBox(IDrawGraphics* ctx, IAmGroot* groot) noexcept override
        {
            bindSelfToContext(ctx, groot);
            return { fX, fY, fWidth, fHeight };
        }

        const BLVar getVariant(IDrawGraphics *ctx, IAmGroot *groot) noexcept override
        {
            if (fImageVar.is_null())
            {
                bindSelfToContext(ctx, groot);
            }
            
            return fImageVar;
        }

        void fixupSelfStyleAttributes(IAmGroot*groot) override
        {
            (void)groot;

            loadDocImageState(fDocState, fAttributes);

            // Get the preserveAspectRatio attribute, 
            // since it can affect how we resolve the image size and placement
            ByteSpan par{};
            fAttributes.getValue(svgattr::preserveAspectRatio(), par);
            if (par)
                fPAR.loadFromChunk(par);


            // We can parse the image here, because the href is not
            // subject to change over time, so we don't need to delay 
            // until bind time.
            if (fDocState.href)
            {
                // First, see if it's embedded data
                if (bspan_starts_with(fDocState.href, "data:"))
                {
                    if (!parseImage(fDocState.href, fImage))
                        return;

                    fSurface = surfaceFromBLImage(fImage);
                    fImageVar = fImage;
                }
                else {
                    // Otherwise, assume it's a file reference
                    auto filepath = toString(fDocState.href);
                    if (filepath.size() > 0)
                    {
                        if (fImage.read_from_file(filepath.c_str()) == BL_SUCCESS)
                        {
                            fSurface = surfaceFromBLImage(fImage);
                            fImageVar = fImage;
                        }
                    }
                }
            }

        }


        void bindSelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            const BLFont* fontOpt = nullptr;    // font not relevant here
            double dpi = groot ? groot->dpi() : 96;
            //double w = 1.0;
            //double h = 1.0;

            const WGRectD paintVP = ctx->state().getViewport();

            //w = paintVP.w;
            //h = paintVP.h;


            LengthResolveCtx cx{}, cy{}, cw{}, ch{};
            cx = makeLengthCtxUser(paintVP.w, 0.0, dpi, fontOpt);
            cy = makeLengthCtxUser(paintVP.h, 0.0, dpi, fontOpt);
            cw = cx; ch = cy;

            fX = resolveLengthOr(fDocState.x, cx, 0);
            fY = resolveLengthOr(fDocState.y, cy, 0);
            fWidth = resolveLengthOr(fDocState.width, cw, fImage.size().w);
            fHeight = resolveLengthOr(fDocState.height, ch, fImage.size().h);
        }


        void drawSelf(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            if (fImage.is_empty())
                return;
            
            if (fWidth <= 0 || fHeight <= 0)
                return;

            // The image's intrinsic size in pixels
            const double iw = double(fImage.size().w);
            const double ih = double(fImage.size().h);
            if (iw <= 0.0 || ih <= 0.0)
                return;

            // don't display, if we have a display attribute of 'none'
            ByteSpan displayAttr{};
            if (fAttributes.getValue(svgattr::display(), displayAttr))
            {
                bspan_trim_spaces(displayAttr);
                InternedKey dv = WSNameSet::INTERN(displayAttr);

                if (dv == svgval::none())
                    return;
            }

            // We want to apply the preserveAspectRatio rules to determine 
            // how to fit the image into the specified width/height.
            const WGRectD viewport{ fX, fY, fWidth, fHeight };
            const WGRectD viewBox{ 0, 0, iw, ih };

            WGMatrix3x3 xform{};
            if (!computeViewBoxToViewport(viewport, viewBox, fPAR, xform))
                return;

            ctx->push();

            // for SLICE, we need to crop to the viewport, so we set a clip
            if (fPAR.align() != AspectRatioAlignKind::SVG_ASPECT_RATIO_NONE &&
                fPAR.meetOrSlice() == AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_SLICE)
            {
                ctx->clipRect(viewport);
            }


            // Apply mapping and draw the image in its intrinsic coordinate space.
            ctx->applyTransform(xform);

            //draw at (0,0) in image pixel space
            ctx->image(fSurface, 0, 0);
            ctx->pop();
        }

    };
}
