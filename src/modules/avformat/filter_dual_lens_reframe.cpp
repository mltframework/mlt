#include "dual_lens_types.h"
#include "osv_vulkan.h"

extern "C" {
#include <framework/mlt.h>
}

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct DualLensReframeContext
{
    std::mutex mutex;
    std::unique_ptr<DualLensVulkanStitcher> stitcher;
    bool initialized = false;

    static std::string resolveReframeShaderPath()
    {
        if (const char *env_path = std::getenv("OSV_REFRAME_SHADER_PATH")) {
            if (std::filesystem::exists(env_path)) {
                return env_path;
            }
        }
        if (const char *mlt_data = std::getenv("MLT_DATA")) {
            std::filesystem::path p1 = std::filesystem::path(mlt_data).parent_path().parent_path()
                                       / "shaders" / "direct_reframe.spv";
            if (std::filesystem::exists(p1))
                return p1.string();
            std::filesystem::path p2
                = std::filesystem::path(mlt_data).parent_path().parent_path().parent_path()
                  / "shaders" / "direct_reframe.spv";
            if (std::filesystem::exists(p2))
                return p2.string();
        }
        try {
            std::filesystem::path exe_dir
                = std::filesystem::read_symlink("/proc/self/exe").parent_path();
            std::vector<std::filesystem::path> candidates
                = {exe_dir / "shaders" / "direct_reframe.spv",
                   exe_dir / ".." / "shaders" / "direct_reframe.spv"};
            for (const auto &c : candidates) {
                if (std::filesystem::exists(c))
                    return c.string();
            }
        } catch (...) {
        }
        return "shaders/direct_reframe.spv";
    }

    bool ensureStitcher()
    {
        if (initialized)
            return true;
        if (!stitcher) {
            stitcher = std::make_unique<DualLensVulkanStitcher>();
            std::string shader_path = resolveReframeShaderPath();
            initialized = stitcher->initializeReframe(shader_path);
            if (!initialized) {
                std::cerr << "[OSV-Reframe-Filter] Failed to initialize Vulkan reframe pipeline: "
                          << shader_path << std::endl;
            }
        }
        return initialized;
    }
};

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter filter = static_cast<mlt_filter>(mlt_frame_pop_service(frame));
    if (!filter) {
        return mlt_frame_get_image(frame, image, format, width, height, writable);
    }

    auto *ctx = static_cast<DualLensReframeContext *>(filter->child);
    mlt_properties filter_props = MLT_FILTER_PROPERTIES(filter);
    mlt_properties frame_props = MLT_FRAME_PROPERTIES(frame);

    int out_w = (*width > 0) ? *width : 1920;
    int out_h = (*height > 0) ? *height : 1080;

    // Pull raw dual-fisheye frame from producer_osv
    int raw_w = 0;
    int raw_h = 0;
    mlt_image_format raw_fmt = mlt_image_rgba;
    uint8_t *raw_img = nullptr;
    int err = mlt_frame_get_image(frame, &raw_img, &raw_fmt, &raw_w, &raw_h, 0);
    if (err != 0 || !raw_img) {
        *image = raw_img;
        *width = raw_w;
        *height = raw_h;
        *format = raw_fmt;
        return err;
    }

    auto *payload = static_cast<DualLensFramePayload *>(
        mlt_properties_get_data(frame_props, "osv.dual_lens_payload", nullptr));
    if (!payload || raw_fmt != mlt_image_rgba || raw_w < 2 || raw_h < 2) {
        // Non-OSV source or fallback: pass through untouched
        *image = raw_img;
        *width = raw_w;
        *height = raw_h;
        *format = raw_fmt;
        return 0;
    }

    mlt_position pos = mlt_filter_get_position(filter, frame);
    mlt_position len = mlt_filter_get_length2(filter, frame);

    ReframeParams params;
    params.yaw_deg = static_cast<float>(
        mlt_properties_anim_get_double(filter_props, "yaw", pos, len));
    params.pitch_deg = static_cast<float>(
        mlt_properties_anim_get_double(filter_props, "pitch", pos, len));
    params.roll_deg = static_cast<float>(
        mlt_properties_anim_get_double(filter_props, "roll", pos, len));
    double fov_val = mlt_properties_anim_get_double(filter_props, "fov", pos, len);
    params.fov_deg = (fov_val > 0.1) ? static_cast<float>(fov_val) : 90.0f;
    params.fisheye = static_cast<float>(
        mlt_properties_anim_get_double(filter_props, "fisheye", pos, len));

    double stitch_radius = mlt_properties_get_double(filter_props, "stitch_radius");
    if (stitch_radius <= 0.0) {
        stitch_radius = 2.2;
    }

    std::lock_guard<std::mutex> lock(ctx->mutex);
    if (!ctx->ensureStitcher()) {
        *image = raw_img;
        *width = raw_w;
        *height = raw_h;
        *format = raw_fmt;
        return 0;
    }
    ctx->stitcher->setStitchRadius(static_cast<float>(stitch_radius));

    int out_size = out_w * out_h * 4;
    uint8_t *out_buf = static_cast<uint8_t *>(mlt_pool_alloc(out_size));
    if (!out_buf) {
        return 1;
    }

    const uint8_t *sbs_ptr = raw_img;
    uint32_t sbs_w = static_cast<uint32_t>(raw_w);
    uint32_t sbs_h = static_cast<uint32_t>(raw_h);
    if (payload->raw_sbs_rgba && !payload->raw_sbs_rgba->empty() && payload->raw_sbs_w >= 2
        && payload->raw_sbs_h >= 2) {
        sbs_ptr = payload->raw_sbs_rgba->data();
        sbs_w = payload->raw_sbs_w;
        sbs_h = payload->raw_sbs_h;
    }

    if (!ctx->stitcher->reframeSideBySide(sbs_ptr,
                                          sbs_w,
                                          sbs_h,
                                          payload->calibration.lens_master,
                                          payload->calibration.lens_slave,
                                          payload->gyro_quat,
                                          params,
                                          static_cast<uint32_t>(out_w),
                                          static_cast<uint32_t>(out_h),
                                          out_buf)) {
        mlt_pool_release(out_buf);
        return 1;
    }

    *image = out_buf;
    *width = out_w;
    *height = out_h;
    *format = mlt_image_rgba;
    mlt_frame_set_image(frame, out_buf, out_size, mlt_pool_release);
    return 0;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "osv.request_raw_dual", 1);
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

static void filter_close(mlt_filter filter)
{
    if (!filter)
        return;
    auto *ctx = static_cast<DualLensReframeContext *>(filter->child);
    delete ctx;
    filter->child = nullptr;
}

extern "C" mlt_filter filter_dual_lens_reframe_init(mlt_profile profile,
                                                    mlt_service_type type,
                                                    const char *id,
                                                    const char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (!filter)
        return nullptr;

    auto *ctx = new DualLensReframeContext();
    filter->child = ctx;
    filter->process = filter_process;
    filter->close = filter_close;

    mlt_properties props = MLT_FILTER_PROPERTIES(filter);
    mlt_properties_set_double(props, "yaw", 0.0);
    mlt_properties_set_double(props, "pitch", 0.0);
    mlt_properties_set_double(props, "roll", 0.0);
    mlt_properties_set_double(props, "fov", 90.0);
    mlt_properties_set_double(props, "fisheye", 0.0);
    mlt_properties_set_double(props, "stitch_radius", 2.2);

    return filter;
}
