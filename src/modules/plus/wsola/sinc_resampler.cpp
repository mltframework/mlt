/*
 * sinc_resampler.cpp -- sample rate converter ported from Chromium
 *
 * Port of media/base/sinc_resampler.cc from the Chromium project.
 *
 * Copyright 2012 The Chromium Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in wsola_algorithm.h.
 */

// Initial input buffer layout, dividing into regions r0_ to r4_ (note: r0_, r3_
// and r4_ will move after the first load):
//
// |----------------|-----------------------------------------|----------------|
//
//                                        request_frames_
//                   <--------------------------------------------------------->
//                                    r0_ (during first load)
//
//  kernel_size_ / 2  kernel_size_ / 2        kernel_size_ / 2  kernel_size_ / 2
// <---------------> <--------------->       <---------------> <--------------->
//        r1_               r2_                     r3_               r4_
//
//                             block_size_ == r4_ - r2_
//                   <--------------------------------------->
//
//                                                  request_frames_
//                                    <------------------ ... ----------------->
//                                               r0_ (during second load)
//
// On the second request r0_ slides to the right by kernel_size_ / 2 and r3_,
// r4_ and block_size_ are reinitialized via step (3) in the algorithm below.
//
// These new regions remain constant until a Flush() occurs.  While complicated,
// this allows us to reduce jitter by always requesting the same amount from the
// provided callback.
//
// The algorithm:
//
// 1) Allocate input_buffer of size: request_frames_ + kernel_size_; this
// ensures
//    there's enough room to read request_frames_ from the callback into region
//    r0_ (which will move between the first and subsequent passes).
//
// 2) Let r1_, r2_ each represent half the kernel centered around r0_:
//
//        r0_ = input_buffer_ + kernel_size_ / 2
//        r1_ = input_buffer_
//        r2_ = r0_
//
//    r0_ is always request_frames_ in size.  r1_, r2_ are kernel_size_ / 2 in
//    size.  r1_ must be zero initialized to avoid convolution with garbage (see
//    step (5) for why).
//
// 3) Let r3_, r4_ each represent half the kernel right aligned with the end of
//    r0_ and choose block_size_ as the distance in frames between r4_ and r2_:
//
//        r3_ = r0_ + request_frames_ - kernel_size_
//        r4_ = r0_ + request_frames_ - kernel_size_ / 2
//        block_size_ = r4_ - r2_ = request_frames_ - kernel_size_ / 2
//
// 4) Consume request_frames_ frames into r0_.
//
// 5) Position kernel centered at start of r2_ and generate output frames until
//    the kernel is centered at the start of r4_ or we've finished generating
//    all the output frames.
//
// 6) Wrap left over data from the r3_ to r1_ and r4_ to r2_.
//
// 7) If we're on the second load, in order to avoid overwriting the frames we
//    just wrapped from r4_ we need to slide r0_ to the right by the size of
//    r4_, which is kernel_size_ / 2:
//
//        r0_ = r0_ + kernel_size_ / 2 = input_buffer_ + kernel_size_
//
//    r3_, r4_, and block_size_ then need to be reinitialized, so goto (3).
//
// 8) Else, if we're not on the second load, goto (4).
//
// Note: we're glossing over how the sub-sample handling works with
// |virtual_source_idx_|, etc.

#include "sinc_resampler.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>

#if defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#elif defined(__ARM_NEON)
#include <arm_neon.h>
#endif

namespace wsola {

namespace {
constexpr size_t kAlignment = 32;

bool IsAligned(const void *p, size_t alignment)
{
    return (reinterpret_cast<uintptr_t>(p) & (alignment - 1)) == 0;
}
} // namespace

static double SincScaleFactor(double io_ratio, size_t kernel_size)
{
    // |sinc_scale_factor| is basically the normalized cutoff frequency of the
    // low-pass filter.
    double sinc_scale_factor = io_ratio > 1.0 ? 1.0 / io_ratio : 1.0;

    // The sinc function is an idealized brick-wall filter, but since we're
    // windowing it the transition from pass to stop does not happen right away.
    // So we should adjust the low pass filter cutoff slightly downward to avoid
    // some aliasing at the very high-end.
    // Note: these values are derived empirically.
    if (kernel_size == SincResampler::kMaxKernelSize) {
        sinc_scale_factor *= 0.92;
    } else {
        assert(kernel_size == SincResampler::kMinKernelSize);
        sinc_scale_factor *= 0.90;
    }

    return sinc_scale_factor;
}

static int CalculateChunkSize(int block_size_, double io_ratio)
{
    return block_size_ / io_ratio;
}

SincResampler::AlignedArray SincResampler::AllocateAligned(size_t size)
{
    float *p = static_cast<float *>(
        ::operator new[](size * sizeof(float), std::align_val_t(kAlignment)));
    std::fill_n(p, size, 0.0f);
    return AlignedArray(p);
}

SincResampler::SincResampler(double io_sample_rate_ratio, int request_frames, ReadCB read_cb)
    : kernel_size_(KernelSizeFromRequestFrames(request_frames))
    , io_sample_rate_ratio_(io_sample_rate_ratio)
    , read_cb_(std::move(read_cb))
    , request_frames_(request_frames)
    , input_buffer_size_(request_frames_ + kernel_size_)
    , kernel_storage_(AllocateAligned(kernel_size_ * (kKernelOffsetCount + 1)))
    , kernel_pre_sinc_storage_(AllocateAligned(kernel_size_ * (kKernelOffsetCount + 1)))
    , kernel_window_storage_(AllocateAligned(kernel_size_ * (kKernelOffsetCount + 1)))
    , input_buffer_(AllocateAligned(input_buffer_size_))
    , r1_(input_buffer_.get())
    , r2_(input_buffer_.get() + kernel_size_ / 2)
{
    assert(request_frames >= static_cast<int>(kMinRequestSize));
    Flush();
    InitializeKernel();
}

SincResampler::~SincResampler() = default;

void SincResampler::UpdateRegions(bool second_load)
{
    // Setup various region pointers in the buffer (see diagram above). If we're
    // on the second load we need to slide r0_ to the right by kernel_size_ / 2.
    r0_ = input_buffer_.get() + (second_load ? kernel_size_ : kernel_size_ / 2);
    r3_ = r0_ + request_frames_ - kernel_size_;
    r4_ = r0_ + request_frames_ - kernel_size_ / 2;
    block_size_ = r4_ - r2_;
    chunk_size_ = CalculateChunkSize(block_size_, io_sample_rate_ratio_);

    // r1_ at the beginning of the buffer.
    assert(r1_ == input_buffer_.get());
    // r1_ left of r2_, r4_ left of r3_ and size correct.
    assert(r2_ - r1_ == r4_ - r3_);
    // r2_ left of r3.
    assert(r2_ < r3_);
}

void SincResampler::InitializeKernel()
{
    // Blackman window parameters.
    static constexpr double kAlpha = 0.16;
    static constexpr double kA0 = 0.5 * (1.0 - kAlpha);
    static constexpr double kA1 = 0.5;
    static constexpr double kA2 = 0.5 * kAlpha;

    // Generates a set of windowed sinc() kernels.
    // We generate a range of sub-sample offsets from 0.0 to 1.0.
    const double sinc_scale_factor = SincScaleFactor(io_sample_rate_ratio_, kernel_size_);
    for (int offset_idx = 0; offset_idx <= kKernelOffsetCount; ++offset_idx) {
        const float subsample_offset = static_cast<float>(offset_idx) / kKernelOffsetCount;
        const int signed_kernel_size = static_cast<int>(kernel_size_);
        for (int i = 0; i < signed_kernel_size; ++i) {
            const int idx = i + offset_idx * kernel_size_;
            const float pre_sinc
                = std::numbers::pi_v<float> * (i - signed_kernel_size / 2 - subsample_offset);
            kernel_pre_sinc_storage_[idx] = pre_sinc;

            // Compute Blackman window, matching the offset of the sinc().
            const float x = (i - subsample_offset) / signed_kernel_size;
            const float window = static_cast<float>(kA0 - kA1 * cos(2.0 * std::numbers::pi * x)
                                                    + kA2 * cos(4.0 * std::numbers::pi * x));
            kernel_window_storage_[idx] = window;

            // Compute the sinc with offset, then window the sinc() function and
            // store at the correct offset.
            kernel_storage_[idx] = static_cast<float>(
                window
                * (pre_sinc ? sin(sinc_scale_factor * pre_sinc) / pre_sinc : sinc_scale_factor));
        }
    }
}

void SincResampler::SetRatio(double io_sample_rate_ratio)
{
    if (fabs(io_sample_rate_ratio_ - io_sample_rate_ratio)
        < std::numeric_limits<double>::epsilon()) {
        return;
    }

    io_sample_rate_ratio_ = io_sample_rate_ratio;
    chunk_size_ = CalculateChunkSize(block_size_, io_sample_rate_ratio_);

    // Optimize reinitialization by reusing values which are independent of
    // |sinc_scale_factor|. Provides a 3x speedup.
    const double sinc_scale_factor = SincScaleFactor(io_sample_rate_ratio_, kernel_size_);
    for (int offset_idx = 0; offset_idx <= kKernelOffsetCount; ++offset_idx) {
        for (size_t i = 0; i < kernel_size_; ++i) {
            const int idx = i + offset_idx * kernel_size_;
            const float window = kernel_window_storage_[idx];
            const float pre_sinc = kernel_pre_sinc_storage_[idx];

            kernel_storage_[idx] = static_cast<float>(
                window
                * (pre_sinc ? sin(sinc_scale_factor * pre_sinc) / pre_sinc : sinc_scale_factor));
        }
    }
}

void SincResampler::Resample(float *destination, int frames)
{
    int remaining_frames = frames;

    // Step (1) -- Prime the input buffer at the start of the input stream.
    if (!buffer_primed_ && remaining_frames) {
        read_cb_(r0_, request_frames_);
        buffer_primed_ = true;
    }

    // Step (2) -- Resample!
    while (remaining_frames) {
        // Note: The loop construct here can severely impact performance on
        // ARM or when built with clang. See https://codereview.chromium.org/18566009/
        while (virtual_source_idx_ < block_size_) {
            // |virtual_source_idx_| lies in between two kernel offsets so
            // figure out what they are.
            const size_t source_idx = static_cast<size_t>(virtual_source_idx_);
            const double virtual_offset_idx = (virtual_source_idx_ - source_idx)
                                              * kKernelOffsetCount;
            const int offset_idx = static_cast<int>(virtual_offset_idx);

            // We'll compute "convolutions" for the two kernels which straddle
            // |virtual_source_idx_|.
            const size_t starting_idx = offset_idx * kernel_size_;
            const float *k1 = kernel_storage_.get() + starting_idx;
            const float *k2 = k1 + kernel_size_;

            // Ensure |k1|, |k2| are 32-byte aligned for SIMD usage. Should
            // always be true so long as kernel_size_ is a multiple of 32.
            assert(IsAligned(k1, kAlignment));
            assert(IsAligned(k2, kAlignment));

            // Initialize input pointer based on quantized
            // |virtual_source_idx_|.
            const float *input_ptr = r1_ + source_idx;

            // Figure out how much to weight each kernel's "convolution".
            const double kernel_interpolation_factor = virtual_offset_idx - offset_idx;
#if defined(__x86_64__) || defined(__i386__)
            *destination++
                = Convolve_SSE(input_ptr, k1, k2, kernel_size_, kernel_interpolation_factor);
#elif defined(__ARM_NEON)
            *destination++
                = Convolve_NEON(input_ptr, k1, k2, kernel_size_, kernel_interpolation_factor);
#else
            *destination++
                = Convolve_C(input_ptr, k1, k2, kernel_size_, kernel_interpolation_factor);
#endif

            // Advance the virtual index.
            virtual_source_idx_ += io_sample_rate_ratio_;
            if (!--remaining_frames) {
                return;
            }
        }

        // Wrap back around to the start.
        assert(virtual_source_idx_ >= block_size_);
        virtual_source_idx_ -= block_size_;

        // Step (3) -- Copy r3_, r4_ to r1_, r2_.
        // This wraps the last input frames back to the start of the buffer.
        std::copy_n(r3_, kernel_size_, r1_);

        // Step (4) -- Reinitialize regions if necessary.
        if (r0_ == r2_) {
            UpdateRegions(true);
        }

        // Step (5) -- Refresh the buffer with more input.
        read_cb_(r0_, request_frames_);
    }
}

void SincResampler::PrimeWithSilence()
{
    // By enforcing the buffer hasn't been primed, we ensure the input buffer
    // has already been zeroed during construction or by a previous Flush().
    assert(!buffer_primed_);
    assert(input_buffer_[0] == 0.0f);
    UpdateRegions(true);
}

void SincResampler::Flush()
{
    virtual_source_idx_ = 0;
    buffer_primed_ = false;
    std::fill_n(input_buffer_.get(), input_buffer_size_, 0.0f);
    UpdateRegions(false);
}

double SincResampler::BufferedFrames() const
{
    return buffer_primed_ ? request_frames_ - virtual_source_idx_ : 0;
}

float SincResampler::Convolve_C(const float *input_ptr,
                                const float *k1,
                                const float *k2,
                                size_t kernel_size,
                                double kernel_interpolation_factor)
{
    float sum1 = 0;
    float sum2 = 0;

    // Generate a single output sample. Unrolling this loop hurt performance in
    // local testing.
    for (size_t i = 0; i < kernel_size; ++i) {
        sum1 += input_ptr[i] * k1[i];
        sum2 += input_ptr[i] * k2[i];
    }

    // Linearly interpolate the two "convolutions".
    return static_cast<float>((1.0 - kernel_interpolation_factor) * sum1
                              + kernel_interpolation_factor * sum2);
}

#if defined(__x86_64__) || defined(__i386__)
float SincResampler::Convolve_SSE(const float *input_ptr,
                                  const float *k1,
                                  const float *k2,
                                  size_t kernel_size,
                                  double kernel_interpolation_factor)
{
    __m128 m_input;
    __m128 m_sums1 = _mm_setzero_ps();
    __m128 m_sums2 = _mm_setzero_ps();

    // Based on |input_ptr| alignment, we need to use loadu or load. Unrolling
    // these loops hurt performance in local testing.
    if (IsAligned(input_ptr, 16)) {
        for (size_t i = 0; i < kernel_size; i += 4) {
            m_input = _mm_load_ps(input_ptr + i);
            m_sums1 = _mm_add_ps(m_sums1, _mm_mul_ps(m_input, _mm_load_ps(k1 + i)));
            m_sums2 = _mm_add_ps(m_sums2, _mm_mul_ps(m_input, _mm_load_ps(k2 + i)));
        }
    } else {
        for (size_t i = 0; i < kernel_size; i += 4) {
            m_input = _mm_loadu_ps(input_ptr + i);
            m_sums1 = _mm_add_ps(m_sums1, _mm_mul_ps(m_input, _mm_load_ps(k1 + i)));
            m_sums2 = _mm_add_ps(m_sums2, _mm_mul_ps(m_input, _mm_load_ps(k2 + i)));
        }
    }

    // Linearly interpolate the two "convolutions".
    m_sums1 = _mm_mul_ps(m_sums1,
                         _mm_set_ps1(static_cast<float>(1.0 - kernel_interpolation_factor)));
    m_sums2 = _mm_mul_ps(m_sums2, _mm_set_ps1(static_cast<float>(kernel_interpolation_factor)));
    m_sums1 = _mm_add_ps(m_sums1, m_sums2);

    // Sum components together.
    float result;
    m_sums2 = _mm_add_ps(_mm_movehl_ps(m_sums1, m_sums1), m_sums1);
    _mm_store_ss(&result, _mm_add_ss(m_sums2, _mm_shuffle_ps(m_sums2, m_sums2, 1)));

    return result;
}
#elif defined(__ARM_NEON)
float SincResampler::Convolve_NEON(const float *input_ptr,
                                   const float *k1,
                                   const float *k2,
                                   size_t kernel_size,
                                   double kernel_interpolation_factor)
{
    float32x4_t m_input;
    float32x4_t m_sums1 = vmovq_n_f32(0);
    float32x4_t m_sums2 = vmovq_n_f32(0);

    for (size_t i = 0; i < kernel_size; i += 4) {
        m_input = vld1q_f32(input_ptr + i);
        m_sums1 = vmlaq_f32(m_sums1, m_input, vld1q_f32(k1 + i));
        m_sums2 = vmlaq_f32(m_sums2, m_input, vld1q_f32(k2 + i));
    }

    // Linearly interpolate the two "convolutions".
    m_sums1 = vmlaq_f32(vmulq_f32(m_sums1, vmovq_n_f32(1.0 - kernel_interpolation_factor)),
                        m_sums2,
                        vmovq_n_f32(kernel_interpolation_factor));

    // Sum components together.
    float32x2_t m_half = vadd_f32(vget_high_f32(m_sums1), vget_low_f32(m_sums1));
    return vget_lane_f32(vpadd_f32(m_half, m_half), 0);
}
#endif

} // namespace wsola
