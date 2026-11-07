/*
 * wsola_algorithm.cpp -- WSOLA time stretcher ported from Chromium
 *
 * Port of media/filters/audio_renderer_algorithm.cc and
 * media/filters/wsola_internals.cc from the Chromium project.
 *
 * Copyright 2012-2013 The Chromium Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in wsola_algorithm.h.
 */

#include "wsola_algorithm.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>
#include <numbers>
#include <numeric>

#if defined(__x86_64__) || defined(__i386__)
#define WSOLA_X86 1
#include <immintrin.h>
#include <xmmintrin.h>
#elif defined(__ARM_NEON)
#define WSOLA_NEON 1
#include <arm_neon.h>
#endif

namespace wsola {

// ---------------------------------------------------------------- AudioBus

std::unique_ptr<AudioBus> AudioBus::Create(int channels, int frames)
{
    std::unique_ptr<AudioBus> bus(new AudioBus);
    bus->frames_ = frames;
    bus->storage_.assign(static_cast<size_t>(channels) * frames, 0.0f);
    for (int ch = 0; ch < channels; ++ch)
        bus->channel_data_.push_back(bus->storage_.data() + static_cast<size_t>(ch) * frames);
    return bus;
}

std::unique_ptr<AudioBus> AudioBus::CreateWrapper(const ChannelVector &channels, int frames)
{
    std::unique_ptr<AudioBus> bus(new AudioBus);
    bus->frames_ = frames;
    bus->channel_data_ = channels;
    return bus;
}

void AudioBus::ZeroFramesPartial(int start_frame, int frames)
{
    for (auto *ch : channel_data_)
        std::fill_n(ch + start_frame, frames, 0.0f);
}

void AudioBus::CopyPartialFramesTo(int source_start_frame,
                                   int frame_count,
                                   int dest_start_frame,
                                   AudioBus *dest) const
{
    assert(dest->channels() == channels());
    assert(source_start_frame + frame_count <= frames_);
    assert(dest_start_frame + frame_count <= dest->frames());
    for (int ch = 0; ch < channels(); ++ch)
        std::memmove(dest->channel_data_[ch] + dest_start_frame,
                     channel_data_[ch] + source_start_frame,
                     sizeof(float) * frame_count);
}

// -------------------------------------------------------- AudioBufferQueue

void AudioBufferQueue::Reset(int channels)
{
    data_.assign(channels, {});
    head_ = 0;
    frames_ = 0;
}

void AudioBufferQueue::Clear()
{
    for (auto &ch : data_)
        ch.clear();
    head_ = 0;
    frames_ = 0;
}

void AudioBufferQueue::Compact()
{
    // Drop consumed frames once they dominate the storage.
    if (head_ < 8192 || head_ < data_[0].size() / 2)
        return;
    for (auto &ch : data_)
        ch.erase(ch.begin(), ch.begin() + head_);
    head_ = 0;
}

void AudioBufferQueue::Append(const float *const *planes, int frames)
{
    for (size_t ch = 0; ch < data_.size(); ++ch)
        data_[ch].insert(data_[ch].end(), planes[ch], planes[ch] + frames);
    frames_ += frames;
}

void AudioBufferQueue::AppendInterleaved(const float *interleaved, int frames)
{
    const size_t channels = data_.size();
    for (size_t ch = 0; ch < channels; ++ch) {
        auto &dst = data_[ch];
        const size_t base = dst.size();
        dst.resize(base + frames);
        for (int i = 0; i < frames; ++i)
            dst[base + i] = interleaved[i * channels + ch];
    }
    frames_ += frames;
}

int AudioBufferQueue::PeekFrames(int frames,
                                 int source_frame_offset,
                                 int dest_frame_offset,
                                 AudioBus *dest) const
{
    const int n = std::max(0, std::min(frames, frames_ - source_frame_offset));
    for (size_t ch = 0; ch < data_.size(); ++ch)
        std::memcpy(dest->channel(ch).data() + dest_frame_offset,
                    data_[ch].data() + head_ + source_frame_offset,
                    sizeof(float) * n);
    return n;
}

int AudioBufferQueue::ReadFrames(int frames, int dest_frame_offset, AudioBus *dest)
{
    const int n = PeekFrames(frames, 0, dest_frame_offset, dest);
    SeekFrames(n);
    return n;
}

void AudioBufferQueue::SeekFrames(int frames)
{
    assert(frames <= frames_);
    head_ += frames;
    frames_ -= frames;
    Compact();
}

// ---------------------------------------------------------------- internals

namespace internal {

namespace {

bool InInterval(size_t n, Interval q)
{
    return n >= q.first && n <= q.second;
}

float MultiChannelSimilarityMeasure(std::span<const float> dot_prod_a_b,
                                    std::span<const float> energy_a,
                                    std::span<const float> energy_b)
{
    assert(dot_prod_a_b.size() == energy_a.size());
    assert(energy_a.size() == energy_b.size());
    const float kEpsilon = 1e-12f;
    float similarity_measure = 0.0f;
    for (size_t n = 0; n < dot_prod_a_b.size(); ++n) {
        similarity_measure += dot_prod_a_b[n] / std::sqrt(energy_a[n] * energy_b[n] + kEpsilon);
    }
    return similarity_measure;
}

#if defined(WSOLA_X86)
void MultiChannelDotProduct_SSE(const AudioBus *a,
                                size_t frame_offset_a,
                                const AudioBus *b,
                                size_t frame_offset_b,
                                size_t num_frames,
                                std::span<float> dot_product)
{
    // SIMD optimized variants can provide a massive speedup to this operation.
    const size_t rem = num_frames % 4;
    const size_t last_index = num_frames - rem;
    const int channels = a->channels();
    for (int ch = 0; ch < channels; ++ch) {
        const float *a_src = a->channel(ch).data() + frame_offset_a;
        const float *b_src = b->channel(ch).data() + frame_offset_b;

        // First sum all components.
        __m128 m_sum = _mm_setzero_ps();
        for (size_t s = 0; s < last_index; s += 4) {
            m_sum = _mm_add_ps(m_sum, _mm_mul_ps(_mm_loadu_ps(&a_src[s]), _mm_loadu_ps(&b_src[s])));
        }

        // Reduce to a single float for this channel. Sadly, SSE1,2 doesn't have a
        // horizontal sum function, so we have to condense manually.
        m_sum = _mm_add_ps(_mm_movehl_ps(m_sum, m_sum), m_sum);
        _mm_store_ss(&dot_product[ch], _mm_add_ss(m_sum, _mm_shuffle_ps(m_sum, m_sum, 1)));
    }

    if (!rem) {
        return;
    }

    frame_offset_a += last_index;
    frame_offset_b += last_index;

    // C version is required to handle remainder of frames (% 4 != 0)
    for (int ch = 0; ch < channels; ++ch) {
        const float *ch_a = a->channel(ch).data() + frame_offset_a;
        const float *ch_b = b->channel(ch).data() + frame_offset_b;
        dot_product[ch] += std::inner_product(ch_a, ch_a + rem, ch_b, 0.0f);
    }
}

__attribute__((target("avx2,fma"))) void MultiChannelDotProduct_AVX2(const AudioBus *a,
                                                                     size_t frame_offset_a,
                                                                     const AudioBus *b,
                                                                     size_t frame_offset_b,
                                                                     size_t num_frames,
                                                                     std::span<float> dot_product)
{
    // SIMD optimized variants can provide a massive speedup to this operation.
    const size_t rem = num_frames % 8;
    const size_t last_index = num_frames - rem;
    const int channels = a->channels();
    for (int ch = 0; ch < channels; ++ch) {
        const float *a_src = a->channel(ch).data() + frame_offset_a;
        const float *b_src = b->channel(ch).data() + frame_offset_b;

        // First sum all components using FMA.
        __m256 m_sum = _mm256_setzero_ps();
        for (size_t s = 0; s < last_index; s += 8) {
            __m256 a_avx = _mm256_loadu_ps(&a_src[s]);
            __m256 b_avx = _mm256_loadu_ps(&b_src[s]);
            m_sum = _mm256_fmadd_ps(a_avx, b_avx, m_sum);
        }

        // Fold the upper 128 bits into the lower 128 bits, then reduce across the
        // 128-bit lane as in MultiChannelDotProduct_SSE().
        __m128 m_sum_128 = _mm_add_ps(_mm256_castps256_ps128(m_sum),
                                      _mm256_extractf128_ps(m_sum, 1));
        m_sum_128 = _mm_add_ps(_mm_movehl_ps(m_sum_128, m_sum_128), m_sum_128);
        _mm_store_ss(&dot_product[ch],
                     _mm_add_ss(m_sum_128, _mm_shuffle_ps(m_sum_128, m_sum_128, 1)));
    }

    if (!rem) {
        return;
    }

    frame_offset_a += last_index;
    frame_offset_b += last_index;

    // C version is required to handle remainder of frames (% 8 != 0)
    for (int ch = 0; ch < channels; ++ch) {
        const float *ch_a = a->channel(ch).data() + frame_offset_a;
        const float *ch_b = b->channel(ch).data() + frame_offset_b;
        dot_product[ch] += std::inner_product(ch_a, ch_a + rem, ch_b, 0.0f);
    }
}

#elif defined(WSOLA_NEON)
void MultiChannelDotProduct_NEON(const AudioBus *a,
                                 size_t frame_offset_a,
                                 const AudioBus *b,
                                 size_t frame_offset_b,
                                 size_t num_frames,
                                 std::span<float> dot_product)
{
    // SIMD optimized variants can provide a massive speedup to this operation.
    const size_t rem = num_frames % 4;
    const size_t last_index = num_frames - rem;
    const int channels = a->channels();
    for (int ch = 0; ch < channels; ++ch) {
        const float *a_src = a->channel(ch).data() + frame_offset_a;
        const float *b_src = b->channel(ch).data() + frame_offset_b;

        // First sum all components.
        float32x4_t m_sum = vmovq_n_f32(0);
        for (size_t s = 0; s < last_index; s += 4) {
            m_sum = vmlaq_f32(m_sum, vld1q_f32(&a_src[s]), vld1q_f32(&b_src[s]));
        }

        // Reduce to a single float for this channel.
        float32x2_t m_half = vadd_f32(vget_high_f32(m_sum), vget_low_f32(m_sum));
        dot_product[ch] = vget_lane_f32(vpadd_f32(m_half, m_half), 0);
    }

    if (!rem) {
        return;
    }

    frame_offset_a += last_index;
    frame_offset_b += last_index;

    // C version is required to handle remainder of frames (% 4 != 0)
    for (int ch = 0; ch < channels; ++ch) {
        const float *ch_a = a->channel(ch).data() + frame_offset_a;
        const float *ch_b = b->channel(ch).data() + frame_offset_b;
        dot_product[ch] += std::inner_product(ch_a, ch_a + rem, ch_b, 0.0f);
    }
}
#else

void MultiChannelDotProduct_C(const AudioBus *a,
                              size_t frame_offset_a,
                              const AudioBus *b,
                              size_t frame_offset_b,
                              size_t num_frames,
                              std::span<float> dot_product)
{
    // Zero out the memory we will accumulate into.
    std::fill(dot_product.begin(), dot_product.end(), 0.0f);

    for (int k = 0; k < a->channels(); ++k) {
        const float *ch_a = a->channel(k).data() + frame_offset_a;
        const float *ch_b = b->channel(k).data() + frame_offset_b;
        dot_product[k] = std::inner_product(ch_a, ch_a + num_frames, ch_b, 0.0f);
    }
}
#endif

// Silent audio can contain non-zero samples small enough to result in
// subnormals internally. Disabling subnormals can be significantly faster in
// these cases. Equivalent of cc::ScopedSubnormalFloatDisabler.
class ScopedSubnormalFloatDisabler
{
public:
    ScopedSubnormalFloatDisabler()
    {
#if defined(WSOLA_X86)
        orig_state_ = _mm_getcsr();
        _mm_setcsr(orig_state_ | 0x8040); // FTZ | DAZ
#endif
    }
    ~ScopedSubnormalFloatDisabler()
    {
#if defined(WSOLA_X86)
        _mm_setcsr(orig_state_);
#endif
    }

private:
    unsigned int orig_state_ = 0;
};

} // namespace

void MultiChannelDotProduct(const AudioBus *a,
                            size_t frame_offset_a,
                            const AudioBus *b,
                            size_t frame_offset_b,
                            size_t num_frames,
                            std::span<float> dot_product)
{
    assert(a->channels() == b->channels());
    assert(dot_product.size() == static_cast<size_t>(a->channels()));
    assert(frame_offset_a + num_frames <= static_cast<size_t>(a->frames()));
    assert(frame_offset_b + num_frames <= static_cast<size_t>(b->frames()));

    static const auto dot_product_func = [] {
#if defined(WSOLA_X86)
        __builtin_cpu_init();
        if (__builtin_cpu_supports("avx2") && __builtin_cpu_supports("fma")) {
            return MultiChannelDotProduct_AVX2;
        }
        return MultiChannelDotProduct_SSE;
#elif defined(WSOLA_NEON)
        return MultiChannelDotProduct_NEON;
#else
        return MultiChannelDotProduct_C;
#endif
    }();

    return dot_product_func(a, frame_offset_a, b, frame_offset_b, num_frames, dot_product);
}

void MultiChannelMovingBlockEnergies(const AudioBus *input,
                                     size_t frames_per_block,
                                     std::span<float> energy)
{
    assert(static_cast<size_t>(input->frames()) >= frames_per_block - 1);
    const size_t num_blocks = input->frames() - (frames_per_block - 1);
    const size_t channels = static_cast<size_t>(input->channels());
    assert(energy.size() == num_blocks * channels);

    for (int k = 0; k < input->channels(); ++k) {
        const float *input_channel = input->channel(k).data();

        energy[k] = std::inner_product(input_channel,
                                       input_channel + frames_per_block,
                                       input_channel,
                                       0.0f);

        const float *slide_out = input_channel;
        const float *slide_in = input_channel + frames_per_block;
        for (size_t n = 1; n < num_blocks; ++n, ++slide_in, ++slide_out) {
            energy[k + n * channels] = energy[k + (n - 1) * channels] - (*slide_out) * (*slide_out)
                                       + (*slide_in) * (*slide_in);
        }
    }
}

// Fit the curve f(x) = a * x^2 + b * x + c such that
//   f(-1) = y[0]
//   f(0) = y[1]
//   f(1) = y[2]
// and return the maximum, assuming that y[0] <= y[1] >= y[2].
void QuadraticInterpolation(std::span<const float, 3> y_values,
                            float *extremum,
                            float *extremum_value)
{
    const float a = 0.5f * (y_values[2] + y_values[0]) - y_values[1];
    const float b = 0.5f * (y_values[2] - y_values[0]);
    const float c = y_values[1];

    if (a == 0.f) {
        // The coordinates are colinear (within floating-point error).
        *extremum = 0;
        *extremum_value = y_values[1];
    } else {
        const float ext = -b / (2.f * a);
        *extremum = ext;
        *extremum_value = a * ext * ext + b * ext + c;
    }
}

size_t DecimatedSearch(size_t decimation,
                       Interval exclude_interval,
                       const AudioBus *target_block,
                       const AudioBus *search_segment,
                       std::span<const float> energy_target_block,
                       std::span<const float> energy_candidate_blocks)
{
    size_t channels = static_cast<size_t>(search_segment->channels());
    size_t block_size = target_block->frames();
    size_t num_candidate_blocks = search_segment->frames() - (block_size - 1);
    std::vector<float> dot_prod(channels);
    float similarity[3]; // Three elements for cubic interpolation.

    size_t n = 0;
    MultiChannelDotProduct(target_block, 0, search_segment, n, block_size, dot_prod);
    similarity[0] = MultiChannelSimilarityMeasure(dot_prod,
                                                  energy_target_block,
                                                  energy_candidate_blocks.subspan(n * channels,
                                                                                  channels));

    // Set the starting point as optimal point.
    float best_similarity = similarity[0];
    size_t optimal_index = 0;

    n += decimation;
    if (n >= num_candidate_blocks) {
        return 0;
    }

    MultiChannelDotProduct(target_block, 0, search_segment, n, block_size, dot_prod);
    similarity[1] = MultiChannelSimilarityMeasure(dot_prod,
                                                  energy_target_block,
                                                  energy_candidate_blocks.subspan(n * channels,
                                                                                  channels));

    n += decimation;
    if (n >= num_candidate_blocks) {
        // We cannot do any more sampling. Compare these two values and return the
        // optimal index.
        return similarity[1] > similarity[0] ? decimation : 0;
    }

    for (; n < num_candidate_blocks; n += decimation) {
        MultiChannelDotProduct(target_block, 0, search_segment, n, block_size, dot_prod);

        similarity[2] = MultiChannelSimilarityMeasure(dot_prod,
                                                      energy_target_block,
                                                      energy_candidate_blocks.subspan(n * channels,
                                                                                      channels));

        if ((similarity[1] > similarity[0] && similarity[1] >= similarity[2])
            || (similarity[1] >= similarity[0] && similarity[1] > similarity[2])) {
            // A local maximum is found. Do a cubic interpolation for a better
            // estimate of candidate maximum.
            float normalized_candidate_index;
            float candidate_similarity;
            QuadraticInterpolation(similarity, &normalized_candidate_index, &candidate_similarity);

            int candidate_index = n - decimation
                                  + static_cast<int>(normalized_candidate_index * decimation + 0.5f);
            if (candidate_similarity > best_similarity
                && !InInterval(candidate_index, exclude_interval)) {
                optimal_index = candidate_index;
                best_similarity = candidate_similarity;
            }
        } else if (n + decimation >= num_candidate_blocks && similarity[2] > best_similarity
                   && !InInterval(n, exclude_interval)) {
            // If this is the end-point and has a better similarity-measure than
            // optimal, then we accept it as optimal point.
            optimal_index = n;
            best_similarity = similarity[2];
        }

        similarity[0] = similarity[1];
        similarity[1] = similarity[2];
    }
    return optimal_index;
}

size_t FullSearch(size_t low_limit,
                  size_t high_limit,
                  Interval exclude_interval,
                  const AudioBus *target_block,
                  const AudioBus *search_block,
                  std::span<const float> energy_target_block,
                  std::span<const float> energy_candidate_blocks)
{
    const size_t channels = static_cast<size_t>(search_block->channels());
    const size_t block_size = static_cast<size_t>(target_block->frames());
    std::vector<float> dot_prod(channels);

    float best_similarity = std::numeric_limits<float>::lowest();
    size_t optimal_index = 0;

    for (size_t n = low_limit; n <= high_limit; ++n) {
        if (InInterval(n, exclude_interval)) {
            continue;
        }
        MultiChannelDotProduct(target_block, 0, search_block, n, block_size, dot_prod);

        float similarity
            = MultiChannelSimilarityMeasure(dot_prod,
                                            energy_target_block,
                                            energy_candidate_blocks.subspan(n * channels, channels));

        if (similarity > best_similarity) {
            best_similarity = similarity;
            optimal_index = n;
        }
    }

    return optimal_index;
}

size_t OptimalIndex(const AudioBus *search_block,
                    const AudioBus *target_block,
                    Interval exclude_interval)
{
    assert(search_block->channels() == target_block->channels());
    const size_t channels = search_block->channels();
    const size_t target_size = target_block->frames();
    assert(static_cast<size_t>(search_block->frames()) >= target_size - 1);
    const size_t num_candidate_blocks = search_block->frames() - (target_size - 1);

    // This is a compromise between complexity reduction and search accuracy. I
    // don't have a proof that down sample of order 5 is optimal. One can compute
    // a decimation factor that minimizes complexity given the size of
    // |search_block| and |target_block|. However, my experiments show the rate of
    // missing the optimal index is significant. This value is chosen
    // heuristically based on experiments.
    constexpr size_t kSearchDecimation = 5;

    std::vector<float> energy_target_block(channels);
    std::vector<float> energy_candidate_blocks(channels * num_candidate_blocks);

    // Energy of all candid frames.
    MultiChannelMovingBlockEnergies(search_block, target_size, energy_candidate_blocks);

    // Energy of target frame.
    MultiChannelDotProduct(target_block, 0, target_block, 0, target_size, energy_target_block);

    const size_t optimal_index = DecimatedSearch(kSearchDecimation,
                                                 exclude_interval,
                                                 target_block,
                                                 search_block,
                                                 energy_target_block,
                                                 energy_candidate_blocks);

    const size_t lim_low = optimal_index < kSearchDecimation ? 0u
                                                             : optimal_index - kSearchDecimation;
    const size_t lim_high = std::min(num_candidate_blocks - 1, optimal_index + kSearchDecimation);
    return FullSearch(lim_low,
                      lim_high,
                      exclude_interval,
                      target_block,
                      search_block,
                      energy_target_block,
                      energy_candidate_blocks);
}

void GetPeriodicHanningWindow(std::span<float> window)
{
    const float scale = 2.0f * std::numbers::pi_v<float> / window.size();
    for (size_t n = 0; n < window.size(); ++n) {
        window[n] = 0.5f * (1.0f - std::cos(n * scale));
    }
}

} // namespace internal

// --------------------------------------------------- AudioRendererAlgorithm

// Waveform Similarity Overlap-and-add (WSOLA).
//
// One WSOLA iteration
//
// 1) Extract |target_block_| as input frames at indices
//    [|target_block_index_|, |target_block_index_| + |ola_window_size_|).
//    Note that |target_block_| is the "natural" continuation of the output.
//
// 2) Extract |search_block_| as input frames at indices
//    [|search_block_index_|,
//     |search_block_index_| + |num_candidate_blocks_| + |ola_window_size_|).
//
// 3) Find a block within the |search_block_| that is most similar
//    to |target_block_|. Let |optimal_index| be the index of such block and
//    write it to |optimal_block_|.
//
// 4) Update:
//    |optimal_block_| = |transition_window_| * |target_block_| +
//    (1 - |transition_window_|) * |optimal_block_|.
//
// 5) Overlap-and-add |optimal_block_| to the |wsola_output_|.
//
// 6) Update:
//    |target_block_| = |optimal_index| + |ola_window_size_| / 2.
//    |output_index_| = |output_index_| + |ola_window_size_| / 2,
//    |search_block_center_index| = |output_index_| * |playback_rate|, and
//    |search_block_index_| = |search_block_center_index| -
//        |search_block_center_offset_|.

// Overlap-and-add window size in milliseconds.
constexpr int kOlaWindowSizeMs = 20;

// Size of search interval in milliseconds. The search interval is
// [-delta delta] around |output_index_| * |playback_rate|. So the search
// interval is 2 * delta.
constexpr int kWsolaSearchIntervalMs = 30;

// Equivalent of AudioTimestampHelper::TimeToFrames(): rounds to nearest.
static int MsToFrames(int ms, int samples_per_second)
{
    return static_cast<int>(std::round(ms * static_cast<double>(samples_per_second) / 1000.0));
}

void AudioRendererAlgorithm::Initialize(int channels, int samples_per_second)
{
    assert(channels > 0 && samples_per_second > 0);

    channels_ = channels;
    samples_per_second_ = samples_per_second;
    audio_buffer_.Reset(channels_);
    num_candidate_blocks_ = MsToFrames(kWsolaSearchIntervalMs, samples_per_second_);
    ola_window_size_ = MsToFrames(kOlaWindowSizeMs, samples_per_second_);

    // Make sure window size is an even number.
    ola_window_size_ += ola_window_size_ & 1;
    ola_hop_size_ = ola_window_size_ / 2;

    // |num_candidate_blocks_| / 2 is the offset of the center of the search
    // block to the center of the first (left most) candidate block. The offset
    // of the center of a candidate block to its left most point is
    // |ola_window_size_| / 2 - 1. Note that |ola_window_size_| is even and in
    // our convention the center belongs to the left half, so we need to subtract
    // one frame to get the correct offset.
    search_block_center_offset_ = num_candidate_blocks_ / 2 + (ola_window_size_ / 2 - 1);

    // Re-initialization discards any state built for the previous format.
    ola_window_.clear();
    output_time_ = 0.0;
    search_block_index_ = 0;
    target_block_index_ = 0;
    num_complete_frames_ = 0;
    effective_playback_rate_ = 0;
    last_mode_ = FillBufferMode::kPassthrough;

    // If no mask is provided, assume all channels are valid.
    channel_mask_.clear();
    SetChannelMask(std::vector<bool>(channels_, true));
}

void AudioRendererAlgorithm::SetChannelMask(std::vector<bool> channel_mask)
{
    assert(channel_mask.size() == static_cast<size_t>(channels_));
    channel_mask_ = std::move(channel_mask);
    if (!ola_window_.empty()) {
        CreateSearchWrappers();
    }
}

int AudioRendererAlgorithm::RunWsolaAndFill(AudioBus *dest,
                                            int dest_offset,
                                            int requested_frames,
                                            double playback_rate)
{
    // Allocate structures on first non-1.0 playback rate; these can eat a fair
    // chunk of memory. ~56kB for stereo 48kHz, up to ~765kB for 7.1 192kHz.
    if (ola_window_.empty()) {
        ola_window_.resize(ola_window_size_);
        internal::GetPeriodicHanningWindow(ola_window_);

        transition_window_.resize(ola_window_size_ * 2);
        internal::GetPeriodicHanningWindow(transition_window_);

        // Initialize for overlap-and-add of the first block.
        wsola_output_ = AudioBus::Create(channels_, ola_window_size_ + ola_hop_size_);
        wsola_output_->Zero();

        // Auxiliary containers.
        optimal_block_ = AudioBus::Create(channels_, ola_window_size_);
        search_block_ = AudioBus::Create(channels_, num_candidate_blocks_ + (ola_window_size_ - 1));
        target_block_ = AudioBus::Create(channels_, ola_window_size_);

        // Create potentially smaller wrappers for playback rate adaptation.
        CreateSearchWrappers();
    }

    internal::ScopedSubnormalFloatDisabler disable_subnormals;

    // WSOLA doesn't actually consume input frames until the WSOLA iteration
    // completes; see RemoveOldInputFrames() in RunOneWsolaIteration().
    const auto initial_input_frames = audio_buffer_.frames();

    int rendered_frames = 0;
    do {
        rendered_frames += WriteCompletedFramesTo(requested_frames - rendered_frames,
                                                  dest_offset + rendered_frames,
                                                  dest);
    } while (rendered_frames < requested_frames && RunOneWsolaIteration(playback_rate));

    // The effective rate is just how many input frames were used to produce the
    // requested number of output frames. We don't want the cumulative value since
    // that is just ~`playback_rate`, but instead the "impulse" of this call.
    //
    // Note: The effective rate may briefly be zero for playback rates below 1.0.
    // Note 2: During end-of-stream, `rendered_frames` may be zero.
    if (rendered_frames > 0) {
        effective_playback_rate_ = (initial_input_frames - audio_buffer_.frames())
                                   / static_cast<double>(rendered_frames);
    } else {
        effective_playback_rate_ = playback_rate;
    }
    return rendered_frames;
}

int AudioRendererAlgorithm::FillBuffer(AudioBus *dest,
                                       int dest_offset,
                                       int requested_frames,
                                       double playback_rate)
{
    if (playback_rate == 0) {
        return 0;
    }

    assert(playback_rate > 0);
    assert(channels_ == dest->channels());

    const FillBufferMode fill_buffer_mode = ChooseBufferMode(playback_rate);
    SetFillBufferMode(fill_buffer_mode);

    switch (fill_buffer_mode) {
    case FillBufferMode::kPassthrough: {
        // Optimize the most common `playback_rate` ~= 1 case to use a single copy
        // instead of copying frame by frame.
        const int frames_to_copy = std::min(audio_buffer_.frames(), requested_frames);
        const int frames_read = audio_buffer_.ReadFrames(frames_to_copy, dest_offset, dest);
        effective_playback_rate_ = 1.0;
        return frames_read;
    }
    case FillBufferMode::kWSOLA:
        return RunWsolaAndFill(dest, dest_offset, requested_frames, playback_rate);
    }
    return 0;
}

AudioRendererAlgorithm::FillBufferMode AudioRendererAlgorithm::ChooseBufferMode(double playback_rate)
{
    // Optimize the `playback_rate` == 1 case to use a single copy instead of
    // copying frame by frame. Chromium also passes through rates that are only
    // close to 1, but here the output must have exactly the requested length.
    if (playback_rate == 1.0) {
        return FillBufferMode::kPassthrough;
    }

    return FillBufferMode::kWSOLA;
}

void AudioRendererAlgorithm::SetFillBufferMode(FillBufferMode mode)
{
    if (last_mode_ == mode)
        return;

    // Clear any state from other fill modes so that we don't produce outdated
    // audio later.
    if (last_mode_ == FillBufferMode::kWSOLA) {
        output_time_ = 0.0;
        search_block_index_ = 0;
        target_block_index_ = 0;
        if (wsola_output_)
            wsola_output_->Zero();
        num_complete_frames_ = 0;
        effective_playback_rate_ = 0;
    }

    last_mode_ = mode;
}

void AudioRendererAlgorithm::FlushBuffers()
{
    // Clear the queue of decoded packets (releasing the buffers).
    audio_buffer_.Clear();
    output_time_ = 0.0;
    search_block_index_ = 0;
    target_block_index_ = 0;
    if (wsola_output_)
        wsola_output_->Zero();
    num_complete_frames_ = 0;
}

void AudioRendererAlgorithm::EnqueueBuffer(const float *const *planes, int frames)
{
    audio_buffer_.Append(planes, frames);
}

void AudioRendererAlgorithm::EnqueueInterleaved(const float *interleaved, int frames)
{
    audio_buffer_.AppendInterleaved(interleaved, frames);
}

double AudioRendererAlgorithm::DelayInFrames(double playback_rate) const
{
    // When |playback_rate| == 1, we read directly from |audio_buffer_|.
    if (playback_rate == 1.0)
        return audio_buffer_.frames();

    const float buffered_output_frames = BufferedFrames() / playback_rate;
    const float unconverted_output_frames = buffered_output_frames - output_time_;
    return unconverted_output_frames + num_complete_frames_;
}

bool AudioRendererAlgorithm::CanPerformWsola() const
{
    const int search_block_size = num_candidate_blocks_ + (ola_window_size_ - 1);
    const int frames = audio_buffer_.frames();
    return target_block_index_ + ola_window_size_ <= frames
           && search_block_index_ + search_block_size <= frames;
}

bool AudioRendererAlgorithm::RunOneWsolaIteration(double playback_rate)
{
    if (!CanPerformWsola())
        return false;

    GetOptimalBlock();

    // Overlap-and-add.
    const size_t ola_hop_size = static_cast<size_t>(ola_hop_size_);
    for (int k = 0; k < channels_; ++k) {
        if (!channel_mask_[k])
            continue;

        const std::span<const float> ch_opt_frame = std::as_const(*optimal_block_).channel(k);
        std::span<float> ch_output = wsola_output_->channel(k).subspan(
            static_cast<size_t>(num_complete_frames_));
        for (size_t n = 0u; n < ola_hop_size; ++n) {
            ch_output[n] = ch_output[n] * ola_window_[ola_hop_size + n]
                           + ch_opt_frame[n] * ola_window_[n];
        }

        // Copy the second half to the output.
        std::copy_n(ch_opt_frame.begin() + ola_hop_size,
                    ola_hop_size,
                    ch_output.begin() + ola_hop_size);
    }

    num_complete_frames_ += ola_hop_size_;
    UpdateOutputTime(playback_rate, ola_hop_size_);
    RemoveOldInputFrames(playback_rate);
    return true;
}

void AudioRendererAlgorithm::UpdateOutputTime(double playback_rate, double time_change)
{
    output_time_ += time_change;
    // Center of the search region, in frames.
    const int search_block_center_index = static_cast<int>(output_time_ * playback_rate + 0.5);
    search_block_index_ = search_block_center_index - search_block_center_offset_;
}

void AudioRendererAlgorithm::RemoveOldInputFrames(double playback_rate)
{
    const int earliest_used_index = std::min(target_block_index_, search_block_index_);
    if (earliest_used_index <= 0)
        return; // Nothing to remove.

    // Remove frames from input and adjust indices accordingly.
    audio_buffer_.SeekFrames(earliest_used_index);
    target_block_index_ -= earliest_used_index;

    // Adjust output index.
    double output_time_change = static_cast<double>(earliest_used_index) / playback_rate;
    assert(output_time_ >= output_time_change);
    UpdateOutputTime(playback_rate, -output_time_change);
}

int AudioRendererAlgorithm::WriteCompletedFramesTo(int requested_frames,
                                                   int dest_offset,
                                                   AudioBus *dest)
{
    int rendered_frames = std::min(num_complete_frames_, requested_frames);

    if (rendered_frames == 0)
        return 0; // There is nothing to read from |wsola_output_|, return.

    wsola_output_->CopyPartialFramesTo(0, rendered_frames, dest_offset, dest);

    // Remove the frames which are read.
    size_t frames_to_move = wsola_output_->frames() - rendered_frames;
    for (int k = 0; k < channels_; ++k) {
        if (!channel_mask_[k])
            continue;
        std::span<float> ch = wsola_output_->channel(k);
        std::memmove(ch.data(), ch.data() + rendered_frames, sizeof(float) * frames_to_move);
    }
    num_complete_frames_ -= rendered_frames;
    return rendered_frames;
}

bool AudioRendererAlgorithm::TargetIsWithinSearchRegion() const
{
    const int search_block_size = num_candidate_blocks_ + (ola_window_size_ - 1);

    return target_block_index_ >= search_block_index_
           && target_block_index_ + ola_window_size_ <= search_block_index_ + search_block_size;
}

void AudioRendererAlgorithm::GetOptimalBlock()
{
    int optimal_index = 0;

    // An interval around last optimal block which is excluded from the search.
    // This is to reduce the buzzy sound. The number 160 is rather arbitrary and
    // derived heuristically.
    const int kExcludeIntervalLengthFrames = 160;
    if (TargetIsWithinSearchRegion()) {
        optimal_index = target_block_index_;
        PeekAudioWithZeroPrepend(optimal_index, optimal_block_.get());
    } else {
        PeekAudioWithZeroPrepend(target_block_index_, target_block_.get());
        PeekAudioWithZeroPrepend(search_block_index_, search_block_.get());
        int last_optimal = target_block_index_ - ola_hop_size_ - search_block_index_;
        // Same int -> size_t conversion as upstream: a negative bound wraps,
        // which effectively disables the exclusion.
        internal::Interval exclude_interval
            = std::make_pair(last_optimal - kExcludeIntervalLengthFrames / 2,
                             last_optimal + kExcludeIntervalLengthFrames / 2);

        // |optimal_index| is in frames and it is relative to the beginning of the
        // |search_block_|.
        optimal_index = internal::OptimalIndex(search_block_wrapper_.get(),
                                               target_block_wrapper_.get(),
                                               exclude_interval);

        // Translate |index| w.r.t. the beginning of |audio_buffer_| and extract the
        // optimal block.
        optimal_index += search_block_index_;
        PeekAudioWithZeroPrepend(optimal_index, optimal_block_.get());

        // Make a transition from target block to the optimal block if different.
        // Target block has the best continuation to the current output.
        // Optimal block is the most similar block to the target, however, it might
        // introduce some discontinuity when over-lap-added. Therefore, we combine
        // them for a smoother transition. The length of transition window is twice
        // as that of the optimal-block which makes it like a weighting function
        // where target-block has higher weight close to zero (weight of 1 at index
        // 0) and lower weight close the end.
        for (int k = 0; k < channels_; ++k) {
            if (!channel_mask_[k])
                continue;
            std::span<float> ch_opt = optimal_block_->channel(k);
            const std::span<const float> ch_target = std::as_const(*target_block_).channel(k);
            for (int n = 0; n < ola_window_size_; ++n) {
                ch_opt[n] = ch_opt[n] * transition_window_[n]
                            + ch_target[n] * transition_window_[ola_window_size_ + n];
            }
        }
    }

    // Next target is one hop ahead of the current optimal.
    target_block_index_ = optimal_index + ola_hop_size_;
}

void AudioRendererAlgorithm::PeekAudioWithZeroPrepend(int read_offset_frames, AudioBus *dest)
{
    assert(read_offset_frames + dest->frames() <= audio_buffer_.frames());

    int write_offset = 0;
    int num_frames_to_read = dest->frames();
    if (read_offset_frames < 0) {
        int num_zero_frames_appended = std::min(-read_offset_frames, num_frames_to_read);
        read_offset_frames = 0;
        num_frames_to_read -= num_zero_frames_appended;
        write_offset = num_zero_frames_appended;
        dest->ZeroFrames(num_zero_frames_appended);
    }
    audio_buffer_.PeekFrames(num_frames_to_read, read_offset_frames, write_offset, dest);
}

void AudioRendererAlgorithm::CreateSearchWrappers()
{
    // WSOLA is quite expensive to run, so if a channel mask exists, use it to
    // reduce the size of our search space.
    AudioBus::ChannelVector active_target_channels;
    AudioBus::ChannelVector active_search_channels;
    for (int ch = 0; ch < channels_; ++ch) {
        if (channel_mask_[ch]) {
            active_target_channels.push_back(target_block_->channel(ch).data());
            active_search_channels.push_back(search_block_->channel(ch).data());
        }
    }

    target_block_wrapper_ = AudioBus::CreateWrapper(active_target_channels, target_block_->frames());
    search_block_wrapper_ = AudioBus::CreateWrapper(active_search_channels, search_block_->frames());
}

} // namespace wsola
