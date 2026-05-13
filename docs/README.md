SVGAndMe is an implementation of the SVG specifications, that uses the blend2d library as a backend renderer.  This is a 'header-only' implmentation written in C++.  The coded started out fairly modestly, only supporting what was in the 'SVG Tiny 1.2' specification, but it has since grown, covering a large part of 1.1, and even the 2.0 spec.

In 2025 - 2026, there has been a good effort towards simplification, clarity, and stability.  Along the way, several feature areas such as filters, masking, clipping, and dashed line support, have been added.

There are extras, that are customizations, done in the 'waavs' namespace.  Namely: supporting media streams using ffmpeg, supporting screen capture, supporting conic gradients (a blend2d feature).

Of all these features, the SVG Filter Effects is probably the largest component.  The code there is easily as large as the rest of the vector drawing code.

At this point, May 13 2026, the work is to continue stabilization, while adding more formance enhancements, such as platform specific SIMD.  Other than that, There are literally thousands of SVG files which have been confirmed to work with the code.  There are still plenty of features which do not work, but for most modern svg, things will just work.



# Usage
All that is required is to include svg.h into your application.  The svgimage demo app shows how this can be done.  svgviewer shows how to connect to a UI based app and do more advanced navigation of images.

# Specifications References
<a href=https://www.w3.org/TR/SVGMobile/>SVG Tiny 1.2</a><br>
<a href=https://www.w3.org/TR/SVG/>SVG Full 2.0</a><br>

# Supported Elements and Attributes

At this point, most of the relevant SVG 1.1 elements and attributes will work as expected.


## Not supported</br>
- Animation  - The svgviewer demo app shows how a frame event can be supported, but general animation is not yet supported
- Scripting
- Events


## Notes
- A potential embedded javascript - https://mujs.com/
- Maybe support Lua as a scripting language
