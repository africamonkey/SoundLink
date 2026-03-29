// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/wav/wav_noise_producer.h"

#include <cmath>

#include "gtest/gtest.h"

#include "src/common/file/io.h"
#include "src/common/interface/proto/wav_params.pb.h"
#include "src/wav/wav_reader.h"
#include "src/wav/wav_writer.h"

namespace wav {

TEST(AudioNoiseTest, AddWhiteNoiseCustomFile) {
  interface::WavParams wav_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params));
  wav_params.set_num_channels(1);
  wav_params.set_bit_depth(16);

  const std::string input_filename = "src/wav/test_data/trival_encoder_sample.wav";
  const std::string output_filename = 
      io::GenerateTestFolder() + "/trival_encoder_sample_noise_output.wav";
  LOG(ERROR) << output_filename;

  AddWhiteNoise(input_filename, output_filename, 0.1);
  // io::DeleteFileIfExists(output_filename);
}

TEST(AudioNoiseTest, AddWhiteNoiseSingleChannel) {
  interface::WavParams wav_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params));
  wav_params.set_num_channels(1);
  wav_params.set_bit_depth(16);

  const std::string input_filename = io::GenerateTestFolder() + "/noise_input.wav";
  const std::string output_filename = io::GenerateTestFolder() + "/noise_output.wav";

  wav::WavWriter writer(input_filename, wav_params);
  const int sample_count = wav_params.sample_rate() * 1;
  for (int i = 0; i < sample_count; ++i) {
    writer.AddSample(0.5);
  }
  writer.Write();

  AddWhiteNoise(input_filename, output_filename, 0.1);

  wav::WavReader reader(output_filename);
  int count = 0;
  double sum_squared = 0.0;
  while (!reader.IsEof()) {
    auto sample = reader.GetSample();
    double diff = sample.first - 0.5;
    sum_squared += diff * diff;
    ++count;
  }
  double rms = std::sqrt(sum_squared / count);
  EXPECT_GT(rms, 0.05);
  EXPECT_LT(rms, 0.2);

  io::DeleteFileIfExists(input_filename);
  io::DeleteFileIfExists(output_filename);
}

TEST(AudioNoiseTest, AddWhiteNoiseTwoChannel) {
  interface::WavParams wav_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params));
  wav_params.set_num_channels(2);
  wav_params.set_bit_depth(16);

  const std::string input_filename = io::GenerateTestFolder() + "/noise_input_stereo.wav";
  const std::string output_filename = io::GenerateTestFolder() + "/noise_output_stereo.wav";

  wav::WavWriter writer(input_filename, wav_params);
  const int sample_count = wav_params.sample_rate() * 1;
  for (int i = 0; i < sample_count; ++i) {
    writer.AddSample(0.3, 0.7);
  }
  writer.Write();

  AddWhiteNoise(input_filename, output_filename, 0.1);

  wav::WavReader reader(output_filename);
  int count = 0;
  double sum_squared_0 = 0.0;
  double sum_squared_1 = 0.0;
  while (!reader.IsEof()) {
    auto sample = reader.GetSample();
    double diff_0 = sample.first - 0.3;
    double diff_1 = sample.second - 0.7;
    sum_squared_0 += diff_0 * diff_0;
    sum_squared_1 += diff_1 * diff_1;
    ++count;
  }
  double rms_0 = std::sqrt(sum_squared_0 / count);
  double rms_1 = std::sqrt(sum_squared_1 / count);
  EXPECT_GT(rms_0, 0.05);
  EXPECT_LT(rms_0, 0.2);
  EXPECT_GT(rms_1, 0.05);
  EXPECT_LT(rms_1, 0.2);

  io::DeleteFileIfExists(input_filename);
  io::DeleteFileIfExists(output_filename);
}

}  // namespace wav