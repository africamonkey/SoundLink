// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/receiver/receiver.h"

#include <algorithm>

#include "glog/logging.h"

namespace receiver {

Receiver::Receiver(int audio_sample_rate, std::shared_ptr<encoder::EncoderBase> decoder)
    : audio_sample_rate_(audio_sample_rate),
      decoder_(decoder),
      capturer_(new audio::AudioCapturer(audio_sample_rate)),
      denoised_buffer_() {}

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
  std::vector<char> decoded_bytes;

  auto audio_callback = [this](const float* samples, size_t num_samples) {
    for (size_t i = 0; i < num_samples; ++i) {
      sample_buffer_.push_back(static_cast<double>(samples[i]));
    }
  };

  capturer_->StartCapture(audio_callback);
  return true;
}

void Receiver::StopCapture() {
  capturer_->StopCapture();

  if (sample_buffer_.empty()) {
    LOG(WARNING) << "No samples captured";
    return;
  }

  denoised_buffer_.clear();
  denoised_buffer_.reserve(sample_buffer_.size());

  denoiser::SimpleDenoiser denoiser(audio_sample_rate_);
  size_t sample_index = 0;

  auto get_next_audio_sample = [this, &sample_index](double* next_sample) -> bool {
    if (sample_index >= sample_buffer_.size()) {
      return false;
    }
    *next_sample = sample_buffer_[sample_index];
    ++sample_index;
    return true;
  };

  auto set_next_audio_sample = [this](double sample) {
    denoised_buffer_.push_back(sample);
  };

  denoiser.Denoise(get_next_audio_sample, set_next_audio_sample);

  LOG(INFO) << "Denoised " << denoised_buffer_.size() << " samples";

  std::vector<char> decoded_bytes;
  sample_index = 0;

  auto get_denoised_sample = [this, &sample_index](double* next_sample) -> bool {
    if (sample_index >= denoised_buffer_.size()) {
      return false;
    }
    *next_sample = denoised_buffer_[sample_index];
    ++sample_index;
    return true;
  };

  auto set_next_byte = [&decoded_bytes](char byte) {
    decoded_bytes.push_back(byte);
  };

  decoder_->Decode(get_denoised_sample, set_next_byte);

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
