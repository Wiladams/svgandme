#pragma once

#include "irendersvg.h"


namespace waavs
{
    struct SvgDrawingContext : public IRenderSVG
    {

    public:

        SvgDrawingContext(FontHandler* fh)
            : IRenderSVG(fh)
        {
        }

        virtual ~SvgDrawingContext()
        {
            BLContext::end();
        }

    };
}
