// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <deque>
#include <functional>

#include "src/denoiser/denoiser_base.h"

namespace denoiser {

class SimpleDenoiser final : public DenoiserBase {
 public:
  SimpleDenoiser(int audio_sample_rate, int window_size = 1024, double noise_floor_estimation_frames = 10);

  void Denoise(const std::function<bool(double*)>& get_next_audio_sample,
               const std::function<void(double)>& set_next_audio_sample) override;

 private:
  double EstimateNoiseFloor(const std::deque<double>& window) const;

  int window_size_;
  int noise_floor_estimation_frames_;
  std::deque<double> amplitude_window_;
  double noise_floor_ = 0.0;
  int frame_count_ = 0;
};

}  // namespace denoiser
