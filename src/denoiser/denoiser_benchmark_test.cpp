// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "gtest/gtest.h"

#include "src/common/file/io.h"
#include "src/common/interface/proto/encoder_params.pb.h"
#include "src/common/interface/proto/wav_params.pb.h"
#include "src/denoiser/simple_denoiser.h"
#include "src/denoiser/adaptive_noise_gating_denoiser.h"
#include "src/encoder/goertzel_encoder.h"
#include "src/wav/wav_noise_producer.h"
#include "src/wav/wav_reader.h"
#include "src/wav/wav_writer.h"

DEFINE_string(test_string, "Hello, World!", "String to encode for benchmarking");

namespace denoiser {

enum class NoiseType {
  kWhite,
  kUniform,
  kPink,
  kBrown,
};

const std::vector<NoiseType> kNoiseTypes = {
    NoiseType::kWhite, NoiseType::kUniform, NoiseType::kPink, NoiseType::kBrown};

const std::vector<std::string> kNoiseTypeNames = {"White", "Uniform", "Pink", "Brown"};

std::string NoiseTypeToString(NoiseType noise_type) {
  switch (noise_type) {
    case NoiseType::kWhite:
      return "White";
    case NoiseType::kUniform:
      return "Uniform";
    case NoiseType::kPink:
      return "Pink";
    case NoiseType::kBrown:
      return "Brown";
  }
  return "Unknown";
}

void AddNoise(NoiseType noise_type, const std::string& input_filename,
              const std::string& output_filename, double noise_level) {
  switch (noise_type) {
    case NoiseType::kWhite:
      wav::AddWhiteNoise(input_filename, output_filename, noise_level);
      break;
    case NoiseType::kUniform:
      wav::AddUniformNoise(input_filename, output_filename, noise_level);
      break;
    case NoiseType::kPink:
      wav::AddPinkNoise(input_filename, output_filename, noise_level);
      break;
    case NoiseType::kBrown:
      wav::AddBrownNoise(input_filename, output_filename, noise_level);
      break;
  }
}

bool EncodeMessage(const std::string& message, const std::string& output_filename,
                   int audio_sample_rate, const interface::EncoderParams& encoder_params,
                   const interface::WavParams& wav_params) {
  encoder::GoertzelEncoder encoder(audio_sample_rate, std::move(encoder_params));
  wav::WavWriter writer(output_filename, wav_params);
  int current_id = 0;
  auto get_next_byte = [&message, &current_id](char* byte) -> bool {
    if (current_id >= (int)message.size()) {
      return false;
    }
    *CHECK_NOTNULL(byte) = message[current_id++];
    return true;
  };
  auto set_next_sample = [&writer](double sample) { writer.AddSample(sample); };
  encoder.Encode(get_next_byte, set_next_sample);
  writer.Write();
  return true;
}

bool DecodeMessage(const std::string& input_filename, std::string* decoded_message) {
  wav::WavReader reader(input_filename);
  interface::EncoderParams decoder_params;
  if (!io::ReadFromProtoInTextFormat("params/encoder_params.txt", &decoder_params)) {
    return false;
  }
  encoder::GoertzelEncoder decoder(reader.GetWavHeader().sample_rate, std::move(decoder_params));
  std::vector<char> decoded_bytes;
  auto get_next_sample = [&reader](double* next_sample) -> bool {
    if (reader.IsEof()) {
      return false;
    }
    std::pair<double, double> sample = reader.GetSample();
    *next_sample = sample.first;
    return true;
  };
  auto set_next_byte = [&decoded_bytes](char byte) { decoded_bytes.push_back(byte); };
  decoder.Decode(get_next_sample, set_next_byte);
  reader.Close();
  decoded_message->assign(decoded_bytes.begin(), decoded_bytes.end());
  return true;
}

bool TestDenoising(denoiser::DenoiserBase* denoiser, const std::string& noisy_filename,
                   const std::string& denoised_filename, int audio_sample_rate,
                   const interface::WavParams& wav_params) {
  wav::WavReader noisy_reader(noisy_filename);
  wav::WavWriter denoised_writer(denoised_filename, wav_params);
  auto get_next = [&noisy_reader](double* next_sample) -> bool {
    if (noisy_reader.IsEof()) {
      return false;
    }
    std::pair<double, double> sample = noisy_reader.GetSample();
    *next_sample = sample.first;
    return true;
  };
  auto set_next = [&denoised_writer](double sample) { denoised_writer.AddSample(sample); };
  denoiser->Denoise(get_next, set_next);
  denoised_writer.Write();
  noisy_reader.Close();
  return true;
}

double BinarySearchMaxNoiseLevel(NoiseType noise_type, denoiser::DenoiserBase* denoiser,
                                  const std::string& message, int audio_sample_rate,
                                  const interface::EncoderParams& encoder_params,
                                  const interface::WavParams& wav_params) {
  const double kMinNoiseLevel = 0.0;
  const double kMaxNoiseLevel = 1.0;
  const double kPrecision = 0.01;

  const std::string clean_filename = io::GenerateTestFolder() + "/benchmark_clean.wav";
  EncodeMessage(message, clean_filename, audio_sample_rate, encoder_params, wav_params);

  double low = kMinNoiseLevel;
  double high = kMaxNoiseLevel;
  double last_success = 0.0;

  while (high - low > kPrecision) {
    double mid = (low + high) / 2.0;
    const std::string noisy_filename = io::GenerateTestFolder() + "/benchmark_noisy.wav";
    AddNoise(noise_type, clean_filename, noisy_filename, mid);

    const std::string denoised_filename = io::GenerateTestFolder() + "/benchmark_denoised.wav";
    TestDenoising(denoiser, noisy_filename, denoised_filename, audio_sample_rate, wav_params);

    std::string decoded;
    bool success = DecodeMessage(denoised_filename, &decoded);

    io::DeleteFileIfExists(noisy_filename);
    io::DeleteFileIfExists(denoised_filename);

    if (success && decoded == message) {
      last_success = mid;
      low = mid;
    } else {
      high = mid;
    }
  }

  io::DeleteFileIfExists(clean_filename);
  return last_success;
}

class SimpleDenoiserBenchmarkTest : public ::testing::TestWithParam<NoiseType> {
 protected:
  void SetUp() override {
    ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/encoder_params.txt", &encoder_params_));
    ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params_));
    audio_sample_rate_ = wav_params_.sample_rate();
  }

  interface::EncoderParams encoder_params_;
  interface::WavParams wav_params_;
  int audio_sample_rate_;
};

TEST_P(SimpleDenoiserBenchmarkTest, MaxNoiseLevel) {
  NoiseType noise_type = GetParam();
  std::string noise_name = NoiseTypeToString(noise_type);

  SimpleDenoiser denoiser(audio_sample_rate_, 1024, 5);
  double max_noise_level =
      BinarySearchMaxNoiseLevel(noise_type, &denoiser, FLAGS_test_string,
                                audio_sample_rate_, encoder_params_, wav_params_);

  LOG(INFO) << "SimpleDenoiser - " << noise_name << " Noise: Max level = " << max_noise_level;
}

INSTANTIATE_TEST_SUITE_P(SimpleDenoiserBenchmark, SimpleDenoiserBenchmarkTest,
                         ::testing::ValuesIn(kNoiseTypes),
                         [](const testing::TestParamInfo<NoiseType>& info) {
                           return NoiseTypeToString(info.param);
                         });

class AdaptiveNoiseGatingDenoiserBenchmarkTest : public ::testing::TestWithParam<NoiseType> {
 protected:
  void SetUp() override {
    ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/encoder_params.txt", &encoder_params_));
    ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params_));
    audio_sample_rate_ = wav_params_.sample_rate();
  }

  interface::EncoderParams encoder_params_;
  interface::WavParams wav_params_;
  int audio_sample_rate_;
};

TEST_P(AdaptiveNoiseGatingDenoiserBenchmarkTest, MaxNoiseLevel) {
  NoiseType noise_type = GetParam();
  std::string noise_name = NoiseTypeToString(noise_type);

  AdaptiveNoiseGatingDenoiser denoiser(audio_sample_rate_, 1024, 10, 0.1, 10.0, 50.0);
  double max_noise_level =
      BinarySearchMaxNoiseLevel(noise_type, &denoiser, FLAGS_test_string,
                                audio_sample_rate_, encoder_params_, wav_params_);

  LOG(INFO) << "AdaptiveNoiseGatingDenoiser - " << noise_name
            << " Noise: Max level = " << max_noise_level;
}

INSTANTIATE_TEST_SUITE_P(AdaptiveNoiseGatingDenoiserBenchmark,
                         AdaptiveNoiseGatingDenoiserBenchmarkTest, ::testing::ValuesIn(kNoiseTypes),
                         [](const testing::TestParamInfo<NoiseType>& info) {
                           return NoiseTypeToString(info.param);
                         });

}  // namespace denoiser