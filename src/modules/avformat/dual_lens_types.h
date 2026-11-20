#ifndef DUAL_LENS_TYPES_H
#define DUAL_LENS_TYPES_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/**
 * @brief Intrinsics, Kannala-Brandt distortion coefficients, and relative
 * sensor orientation quaternion for a single fisheye lens.
 */
struct LensCalibration {
    float fx = 1056.0f;
    float fy = 1056.0f;
    float cx = 1920.0f;
    float cy = 1920.0f;
    float k1 = 0.0f;
    float k2 = 0.0f;
    float k3 = 0.0f;
    float k4 = 0.0f;
    float k5 = 0.0f;
    float quat[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // x, y, z, w
};

/**
 * @brief Vendor-agnostic two-lens calibration and stream timing metadata
 * consumed by the Vulkan stitcher and dual-lens MLT producer.
 */
struct DualLensCalibration {
    LensCalibration lens_master;
    LensCalibration lens_slave;
    uint32_t sensor_w = 3840;
    uint32_t sensor_h = 3840;
    int fps_num = 30000;
    int fps_den = 1001;
    double fps = 29.97002997002997;

    std::string camera_model;
    std::string serial_number;
    size_t telemetry_sample_count = 0;
};

enum class StabilizationMode {
    Off = 0,
    HorizonLock = 1,
    FullLock = 2
};

/**
 * @brief Abstract interface for vendor-specific demux/telemetry parsers
 * (e.g., DJI OSV protobuf, GoPro MAX GPMF) feeding the common stitching pipeline.
 */
class DualLensTelemetrySource {
public:
    virtual ~DualLensTelemetrySource() = default;
    virtual bool parseFromFile(const std::string& path, DualLensCalibration& out_cal) = 0;
    virtual void getStabilizationQuat(int64_t frame_idx, int mode, float out_quat[4]) const = 0;
};

struct CompanionProxyInfo {
    std::string path;
    bool is_side_by_side = true;
};

struct DualLensFramePayload {
    DualLensCalibration calibration;
    float gyro_quat[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    std::shared_ptr<std::vector<uint8_t>> raw_sbs_rgba;
    uint32_t raw_sbs_w = 0;
    uint32_t raw_sbs_h = 0;
};

using CompanionProxyResolver =
    std::function<std::optional<CompanionProxyInfo>(const std::string& source_path)>;

#endif // DUAL_LENS_TYPES_H
