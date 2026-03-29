// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "simple_denoiser.h"

#include <algorithm>
#include <cmath>

#include "glog/logging.h"

namespace denoiser {

SimpleDenoiser::SimpleDenoiser(int audio_sample_rate, int window_size, double noise_floor_estimation_frames)
    : DenoiserBase(audio_sample_rate),
      window_size_(window_size),
      noise_floor_estimation_frames_(noise_floor_estimation_frames) {
  CHECK_GT(window_size_, 0);
  CHECK_GT(noise_floor_estimation_frames_, 0);
}

void SimpleDenoiser::Denoise(const std::function<bool(double*)>& get_next_audio_sample,
                             const std::function<void(double)>& set_next_audio_sample) {
  double next_sample = 0.0;
  double smoothed_sample = 0.0;
  constexpr double kSmoothingFactor = 0.1;

  while (get_next_audio_sample(&next_sample)) {
    amplitude_window_.push_back(std::abs(next_sample));
    if ((int)amplitude_window_.size() > window_size_) {
      amplitude_window_.pop_front();
    }

    smoothed_sample = (1 - kSmoothingFactor) * smoothed_sample + kSmoothingFactor * next_sample;

    if (frame_count_ < noise_floor_estimation_frames_ * window_size_) {
      noise_floor_ = EstimateNoiseFloor(amplitude_window_);
      set_next_audio_sample(next_sample);
    } else {
      if (std::abs(next_sample) > noise_floor_) {
        set_next_audio_sample(next_sample);
      } else {
        set_next_audio_sample(smoothed_sample);
      }
    }
    ++frame_count_;
  }
}

double SimpleDenoiser::EstimateNoiseFloor(const std::deque<double>& window) const {
  if (window.empty()) {
    return 0.0;
  }
  std::deque<double> sorted_window = window;
  std::sort(sorted_window.begin(), sorted_window.end());
  return sorted_window[static_cast<size_t>(sorted_window.size() * 0.1)];
}

}  // namespace denoiser
