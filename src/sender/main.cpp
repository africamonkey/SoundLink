// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <iostream>
#include <memory>
#include <string>

#include "src/common/file/io.h"
#include "src/common/interface/proto/encoder_params.pb.h"
#include "src/encoder/trival_encoder.h"
#include "src/sender/sender.h"

DEFINE_string(encoder_params, "params/encoder_params.txt", "Path to encoder params file");

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InstallFailureSignalHandler();

  interface::EncoderParams encoder_params;
  if (!io::ReadFromProtoInTextFormat(FLAGS_encoder_params, &encoder_params)) {
    std::cerr << "Failed to read encoder params from " << FLAGS_encoder_params << std::endl;
    return 1;
  }

  constexpr int kAudioSampleRate = 44100;
  auto encoder = std::make_shared<encoder::TrivalEncoder>(kAudioSampleRate, encoder_params);

  sender::Sender sender(kAudioSampleRate, encoder);
  if (!sender.Initialize()) {
    std::cerr << "Failed to initialize sender" << std::endl;
    return 1;
  }

  std::string message;
  std::cout << "Enter message to send: ";
  std::getline(std::cin, message);

  if (!sender.SendMessage(message)) {
    std::cerr << "Failed to send message" << std::endl;
    return 1;
  }

  std::cout << "Message sent successfully!" << std::endl;
  return 0;
}
