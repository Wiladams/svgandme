## SVGandMe PathSegment Pipeline Architecture

### Overview

The `svgandme` library implements a lightweight, modular, and composable **streaming vector pipeline** based on a `PathSegment` structure. Each stage of the pipeline consumes and optionally transforms path segments into new ones. It allows for arbitrary post-processing such as dashing, outline generation, variable width, and final figure assembly.

---

### Core Data Structure: `PathSegment`

```cpp
struct PathSegment final {
    float fArgs[8];              // 32 bytes: argument values (e.g., x, y, rx, ry, etc.)
    char fArgTypes[8];           // 8 bytes: semantic tags ('x', 'y', 'r', 'flag', etc.)
    uint8_t fArgCount;           // 1 byte: number of arguments in use
    SVGPathCommand fSegmentKind; // 1 byte: SVG command character ('M', 'L', etc.)
    uint8_t fIteration;          // 1 byte: optional loop or sampling iteration
    uint8_t _pad;                // 1 byte: alignment
    uint32_t _reserved;          // 4 bytes: reserved for future expansion
};
```

> 🧠 Total size: 48 bytes, cache-aligned and compact.

---

### Command Type: `SVGPathCommand`

```cpp
enum class SVGPathCommand : uint8_t {
    M = 'M', m = 'm', // MoveTo
    L = 'L', l = 'l', // LineTo
    H = 'H', h = 'h', // Horizontal LineTo
    V = 'V', v = 'v', // Vertical LineTo
    C = 'C', c = 'c', // Cubic Bezier
    S = 'S', s = 's', // Smooth Cubic Bezier
    Q = 'Q', q = 'q', // Quadratic Bezier
    T = 'T', t = 't', // Smooth Quadratic Bezier
    A = 'A', a = 'a', // Arc
    Z = 'Z', z = 'z'  // ClosePath
};
```

---

### Interfaces

```cpp
template<typename T>
struct IPipelineSource {
    virtual bool next(T& out) = 0;
    virtual ~IPipelineSource() = default;
};

template<typename T>
struct IPipelineSink {
    virtual void push(const T& item) = 0;
    virtual void finish() = 0;
    virtual ~IPipelineSink() = default;
};

template<typename T>
struct IPipelineFilter : public IPipelineSource<T>, public IPipelineSink<T> {
    virtual void setInput(std::function<bool(T&)>) = 0;
};
```

These provide the standard model:

* **Source** generates segments.
* **Filter** transforms them.
* **Sink** consumes and finalizes.

---

### Example Pipeline

```cpp
auto parser = std::make_shared<PathParser>(svgPathD);
auto dash = std::make_shared<PVXDashFilter>(std::vector<double>{5.0, 3.0});
dash->setInput([&](PathSegment& s) { return parser->next(s); });

PathSegment seg;
while (dash->next(seg)) {
    builder.push(seg); // e.g., PathSegmentFabricSink
}
builder.finish();
```

---

### Pipeline Stages

| Stage                   | Description                                         |
| ----------------------- | --------------------------------------------------- |
| `PathParser`            | Parses SVG path `d` attribute into `PathSegment`s   |
| `PVXDashFilter`         | Splits segments into dashed segments by arc length  |
| `PVXStrokeBrush`        | Converts segments into outline polygons with width  |
| `PathSegmentFabricSink` | Emits a Fabric.js-compatible JSON path              |
| `PathSegmentJsonSink`   | Emits a generic JSON array of segments              |
| `PVXBuilder`            | Assembles segments into composite paths and figures |

---

### Design Principles

* **Streaming model**: Each stage consumes and yields one segment at a time
* **Composable**: Filters can be chained with minimal coupling
* **Data-driven**: `argTypes[]` allow interpretation of meaning, not just position
* **Flexible**: Allows dash filters, variable stroke, symbol stamping, glyphs, etc.

---

### Future Extensions

* `PathSegmentSymbolFilter` to place symbols using tangent/normal
* `PathSegmentOutlineBuilder` for accurate offsetting
* `PathSegmentGCodeSink` for CNC/G-code export
* Coordinate transforms (`TransformFilter`) as reusable building blocks
