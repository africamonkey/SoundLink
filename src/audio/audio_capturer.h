// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "third_party/portaudio/portaudio.h"

namespace audio {

class AudioCapturer {
 public:
  using AudioCallback = std::function<void(const float* samples, size_t num_samples)>;

  AudioCapturer(int sample_rate = 44100, int channels = 1);
  virtual ~AudioCapturer();

  virtual bool Initialize();
  virtual bool StartCapture(AudioCallback callback);
  virtual void StopCapture();
  virtual void Close();

  int sample_rate() const { return sample_rate_; }
  int channels() const { return channels_; }

 protected:
  void SetCallbackForTesting(AudioCallback callback) { impl_->callback = callback; }

 public:
  struct Impl {
    PaStream* stream = nullptr;
    AudioCallback callback;
  };
  int sample_rate_;
  int channels_;
  std::unique_ptr<Impl> impl_;
};

int CaptureCallback(const void* input_buffer, void* output_buffer,
                   unsigned long frames_per_buffer,
                   const PaStreamCallbackTimeInfo* time_info,
                   PaStreamCallbackFlags status_flags, void* user_data);

}  // namespace audio
