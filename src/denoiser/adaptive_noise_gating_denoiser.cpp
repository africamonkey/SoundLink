// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "adaptive_noise_gating_denoiser.h"

#include <algorithm>
#include <cmath>

#include "glog/logging.h"

namespace denoiser {

AdaptiveNoiseGatingDenoiser::AdaptiveNoiseGatingDenoiser(int audio_sample_rate,
                                                         int window_size,
                                                         int noise_estimation_frames,
                                                         double attenuation_factor,
                                                         double noise_floor_percentile,
                                                         double signal_floor_percentile)
    : DenoiserBase(audio_sample_rate),
      window_size_(window_size),
      noise_estimation_frames_(noise_estimation_frames),
      attenuation_factor_(attenuation_factor),
      noise_floor_percentile_(noise_floor_percentile),
      signal_floor_percentile_(signal_floor_percentile),
      noise_floor_(0.0),
      signal_median_(0.0),
      frame_count_(0),
      smoothed_sample_(0.0) {
  CHECK_GT(window_size_, 0);
  CHECK_GT(noise_estimation_frames_, 0);
  CHECK_GE(attenuation_factor_, 0.0);
  CHECK_LE(attenuation_factor_, 1.0);
}

double AdaptiveNoiseGatingDenoiser::EstimatePercentile(const std::deque<double>& sorted_data,
                                                      double percentile) const {
  if (sorted_data.empty()) {
    return 0.0;
  }
  double index = (percentile / 100.0) * (sorted_data.size() - 1);
  int lower = static_cast<int>(std::floor(index));
  int upper = static_cast<int>(std::ceil(index));
  if (lower == upper) {
    return sorted_data[lower];
  }
  double fraction = index - lower;
  return sorted_data[lower] * (1.0 - fraction) + sorted_data[upper] * fraction;
}

double AdaptiveNoiseGatingDenoiser::ComputeLocalSNR(const std::deque<double>& window,
                                                    double noise_floor) const {
  double signal_level = EstimatePercentile(window, signal_floor_percentile_);
  double snr_linear = (signal_level + 1e-10) / (noise_floor + 1e-10);
  return 10.0 * std::log10(snr_linear + 1e-10);
}

double AdaptiveNoiseGatingDenoiser::ApplyAdaptiveSmoothing(double sample, double snr) const {
  double attenuation = 1.0;
  if (snr < 0) {
    attenuation = std::pow(attenuation_factor_, -snr / 20.0);
  } else if (snr < 10) {
    double t = snr / 10.0;
    attenuation = 1.0 - (1.0 - attenuation_factor_) * (1.0 - t);
  }
  return sample * attenuation;
}

void AdaptiveNoiseGatingDenoiser::Denoise(const std::function<bool(double*)>& get_next_audio_sample,
                                          const std::function<void(double)>& set_next_audio_sample) {
  double next_sample = 0.0;
  constexpr double kMinSmoothingFactor = 0.05;
  constexpr double kMaxSmoothingFactor = 0.3;

  while (get_next_audio_sample(&next_sample)) {
    amplitude_window_.push_back(std::abs(next_sample));
    if ((int)amplitude_window_.size() > window_size_) {
      amplitude_window_.pop_front();
    }

    double current_smoothing_factor = kMinSmoothingFactor;
    double output_sample = next_sample;

    if (frame_count_ < noise_estimation_frames_ * window_size_) {
      noise_floor_ = EstimatePercentile(amplitude_window_, noise_floor_percentile_);
      smoothed_sample_ = (1 - kMinSmoothingFactor) * smoothed_sample_ + kMinSmoothingFactor * next_sample;
      output_sample = smoothed_sample_;
    } else {
      double local_snr = ComputeLocalSNR(amplitude_window_, noise_floor_);
      current_smoothing_factor = kMinSmoothingFactor + (kMaxSmoothingFactor - kMinSmoothingFactor) *
                                std::max(0.0, std::min(1.0, (local_snr + 10) / 20.0));

      if (local_snr < 0) {
        output_sample = ApplyAdaptiveSmoothing(next_sample, local_snr);
        smoothed_sample_ = (1 - current_smoothing_factor) * smoothed_sample_ +
                          current_smoothing_factor * output_sample;
        output_sample = smoothed_sample_;
      } else {
        smoothed_sample_ = (1 - current_smoothing_factor) * smoothed_sample_ +
                          current_smoothing_factor * next_sample;
        output_sample = smoothed_sample_;
      }
    }

    output_sample = std::max(-1.0, std::min(1.0, output_sample));
    set_next_audio_sample(output_sample);
    ++frame_count_;
  }
}

}  // namespace denoiser
