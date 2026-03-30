// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/encoder/simple_encoder.h"

#include "glog/logging.h"
#include "gtest/gtest.h"

#include "src/common/file/io.h"
#include "src/common/interface/proto/encoder_params.pb.h"
#include "src/common/interface/proto/wav_params.pb.h"
#include "src/wav/wav_reader.h"
#include "src/wav/wav_writer.h"

namespace encoder {

TEST(SimpleEncoderTest, Encode) {
  interface::EncoderParams encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/encoder_params.txt", &encoder_params));
  interface::WavParams wav_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params));
  const int audio_sample_rate = wav_params.sample_rate();
  SimpleEncoder simple_encoder(audio_sample_rate, std::move(encoder_params));
  const std::string kStringToBeEncoded = "1f745684946ba0c5ccd19205003c387f637cfc736fe98af5c341c4c02bc54bb7";

  int encode_current_id = 0;
  std::function get_next_byte = [&kStringToBeEncoded, &encode_current_id](char *byte) -> bool {
    if (encode_current_id >= (int) kStringToBeEncoded.size()) {
      return false;
    }
    *CHECK_NOTNULL(byte) = kStringToBeEncoded[encode_current_id++];
    return true;
  };
  const std::string temp_filename = io::GenerateTestFolder() + "/trival_encoded.wav";
  wav::WavWriter wav_writer(temp_filename, wav_params);
  std::function set_next_sample = [&wav_writer](double sample) {
    wav_writer.AddSample(sample);
  };
  simple_encoder.Encode(get_next_byte, set_next_sample);
  wav_writer.Write();
  io::DeleteFileIfExists(temp_filename);
}

TEST(SimpleEncoderTest, DecodeEasy) {
  interface::EncoderParams encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat(
      "src/encoder/simple_encoder_test_data/easy_encoder_params.txt", &encoder_params));

  wav::WavReader reader("src/encoder/simple_encoder_test_data/easy.wav");

  SimpleEncoder decoder(reader.GetWavHeader().sample_rate, encoder_params);

  std::vector<char> decoded_bytes;
  size_t sample_index = 0;

  auto get_next_audio_sample = [&reader, &sample_index](double* next_sample) -> bool {
    if (reader.IsEof()) {
      return false;
    }
    std::pair<double, double> sample = reader.GetSample();
    *next_sample = sample.first;
    ++sample_index;
    return true;
  };

  auto set_next_byte = [&decoded_bytes](char byte) {
    decoded_bytes.push_back(byte);
  };

  decoder.Decode(get_next_audio_sample, set_next_byte);

  std::string decoded_message(decoded_bytes.begin(), decoded_bytes.end());
  EXPECT_EQ(decoded_message, "kQ");
}

TEST(SimpleEncoderTest, DecodeMedium) {
  interface::EncoderParams encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat(
      "src/encoder/simple_encoder_test_data/medium_encoder_params.txt", &encoder_params));

  wav::WavReader reader("src/encoder/simple_encoder_test_data/medium.wav");

  SimpleEncoder decoder(reader.GetWavHeader().sample_rate, encoder_params);

  std::vector<char> decoded_bytes;

  auto get_next_audio_sample = [&reader](double* next_sample) -> bool {
    if (reader.IsEof()) {
      return false;
    }
    std::pair<double, double> sample = reader.GetSample();
    *next_sample = sample.first;
    return true;
  };

  auto set_next_byte = [&decoded_bytes](char byte) {
    decoded_bytes.push_back(byte);
  };

  decoder.Decode(get_next_audio_sample, set_next_byte);

  std::string decoded_message(decoded_bytes.begin(), decoded_bytes.end());
  EXPECT_EQ(decoded_message, "1f745684946ba0c5ccd19205003c387f637cfc736fe98af5c341c4c02bc54bb7");
}

}  // namespace encoder
