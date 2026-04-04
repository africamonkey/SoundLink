// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "src/common/file/io.h"
#include "src/common/interface/proto/encoder_params.pb.h"
#include "src/encoder/chirp_encoder.h"
#include "src/receiver/receiver.h"

DEFINE_string(encoder_params, "params/encoder_params.txt", "Path to encoder params file");
DEFINE_int32(capture_duration_seconds, 10, "Duration to capture audio in seconds");

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
  auto decoder = std::make_shared<encoder::ChirpEncoder>(kAudioSampleRate, encoder_params);

  receiver::Receiver receiver(kAudioSampleRate, decoder);
  if (!receiver.Initialize()) {
    std::cerr << "Failed to initialize receiver" << std::endl;
    return 1;
  }

  receiver.SetMessageCallback([](const std::string& byte) {
    std::cout << byte << std::flush;
  });

  receiver.SetDataCallback([](const std::vector<char>& data) {
  });

  std::cout << "Starting real-time capture (Ctrl+C to stop)..." << std::endl;

  if (!receiver.StartCapture()) {
    std::cerr << "Failed to start capture" << std::endl;
    return 1;
  }

  std::cout << std::endl;

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}
