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
#include "viewport.h"

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
        ByteSpan value = chunk_trim(inChunk, chrWspChars);

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
            //BLResult res = img.readFromData(outBuff.data(), outBuff.size());
            BLResult res = img.readFromData(outBuff.data(), decodedSize);

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

        out.x = parseLengthAttr(fX);
        out.y = parseLengthAttr(fY);
        out.width = parseLengthAttr(fWidth);
        out.height = parseLengthAttr(fHeight);

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


        BLRect objectBoundingBox() const override
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

        void fixupSelfStyleAttributes(IAmGroot*groot) override
        {
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
                if (chunk_starts_with_cstr(fDocState.href, "data:"))
                {
                    if (parseImage(fDocState.href, fImage))
                        fImageVar = fImage;
                }
                else {
                    // Otherwise, assume it's a file reference
                    auto filepath = toString(fDocState.href);
                    if (filepath.size() > 0)
                    {
                        if (fImage.readFromFile(filepath.c_str()) == BL_SUCCESS)
                            fImageVar = fImage;
                    }
                }
            }

        }


        void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            const BLFont* fontOpt = nullptr;    // font not relevant here
            double dpi = groot ? groot->dpi() : 96;
            double w = 1.0;
            double h = 1.0;

            const BLRect paintVP = ctx->viewport();

            w = paintVP.w;
            h = paintVP.h;


            LengthResolveCtx cx{}, cy{}, cw{}, ch{};
            cx = makeLengthCtxUser(paintVP.w, 0.0, dpi, fontOpt);
            cy = makeLengthCtxUser(paintVP.h, 0.0, dpi, fontOpt);
            cw = cx; ch = cy;

            fX = resolveLengthOr(fDocState.x, cx, 0);
            fY = resolveLengthOr(fDocState.y, cy, 0);
            fWidth = resolveLengthOr(fDocState.width, cw, fImage.size().w);
            fHeight = resolveLengthOr(fDocState.height, ch, fImage.size().h);
        }


        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            if (fImage.empty())
                return;
            
            if (fWidth <= 0 || fHeight <= 0)
                return;

            // The image's intrinsic size in pixels
            const double iw = double(fImage.size().w);
            const double ih = double(fImage.size().h);
            if (iw <= 0.0 || ih <= 0.0)
                return;

            // We want to apply the preserveAspectRatio rules to determine 
            // how to fit the image into the specified width/height.
            const BLRect viewport{ fX, fY, fWidth, fHeight };
            const BLRect viewBox{ 0, 0, iw, ih };

            BLMatrix2D xform{};
            if (!computeViewBoxToViewport(viewport, viewBox, fPAR, xform))
                return;

            ctx->push();
            
            //if (fHasTransform)
            //{
                // Apply the image's own transform, if it has one, 
                // on top of the preserveAspectRatio transform
            //    ctx->applyTransform(fTransform);
            //}

            // for SLICE, we need to crop to the viewport, so we set a clip
            if (fPAR.align() != AspectRatioAlignKind::SVG_ASPECT_RATIO_NONE &&
                fPAR.meetOrSlice() == AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_SLICE)
            {
                ctx->clipRect(viewport);
            }


            // Apply mapping and draw the image in its intrinsic coordinate space.
            ctx->applyTransform(xform);

            //draw at (0,0) in image pixel space
            ctx->image(fImage, 0, 0);
            ctx->pop();
        }

    };
}
