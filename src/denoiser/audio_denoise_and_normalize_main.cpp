// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "src/common/file/io.h"
#include "src/denoiser/audio_normalizer.h"
#include "src/denoiser/simple_denoiser.h"
#include "src/wav/wav_reader.h"
#include "src/wav/wav_writer.h"

DEFINE_string(input_wav, "src/denoiser/test_data/noisy_small_volume.wav", "Input WAV file path");
DEFINE_int32(sample_rate, 44100, "Audio sample rate");
DEFINE_int32(window_size, 1024, "Window size for denoiser and normalizer");
DEFINE_double(target_amplitude, 0.9, "Target amplitude for normalizer");
DEFINE_double(noise_floor_estimation_frames, 10, "Noise floor estimation frames for simple_denoiser");

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InstallFailureSignalHandler();

  const std::string input_filename = FLAGS_input_wav;
  const std::string temp_folder = io::GenerateTestFolder();
  const std::string output_filename = temp_folder + "/denoised_and_normalized.wav";

  wav::WavReader reader(input_filename);
  const wav::WavHeader header = reader.GetWavHeader();

  interface::WavParams wav_params;
  wav_params.set_sample_rate(header.sample_rate);
  wav_params.set_bit_depth(header.bit_depth);
  wav_params.set_num_channels(header.num_channels);

  std::vector<double> samples;
  while (!reader.IsEof()) {
    std::pair<double, double> sample = reader.GetSample();
    samples.push_back(sample.first);
  }
  reader.Close();
  LOG(INFO) << "Loaded " << samples.size() << " samples";

  std::vector<double> after_denoiser1;
  {
    std::vector<double> input = samples;
    size_t idx = 0;
    auto get_next = [&input, &idx](double *s) -> bool {
      if (idx >= input.size()) return false;
      *s = input[idx++];
      return true;
    };
    auto set_next = [&after_denoiser1](double s) {
      after_denoiser1.push_back(s);
    };
    denoiser::SimpleDenoiser denoiser(FLAGS_sample_rate, FLAGS_window_size, FLAGS_noise_floor_estimation_frames);
    denoiser.Denoise(get_next, set_next);
  }
  LOG(INFO) << "After denoiser1: " << after_denoiser1.size() << " samples";

  std::vector<double> after_normalizer;
  {
    std::vector<double> input = after_denoiser1;
    size_t idx = 0;
    auto get_next = [&input, &idx](double *s) -> bool {
      if (idx >= input.size()) return false;
      *s = input[idx++];
      return true;
    };
    auto set_next = [&after_normalizer](double s) {
      after_normalizer.push_back(s);
    };
    denoiser::AudioNormalizer normalizer(FLAGS_sample_rate, FLAGS_window_size, FLAGS_target_amplitude);
    normalizer.Normalize(get_next, set_next);
  }
  LOG(INFO) << "After normalizer: " << after_normalizer.size() << " samples";

  std::vector<double> after_denoiser2;
  {
    std::vector<double> input = after_normalizer;
    size_t idx = 0;
    auto get_next = [&input, &idx](double *s) -> bool {
      if (idx >= input.size()) return false;
      *s = input[idx++];
      return true;
    };
    auto set_next = [&after_denoiser2](double s) {
      after_denoiser2.push_back(s);
    };
    denoiser::SimpleDenoiser denoiser(FLAGS_sample_rate, FLAGS_window_size, FLAGS_noise_floor_estimation_frames);
    denoiser.Denoise(get_next, set_next);
  }
  LOG(INFO) << "After denoiser2: " << after_denoiser2.size() << " samples";

  wav::WavWriter writer(output_filename, wav_params);
  for (double sample : after_denoiser2) {
    writer.AddSample(sample);
  }
  writer.Write();
  LOG(INFO) << "Output written to: " << output_filename;

  return 0;
}
