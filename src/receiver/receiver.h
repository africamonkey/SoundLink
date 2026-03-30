// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "src/audio/audio_capturer.h"
#include "src/denoiser/simple_denoiser.h"
#include "src/encoder/encoder_base.h"

namespace receiver {

class Receiver {
 public:
  using MessageCallback = std::function<void(const std::string&)>;
  using DataCallback = std::function<void(const std::vector<char>&)>;

  Receiver(int audio_sample_rate, std::shared_ptr<encoder::EncoderBase> decoder);
  ~Receiver();

  bool Initialize();
  void SetMessageCallback(MessageCallback callback);
  void SetDataCallback(DataCallback callback);
  bool StartCapture();
  void StopCapture();
  void Close();

 private:
  int audio_sample_rate_;
  std::shared_ptr<encoder::EncoderBase> decoder_;
  std::unique_ptr<audio::AudioCapturer> capturer_;
  std::vector<double> sample_buffer_;
  std::vector<double> denoised_buffer_;
  MessageCallback message_callback_;
  DataCallback data_callback_;
};

}  // namespace receiver
