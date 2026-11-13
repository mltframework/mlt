#ifndef OSV_METADATA_H
#define OSV_METADATA_H

#include <cstdint>
#include <string>
#include <vector>

struct LensCalibration
{
    float fx = 1056.0f;
    float fy = 1056.0f;
    float cx = 1920.0f;
    float cy = 1920.0f;
    float k1 = 0.0f;
    float k2 = 0.0f;
    float k3 = 0.0f;
    float k4 = 0.0f;
    float quat[4] = {1.0f, 0.0f, 0.0f, 0.0f}; // w, x, y, z (identity)
};

enum class OsvStabilizationMode { Off = 0, HorizonLock = 1, FullLock = 2 };

struct OsvTelemetryFrame
{
    int64_t timestamp_us = 0;
    float raw_quat[4] = {0.0f, 0.0f, 0.0f, 1.0f};     // Stored floats f0, f1, f2, f3
    float gyro_quat[4] = {1.0f, 0.0f, 0.0f, 0.0f};    // w, x, y, z (body to world)
    float horizon_quat[4] = {1.0f, 0.0f, 0.0f, 0.0f}; // w, x, y, z (horizon leveling)
    float gyro_rad_s[3] = {0.0f, 0.0f, 0.0f};         // wx, wy, wz
    float accel_g[3] = {0.0f, 0.0f, 0.0f};            // ax, ay, az
};

struct OsvMetadata
{
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

class OsvMetadataParser
{
public:
    static bool parseFromBuffer(const uint8_t *data, size_t size, OsvMetadata &out_meta);
    static bool parseFromFile(const std::string &osv_path, OsvMetadata &out_meta);
};

#endif // OSV_METADATA_H
