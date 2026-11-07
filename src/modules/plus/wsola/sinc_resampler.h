/*
 * sinc_resampler.h -- sample rate converter ported from Chromium
 *
 * Port of media/base/sinc_resampler.{h,cc} from the Chromium project. The
 * algorithm, constants and index arithmetic are kept identical to upstream;
 * base::span, base::RepeatingCallback and base::AlignedHeapArray were
 * replaced by pointers, std::function and an aligned buffer, and the AVX2
 * path (which needs run time CPU detection) was left out.
 *
 * Copyright 2012 The Chromium Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in wsola_algorithm.h.
 */

#ifndef MLT_WSOLA_SINC_RESAMPLER_H
#define MLT_WSOLA_SINC_RESAMPLER_H

#include <cstddef>
#include <functional>
#include <memory>
#include <new>

namespace wsola {

// SincResampler is a high-quality single-channel sample-rate converter.
class SincResampler
{
public:
    // The kernel size can be adjusted for quality (higher is better) at the
    // expense of performance. Must be a multiple of 32. 64 is used for
    // perceptible audio quality, with a fallback to 32 when |request_frames|
    // is too small.
    static constexpr size_t kMaxKernelSize = 64;
    static constexpr size_t kMinKernelSize = 32;

    // Minimum request size. SincResampler requires request_frames >= 1.5 *
    // |kernel_size_|, where the minimum kernel size is |kMinKernelSize|.
    static constexpr size_t kMinRequestSize = kMinKernelSize * 3 / 2;

    // Default request size. Affects how often and for how much SincResampler
    // calls back for input. Must be greater than 1.5 * |kernel_size_|.
    static constexpr int kDefaultRequestSize = 512;

    // A smaller request size, which still allows higher quality resampling, by
    // guaranteeing we will use kMaxKernelSize.
    static constexpr int kSmallRequestSize = kMaxKernelSize * 2;

    // The kernel offset count is used for interpolation and is the number of
    // sub-sample kernel shifts. Can be adjusted for quality (higher is better)
    // at the expense of allocating more memory.
    static constexpr int kKernelOffsetCount = 32;

    // Callback type for providing more data into the resampler. Expects
    // |frames| of data to be rendered into |destination|; zero padded if not
    // enough frames are available to satisfy the request.
    using ReadCB = std::function<void(float *destination, int frames)>;

    // Returns the kernel size which will be used for a given |request_frames|.
    static constexpr size_t KernelSizeFromRequestFrames(int request_frames)
    {
        return request_frames >= static_cast<int>(kMaxKernelSize * 3 / 2) ? kMaxKernelSize
                                                                          : kMinKernelSize;
    }

    // Constructs a SincResampler with the specified |read_cb|, which is used to
    // acquire audio data for resampling. |io_sample_rate_ratio| is the ratio
    // of input / output sample rates. |request_frames| controls the size in
    // frames of the buffer requested by each |read_cb| call. The value must be
    // greater than 1.5*|kernel_size_|.
    SincResampler(double io_sample_rate_ratio, int request_frames, ReadCB read_cb);
    SincResampler(const SincResampler &) = delete;
    SincResampler &operator=(const SincResampler &) = delete;
    ~SincResampler();

    // Resample |frames| of data from |read_cb_| into |destination|.
    void Resample(float *destination, int frames);

    // The maximum size in output frames that guarantees Resample() will only
    // make a single call to |read_cb_| for more data. Note: If
    // PrimeWithSilence() is not called, chunk size will grow after the first
    // two Resample() calls by |kernel_size_| / (2 * io_sample_rate_ratio).
    int ChunkSize() const { return static_cast<int>(chunk_size_); }

    // Guarantees that ChunkSize() will not change between calls by
    // initializing the input buffer with silence. Note, this will cause the
    // first few samples of output to be biased towards silence. Must be called
    // again after Flush().
    void PrimeWithSilence();

    // Flush all buffered data and reset internal indices. Note, if
    // PrimeWithSilence() was previously called it must be called again after
    // the Flush().
    void Flush();

    // Update |io_sample_rate_ratio_|. SetRatio() will cause a reconstruction of
    // the kernels used for resampling.
    void SetRatio(double io_sample_rate_ratio);

    // Return number of input frames consumed by a callback but not yet
    // processed. Since input/output ratio can be fractional, so can this value.
    // Zero before first call to Resample().
    double BufferedFrames() const;

    size_t KernelSize() const { return kernel_size_; }

private:
    struct AlignedDelete
    {
        void operator()(float *p) const { ::operator delete[](p, std::align_val_t(32)); }
    };
    using AlignedArray = std::unique_ptr<float[], AlignedDelete>;
    static AlignedArray AllocateAligned(size_t size);

    void InitializeKernel();
    void UpdateRegions(bool second_load);

    // Compute convolution of |k1| and |k2| over |input_ptr|, resultant sums
    // are linearly interpolated using |kernel_interpolation_factor|.
    static float Convolve_C(const float *input_ptr,
                            const float *k1,
                            const float *k2,
                            size_t kernel_size,
                            double kernel_interpolation_factor);
#if defined(__x86_64__) || defined(__i386__)
    static float Convolve_SSE(const float *input_ptr,
                              const float *k1,
                              const float *k2,
                              size_t kernel_size,
                              double kernel_interpolation_factor);
#elif defined(__ARM_NEON)
    static float Convolve_NEON(const float *input_ptr,
                               const float *k1,
                               const float *k2,
                               size_t kernel_size,
                               double kernel_interpolation_factor);
#endif

    const size_t kernel_size_;

    // The ratio of input / output sample rates.
    double io_sample_rate_ratio_;

    // An index on the source input buffer with sub-sample precision. It must
    // be double precision to avoid drift.
    double virtual_source_idx_;

    // The buffer is primed once at the very beginning of processing.
    bool buffer_primed_;

    // Source of data for resampling.
    const ReadCB read_cb_;

    // The size (in samples) to request from each |read_cb_| execution.
    const size_t request_frames_;

    // The number of source frames processed per pass.
    size_t block_size_;

    // Cached value used for ChunkSize(). The maximum size in frames that
    // guarantees Resample() will only ask for input at most once.
    size_t chunk_size_;

    // The size (in samples) of the internal buffer used by the resampler.
    const size_t input_buffer_size_;

    // Contains kKernelOffsetCount kernels back-to-back, each of size
    // |kernel_size_|. The kernel offsets are sub-sample shifts of a windowed
    // sinc shifted from 0.0 to 1.0 sample.
    AlignedArray kernel_storage_;
    AlignedArray kernel_pre_sinc_storage_;
    AlignedArray kernel_window_storage_;

    // Data from the source is copied into this buffer for each processing
    // pass.
    AlignedArray input_buffer_;

    // Pointers to the various regions inside |input_buffer_|. See the diagram
    // at the top of the .cpp file for more information.
    float *r0_;
    float *const r1_;
    float *const r2_;
    float *r3_;
    float *r4_;
};

} // namespace wsola

#endif // MLT_WSOLA_SINC_RESAMPLER_H
