# MLT Framework — Copilot Instructions

MLT is a LGPL multimedia framework for video editing, written primarily in C with C++ for Qt-dependent modules.
Version: 7.x (see `CMakeLists.txt`). Schema v7.0 metadata YAML files describe every plugin service.

---

## Build

```bash
# Standard out-of-tree build
# Configure (from repo root; output goes to build/cc-debug)
cmake --preset cc-debug

# Reformat code with clang-format
cmake --build build/cc-debug --target clang-format

# Build
cmake --build build/cc-debug

# Install (optional, default prefix is $HOME/opt)
cmake --install build/cc-debug

# Or install to system (also optional, requires sudo)
cmake --install build/cc-debug --prefix /usr/local

# With vcpkg preset (BUILD_TESTING=ON included)
cmake --preset vcpkg          # or vcpkg-ninja
cmake --build build --parallel $(($(nproc) - 1))
```

After building, source `setenv` from the repo root to configure runtime paths for the uninstalled build:

```bash
ln -s build/cc-debug/out
source setenv    # bash only; sets MLT_REPOSITORY, LD_LIBRARY_PATH, etc.
```

Check formatting (CI enforces clang-format-14):

```bash
cmake --build build/cc-debug --target clang-format-check  # from build dir configured with -DCLANG_FORMAT=ON
```

## Tests

```bash
ln -s build/cc-debug/out
source ../setenv
cd build
ctest --output-on-failure
```

Test suites live in `src/tests/` (one subdirectory per area: `test_filter`, `test_properties`, etc.).

---

## Architecture

```
src/
  framework/   Pure-C core library: mlt_filter, mlt_frame, mlt_properties,
               mlt_producer, mlt_consumer, mlt_tractor, mlt_playlist, mlt_service
  modules/     Optional plugins (one shared library each, loaded at runtime)
  melt/        CLI tool
  mlt++/       C++ wrapper classes
  swig/        Language bindings (Python, etc.)
  tests/       Unit tests
```

Plugins are enabled at configure time via `MOD_*` CMake options (e.g. `MOD_QT6`, `MOD_PLUS`).

---

## Module / Plugin Conventions

Each module in `src/modules/<name>/` contains:

| File | Purpose |
|------|---------|
| `factory.c` | Registers all services with `mlt_register_*`; declares `extern` init functions |
| `filter_<name>.c/.cpp` | Filter implementation |
| `filter_<name>.yml` | Metadata descriptor (schema_version 7.0) |
| `producer_<name>.c/.yml`, `consumer_<name>.c/.yml`, `transition_<name>.c/.yml` | Same pattern for other service types |
| `CMakeLists.txt` | Module-local build rules |

**Language by module:**
- `src/framework/` and most modules (core, avformat, plus majority) → pure C (`.c`)
- Modules requiring Qt (`qt/`) → C++ (`.cpp`)
- `plus/` is mixed: mostly C with a few `.cpp` files for subtitle/gradient helpers
- `mlt++/` → C++

---

## Filter Implementation Pattern

### Pixel-processing filter (push/pop)

```c
static int filter_get_image(mlt_frame frame, uint8_t **image,
    mlt_image_format *format, int *width, int *height, int writable)
{
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    // read properties, call mlt_frame_get_image(), process pixels
    return mlt_frame_get_image(frame, image, format, width, height, writable);
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}
```

### Delegating filter (pass-through to another filter)

```c
static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_filter other = (mlt_filter) mlt_properties_get_data(
        MLT_FILTER_PROPERTIES(filter), "_other", NULL);
    mlt_properties text_props = mlt_frame_unique_properties(frame, MLT_FILTER_SERVICE(other));
    mlt_properties_pass_list(text_props, MLT_FILTER_PROPERTIES(filter), "key1 key2 key3");
    return mlt_filter_process(other, frame);
}
```

### Init function

```c
extern "C"   // for .cpp files
mlt_filter filter_<name>_init(mlt_profile profile,
    mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter) {
        mlt_properties p = MLT_FILTER_PROPERTIES(filter);
        mlt_properties_set_string(p, "key", "default");
        mlt_properties_set_int(p, "_filter_private", 1);  // suppress in serialization
        filter->process = filter_process;
    }
    return filter;
}
```

---

## Key API Reference

| API | Use |
|-----|-----|
| `MLT_FILTER_PROPERTIES(f)` | Get `mlt_properties` from a filter |
| `MLT_FILTER_SERVICE(f)` | Get `mlt_service` from a filter |
| `MLT_FRAME_PROPERTIES(f)` | Get `mlt_properties` from a frame |
| `mlt_properties_set/get` | String property access |
| `mlt_properties_set_int/get_int` | Integer property access |
| `mlt_properties_set_data/get_data` | Store owned C++ objects with destructor |
| `mlt_properties_set_int64/get_int64` | 64-bit integer (e.g. timestamps) |
| `mlt_properties_pass_list(dst, src, "k1 k2")` | Copy named properties between objects |
| `mlt_properties_anim_get_double(p, "key", pos, len)` | Keyframed (animated) value |
| `mlt_properties_exists(p, "key")` | Test presence before reading |
| `mlt_frame_unique_properties(frame, service)` | Per-frame filter properties (thread-safe) |
| `mlt_events_listen(p, owner, "property-changed", cb)` | React to property changes |
| `mlt_slices_run_normal(n, proc, cookie)` | Multithreaded pixel loop |
| `mlt_log_debug/info/error(service, fmt, ...)` | Structured logging |

**Private properties** (not serialized to XML) must be prefixed with `_` (e.g. `"_subtitles"`, `"_mtime"`).

---

## YAML Metadata (.yml)

Schema version: **7.0**. Validated against `src/framework/metaschema.yaml`.

Required top-level fields: `schema_version`, `type`, `identifier`, `language`.

Parameter `type` values: `string`, `integer`, `float`, `boolean`, `rect`, `color`, `binary`.

Widget hints: `spinner`, `slider`, `combo`, `checkbox`, `color`, `text`.

Add `animation: yes` for keyframeable parameters, `mutable: yes` for runtime-changeable ones.

---

## Naming Conventions

- **Source files:** `filter_<name>.c/.cpp`, `producer_<name>.c`, `transition_<name>.c`, `consumer_<name>.c`
- **Init function:** `filter_<name>_init(...)` — must be declared `extern "C"` in `.cpp` files
- **Internal static functions:** `filter_process`, `filter_get_image`, `property_changed` — `snake_case`
- **Structs:** `snake_case`
- **C++ classes:** `PascalCase` (e.g. `Subtitles::SubtitleVector`)
- **Private/internal properties:** prefix with `_`

---

## Code Style

- 4-space indentation (no tabs)
- Formatting enforced by `.clang-format` (derived from Qt Creator style)
- Run `make clang-format-check` before committing; CI will fail on violations
- `formatOnSave` is enabled in the dev container VS Code settings
- Always update the copyright year in the header of any modified file

---

## Dev Container

The repo includes `.devcontainer/` with a `devcontainer.json` and `Dockerfile`.

- Default "Restricted Mode": audio routed to dummy driver, GPU via DRI passthrough only
- For full hardware audio: uncomment `--privileged` / `--ipc=host` in `devcontainer.json` and set `SDL_AUDIODRIVER=alsa` or `pulseaudio`
- After rebuilding: `Dev Containers: Rebuild Container` in VS Code

---

## Common Pitfalls

- **Thread safety:** use `mlt_frame_unique_properties(frame, service)` when setting per-frame filter properties; never write directly to shared filter properties during `filter_get_image`.
- **Missing `extern "C"`:** every `filter_*_init` in a `.cpp` file must be wrapped in `extern "C" { ... }` so the loader can find it.
- **Property serialization:** properties starting with `_` are private and not written to XML. Use this for cached data, file modification times, sub-filter handles, and reset flags.
- **`mlt_properties_pass_list` in `filter_process`:** pass properties to the sub-filter on `mlt_frame_unique_properties`, not on the sub-filter's own properties, to avoid data races.
- **YAML `readonly`:** set `readonly: no` on any parameter the user is expected to set; omitting it defaults to read-only in some tooling.
- **Delegating to `qtext` vs `text`:** `filter_subtitle` tries `qtext` first, then falls back to `text`. Properties specific to `qtext` (e.g. `typewriter.*`) are silently ignored by the `text` fallback.
