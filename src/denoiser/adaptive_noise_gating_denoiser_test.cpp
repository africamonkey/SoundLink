// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "adaptive_noise_gating_denoiser.h"

#include <string>
#include <vector>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "gtest/gtest.h"

#include "src/common/file/io.h"
#include "src/common/interface/proto/encoder_params.pb.h"
#include "src/common/interface/proto/wav_params.pb.h"
#include "src/encoder/simple_encoder.h"
#include "src/wav/wav_noise_producer.h"
#include "src/wav/wav_reader.h"
#include "src/wav/wav_writer.h"

namespace denoiser {

TEST(AdaptiveNoiseGatingDenoiserTest, DenoiseWhiteNoise) {
  interface::EncoderParams encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/encoder_params.txt", &encoder_params));
  interface::WavParams wav_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params));
  const int audio_sample_rate = wav_params.sample_rate();
  encoder::SimpleEncoder encoder(audio_sample_rate, std::move(encoder_params));
  const std::string kStringToBeEncoded = "Hello, World!";

  const std::string clean_wav_filename =
      io::GenerateTestFolder() + "/adaptive_denoiser_white_noise_test_clean.wav";
  wav::WavWriter clean_writer(clean_wav_filename, wav_params);
  int encode_current_id = 0;
  std::function get_next_byte = [&kStringToBeEncoded, &encode_current_id](char *byte) -> bool {
    if (encode_current_id >= (int)kStringToBeEncoded.size()) {
      return false;
    }
    *CHECK_NOTNULL(byte) = kStringToBeEncoded[encode_current_id++];
    return true;
  };
  std::function set_next_sample_clean = [&clean_writer](double sample) {
    clean_writer.AddSample(sample);
  };
  encoder.Encode(get_next_byte, set_next_sample_clean);
  clean_writer.Write();

  const std::string noisy_wav_filename =
      io::GenerateTestFolder() + "/adaptive_denoiser_white_noise_test_noisy.wav";
  wav::AddWhiteNoise(clean_wav_filename, noisy_wav_filename, 0.1);

  const std::string denoised_wav_filename = 
      io::GenerateTestFolder() + "/adaptive_denoiser_white_noise_test_denoised.wav";
  wav::WavReader noisy_reader(noisy_wav_filename);
  wav::WavWriter denoised_writer(denoised_wav_filename, wav_params);

  AdaptiveNoiseGatingDenoiser denoiser(audio_sample_rate, 1024, 5, 0.1);
  auto get_next_audio_sample = [&noisy_reader](double *next_sample) -> bool {
    if (noisy_reader.IsEof()) {
      return false;
    }
    std::pair<double, double> sample = noisy_reader.GetSample();
    *next_sample = sample.first;
    return true;
  };
  auto set_next_audio_sample_denoised = [&denoised_writer](double sample) {
    denoised_writer.AddSample(sample);
  };
  denoiser.Denoise(get_next_audio_sample, set_next_audio_sample_denoised);
  denoised_writer.Write();
  noisy_reader.Close();

  wav::WavReader denoised_reader(denoised_wav_filename);
  interface::EncoderParams decoder_encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/encoder_params.txt", &decoder_encoder_params));
  encoder::SimpleEncoder decoder(denoised_reader.GetWavHeader().sample_rate, std::move(decoder_encoder_params));

  std::vector<char> decoded_bytes;
  auto get_next_audio_sample_for_decode = [&denoised_reader](double *next_sample) -> bool {
    if (denoised_reader.IsEof()) {
      return false;
    }
    std::pair<double, double> sample = denoised_reader.GetSample();
    *next_sample = sample.first;
    return true;
  };
  auto set_next_byte = [&decoded_bytes](char byte) {
    decoded_bytes.push_back(byte);
  };
  decoder.Decode(get_next_audio_sample_for_decode, set_next_byte);
  denoised_reader.Close();

  std::string decoded_message(decoded_bytes.begin(), decoded_bytes.end());
  EXPECT_EQ(decoded_message, kStringToBeEncoded);

  io::DeleteFileIfExists(clean_wav_filename);
  io::DeleteFileIfExists(noisy_wav_filename);
  io::DeleteFileIfExists(denoised_wav_filename);
}

TEST(AdaptiveNoiseGatingDenoiserTest, DenoiseUniformNoise) {
  interface::EncoderParams encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/encoder_params.txt", &encoder_params));
  interface::WavParams wav_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params));
  const int audio_sample_rate = wav_params.sample_rate();
  encoder::SimpleEncoder encoder(audio_sample_rate, std::move(encoder_params));
  const std::string kStringToBeEncoded = "Test123";

  const std::string clean_wav_filename = io::GenerateTestFolder() + "/adaptive_denoiser_test_uniform_clean.wav";
  wav::WavWriter clean_writer(clean_wav_filename, wav_params);
  int encode_current_id = 0;
  std::function get_next_byte = [&kStringToBeEncoded, &encode_current_id](char *byte) -> bool {
    if (encode_current_id >= (int)kStringToBeEncoded.size()) {
      return false;
    }
    *CHECK_NOTNULL(byte) = kStringToBeEncoded[encode_current_id++];
    return true;
  };
  std::function set_next_sample_clean = [&clean_writer](double sample) {
    clean_writer.AddSample(sample);
  };
  encoder.Encode(get_next_byte, set_next_sample_clean);
  clean_writer.Write();

  const std::string noisy_wav_filename = io::GenerateTestFolder() + "/adaptive_denoiser_test_uniform_noisy.wav";
  wav::AddUniformNoise(clean_wav_filename, noisy_wav_filename, 0.1);

  const std::string denoised_wav_filename = io::GenerateTestFolder() + "/adaptive_denoiser_test_uniform_denoised.wav";
  wav::WavReader noisy_reader(noisy_wav_filename);
  wav::WavWriter denoised_writer(denoised_wav_filename, wav_params);

  AdaptiveNoiseGatingDenoiser denoiser(audio_sample_rate, 1024, 5, 0.1);
  auto get_next_audio_sample = [&noisy_reader](double *next_sample) -> bool {
    if (noisy_reader.IsEof()) {
      return false;
    }
    std::pair<double, double> sample = noisy_reader.GetSample();
    *next_sample = sample.first;
    return true;
  };
  auto set_next_audio_sample_denoised = [&denoised_writer](double sample) {
    denoised_writer.AddSample(sample);
  };
  denoiser.Denoise(get_next_audio_sample, set_next_audio_sample_denoised);
  denoised_writer.Write();
  noisy_reader.Close();

  wav::WavReader denoised_reader(denoised_wav_filename);
  interface::EncoderParams decoder_encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/encoder_params.txt", &decoder_encoder_params));
  encoder::SimpleEncoder decoder(denoised_reader.GetWavHeader().sample_rate, std::move(decoder_encoder_params));

  std::vector<char> decoded_bytes;
  auto get_next_audio_sample_for_decode = [&denoised_reader](double *next_sample) -> bool {
    if (denoised_reader.IsEof()) {
      return false;
    }
    std::pair<double, double> sample = denoised_reader.GetSample();
    *next_sample = sample.first;
    return true;
  };
  auto set_next_byte = [&decoded_bytes](char byte) {
    decoded_bytes.push_back(byte);
  };
  decoder.Decode(get_next_audio_sample_for_decode, set_next_byte);
  denoised_reader.Close();

  std::string decoded_message(decoded_bytes.begin(), decoded_bytes.end());
  EXPECT_EQ(decoded_message, kStringToBeEncoded);

  io::DeleteFileIfExists(clean_wav_filename);
  io::DeleteFileIfExists(noisy_wav_filename);
  io::DeleteFileIfExists(denoised_wav_filename);
}

TEST(AdaptiveNoiseGatingDenoiserTest, DifferentWindowSizes) {
  interface::EncoderParams encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/encoder_params.txt", &encoder_params));
  interface::WavParams wav_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params));
  const int audio_sample_rate = wav_params.sample_rate();
  encoder::SimpleEncoder encoder(audio_sample_rate, std::move(encoder_params));
  const std::string kStringToBeEncoded = "Window Test";

  const std::string clean_wav_filename =
      io::GenerateTestFolder() + "/adaptive_denoiser_window_size_test_clean.wav";
  wav::WavWriter clean_writer(clean_wav_filename, wav_params);
  int encode_current_id = 0;
  std::function get_next_byte = [&kStringToBeEncoded, &encode_current_id](char *byte) -> bool {
    if (encode_current_id >= (int)kStringToBeEncoded.size()) {
      return false;
    }
    *CHECK_NOTNULL(byte) = kStringToBeEncoded[encode_current_id++];
    return true;
  };
  std::function set_next_sample_clean = [&clean_writer](double sample) {
    clean_writer.AddSample(sample);
  };
  encoder.Encode(get_next_byte, set_next_sample_clean);
  clean_writer.Write();

  const std::string noisy_wav_filename =
      io::GenerateTestFolder() + "/adaptive_denoiser_window_size_test_noisy.wav";
  wav::AddWhiteNoise(clean_wav_filename, noisy_wav_filename, 0.05);

  const std::string denoised_wav_filename =
      io::GenerateTestFolder() + "/adaptive_denoiser_window_size_test_denoised.wav";
  wav::WavReader noisy_reader(noisy_wav_filename);
  wav::WavWriter denoised_writer(denoised_wav_filename, wav_params);

  AdaptiveNoiseGatingDenoiser denoiser(audio_sample_rate, 2048, 5, 0.1);
  auto get_next_audio_sample = [&noisy_reader](double *next_sample) -> bool {
    if (noisy_reader.IsEof()) {
      return false;
    }
    std::pair<double, double> sample = noisy_reader.GetSample();
    *next_sample = sample.first;
    return true;
  };
  auto set_next_audio_sample_denoised = [&denoised_writer](double sample) {
    denoised_writer.AddSample(sample);
  };
  denoiser.Denoise(get_next_audio_sample, set_next_audio_sample_denoised);
  denoised_writer.Write();
  noisy_reader.Close();

  wav::WavReader denoised_reader(denoised_wav_filename);
  interface::EncoderParams decoder_encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/encoder_params.txt", &decoder_encoder_params));
  encoder::SimpleEncoder decoder(denoised_reader.GetWavHeader().sample_rate, std::move(decoder_encoder_params));

  std::vector<char> decoded_bytes;
  auto get_next_audio_sample_for_decode = [&denoised_reader](double *next_sample) -> bool {
    if (denoised_reader.IsEof()) {
      return false;
    }
    std::pair<double, double> sample = denoised_reader.GetSample();
    *next_sample = sample.first;
    return true;
  };
  auto set_next_byte = [&decoded_bytes](char byte) {
    decoded_bytes.push_back(byte);
  };
  decoder.Decode(get_next_audio_sample_for_decode, set_next_byte);
  denoised_reader.Close();

  std::string decoded_message(decoded_bytes.begin(), decoded_bytes.end());
  EXPECT_EQ(decoded_message, kStringToBeEncoded);

  io::DeleteFileIfExists(clean_wav_filename);
  io::DeleteFileIfExists(noisy_wav_filename);
  io::DeleteFileIfExists(denoised_wav_filename);
}

}  // namespace denoiser
