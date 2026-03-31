// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "audio_normalizer.h"

#include <string>
#include <vector>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "gtest/gtest.h"

#include "src/common/file/io.h"
#include "src/common/interface/proto/encoder_params.pb.h"
#include "src/common/interface/proto/wav_params.pb.h"
#include "src/encoder/simple_encoder.h"
#include "src/wav/wav_reader.h"
#include "src/wav/wav_writer.h"

namespace denoiser {

TEST(AudioNormalizerTest, NormalizeQuietSignal) {
  interface::EncoderParams encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/encoder_params.txt", &encoder_params));
  interface::WavParams wav_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params));
  const int audio_sample_rate = wav_params.sample_rate();
  encoder::SimpleEncoder encoder(audio_sample_rate, std::move(encoder_params));
  const std::string kStringToBeEncoded = "Hello, World!";

  const std::string quiet_wav_filename =
      io::GenerateTestFolder() + "/normalizer_quiet_test.wav";
  wav::WavWriter quiet_writer(quiet_wav_filename, wav_params);
  int encode_current_id = 0;
  std::function get_next_byte = [&kStringToBeEncoded, &encode_current_id](char *byte) -> bool {
    if (encode_current_id >= (int)kStringToBeEncoded.size()) {
      return false;
    }
    *CHECK_NOTNULL(byte) = kStringToBeEncoded[encode_current_id++];
    return true;
  };
  std::function set_next_sample_quiet = [&quiet_writer](double sample) {
    quiet_writer.AddSample(sample * 0.1);
  };
  encoder.Encode(get_next_byte, set_next_sample_quiet);
  quiet_writer.Write();

  const std::string normalized_wav_filename =
      io::GenerateTestFolder() + "/normalizer_quiet_test_normalized.wav";
  wav::WavReader quiet_reader(quiet_wav_filename);
  wav::WavWriter normalized_writer(normalized_wav_filename, wav_params);

  AudioNormalizer normalizer(audio_sample_rate, 1024, 0.9);
  auto get_next_audio_sample = [&quiet_reader](double *next_sample) -> bool {
    if (quiet_reader.IsEof()) {
      return false;
    }
    std::pair<double, double> sample = quiet_reader.GetSample();
    *next_sample = sample.first;
    return true;
  };
  auto set_next_audio_sample_normalized = [&normalized_writer](double sample) {
    normalized_writer.AddSample(sample);
  };
  normalizer.Normalize(get_next_audio_sample, set_next_audio_sample_normalized);
  normalized_writer.Write();
  quiet_reader.Close();

  wav::WavReader normalized_reader(normalized_wav_filename);
  interface::EncoderParams decoder_encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/encoder_params.txt", &decoder_encoder_params));
  encoder::SimpleEncoder decoder(normalized_reader.GetWavHeader().sample_rate, std::move(decoder_encoder_params));

  std::vector<char> decoded_bytes;
  auto get_next_audio_sample_for_decode = [&normalized_reader](double *next_sample) -> bool {
    if (normalized_reader.IsEof()) {
      return false;
    }
    std::pair<double, double> sample = normalized_reader.GetSample();
    *next_sample = sample.first;
    return true;
  };
  auto set_next_byte = [&decoded_bytes](char byte) {
    decoded_bytes.push_back(byte);
  };
  decoder.Decode(get_next_audio_sample_for_decode, set_next_byte);
  normalized_reader.Close();

  std::string decoded_message(decoded_bytes.begin(), decoded_bytes.end());
  EXPECT_EQ(decoded_message, kStringToBeEncoded);

  io::DeleteFileIfExists(quiet_wav_filename);
  io::DeleteFileIfExists(normalized_wav_filename);
}

TEST(AudioNormalizerTest, NormalizeAmplitudeNotExceedOne) {
  interface::EncoderParams encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/encoder_params.txt", &encoder_params));
  interface::WavParams wav_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params));
  const int audio_sample_rate = wav_params.sample_rate();
  encoder::SimpleEncoder encoder(audio_sample_rate, std::move(encoder_params));
  const std::string kStringToBeEncoded = "TestSignal";

  const std::string loud_wav_filename =
      io::GenerateTestFolder() + "/normalizer_loud_test.wav";
  wav::WavWriter loud_writer(loud_wav_filename, wav_params);
  int encode_current_id = 0;
  std::function get_next_byte = [&kStringToBeEncoded, &encode_current_id](char *byte) -> bool {
    if (encode_current_id >= (int)kStringToBeEncoded.size()) {
      return false;
    }
    *CHECK_NOTNULL(byte) = kStringToBeEncoded[encode_current_id++];
    return true;
  };
  std::function set_next_sample_loud = [&loud_writer](double sample) {
    loud_writer.AddSample(sample);
  };
  encoder.Encode(get_next_byte, set_next_sample_loud);
  loud_writer.Write();

  const std::string normalized_wav_filename =
      io::GenerateTestFolder() + "/normalizer_loud_test_normalized.wav";
  wav::WavReader loud_reader(loud_wav_filename);
  wav::WavWriter normalized_writer(normalized_wav_filename, wav_params);

  AudioNormalizer normalizer(audio_sample_rate, 1024, 0.9);
  auto get_next_audio_sample = [&loud_reader](double *next_sample) -> bool {
    if (loud_reader.IsEof()) {
      return false;
    }
    std::pair<double, double> sample = loud_reader.GetSample();
    *next_sample = sample.first;
    return true;
  };
  auto set_next_audio_sample_normalized = [&normalized_writer](double sample) {
    normalized_writer.AddSample(sample);
  };
  normalizer.Normalize(get_next_audio_sample, set_next_audio_sample_normalized);
  normalized_writer.Write();
  loud_reader.Close();

  wav::WavReader normalized_reader(normalized_wav_filename);
  double max_amplitude = 0.0;
  while (!normalized_reader.IsEof()) {
    std::pair<double, double> sample = normalized_reader.GetSample();
    max_amplitude = std::max(max_amplitude, std::abs(sample.first));
  }
  normalized_reader.Close();

  EXPECT_LE(max_amplitude, 0.95);

  io::DeleteFileIfExists(loud_wav_filename);
  io::DeleteFileIfExists(normalized_wav_filename);
}

}  // namespace denoiser
