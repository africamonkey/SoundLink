// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace audio {

class AudioPlayer {
 public:
  AudioPlayer(int sample_rate = 44100, int channels = 1);
  ~AudioPlayer();

  bool Initialize();
  bool Play(const std::vector<double>& samples);
  bool Play(const double* samples, size_t num_samples);
  void Close();

  int sample_rate() const { return sample_rate_; }
  int channels() const { return channels_; }

 private:
  int sample_rate_;
  int channels_;
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace audio
