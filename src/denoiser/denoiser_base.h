// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <functional>

namespace denoiser {

class DenoiserBase {
 public:
  explicit DenoiserBase(int audio_sample_rate);
  virtual ~DenoiserBase() = default;

  virtual void Denoise(const std::function<bool(double*)>& get_next_audio_sample,
                       const std::function<void(double)>& set_next_audio_sample) = 0;

 protected:
  int audio_sample_rate_;
};

}  // namespace denoiser
