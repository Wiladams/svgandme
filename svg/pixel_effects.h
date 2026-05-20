#pragma once

#include "filter_program_exec_b2d.h"
#include "svgfilter.h"
#include "blend2d_connect.h"
#include "svgb2ddriver.h"
#include "svgfactory.h"

//
// A standalone single function application of SVG filtering
//
namespace waavs
{
    static WGResult applyPixelEffects(const ByteSpan& pspan,
        BLImage& srcBitmap, 
        BLImage& dstBitmap) noexcept;

    // createPixelEffects()
    //
    // Given a ByteSpan containing an SVG filter element (or document), 
    // parse it and create a FilterProgramStream that can be executed.
    static std::shared_ptr<FilterProgramStream> createPixelEffects(ByteSpan pspan) noexcept
    {
        // We have to treat the fragment as a complete xml document
        // Then, the root element should be the filter element, 
        // and we can pull the program stream from it.


        auto doc = SVGFactory::createFromChunk(pspan, 0,0);

        auto root = doc->rootNode();
        if (!root)
            return nullptr;

        auto filterElement = dynamic_cast<SVGFilterElement*>(root.get());
        if (!filterElement)
            return nullptr;

        
        // So, we just assume it was successful
        // Now we can transfer the program to the output
        auto programStream = filterElement->getFilterProgramStream(nullptr);

        return programStream;
    }

    // applyPixelEffects()
    // 
    // Execute the pixel effects program, using the SourceGraphic as input
    // and writing to dstGraphic as output.
    //
    static WGResult applyEffectsProgram(
        const FilterProgramStream& program,
        const Surface& srcSurface,
        Surface& dstSurface) noexcept
    {
        if (srcSurface.empty() || dstSurface.empty())
            return WG_ERROR_Invalid_Argument;

        WGRectD filterRectUS{
            0,
            0,
            (double)srcSurface.width(),
            (double)srcSurface.height()
        };

        WGRectD objectBBoxUS = filterRectUS;

        FilterRunState runState{};
        runState.filterUnits = program.filterUnits;
        runState.primitiveUnits = program.primitiveUnits;
        runState.colorInterpolation = program.colorInterpolation;
        runState.filterRectUS = filterRectUS;
        runState.objectBBoxUS = objectBBoxUS;

        WGMatrix3x3 ctm = WGMatrix3x3::makeIdentity();

        FilterSpace fSpace{};
        fSpace.sx = 1.0;
        fSpace.sy = 1.0;
        fSpace.filterRectUS = filterRectUS;
        fSpace.filterExtentUS = WGRectD{ 0.0, 0.0, filterRectUS.w, filterRectUS.h };
        fSpace.filterRectPX = WGRectI{
            0,
            0,
            (int)filterRectUS.w,
            (int)filterRectUS.h
        };
        fSpace.ctm = ctm;
        fSpace.invCtm = ctm;
        (void)fSpace.invCtm.invert();

        B2DFilterExecutor executor;
        executor.setLastKey({});
        executor.fRunState = runState;
        executor.fSpace = fSpace;

        IAmGroot* groot = nullptr;

        SVGB2DDriver ctx{};
        ctx.attach(dstSurface, 1);
        ctx.renew();

        B2DFilterResourceResolver<Surface> fResolver(groot, &ctx, &executor);

        if (!executor.putImage(filter::SourceGraphic(), srcSurface))
            return WG_ERROR_Invalid_Argument;

        Surface srcAlpha = executor.makeSourceAlpha(srcSurface);
        if (srcAlpha.empty())
            return WG_ERROR_Invalid_Argument;

        if (!executor.putImage(filter::SourceAlpha(), srcAlpha))
            return WG_ERROR_Invalid_Argument;

        if (!executor.execute(program, executor))
            return WG_ERROR_Invalid_Argument;

        InternedKey outKey = executor.lastKey();
        if (!outKey)
            outKey = filter::Filter_Last();

        Surface outImg = executor.getImage(outKey);
        if (outImg.empty())
            outImg = executor.getImage(filter::SourceGraphic());

        if (outImg.empty())
            return WG_ERROR_Invalid_Argument;

        ctx.image(outImg, 0, 0);
        ctx.flush();
        ctx.detach();

        return WG_SUCCESS;
    }

    // 
    // applyPixelEffects()
    //
    // 
    static WGResult applyPixelEffects(const ByteSpan &pspan,
        Surface& srcBitmap, Surface& dstBitmap) noexcept
    {
        auto programStream = createPixelEffects(pspan);
        if (!programStream)
            return WG_ERROR_Invalid_Argument;
        return applyEffectsProgram(*programStream, srcBitmap, dstBitmap);
    }
}