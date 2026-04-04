// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/sender/sender.h"

#include <algorithm>

#include "glog/logging.h"

namespace sender {

Sender::Sender(int audio_sample_rate, std::shared_ptr<encoder::EncoderBase> encoder)
    : encoder_(encoder), player_(new audio::AudioPlayer(audio_sample_rate)) {}

Sender::~Sender() { Close(); }

bool Sender::Initialize() {
  if (!player_->Initialize()) {
    LOG(ERROR) << "Failed to initialize audio player";
    return false;
  }
  return true;
}

bool Sender::SendMessage(const std::string& message) {
  std::vector<char> data(message.begin(), message.end());
  data.push_back('\0');
  return SendData(data);
}

bool Sender::SendData(const std::vector<char>& data) {
  std::vector<double> audio_samples;
  size_t data_index = 0;

  auto get_next_byte = [&data, &data_index](char* next_byte) -> bool {
    if (data_index >= data.size()) {
      return false;
    }
    *next_byte = data[data_index];
    ++data_index;
    return true;
  };

  auto set_next_audio_sample = [&audio_samples](double sample) {
    audio_samples.push_back(sample);
  };

  encoder_->Encode(get_next_byte, set_next_audio_sample);

  LOG(INFO) << "Encoded " << data.size() << " bytes into " << audio_samples.size() << " samples";

  return player_->Play(audio_samples);
}

void Sender::Close() {
  if (player_) {
    player_->Close();
  }
}

}  // namespace sender
