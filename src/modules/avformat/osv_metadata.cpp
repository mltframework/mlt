#include "osv_metadata.h"
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

namespace {

uint64_t readVarint(const uint8_t *&ptr, const uint8_t *end)
{
    uint64_t result = 0;
    int shift = 0;
    int bytes = 0;
    while (ptr < end && bytes < 10) {
        uint8_t byte = *ptr++;
        bytes++;
        if (shift == 63 && (byte & 0x7E) != 0) {
            ptr = end;
            return 0;
        }
        result |= static_cast<uint64_t>(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0)
            return result;
        shift += 7;
    }
    ptr = end;
    return 0;
}

float readFloat(const uint8_t *&ptr, const uint8_t *end)
{
    if (ptr + 4 > end)
        return 0.0f;
    float val;
    std::memcpy(&val, ptr, 4);
    ptr += 4;
    return val;
}

void parseLensMessage(const uint8_t *data, size_t size, LensCalibration &lens)
{
    const uint8_t *ptr = data;
    const uint8_t *end = data + size;

    while (ptr < end) {
        uint64_t tag = readVarint(ptr, end);
        uint32_t field_num = tag >> 3;
        uint32_t wire_type = tag & 0x7;

        if (wire_type == 5) { // 32-bit float
            if (end - ptr < 4) {
                break;
            }
            float val = readFloat(ptr, end);
            switch (field_num) {
            case 1:
                lens.fx = val;
                break;
            case 2:
                lens.fy = val;
                break;
            case 3:
                lens.cx = val;
                break;
            case 4:
                lens.cy = val;
                break;
            case 5:
                lens.k1 = val;
                break;
            case 6:
                lens.k2 = val;
                break;
            case 7:
                lens.k3 = val;
                break;
            case 8:
                lens.k4 = val;
                break;
            default:
                break;
            }
        } else if (wire_type == 2) { // length-delimited
            uint64_t len = readVarint(ptr, end);
            if (len > static_cast<size_t>(end - ptr)) {
                break;
            }
            if (field_num == 28 && len >= 16) { // orientation quaternion submessage
                const uint8_t *sub_ptr = ptr;
                const uint8_t *sub_end = ptr + len;
                int q_idx = 0;
                while (sub_ptr < sub_end && q_idx < 4) {
                    uint64_t sub_tag = readVarint(sub_ptr, sub_end);
                    uint32_t sub_wire = sub_tag & 0x7;
                    if (sub_wire == 5) {
                        if (sub_end - sub_ptr < 4) {
                            break;
                        }
                        lens.quat[q_idx++] = readFloat(sub_ptr, sub_end);
                    } else if (sub_wire == 0) {
                        readVarint(sub_ptr, sub_end);
                    } else if (sub_wire == 2) {
                        uint64_t l = readVarint(sub_ptr, sub_end);
                        if (l > static_cast<size_t>(sub_end - sub_ptr)) {
                            break;
                        }
                        sub_ptr += l;
                    } else if (sub_wire == 1) {
                        if (sub_end - sub_ptr < 8) {
                            break;
                        }
                        sub_ptr += 8;
                    } else {
                        break;
                    }
                }
            }
            ptr += len;
        } else if (wire_type == 0) {
            readVarint(ptr, end);
        } else if (wire_type == 1) {
            if (end - ptr < 8) {
                break;
            }
            ptr += 8;
        } else {
            break;
        }
    }
}

// Compute horizon lock correction quaternion:
// Given body->world quaternion b2w = (w, x, y, z),
// produces gyro quaternion in (w, x, y, z) that levels true gravity vertical (+Z)
// to equirectangular vertical (0, 0, 1) while preserving camera heading.
void computeHorizonLock(const float b2w[4], float out[4])
{
    // Conjugate of b2w maps world -> body:
    float qw = b2w[0], qx = -b2w[1], qy = -b2w[2], qz = -b2w[3];

    // Body ray pointing to world vertical (0, 0, 1): up_b = R(qw, qx, qy, qz) * (0, 0, 1)
    float uv_x = qy * 1.0f;
    float uv_y = -qx * 1.0f;
    float uv_z = 0.0f;

    float uuv_x = qy * uv_z - qz * uv_y;
    float uuv_y = qz * uv_x - qx * uv_z;
    float uuv_z = qx * uv_y - qy * uv_x;

    float up_bx = (uv_x * qw + uuv_x) * 2.0f;
    float up_by = (uv_y * qw + uuv_y) * 2.0f;
    float up_bz = 1.0f + (uv_z * qw + uuv_z) * 2.0f;

    float up_len = std::sqrt(up_bx * up_bx + up_by * up_by + up_bz * up_bz);
    if (up_len > 1e-6f) {
        up_bx /= up_len;
        up_by /= up_len;
        up_bz /= up_len;
    } else {
        up_bx = 0.0f;
        up_by = 0.0f;
        up_bz = 1.0f;
    }

    // Camera body forward is (0, 1, 0).
    // Project camera forward onto plane perpendicular to up_b:
    float dot = up_by;
    float fwd_bx = -up_bx * dot;
    float fwd_by = 1.0f - up_by * dot;
    float fwd_bz = -up_bz * dot;

    float fwd_len = std::sqrt(fwd_bx * fwd_bx + fwd_by * fwd_by + fwd_bz * fwd_bz);
    if (fwd_len > 1e-6f) {
        fwd_bx /= fwd_len;
        fwd_by /= fwd_len;
        fwd_bz /= fwd_len;
    } else {
        fwd_bx = 0.0f;
        fwd_by = 1.0f;
        fwd_bz = 0.0f;
    }

    // right_b = fwd_b x up_b
    float right_bx = fwd_by * up_bz - fwd_bz * up_by;
    float right_by = fwd_bz * up_bx - fwd_bx * up_bz;
    float right_bz = fwd_bx * up_by - fwd_by * up_bx;

    // Basis matrix M = [right_b, fwd_b, up_b] converts equirect ray to body ray (invGyro).
    float m00 = right_bx, m01 = fwd_bx, m02 = up_bx;
    float m10 = right_by, m11 = fwd_by, m12 = up_by;
    float m20 = right_bz, m21 = fwd_bz, m22 = up_bz;

    float trace = m00 + m11 + m22;
    float q_e2b_w, q_e2b_x, q_e2b_y, q_e2b_z;
    if (trace > 0.0f) {
        float s = 0.5f / std::sqrt(trace + 1.0f);
        q_e2b_w = 0.25f / s;
        q_e2b_x = (m21 - m12) * s;
        q_e2b_y = (m02 - m20) * s;
        q_e2b_z = (m10 - m01) * s;
    } else if (m00 > m11 && m00 > m22) {
        float s = 2.0f * std::sqrt(1.0f + m00 - m11 - m22);
        q_e2b_w = (m21 - m12) / s;
        q_e2b_x = 0.25f * s;
        q_e2b_y = (m01 + m10) / s;
        q_e2b_z = (m02 + m20) / s;
    } else if (m11 > m22) {
        float s = 2.0f * std::sqrt(1.0f + m11 - m00 - m22);
        q_e2b_w = (m02 - m20) / s;
        q_e2b_x = (m01 + m10) / s;
        q_e2b_y = 0.25f * s;
        q_e2b_z = (m12 + m21) / s;
    } else {
        float s = 2.0f * std::sqrt(1.0f + m22 - m00 - m11);
        q_e2b_w = (m10 - m01) / s;
        q_e2b_x = (m02 + m20) / s;
        q_e2b_y = (m12 + m21) / s;
        q_e2b_z = 0.25f * s;
    }

    // Since in shader dBody = quatRotate(invGyro, dEquirect),
    // and invGyro is q_e2b, gyro is conjugate of q_e2b:
    out[0] = q_e2b_w;
    out[1] = -q_e2b_x;
    out[2] = -q_e2b_y;
    out[3] = -q_e2b_z;
}

void parseFrameMeta(const uint8_t *data, size_t size, OsvMetadata &out_meta)
{
    const uint8_t *ptr = data;
    const uint8_t *end = data + size;

    OsvTelemetryFrame frame;
    bool has_attitude = false;

    while (ptr < end) {
        uint64_t tag = readVarint(ptr, end);
        uint32_t fn = tag >> 3;
        uint32_t wt = tag & 0x7;

        if (wt != 2) {
            if (wt == 0)
                readVarint(ptr, end);
            else if (wt == 1) {
                if (end - ptr < 8)
                    break;
                ptr += 8;
            } else if (wt == 5) {
                if (end - ptr < 4)
                    break;
                ptr += 4;
            }
            continue;
        }

        uint64_t len = readVarint(ptr, end);
        if (len > static_cast<size_t>(end - ptr))
            break;

        if (fn == 1) { // FrameMetaHeader
            const uint8_t *h_ptr = ptr;
            const uint8_t *h_end = ptr + len;
            while (h_ptr < h_end) {
                uint64_t ht = readVarint(h_ptr, h_end);
                uint32_t hfn = ht >> 3;
                uint32_t hwt = ht & 0x7;
                if (hwt == 0) {
                    uint64_t v = readVarint(h_ptr, h_end);
                    if (hfn == 2)
                        frame.timestamp_us = static_cast<int64_t>(v);
                } else if (hwt == 1) {
                    if (h_end - h_ptr < 8)
                        break;
                    h_ptr += 8;
                } else if (hwt == 5) {
                    if (h_end - h_ptr < 4)
                        break;
                    h_ptr += 4;
                } else if (hwt == 2) {
                    uint64_t hl = readVarint(h_ptr, h_end);
                    if (hl > static_cast<size_t>(h_end - h_ptr))
                        break;
                    h_ptr += hl;
                }
            }
        } else if (fn == 2) { // FrameMetaOfCamera
            const uint8_t *c_ptr = ptr;
            const uint8_t *c_end = ptr + len;
            while (c_ptr < c_end) {
                uint64_t ct = readVarint(c_ptr, c_end);
                uint32_t cfn = ct >> 3;
                uint32_t cwt = ct & 0x7;
                if (cwt == 2) {
                    uint64_t cl = readVarint(c_ptr, c_end);
                    if (cl > static_cast<size_t>(c_end - c_ptr))
                        break;
                    if (cfn == 9) { // camera_attitude (Quaternion)
                        const uint8_t *q_ptr = c_ptr;
                        const uint8_t *q_end = c_ptr + cl;
                        float f[4] = {0.0f, 0.0f, 0.0f, 1.0f};
                        while (q_ptr < q_end) {
                            uint64_t qt = readVarint(q_ptr, q_end);
                            uint32_t qfn = qt >> 3;
                            uint32_t qwt = qt & 0x7;
                            if (qwt == 5) {
                                if (q_end - q_ptr < 4)
                                    break;
                                float val = readFloat(q_ptr, q_end);
                                if (qfn >= 1 && qfn <= 4)
                                    f[qfn - 1] = val;
                            } else if (qwt == 0)
                                readVarint(q_ptr, q_end);
                            else if (qwt == 1) {
                                if (q_end - q_ptr < 8)
                                    break;
                                q_ptr += 8;
                            } else if (qwt == 2) {
                                uint64_t ql = readVarint(q_ptr, q_end);
                                if (ql > static_cast<size_t>(q_end - q_ptr))
                                    break;
                                q_ptr += ql;
                            }
                        }
                        frame.raw_quat[0] = f[0];
                        frame.raw_quat[1] = f[1];
                        frame.raw_quat[2] = f[2];
                        frame.raw_quat[3] = f[3];

                        // OpenOSV body -> world quaternion convention:
                        // q' = (w = f3; x = -f1, y = f2, z = f0)
                        float b2w[4] = {f[3], -f[1], f[2], f[0]};
                        float norm = std::sqrt(b2w[0] * b2w[0] + b2w[1] * b2w[1] + b2w[2] * b2w[2]
                                               + b2w[3] * b2w[3]);
                        if (norm > 1e-6f) {
                            b2w[0] /= norm;
                            b2w[1] /= norm;
                            b2w[2] /= norm;
                            b2w[3] /= norm;
                        } else {
                            b2w[0] = 1.0f;
                            b2w[1] = 0.0f;
                            b2w[2] = 0.0f;
                            b2w[3] = 0.0f;
                        }
                        std::memcpy(frame.gyro_quat, b2w, 4 * sizeof(float));

                        computeHorizonLock(b2w, frame.horizon_quat);
                        has_attitude = true;
                    }
                    c_ptr += cl;
                } else if (cwt == 0)
                    readVarint(c_ptr, c_end);
                else if (cwt == 1) {
                    if (c_end - c_ptr < 8)
                        break;
                    c_ptr += 8;
                } else if (cwt == 5) {
                    if (c_end - c_ptr < 4)
                        break;
                    c_ptr += 4;
                }
            }
        }
        ptr += len;
    }

    if (has_attitude) {
        out_meta.telemetry_frames.push_back(frame);
    }
}

} // namespace

void OsvMetadata::getStabilizationQuat(int64_t frame_idx, int mode, float out_quat[4]) const
{
    if (mode == static_cast<int>(OsvStabilizationMode::Off) || telemetry_frames.empty()) {
        out_quat[0] = 1.0f;
        out_quat[1] = 0.0f;
        out_quat[2] = 0.0f;
        out_quat[3] = 0.0f;
        return;
    }

    size_t idx = 0;
    if (frame_idx < 0) {
        idx = 0;
    } else if (static_cast<size_t>(frame_idx) >= telemetry_frames.size()) {
        idx = telemetry_frames.size() - 1;
    } else {
        idx = static_cast<size_t>(frame_idx);
    }

    const auto &tf = telemetry_frames[idx];
    if (mode == static_cast<int>(OsvStabilizationMode::FullLock)) {
        std::memcpy(out_quat, tf.gyro_quat, 4 * sizeof(float));
    } else { // HorizonLock (default)
        std::memcpy(out_quat, tf.horizon_quat, 4 * sizeof(float));
    }
}

bool OsvMetadataParser::parseFromBuffer(const uint8_t *data, size_t size, OsvMetadata &out_meta)
{
    const uint8_t *ptr = data;
    const uint8_t *end = data + size;
    bool has_calibration = false;

    while (ptr < end) {
        uint64_t tag = readVarint(ptr, end);
        uint32_t field_num = tag >> 3;
        uint32_t wire_type = tag & 0x7;

        if (wire_type != 2) {
            if (wire_type == 0)
                readVarint(ptr, end);
            else if (wire_type == 1) {
                if (end - ptr < 8)
                    break;
                ptr += 8;
            } else if (wire_type == 5) {
                if (end - ptr < 4)
                    break;
                ptr += 4;
            }
            continue;
        }

        uint64_t len = readVarint(ptr, end);
        if (len > static_cast<size_t>(end - ptr))
            break;

        const uint8_t *msg_start = ptr;
        const uint8_t *msg_end = ptr + len;

        if (field_num == 1) {
            // Header message
            const uint8_t *h_ptr = msg_start;
            while (h_ptr < msg_end) {
                uint64_t h_tag = readVarint(h_ptr, msg_end);
                uint32_t h_field = h_tag >> 3;
                uint32_t h_wire = h_tag & 0x7;
                if (h_wire == 2) {
                    uint64_t s_len = readVarint(h_ptr, msg_end);
                    if (s_len > static_cast<size_t>(msg_end - h_ptr))
                        break;
                    if (h_field == 1) {
                        // Submessage: proto name, serial, model
                        const uint8_t *sub_ptr = h_ptr;
                        const uint8_t *sub_end = h_ptr + s_len;
                        while (sub_ptr < sub_end) {
                            uint64_t t = readVarint(sub_ptr, sub_end);
                            uint32_t fn = t >> 3;
                            uint32_t wt = t & 0x7;
                            if (wt == 2) {
                                uint64_t sl = readVarint(sub_ptr, sub_end);
                                if (sl > static_cast<size_t>(sub_end - sub_ptr))
                                    break;
                                std::string str(reinterpret_cast<const char *>(sub_ptr), sl);
                                if (fn == 1)
                                    out_meta.proto_name = str;
                                else if (fn == 2)
                                    out_meta.firmware_version = str;
                                else if (fn == 5)
                                    out_meta.serial_number = str;
                                else if (fn == 10)
                                    out_meta.camera_model = str;
                                sub_ptr += sl;
                            } else if (wt == 0)
                                readVarint(sub_ptr, sub_end);
                            else if (wt == 1) {
                                if (sub_end - sub_ptr < 8)
                                    break;
                                sub_ptr += 8;
                            } else if (wt == 5) {
                                if (sub_end - sub_ptr < 4)
                                    break;
                                sub_ptr += 4;
                            }
                        }
                    }
                    h_ptr += s_len;
                } else if (h_wire == 0) {
                    uint64_t val = readVarint(h_ptr, msg_end);
                    if (h_field == 10)
                        out_meta.sample_rate_hz = static_cast<uint32_t>(val);
                } else if (h_wire == 5) {
                    if (msg_end - h_ptr < 4)
                        break;
                    h_ptr += 4;
                } else if (h_wire == 1) {
                    if (msg_end - h_ptr < 8)
                        break;
                    h_ptr += 8;
                }
            }
        } else if (field_num == 2) {
            // Video / Calibration packet
            const uint8_t *v_ptr = msg_start;
            while (v_ptr < msg_end) {
                uint64_t v_tag = readVarint(v_ptr, msg_end);
                uint32_t v_field = v_tag >> 3;
                uint32_t v_wire = v_tag & 0x7;

                if (v_wire == 2) {
                    uint64_t v_len = readVarint(v_ptr, msg_end);
                    if (v_len > static_cast<size_t>(msg_end - v_ptr))
                        break;
                    if (v_field == 6) { // Message containing lens 1 and lens 2 calibrations
                        const uint8_t *l_ptr = v_ptr;
                        const uint8_t *l_end = v_ptr + v_len;
                        while (l_ptr < l_end) {
                            uint64_t l_tag = readVarint(l_ptr, l_end);
                            uint32_t l_field = l_tag >> 3;
                            uint32_t l_wire = l_tag & 0x7;
                            if (l_wire == 2) {
                                uint64_t lens_len = readVarint(l_ptr, l_end);
                                if (lens_len > static_cast<size_t>(l_end - l_ptr))
                                    break;
                                if (l_field == 1) {
                                    parseLensMessage(l_ptr, lens_len, out_meta.lens_front);
                                    has_calibration = true;
                                } else if (l_field == 2) {
                                    parseLensMessage(l_ptr, lens_len, out_meta.lens_rear);
                                    has_calibration = true;
                                }
                                l_ptr += lens_len;
                            } else if (l_wire == 0)
                                readVarint(l_ptr, l_end);
                            else if (l_wire == 5) {
                                if (l_end - l_ptr < 4)
                                    break;
                                l_ptr += 4;
                            } else if (l_wire == 1) {
                                if (l_end - l_ptr < 8)
                                    break;
                                l_ptr += 8;
                            }
                        }
                    }
                    v_ptr += v_len;
                } else if (v_wire == 0)
                    readVarint(v_ptr, msg_end);
                else if (v_wire == 5) {
                    if (msg_end - v_ptr < 4)
                        break;
                    v_ptr += 4;
                } else if (v_wire == 1) {
                    if (msg_end - v_ptr < 8)
                        break;
                    v_ptr += 8;
                }
            }
        } else if (field_num == 3) {
            // FrameMeta: timestamp, attitude, imu
            parseFrameMeta(msg_start, len, out_meta);
        }

        ptr += len;
    }

    return !out_meta.proto_name.empty() || has_calibration || !out_meta.telemetry_frames.empty();
}

bool OsvMetadataParser::parseFromFile(const std::string &osv_path, OsvMetadata &out_meta)
{
    AVFormatContext *fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, osv_path.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Failed to open video container: " << osv_path << std::endl;
        return false;
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        avformat_close_input(&fmt_ctx);
        return false;
    }

    // Extract video stream frame rate
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVRational r = fmt_ctx->streams[i]->avg_frame_rate;
            if (r.num <= 0 || r.den <= 0)
                r = fmt_ctx->streams[i]->r_frame_rate;
            if (r.num > 0 && r.den > 0) {
                out_meta.fps_num = r.num;
                out_meta.fps_den = r.den;
                out_meta.fps = static_cast<double>(r.num) / r.den;
            }
            break;
        }
    }

    int djmd_stream_idx = -1;
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
        AVStream *st = fmt_ctx->streams[i];
        if (st->codecpar->codec_tag == MKTAG('d', 'j', 'm', 'd')) {
            djmd_stream_idx = i;
            break;
        }
    }

    if (djmd_stream_idx < 0) {
        std::cerr << "No 'djmd' telemetry stream found in: " << osv_path << std::endl;
        avformat_close_input(&fmt_ctx);
        return false;
    }

    // Discard non-djmd streams to skip decoding heavy video/audio packets
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (static_cast<int>(i) != djmd_stream_idx) {
            fmt_ctx->streams[i]->discard = AVDISCARD_ALL;
        }
    }

    AVPacket *pkt = av_packet_alloc();
    bool parsed_header = false;

    // Read all packets in the metadata stream to extract calibration and per-frame telemetry
    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == djmd_stream_idx) {
            if (parseFromBuffer(pkt->data, pkt->size, out_meta)) {
                parsed_header = true;
            }
        }
        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
    avformat_close_input(&fmt_ctx);
    return parsed_header;
}
