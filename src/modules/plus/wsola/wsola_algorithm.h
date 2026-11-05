/*
 * wsola_algorithm.h -- WSOLA time stretcher ported from Chromium
 *
 * Port of media/filters/audio_renderer_algorithm.{h,cc} and
 * media/filters/wsola_internals.{h,cc} from the Chromium project.
 * The algorithm, constants and index arithmetic are kept identical to
 * upstream; only Chromium's support types (AudioBus, AudioBufferQueue,
 * base::*) were replaced by the minimal equivalents below.
 *
 * Copyright 2012-2013 The Chromium Authors
 * Use of this source code is governed by a BSD-style license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google LLC nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MLT_WSOLA_ALGORITHM_H
#define MLT_WSOLA_ALGORITHM_H

#include <cstddef>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace wsola {

// Minimal stand-in for media::AudioBus: planar float channels, either owned
// or wrapping external memory.
class AudioBus
{
public:
    using ChannelVector = std::vector<float *>;

    static std::unique_ptr<AudioBus> Create(int channels, int frames);
    static std::unique_ptr<AudioBus> CreateWrapper(const ChannelVector &channels, int frames);

    int channels() const { return static_cast<int>(channel_data_.size()); }
    int frames() const { return frames_; }
    std::span<float> channel(int ch) { return {channel_data_[ch], static_cast<size_t>(frames_)}; }
    std::span<const float> channel(int ch) const
    {
        return {channel_data_[ch], static_cast<size_t>(frames_)};
    }

    void Zero() { ZeroFramesPartial(0, frames_); }
    void ZeroFrames(int frames) { ZeroFramesPartial(0, frames); }
    void ZeroFramesPartial(int start_frame, int frames);
    void CopyPartialFramesTo(int source_start_frame,
                             int frame_count,
                             int dest_start_frame,
                             AudioBus *dest) const;

private:
    AudioBus() = default;
    int frames_ = 0;
    std::vector<float> storage_;
    ChannelVector channel_data_;
};

// Minimal stand-in for media::AudioBufferQueue: a FIFO of planar frames.
class AudioBufferQueue
{
public:
    void Reset(int channels);
    void Clear();
    int frames() const { return frames_; }

    void Append(const float *const *planes, int frames);
    void AppendInterleaved(const float *interleaved, int frames);
    int ReadFrames(int frames, int dest_frame_offset, AudioBus *dest);
    int PeekFrames(int frames, int source_frame_offset, int dest_frame_offset, AudioBus *dest) const;
    void SeekFrames(int frames);

private:
    void Compact();
    std::vector<std::vector<float>> data_;
    size_t head_ = 0;
    int frames_ = 0;
};

namespace internal {

using Interval = std::pair<size_t, size_t>;

void MultiChannelDotProduct(const AudioBus *a,
                            size_t frame_offset_a,
                            const AudioBus *b,
                            size_t frame_offset_b,
                            size_t num_frames,
                            std::span<float> dot_product);
void MultiChannelMovingBlockEnergies(const AudioBus *input,
                                     size_t frames_per_window,
                                     std::span<float> energy);
void QuadraticInterpolation(std::span<const float, 3> y_values,
                            float *extremum,
                            float *extremum_value);
size_t DecimatedSearch(size_t decimation,
                       Interval exclude_interval,
                       const AudioBus *target_block,
                       const AudioBus *search_segment,
                       std::span<const float> energy_target_block,
                       std::span<const float> energy_candid_blocks);
size_t FullSearch(size_t low_limit,
                  size_t high_limit,
                  Interval exclude_interval,
                  const AudioBus *target_block,
                  const AudioBus *search_block,
                  std::span<const float> energy_target_block,
                  std::span<const float> energy_candidate_blocks);
size_t OptimalIndex(const AudioBus *search_block,
                    const AudioBus *target_block,
                    Interval exclude_interval);
void GetPeriodicHanningWindow(std::span<float> window);

} // namespace internal

// Port of media::AudioRendererAlgorithm, restricted to the pitch preserving
// paths (passthrough near 1.0x and WSOLA). Chromium's queue capacity and
// latency hint bookkeeping belong to its renderer, not to the algorithm, and
// were left out. The resampler path (preserves_pitch == false) is left out
// too: in MLT plain resampling is what the normalizers already do.
//
// Not thread-safe; callers must serialize access.
class AudioRendererAlgorithm
{
public:
    enum class FillBufferMode {
        kPassthrough,
        kWSOLA,
    };

    AudioRendererAlgorithm() = default;
    AudioRendererAlgorithm(const AudioRendererAlgorithm &) = delete;
    AudioRendererAlgorithm &operator=(const AudioRendererAlgorithm &) = delete;

    void Initialize(int channels, int samples_per_second);
    void SetChannelMask(std::vector<bool> channel_mask);

    // Tries to fill |requested_frames| frames into |dest| with possibly scaled
    // data from the queue. Data is consumed in proportion to |playback_rate|.
    // Returns the number of frames written.
    int FillBuffer(AudioBus *dest, int dest_offset, int requested_frames, double playback_rate);

    void FlushBuffers();

    void EnqueueBuffer(const float *const *planes, int frames);
    void EnqueueInterleaved(const float *interleaved, int frames);

    int BufferedFrames() const { return audio_buffer_.frames(); }
    double DelayInFrames(double playback_rate) const;
    int channels() const { return channels_; }
    int samples_per_second() const { return samples_per_second_; }
    int ola_window_size() const { return ola_window_size_; }
    // Input frames that must be queued, beyond what is consumed, before a
    // WSOLA iteration can run (the search region extends past the target).
    int LookaheadFrames() const
    {
        return num_candidate_blocks_ + ola_window_size_ - search_block_center_offset_;
    }
    double effective_playback_rate() const { return effective_playback_rate_; }
    FillBufferMode last_mode() const { return last_mode_; }

private:
    FillBufferMode ChooseBufferMode(double playback_rate);
    void SetFillBufferMode(FillBufferMode mode);
    void GetOptimalBlock();
    int WriteCompletedFramesTo(int requested_frames, int output_offset, AudioBus *dest);
    void PeekAudioWithZeroPrepend(int read_offset_frames, AudioBus *dest);
    bool RunOneWsolaIteration(double playback_rate);
    void RemoveOldInputFrames(double playback_rate);
    void UpdateOutputTime(double playback_rate, double time_change);
    bool TargetIsWithinSearchRegion() const;
    bool CanPerformWsola() const;
    void CreateSearchWrappers();
    int RunWsolaAndFill(AudioBus *dest, int dest_offset, int requested_frames, double playback_rate);

    int channels_ = 0;
    int samples_per_second_ = 0;
    AudioBufferQueue audio_buffer_;
    double output_time_ = 0.0;
    int search_block_center_offset_ = 0;
    int search_block_index_ = 0;
    int num_candidate_blocks_ = 0;
    int target_block_index_ = 0;
    int ola_window_size_ = 0;
    int ola_hop_size_ = 0;
    int num_complete_frames_ = 0;

    std::unique_ptr<AudioBus> wsola_output_;
    std::vector<float> ola_window_;
    std::vector<float> transition_window_;
    std::unique_ptr<AudioBus> optimal_block_;
    std::unique_ptr<AudioBus> search_block_;
    std::unique_ptr<AudioBus> target_block_;
    std::vector<bool> channel_mask_;
    std::unique_ptr<AudioBus> search_block_wrapper_;
    std::unique_ptr<AudioBus> target_block_wrapper_;

    double effective_playback_rate_ = 0;
    FillBufferMode last_mode_ = FillBufferMode::kPassthrough;
};

} // namespace wsola

#endif
