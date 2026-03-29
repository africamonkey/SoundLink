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

}  // namespace wav