// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/wav/wav_noise_producer.h"

#include <cmath>
#include <random>

#include "glog/logging.h"

#include "src/common/interface/proto/wav_params.pb.h"
#include "src/wav/wav_reader.h"
#include "src/wav/wav_writer.h"

namespace wav {

void AddWhiteNoise(const std::string &input_filename, const std::string &output_filename, double noise_level) {
  wav::WavReader reader(input_filename);
  auto header = reader.GetWavHeader();

  interface::WavParams wav_params;
  wav_params.set_num_channels(header.num_channels);
  wav_params.set_sample_rate(header.sample_rate);
  wav_params.set_bit_depth(header.bit_depth);

  wav::WavWriter writer(output_filename, wav_params);

  std::normal_distribution<double> distribution(0.0, 1.0);
  std::mt19937 rng(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));

  while (!reader.IsEof()) {
    auto sample = reader.GetSample();
    double noise_0 = distribution(rng) * noise_level;
    double noise_1 = distribution(rng) * noise_level;
    double noisy_sample_0 = std::clamp(sample.first + noise_0, -1.0, 1.0);
    double noisy_sample_1 = std::clamp(sample.second + noise_1, -1.0, 1.0);
    if (header.num_channels == 1) {
      writer.AddSample(noisy_sample_0);
    } else {
      writer.AddSample(noisy_sample_0, noisy_sample_1);
    }
  }
  writer.Write();
}

void AddUniformNoise(const std::string &input_filename, const std::string &output_filename, double noise_level) {
  wav::WavReader reader(input_filename);
  auto header = reader.GetWavHeader();

  interface::WavParams wav_params;
  wav_params.set_num_channels(header.num_channels);
  wav_params.set_sample_rate(header.sample_rate);
  wav_params.set_bit_depth(header.bit_depth);

  wav::WavWriter writer(output_filename, wav_params);

  std::uniform_real_distribution<double> distribution(-1.0, 1.0);
  std::mt19937 rng(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));

  while (!reader.IsEof()) {
    auto sample = reader.GetSample();
    double noise_0 = distribution(rng) * noise_level;
    double noise_1 = distribution(rng) * noise_level;
    double noisy_sample_0 = std::clamp(sample.first + noise_0, -1.0, 1.0);
    double noisy_sample_1 = std::clamp(sample.second + noise_1, -1.0, 1.0);
    if (header.num_channels == 1) {
      writer.AddSample(noisy_sample_0);
    } else {
      writer.AddSample(noisy_sample_0, noisy_sample_1);
    }
  }
  writer.Write();
}

void AddPinkNoise(const std::string &input_filename, const std::string &output_filename, double noise_level) {
  wav::WavReader reader(input_filename);
  auto header = reader.GetWavHeader();

  interface::WavParams wav_params;
  wav_params.set_num_channels(header.num_channels);
  wav_params.set_sample_rate(header.sample_rate);
  wav_params.set_bit_depth(header.bit_depth);

  wav::WavWriter writer(output_filename, wav_params);

  std::normal_distribution<double> white_distribution(0.0, 1.0);
  std::mt19937 rng(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));

  double b0_0 = 0.0, b1_0 = 0.0, b2_0 = 0.0, b3_0 = 0.0, b4_0 = 0.0, b5_0 = 0.0, b6_0 = 0.0;
  double b0_1 = 0.0, b1_1 = 0.0, b2_1 = 0.0, b3_1 = 0.0, b4_1 = 0.0, b5_1 = 0.0, b6_1 = 0.0;

  while (!reader.IsEof()) {
    auto sample = reader.GetSample();
    double white_0 = white_distribution(rng);
    double white_1 = white_distribution(rng);

    b0_0 = 0.99886 * b0_0 + white_0 * 0.0555179;
    b1_0 = 0.99332 * b1_0 + white_0 * 0.0750759;
    b2_0 = 0.96900 * b2_0 + white_0 * 0.1538520;
    b3_0 = 0.86650 * b3_0 + white_0 * 0.3104856;
    b4_0 = 0.55000 * b4_0 + white_0 * 0.5329522;
    b5_0 = -0.7616 * b5_0 - white_0 * 0.0168980;
    double pink_0 = (b0_0 + b1_0 + b2_0 + b3_0 + b4_0 + b5_0 + b6_0 + white_0 * 0.5362) * 0.11;
    b6_0 = white_0 * 0.115926;

    b0_1 = 0.99886 * b0_1 + white_1 * 0.0555179;
    b1_1 = 0.99332 * b1_1 + white_1 * 0.0750759;
    b2_1 = 0.96900 * b2_1 + white_1 * 0.1538520;
    b3_1 = 0.86650 * b3_1 + white_1 * 0.3104856;
    b4_1 = 0.55000 * b4_1 + white_1 * 0.5329522;
    b5_1 = -0.7616 * b5_1 - white_1 * 0.0168980;
    double pink_1 = (b0_1 + b1_1 + b2_1 + b3_1 + b4_1 + b5_1 + b6_1 + white_1 * 0.5362) * 0.11;
    b6_1 = white_1 * 0.115926;

    double noisy_sample_0 = std::clamp(sample.first + pink_0 * noise_level, -1.0, 1.0);
    double noisy_sample_1 = std::clamp(sample.second + pink_1 * noise_level, -1.0, 1.0);
    if (header.num_channels == 1) {
      writer.AddSample(noisy_sample_0);
    } else {
      writer.AddSample(noisy_sample_0, noisy_sample_1);
    }
  }
  writer.Write();
}

void AddBrownNoise(const std::string &input_filename, const std::string &output_filename, double noise_level) {
  wav::WavReader reader(input_filename);
  auto header = reader.GetWavHeader();

  interface::WavParams wav_params;
  wav_params.set_num_channels(header.num_channels);
  wav_params.set_sample_rate(header.sample_rate);
  wav_params.set_bit_depth(header.bit_depth);

  wav::WavWriter writer(output_filename, wav_params);

  std::normal_distribution<double> white_distribution(0.0, 1.0);
  std::mt19937 rng(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));

  double last_0 = 0.0;
  double last_1 = 0.0;

  while (!reader.IsEof()) {
    auto sample = reader.GetSample();
    double white_0 = white_distribution(rng);
    double white_1 = white_distribution(rng);

    last_0 = (last_0 + 0.02 * white_0) / 1.02;
    last_1 = (last_1 + 0.02 * white_1) / 1.02;
    double brown_0 = last_0 * 3.5;
    double brown_1 = last_1 * 3.5;

    double noisy_sample_0 = std::clamp(sample.first + brown_0 * noise_level, -1.0, 1.0);
    double noisy_sample_1 = std::clamp(sample.second + brown_1 * noise_level, -1.0, 1.0);
    if (header.num_channels == 1) {
      writer.AddSample(noisy_sample_0);
    } else {
      writer.AddSample(noisy_sample_0, noisy_sample_1);
    }
  }
  writer.Write();
}

}  // namespace wav