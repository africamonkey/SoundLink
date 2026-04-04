// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "src/audio/audio_capturer.h"
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
  std::shared_ptr<encoder::EncoderBase> decoder_;
  std::unique_ptr<audio::AudioCapturer> capturer_;
  std::vector<double> sample_buffer_;
  std::deque<double> local_buffer_;
  MessageCallback message_callback_;
  DataCallback data_callback_;
  std::thread decode_thread_;
  std::mutex buffer_mutex_;
  std::condition_variable buffer_cv_;
  bool stop_decode_ = false;
  bool buffer_has_data_ = false;
  std::atomic<int> samples_count_{0};
  std::atomic<bool> decoding_in_progress_{false};
  std::mutex samples_mutex_;
  std::condition_variable samples_cv_;
  void DecodeLoop();
};

}  // namespace receiver
