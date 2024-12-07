extern "C" {
#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_slices.h>
}

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#define MAX_STOPS 32

struct stop
{
    mlt_color color;
    double pos;
};

bool operator<(const stop &lhs, const stop &rhs)
{
    return lhs.pos < rhs.pos;
}

bool operator==(const stop &lhs, const stop &rhs)
{
    return lhs.pos == rhs.pos;
}

struct gradient_cache
{
    std::vector<stop> gradient;
    std::vector<mlt_color> colors;
};

typedef std::map<std::string, gradient_cache> gradientmap_cache;

struct slice_desc
{
    mlt_image_s image;
    const std::vector<mlt_color> *colors;
};

static std::string gradient_key(mlt_properties props)
{
    int count = mlt_properties_count(props);
    std::string res;

    // This function will ignore tailing color.* values if one of the indices
    // is missing.
    for (int i = 1; i <= MIN(count, MAX_STOPS); ++i) {
        char name[9] = "";
        sprintf(name, "stop.%d", i);
        char *value = mlt_properties_get(props, name);
        if (!value)
            break;
        res += value;
        res += ";";
    }

    // Gradient cache keys are stops joined by semicolons.
    return res;
}

static int parse_color(const char *value, char **end)
{
    // Parse a hex color value as #RRGGBB or #AARRGGBB.
    if (value[0] == '#') {
        unsigned int rgb = strtoul(value + 1, end, 16);
        unsigned int alpha = (value - *end > 7) ? (rgb >> 24) : 0xff;
        return (rgb << 8) | alpha;
    }
    // Do hex and decimal explicitly to avoid decimal value with leading zeros
    // interpreted as octal.
    else if (value[0] == '0' && value[1] == 'x') {
        return strtoul(value + 2, end, 16);
    } else {
        return strtol(value, end, 10);
    }
}

static void deserialize_gradient(mlt_properties props, std::vector<stop> &gradient)
{
    int count = mlt_properties_count(props);

    for (int i = 1; i <= MIN(count, MAX_STOPS); ++i) {
        char name[9];
        sprintf(name, "stop.%d", i);
        char *value = mlt_properties_get(props, name);
        if (!value)
            break;
        char *p = nullptr;
        int c = parse_color(value, &p);
        value = p;
        double pos = strtod(value, &p);
        if (p == value)
            continue;
        if (p[0] == '%')
            pos /= 100.0;

        mlt_color color = {uint8_t((c >> 24) & 0xff),
                           uint8_t((c >> 16) & 0xff),
                           uint8_t((c >> 8) & 0xff),
                           uint8_t(c & 0xff)};
        gradient.push_back({color, pos});
    }

    if (gradient.size() == 0) {
        // Use a black to white range by default.
        gradient.push_back({{0x00, 0x00, 0x00, 0xff}, 0.0});
        gradient.push_back({{0xff, 0xff, 0xff, 0xff}, 1.0});
    } else {
        // Sort the gradient vector using a stable sort.
        std::stable_sort(gradient.begin(), gradient.end());
        // Deduplicate stops by their position in the gradient, first come,
        // only served.
        auto last = std::unique(gradient.begin(), gradient.end());
        gradient.erase(last, gradient.end());
    }
}

static void compute_colors(const std::vector<stop> &gradient, std::vector<mlt_color> &colors)
{
    for (size_t c = 0; c < colors.size(); ++c) {
        // Intensity is proportional to its order in the cache.
        float intensity = float(c) / float(colors.size());
        const stop &front = gradient.front();
        const stop &back = gradient.back();
        // Any value positioned before or after the extremes maps to those.
        if (intensity <= front.pos) {
            colors[c] = front.color;
            continue;
        } else if (intensity >= back.pos) {
            colors[c] = back.color;
            continue;
        }
        for (size_t i = 0; i < gradient.size() - 1; ++i) {
            const stop &s = gradient[i];
            const stop &e = gradient[i + 1];
            // Only care if the intensity is bounded by start and end.
            if (intensity > e.pos) {
                continue;
            } else if (intensity == s.pos) {
                colors[c] = s.color;
                break;
            } else if (intensity == e.pos) {
                colors[c] = e.color;
                break;
            }
            // This interpolation can result in values outside the 0 to 1 range
            // due to roundoff errors.
            double pos = (intensity - s.pos) / (e.pos - s.pos);
            // Calculated color values can be above 255 or below 0 due to above,
            // so clamp it to avoid incorrect values in the color cache.
            colors[c].r = CLAMP(pos * e.color.r + (1 - pos) * s.color.r, 0, 255);
            colors[c].g = CLAMP(pos * e.color.g + (1 - pos) * s.color.g, 0, 255);
            colors[c].b = CLAMP(pos * e.color.b + (1 - pos) * s.color.b, 0, 255);
            colors[c].a = CLAMP(pos * e.color.a + (1 - pos) * s.color.a, 0, 255);
            break;
        }
    }
}

static mlt_color gradient_map(
    const std::vector<mlt_color> &colors, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    // This is what Krita does, although other methods could be explored.
    float intensity = (r * 0.30 + g * 0.59 + b * 0.11) / 255;
    uint32_t t = intensity * colors.size() + 0.5;
    if (colors.size() > t) {
        mlt_color c = colors[t];
        c.a = a;
        return c;
    } else {
        // Because the colors are sorted by position this is the last of the
        // gradient stops.
        return colors.back();
    }
}

static int sliced_proc(int id, int index, int jobs, void *data)
{
    (void) id;
    slice_desc *desc = reinterpret_cast<slice_desc *>(data);
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, desc->image.height, &slice_line_start);
    int slice_line_end = slice_line_start + slice_height;
    int line_size = desc->image.strides[0];
    for (int j = slice_line_start; j < slice_line_end; ++j) {
        uint8_t *p = desc->image.planes[0] + j * line_size;
        for (int i = 0; i < line_size; i += 4) {
            mlt_color c = gradient_map(*desc->colors, p[i], p[i + 1], p[i + 2], p[i + 3]);
            p[i] = c.r;
            p[i + 1] = c.g;
            p[i + 2] = c.b;
            p[i + 3] = c.a;
        }
    }
    return 0;
}

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter filter = reinterpret_cast<mlt_filter>(mlt_frame_pop_service(frame));
    gradientmap_cache *cache = reinterpret_cast<gradientmap_cache *>(filter->child);

    *format = mlt_image_rgba;
    int error = mlt_frame_get_image(frame, image, format, width, height, 1);

    if (error == 0) {
        // As odd as it sounds, a nested cache is used to speed things up. A
        // parent holds as many children identified by a key computed from the
        // color stops and each one of those contains cached results calculated
        // from the gradient. A color cache contains width plus height entries,
        // so this might result in a loss of color resolution. Else, the process
        // is really slow (O(n^3)) if each pixel has to be computed.
        std::string k = gradient_key(MLT_FILTER_PROPERTIES(filter));
        auto it = cache->find(k);
        if (it == cache->end()) {
            it = cache->insert(it, {k, {}});
            gradient_cache &e = it->second;
            e.gradient.reserve(MAX_STOPS);
            e.colors.resize(*width + *height, {0x00, 0x00, 0x00, 0xff});
            deserialize_gradient(MLT_FILTER_PROPERTIES(filter), e.gradient);
            compute_colors(e.gradient, e.colors);
        }
        const std::vector<mlt_color> &colors = it->second.colors;

        slice_desc desc;
        desc.colors = &colors;
        mlt_image_set_values(&desc.image, *image, *format, *width, *height);
        mlt_slices_run_normal(0, sliced_proc, &desc);
    }

    return error;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    // Push the frame filter
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

static void filter_close(mlt_filter filter)
{
    gradientmap_cache *cache = reinterpret_cast<gradientmap_cache *>(filter->child);

    delete cache;
    filter->child = nullptr;
    filter->close = nullptr;
    filter->parent.close = nullptr;
    mlt_service_close(&filter->parent);
}

extern "C" {

mlt_filter filter_gradientmap_init(mlt_profile profile,
                                   mlt_service_type type,
                                   const char *id,
                                   char *arg)
{
    mlt_filter filter = mlt_filter_new();
    gradientmap_cache *cache = new gradientmap_cache;
    if (filter && cache) {
        filter->process = filter_process;
        filter->close = filter_close;
        filter->child = cache;
    }
    return filter;
}
}
