// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/receiver/receiver.h"

#include <algorithm>
#include <chrono>

#include "glog/logging.h"

namespace receiver {

Receiver::Receiver(int audio_sample_rate, std::shared_ptr<encoder::EncoderBase> decoder)
    : decoder_(decoder),
      capturer_(std::make_unique<audio::AudioCapturer>(audio_sample_rate)),
      stop_decode_(false),
      buffer_has_data_(false) {}

Receiver::Receiver(int audio_sample_rate, std::shared_ptr<encoder::EncoderBase> decoder,
                   std::unique_ptr<audio::AudioCapturer> capturer)
    : decoder_(decoder),
      capturer_(std::move(capturer)),
      stop_decode_(false),
      buffer_has_data_(false) {}

Receiver::~Receiver() { Close(); }

bool Receiver::Initialize() {
  if (!capturer_->Initialize()) {
    LOG(ERROR) << "Failed to initialize audio capturer";
    return false;
  }
  return true;
}

void Receiver::SetMessageCallback(MessageCallback callback) {
  message_callback_ = callback;
}

void Receiver::SetDataCallback(DataCallback callback) {
  data_callback_ = callback;
}

bool Receiver::StartCapture() {
  auto audio_callback = [this](const float* samples, size_t num_samples) {
    {
      std::lock_guard<std::mutex> lock(buffer_mutex_);
      for (size_t i = 0; i < num_samples; ++i) {
        sample_buffer_.push_back(static_cast<double>(samples[i]));
      }
      buffer_has_data_ = true;
    }
    buffer_cv_.notify_one();
    LOG_EVERY_N(INFO, 100) << "Captured " << num_samples << " samples, buffer size: " << sample_buffer_.size();
  };

  LOG(INFO) << "Starting audio capture...";

  stop_decode_ = false;
  buffer_has_data_ = false;
  decode_thread_ = std::thread(&Receiver::DecodeLoop, this);

  capturer_->StartCapture(audio_callback);
  return true;
}

void Receiver::StopCapture() {
  capturer_->StopCapture();

  LOG(INFO) << "Capture stopped.";

  {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    stop_decode_ = true;
  }
  buffer_cv_.notify_one();

  if (decode_thread_.joinable()) {
    decode_thread_.join();
  }
}

void Receiver::DecodeLoop() {
  std::deque<double> local_buffer;
  size_t decoded_samples = 0;
  auto get_sample = [this, &local_buffer, &decoded_samples](double* next_sample) -> bool {
    if (local_buffer.empty()) {
      std::unique_lock<std::mutex> lock(buffer_mutex_);
      while (!stop_decode_ && !buffer_has_data_) {
        buffer_cv_.wait(lock);
      }
      if (stop_decode_ && local_buffer.empty()) {
        return false;
      }
      local_buffer.insert(local_buffer.end(), sample_buffer_.begin(), sample_buffer_.end());
      sample_buffer_.clear();
      buffer_has_data_ = false;
    }

    *next_sample = local_buffer.front();
    ++decoded_samples;
    LOG_EVERY_N(INFO, 10000) << "num_decoded_samples=" << decoded_samples << ", "
                             << "local_buffer_size=" << local_buffer.size() << ".";
    local_buffer.pop_front();
    return true;
  };

  auto set_next_byte = [this](char byte) {
    if (message_callback_) {
      message_callback_(std::string(1, byte));
    }
  };
  decoder_->Decode(get_sample, set_next_byte);

  LOG(INFO) << "Decode thread exiting";
}

void Receiver::Close() {
  {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    stop_decode_ = true;
  }
  buffer_cv_.notify_one();
  if (decode_thread_.joinable()) {
    decode_thread_.join();
  }
  if (capturer_) {
    capturer_->Close();
  }
}

}  // namespace receiver
