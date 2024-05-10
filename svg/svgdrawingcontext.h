#include "irendersvg.h"

namespace waavs
{
    class SvgDrawingContext : public IRenderSvg
    {
        bool fOwnsImage;
        BLImage fImage;
        BLContext fContext;

    public:
        SvgDrawingContext();
        virtual ~SvgDrawingContext();

        // A couple of factory methods to create the context
        // with a BLImage backing store
        // Returns a shared pointer to the context
        static std::shared_ptr<SvgDrawingContext> create(const size_t width, const size_t height)
        {
            return std::make_shared<SvgDrawingContext>(width, height);
        }

        // Create a context given data; width, height, stride, pixel format and data
        static std::shared_ptr<SvgDrawingContext> create(const size_t width, const size_t height, const size_t stride, const BLFormat format, const void *data)
        {
            return std::make_shared<SvgDrawingContext>(width, height, stride, format, data);
        }

        // Create from an existing BLContext
        static std::shared_ptr<SvgDrawingContext> create(const BLContext &context)
        {
            return std::make_shared<SvgDrawingContext>(context);
        }
    };
}