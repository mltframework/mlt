---
description: "Use when writing or editing MLT filter, producer, consumer, or transition plugins in src/modules/. Covers extern C for C++ init functions, private property naming, and thread-safe per-frame property access."
applyTo: "src/modules/**"
---

## C++ init functions — mandatory `extern "C"`

Every `filter_<name>_init` (or `producer_*`, `consumer_*`, `transition_*`) in a `.cpp` file **must** be wrapped:

```cpp
extern "C" {

mlt_filter filter_<name>_init(mlt_profile profile,
                               mlt_service_type type,
                               const char *id,
                               char *arg)
{ ... }

} // extern "C"
```

Without this, C++ name mangling prevents the dynamic loader from finding the symbol. The service silently fails to load — no error is logged.

## Private properties — `_` prefix

Properties prefixed with `_` are **never serialized to XML**. Use this for:
- Cached data (`"_subtitles"`, `"_mtime"`)
- Owned C++ objects stored via `mlt_properties_set_data`
- Sub-filter handles (`"_t"`)
- One-shot reset flags (`"_reset"`)

Do **not** prefix user-visible parameters with `_`. Do **not** list `_`-prefixed properties in `.yml` metadata files.

## Thread-safe per-frame state — `mlt_frame_unique_properties`

`filter_get_image` runs on worker threads concurrently for different frames. Rules:

- **Reading** from shared filter properties is safe.
- **Writing** to shared filter properties is a data race — never do it.
- Use `mlt_frame_unique_properties(frame, MLT_FILTER_SERVICE(filter))` to get a per-frame writable property bag.

For delegating filters, pass properties to the **frame's** unique bag, not to the sub-filter's own properties:

```c
// CORRECT — thread-safe
mlt_properties fp = mlt_frame_unique_properties(frame, MLT_FILTER_SERVICE(sub_filter));
mlt_properties_pass_list(fp, MLT_FILTER_PROPERTIES(filter), "key1 key2");

// WRONG — data race across concurrent frames
mlt_properties_pass_list(MLT_FILTER_PROPERTIES(sub_filter),
                         MLT_FILTER_PROPERTIES(filter), "key1 key2");
```
