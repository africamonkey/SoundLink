// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <deque>
#include <functional>
#include <vector>

#include "src/denoiser/denoiser_base.h"

namespace denoiser {

class AdaptiveNoiseGatingDenoiser final : public DenoiserBase {
 public:
  AdaptiveNoiseGatingDenoiser(int audio_sample_rate,
                              int window_size = 1024,
                              int noise_estimation_frames = 10,
                              double attenuation_factor = 0.1,
                              double noise_floor_percentile = 10.0,
                              double signal_floor_percentile = 50.0);

  void Denoise(const std::function<bool(double*)>& get_next_audio_sample,
               const std::function<void(double)>& set_next_audio_sample) override;

 private:
  double EstimatePercentile(const std::deque<double>& sorted_data, double percentile) const;
  double ComputeLocalSNR(const std::deque<double>& window, double noise_floor) const;
  double ApplyAdaptiveSmoothing(double sample, double snr) const;

  int window_size_;
  int noise_estimation_frames_;
  double attenuation_factor_;
  double noise_floor_percentile_;
  double signal_floor_percentile_;

  std::deque<double> amplitude_window_;
  double noise_floor_;
  double signal_median_;
  int frame_count_;
  double smoothed_sample_;
};

}  // namespace denoiser
