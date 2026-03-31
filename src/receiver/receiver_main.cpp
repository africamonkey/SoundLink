// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <iostream>
#include <memory>
#include <string>

#include "src/common/file/io.h"
#include "src/common/interface/proto/encoder_params.pb.h"
#include "src/encoder/goertzel_encoder.h"
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

  if (encoder_params.has_goertzel_encoder_params()) {
    const auto& p = encoder_params.goertzel_encoder_params();
    LOG(INFO) << "Decoder using GoertzelEncoder params:";
    LOG(INFO) << "  encoder_rate: " << p.encoder_rate();
    LOG(INFO) << "  bit_0 freq: " << p.encode_frequency_for_bit_0();
    LOG(INFO) << "  bit_1 freq: " << p.encode_frequency_for_bit_1();
    LOG(INFO) << "  rest freq: " << p.encode_frequency_for_rest();
    LOG(INFO) << "  min amplitude: " << p.minimum_absolute_amplitude();
  }

  constexpr int kAudioSampleRate = 44100;
  auto decoder = std::make_shared<encoder::GoertzelEncoder>(kAudioSampleRate, encoder_params);

  receiver::Receiver receiver(kAudioSampleRate, decoder);
  if (!receiver.Initialize()) {
    std::cerr << "Failed to initialize receiver" << std::endl;
    return 1;
  }

  receiver.SetMessageCallback([](const std::string& message) {
    std::cout << "Received message: " << message << std::endl;
  });

  receiver.SetDataCallback([](const std::vector<char>& data) {
    std::cout << "Received " << data.size() << " bytes of data" << std::endl;
  });

  std::cout << "Starting capture for " << FLAGS_capture_duration_seconds << " seconds..." << std::endl;

  if (!receiver.StartCapture()) {
    std::cerr << "Failed to start capture" << std::endl;
    return 1;
  }

  sleep(FLAGS_capture_duration_seconds);

  receiver.StopCapture();
  receiver.Close();

  std::cout << "Capture completed!" << std::endl;
  return 0;
}
