// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <iostream>
#include <memory>
#include <string>

#include "src/audio/audio_capturer.h"
#include "src/common/file/io.h"
#include "src/wav/wav_writer.h"

DEFINE_int32(capture_duration_seconds, 10, "Duration to capture audio in seconds");

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InstallFailureSignalHandler();

  constexpr int kAudioSampleRate = 44100;
  constexpr int kBitDepth = 16;
  constexpr int kNumChannels = 1;

  interface::WavParams wav_params;
  wav_params.set_sample_rate(kAudioSampleRate);
  wav_params.set_bit_depth(kBitDepth);
  wav_params.set_num_channels(kNumChannels);

  std::string temp_folder = io::GenerateTestFolder();
  std::string temp_filename = temp_folder + "/recorded_audio.wav";
  LOG(INFO) << "Recording to: " << temp_filename;

  auto capturer = std::make_unique<audio::AudioCapturer>(kAudioSampleRate);
  if (!capturer->Initialize()) {
    std::cerr << "Failed to initialize audio capturer" << std::endl;
    return 1;
  }

  wav::WavWriter writer(temp_filename, wav_params);

  auto audio_callback = [&writer](const float* samples, size_t num_samples) {
    for (size_t i = 0; i < num_samples; ++i) {
      writer.AddSample(static_cast<double>(samples[i]));
    }
    LOG_EVERY_N(INFO, 100) << "Recorded " << num_samples << " samples";
  };

  std::cout << "Starting recording for " << FLAGS_capture_duration_seconds << " seconds..." << std::endl;

  if (!capturer->StartCapture(audio_callback)) {
    std::cerr << "Failed to start capture" << std::endl;
    return 1;
  }

  sleep(FLAGS_capture_duration_seconds);

  capturer->StopCapture();
  capturer->Close();

  std::cout << "Recording completed! Saved to: " << temp_filename << std::endl;
  return 0;
}