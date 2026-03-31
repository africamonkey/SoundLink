// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/receiver/receiver.h"

#include <algorithm>

#include "glog/logging.h"

namespace receiver {

Receiver::Receiver(int audio_sample_rate, std::shared_ptr<encoder::EncoderBase> decoder)
    : decoder_(decoder),
      capturer_(new audio::AudioCapturer(audio_sample_rate)) {}

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
    for (size_t i = 0; i < num_samples; ++i) {
      sample_buffer_.push_back(static_cast<double>(samples[i]));
    }
    LOG_EVERY_N(INFO, 100) << "Captured " << num_samples << " samples, buffer size: " << sample_buffer_.size();
  };

  LOG(INFO) << "Starting audio capture...";
  capturer_->StartCapture(audio_callback);
  return true;
}

void Receiver::StopCapture() {
  capturer_->StopCapture();

  LOG(INFO) << "Capture stopped. Total samples captured: " << sample_buffer_.size();

  if (sample_buffer_.empty()) {
    LOG(WARNING) << "No samples captured";
    return;
  }

  double min_sample = *std::min_element(sample_buffer_.begin(), sample_buffer_.end());
  double max_sample = *std::max_element(sample_buffer_.begin(), sample_buffer_.end());
  LOG(INFO) << "Sample range: [" << min_sample << ", " << max_sample << "]";

  std::vector<char> decoded_bytes;
  size_t sample_index = 0;

  auto get_next_audio_sample = [this, &sample_index](double* next_sample) -> bool {
    if (sample_index >= sample_buffer_.size()) {
      return false;
    }
    *next_sample = sample_buffer_[sample_index];
    ++sample_index;
    return true;
  };

  auto set_next_byte = [&decoded_bytes](char byte) {
    decoded_bytes.push_back(byte);
  };

  decoder_->Decode(get_next_audio_sample, set_next_byte);

  LOG(INFO) << "Decoded " << decoded_bytes.size() << " bytes";

  if (data_callback_) {
    data_callback_(decoded_bytes);
  }

  std::string message(decoded_bytes.begin(), decoded_bytes.end());
  if (message_callback_) {
    message_callback_(message);
  }
}

void Receiver::Close() {
  if (capturer_) {
    capturer_->Close();
  }
}

}  // namespace receiver
