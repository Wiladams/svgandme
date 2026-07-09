#pragma once

#include "render_interface.h"


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
