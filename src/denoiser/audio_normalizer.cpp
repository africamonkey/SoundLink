// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "audio_normalizer.h"

#include <algorithm>
#include <cmath>

#include "glog/logging.h"

namespace denoiser {

AudioNormalizer::AudioNormalizer(int audio_sample_rate, int window_size, double target_amplitude)
    : window_size_(window_size), target_amplitude_(target_amplitude) {
  CHECK_GT(window_size_, 0);
  CHECK_GT(target_amplitude_, 0);
  CHECK_LE(target_amplitude_, 1.0);
}

void AudioNormalizer::Normalize(const std::function<bool(double*)>& get_next_audio_sample,
                                 const std::function<void(double)>& set_next_audio_sample) {
  double next_sample = 0.0;
  constexpr double kDecayFactor = 0.9995;

  while (get_next_audio_sample(&next_sample)) {
    double abs_sample = std::abs(next_sample);
    amplitude_window_.push_back(abs_sample);
    if ((int)amplitude_window_.size() > window_size_) {
      amplitude_window_.pop_front();
    }

    double window_max = *std::max_element(amplitude_window_.begin(), amplitude_window_.end());
    max_amplitude_ = std::max(window_max, max_amplitude_ * kDecayFactor);

    double normalized_sample = next_sample;
    if (max_amplitude_ > 1e-6) {
      normalized_sample = next_sample * (target_amplitude_ / max_amplitude_);
      normalized_sample = std::clamp(normalized_sample, -1.0, 1.0);
    }
    set_next_audio_sample(normalized_sample);
  }
}

}  // namespace denoiser
