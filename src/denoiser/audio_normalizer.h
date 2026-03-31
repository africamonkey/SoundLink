// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <deque>
#include <functional>

namespace denoiser {

class AudioNormalizer final {
 public:
  AudioNormalizer(int audio_sample_rate, int window_size = 1024, double target_amplitude = 0.9);

  void Normalize(const std::function<bool(double*)>& get_next_audio_sample,
                 const std::function<void(double)>& set_next_audio_sample);

 private:
  int window_size_;
  double target_amplitude_;
  std::deque<double> amplitude_window_;
  double max_amplitude_ = 0.0;
};

}  // namespace denoiser
