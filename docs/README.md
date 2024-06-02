SVG is a sprawling complex of specifications.  It is fairly impossible to implement the full specification in a small library.  SVGAndMe provides a very workable subset of the specification to support SVG found in common usage.  It implements enough of the basic features to accurately render thousands of common svg files.  When tayloring your own SVG, if you limit yourself to the features provided, of course you will have great results.

The closes spec SVGAndMe conforms to would be the SVG Tiny 1.2 profile.  As this codebase continues to evolve, it will be to more fully support this particular profile, with some additions along the way to support various other features that have proven to be useful, or in common usage.

In many cases, there is support for things beyond the profile for the purpose of giving a somewhat reasonable, if not complete, rendering.

# Specifications References
<a href=https://www.w3.org/TR/SVGMobile/>SVG Tiny 1.2</a>

# Supported Elements and Attributes

### Meta Information
- Comments
- 'defs' 
- 'meta' 
- 'desc' 
- 'title'

### Basic Shapes

- PolyLines
- circle
- ellipse
- line
- polygon
- polyline
- path
- rect
- Text

### Style
- 'style' element
- 'style' property
- display properties
- CDATA stylesheet

### Attributes
- transform
- opacity
- clip-path (only rectangle clipping)
- fill (all kinds of paint)
- fill-opacity
- fill-rule
- font-size
- font-family
- font-style
- font-stretch
- font-weight
- marker
- marker-start
- marker-mid
- marker_end
- stroke (all kinds of paint)
- stroke-opacity
- stroke-linecap
- stroke-linecap-start
- stroke-linecap-end
- stroke-linejoin
- stroke-miterlimit
- stroke-width
- marker
- paint-order
- systemLanguage
- text-anchor
- vector-effect
- viewBox


### Structural Nodes
- 'a'
- 'g'
- 'image' (png, jpeg, bmp, gif)
- 'svg'
- 'style'
- 'switch'
- 'text'
- 'tspan'
- 'use'

### Non-Structural Nodes
- 'solidColor'
- 'clipPath' (limited to rectangle)
- 'defs'
- 'conicGradient'
- 'linearGradient'
- 'radialGradient'
- 'marker'
- 'pattern'
- 'symbol'

### Font Support
- 'font'
- 'font-face'
- 'glyph'
- 'missing-glyph'
- 'font-face-name'
- 'font-face-src'


## Not supported</br>
- Animation  - Very runtime specific, not likely to ever be included
- Filters    - will depend on future blend2d support
- Path based clipping - will depend on future blend2d support
