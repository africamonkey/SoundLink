// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "src/audio/audio_player.h"
#include "src/encoder/encoder_base.h"

namespace sender {

class Sender {
 public:
  Sender(int audio_sample_rate, std::shared_ptr<encoder::EncoderBase> encoder);
  ~Sender();

  bool Initialize();
  bool SendMessage(const std::string& message);
  bool SendData(const std::vector<char>& data);
  void Close();

 private:
  std::shared_ptr<encoder::EncoderBase> encoder_;
  std::unique_ptr<audio::AudioPlayer> player_;
};

}  // namespace sender
