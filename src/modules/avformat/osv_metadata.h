#ifndef OSV_METADATA_H
#define OSV_METADATA_H

#include "dual_lens_types.h"
#include <string>
#include <vector>
#include <cstdint>

using OsvStabilizationMode = StabilizationMode;

struct OsvTelemetryFrame {
    int64_t timestamp_us = 0;
    float raw_quat[4] = {0.0f, 0.0f, 0.0f, 1.0f};     // Stored floats f0, f1, f2, f3
    float gyro_quat[4] = {1.0f, 0.0f, 0.0f, 0.0f};    // w, x, y, z (body to world)
    float horizon_quat[4] = {1.0f, 0.0f, 0.0f, 0.0f}; // w, x, y, z (horizon leveling)
    float gyro_rad_s[3] = {0.0f, 0.0f, 0.0f};       // wx, wy, wz
    float accel_g[3] = {0.0f, 0.0f, 0.0f};          // ax, ay, az
};

struct OsvMetadata {
    std::string proto_name;
    std::string camera_model;
    std::string serial_number;
    std::string firmware_version;
    uint32_t width = 3840;
    uint32_t height = 3840;
    uint32_t sample_rate_hz = 1000;
    int fps_num = 30000;
    int fps_den = 1001;
    double fps = 29.97002997002997;

    LensCalibration lens_front;
    LensCalibration lens_rear;

    std::vector<OsvTelemetryFrame> telemetry_frames;

    void getStabilizationQuat(int64_t frame_idx, int mode, float out_quat[4]) const;
};

class OsvMetadataParser {
public:
    static bool parseFromBuffer(const uint8_t* data, size_t size, OsvMetadata& out_meta);
    static bool parseFromFile(const std::string& osv_path, OsvMetadata& out_meta);
};

/**
 * @brief DJI Osmo 360 (.OSV) implementation of DualLensTelemetrySource.
 */
class OsvTelemetrySource : public DualLensTelemetrySource {
public:
    bool parseFromFile(const std::string& path, DualLensCalibration& out_cal) override {
        if (!OsvMetadataParser::parseFromFile(path, meta_)) {
            return false;
        }
        out_cal.lens_master = meta_.lens_rear;
        out_cal.lens_slave = meta_.lens_front;
        out_cal.sensor_w = meta_.width;
        out_cal.sensor_h = meta_.height;
        out_cal.fps_num = meta_.fps_num;
        out_cal.fps_den = meta_.fps_den;
        out_cal.fps = meta_.fps;
        out_cal.camera_model = meta_.camera_model;
        out_cal.serial_number = meta_.serial_number;
        out_cal.telemetry_sample_count = meta_.telemetry_frames.size();
        return true;
    }

    void getStabilizationQuat(int64_t frame_idx, int mode, float out_quat[4]) const override {
        meta_.getStabilizationQuat(frame_idx, mode, out_quat);
    }

private:
    OsvMetadata meta_;
};

#endif // OSV_METADATA_H
