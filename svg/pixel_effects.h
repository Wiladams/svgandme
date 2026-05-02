#pragma once

#include "filter_program_exec_b2d.h"
#include "svgfilter.h"
#include "blend2d_connect.h"
#include "svgb2ddriver.h"


namespace waavs
{
    static std::shared_ptr<FilterProgramStream> createPixelEffects(ByteSpan pspan) noexcept
    {
        // Create an XmlPull from the span,
        // Parse the filter element and its children, and build a filter program.
        // Let's see if we can get away with just loading a document fragment
        // we might need to require a complete document, so that
        // groot exists, and things can be looked up there, like
        // for chained filters, although that doesn't really seem to be a thing
        // in practice.

        XmlPull xp(pspan);
        SVGFilterElement programElement{};
        programElement.loadFromXmlPull(xp, nullptr);
        
        // So, we just assume it was successful
        // Now we can transfer the program to the output
        auto programStream = programElement.getFilterProgramStream(nullptr);

        return programStream;
    }

    // applyPixelEffects()
    // 
    // Execute the pixel effects program, using the SourceGraphic as input
    // and writing to dstGraphic as output.
    //
    static WGResult applyEffectsProgram(const FilterProgramStream& program, 
        BLImage &srcBitmap, BLImage &dstBitmap) noexcept
    {
        Surface srcSurface = surfaceFromBLImage(srcBitmap);
        
        // Establish the filter region and object bounding box in user space.
        WGRectD filterRectUS{ 0, 0, (double)srcSurface.width(), (double)srcSurface.height() };
        WGRectD objectBBoxUS = filterRectUS; // In SVG, the primitive subregion is always the filter region, so we can just use filterRectUS here.
        
        // Setup runstate as soon as possible
        FilterRunState runState{};

        runState.filterUnits = program.filterUnits;
        runState.primitiveUnits = program.primitiveUnits;
        runState.colorInterpolation = program.colorInterpolation;
        runState.filterRectUS = filterRectUS;
        runState.objectBBoxUS = objectBBoxUS;

        WGMatrix3x3 ctm = WGMatrix3x3::makeIdentity();
        FilterSpace fSpace{};
        fSpace = {};
        fSpace.sx = 1.0;
        fSpace.sy = 1.0;
        fSpace.filterRectUS = filterRectUS;
        fSpace.filterExtentUS = WGRectD{ 0.0, 0.0, filterRectUS.w, filterRectUS.h };
        fSpace.filterRectPX = WGRectI{ 0, 0, filterRectUS.w, filterRectUS.h };
        fSpace.ctm = ctm;
        fSpace.invCtm = ctm;
        (void)fSpace.invCtm.invert();

        // Setup the executor to start
        B2DFilterExecutor executor;
        executor.setLastKey({});

        // Setup drawing context on the output surface
        IAmGroot* groot = nullptr; 

        Surface dstSurface = surfaceFromBLImage(dstBitmap);
        SVGB2DDriver ctx{};
        ctx.attach(dstSurface, 1);
        ctx.renew();

        // Setup resolver, so intermediate images can be handled
        B2DFilterResourceResolver<Surface> fResolver(groot, &ctx, &executor);

        // Put the source graphic into the executor's registry 
        // under the reserved key "SourceGraphic"
        InternedKey srcGraphicKey = filter::SourceGraphic();
        if (!executor.putImage(srcGraphicKey, srcSurface))
        {
            return WG_ERROR_Invalid_Argument;
        }

        auto srcAlpha = executor.makeSourceAlpha(srcSurface);
        if (srcAlpha.empty())
            return WG_ERROR_Invalid_Argument;

        if (!executor.putImage(filter::SourceAlpha(), srcAlpha))
            return WG_ERROR_Invalid_Argument;

        // Execute the program
        if (!executor.execute(program, executor))
            return WG_ERROR_Invalid_Argument;

        // Resolve final output surface
        InternedKey outKey = executor.lastKey();
        if (!outKey)
            outKey = filter::Filter_Last();

        Surface outImg = executor.getImage(outKey);
        if (outImg.empty())
            outImg = executor.getImage(filter::SourceGraphic());
        if (outImg.empty())
            return false;

        // --------------------------------------------------
        // Composite result back to main context
        // --------------------------------------------------


        ctx.image(outImg, 0, 0);
        ctx.flush();



        return WG_SUCCESS;
    }

    // 
    // applyPixelEffects()
    //
    // 
    static WGResult applyPixelEffects(const ByteSpan &pspan,
        BLImage& srcBitmap, BLImage& dstBitmap) noexcept
    {
        auto programStream = createPixelEffects(pspan);
        if (!programStream)
            return WG_ERROR_Invalid_Argument;
        return applyEffectsProgram(*programStream, srcBitmap, dstBitmap);
    }
}