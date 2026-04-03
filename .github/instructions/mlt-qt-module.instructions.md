---
description: "Use when writing or editing Qt module plugins in src/modules/qt/. Covers QApplication bootstrapping, QImage/QPainter pixel conversion, common.h utilities, Mlt C++ wrappers, animated property access, and Qt-version compatibility guards."
applyTo: "src/modules/qt/**"
---

## Bootstrap: `createQApplicationIfNeeded`

Every Qt filter init function must call `createQApplicationIfNeeded` before using any Qt class.
If it returns `false`, close the filter and return `nullptr` — no display server is available.

```cpp
#include "common.h"

mlt_filter filter_foo_init(...)
{
    mlt_filter filter = mlt_filter_new();
    if (!filter) return nullptr;

    if (!createQApplicationIfNeeded(MLT_FILTER_SERVICE(filter))) {
        mlt_filter_close(filter);
        return nullptr;
    }
    // ...
}
```

---

## Image Format Selection: `choose_image_format`

Use `choose_image_format` to respect the upstream format request (supports `mlt_image_rgba64`):

```cpp
*image_format = choose_image_format(*image_format);
int error = mlt_frame_get_image(frame, image, image_format, width, height, writable);
```

Never hard-code `mlt_image_rgba` in Qt filters — `rgba64` is supported and improves quality.

---

## MLT ↔ QImage Pixel Conversion

Two helpers in `common.h` handle the buffer sharing between `mlt_frame` and `QImage`.
The `QImage` wraps the existing MLT buffer — **no copy is made**:

```cpp
// MLT buffer → QImage (wraps in-place, writable=1 required upstream)
QImage qimg;
convert_mlt_to_qimage(*image, &qimg, *width, *height, *image_format);

// ... render into qimg with QPainter ...

// QImage → MLT buffer (no-op: same pointer, just asserts)
convert_qimage_to_mlt(&qimg, *image, *width, *height);
```

`convert_mlt_to_qimage` selects `QImage::Format_RGBA64` or `Format_RGBA8888` based on format.
`convert_qimage_to_mlt` asserts that `*image == qimg.constBits()` — do not replace the buffer.

---

## Creating a Blank Frame: `create_image`

For producer-style filters that generate content from scratch (no upstream image):

```cpp
int error = create_image(frame, image, image_format, width, height, writable);
// Returns a zeroed (transparent) mlt_pool_alloc'd buffer, registered on the frame.
```

This also reads `rescale_width`/`rescale_height` and `meta.media.width`/`meta.media.height`
from the frame to choose the right dimensions.

---

## Mlt C++ Wrapper (`mlt++/`)

Qt filters commonly use the `Mlt::Filter` and `Mlt::Frame` C++ wrappers for animated property access:

```cpp
#include <MltFilter.h>

static int get_image(mlt_frame frame, ...)
{
    auto filter = Mlt::Filter(mlt_filter(mlt_frame_pop_service(frame)));
    auto f = Mlt::Frame(frame);

    mlt_position pos = filter.get_position(f);
    mlt_position len = filter.get_length2(f);

    // Animated double (keyframeable)
    double radius = filter.anim_get_double("radius", pos, len);

    // Animated color — also convert TRC for display
    mlt_color color = filter.anim_get_color("color", pos, len);
    color = ::mlt_color_convert_trc(color, f.get("color_trc"));
    QColor qcolor(color.r, color.g, color.b, color.a);

    // Animated rect (geometry)
    mlt_rect rect = filter.anim_get_rect("geometry", pos, len);
}
```

Use `Mlt::Properties` for convenient property access in init:

```cpp
auto properties = Mlt::Properties(filter);
properties.set("color", "#aa636363");
properties.set("radius", 5.0);
```

---


## Thread Safety with QMutex

Qt rendering (especially `QPainter`, `QTextDocument`) is not thread-safe. Protect with a file-scope mutex when the same static Qt object is accessed from multiple filter instances:

```cpp
static QMutex g_mutex;

static int get_image(mlt_frame frame, ...)
{
    QMutexLocker locker(&g_mutex);
    // ... QPainter / QTextDocument work ...
}
```

Use `QMutexLocker` (RAII) — never call `g_mutex.unlock()` manually.

---

## YAML Color Property Type

For color parameters in Qt filter YAML, use `type: color` (not `type: string`) to enable the color picker widget and animation support:

```yaml
- identifier: color
  title: Color
  type: color
  default: "#aa000000"
  mutable: yes
  widget: color
  animation: yes
```
