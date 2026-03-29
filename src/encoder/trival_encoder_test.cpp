// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/encoder/trival_encoder.h"

#include "glog/logging.h"
#include "gtest/gtest.h"

#include "src/common/file/io.h"
#include "src/common/interface/proto/encoder_params.pb.h"
#include "src/common/interface/proto/wav_params.pb.h"
#include "src/wav/wav_reader.h"
#include "src/wav/wav_writer.h"

TEST(TrivalEncoderTest, Encode) {
  interface::EncoderParams encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/encoder_params.txt", &encoder_params));
  interface::WavParams wav_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params));
  const int audio_sample_rate = wav_params.sample_rate();
  encoder::TrivalEncoder trival_encoder(audio_sample_rate, std::move(encoder_params));
  const std::string kStringToBeEncoded = "1f745684946ba0c5ccd19205003c387f637cfc736fe98af5c341c4c02bc54bb7";

  // encode
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
  trival_encoder.Encode(get_next_byte, set_next_sample);
  wav_writer.Write();
  io::DeleteFileIfExists(temp_filename);
}

TEST(TrivalEncoderTest, DecodeEasy) {
  interface::EncoderParams encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("src/encoder/test_data/easy_encoder_params.txt",
                                            &encoder_params));
  wav::WavReader wav_reader("src/encoder/test_data/easy.wav");
  encoder::TrivalEncoder trival_encoder(wav_reader.GetWavHeader().sample_rate, std::move(encoder_params));

  // decode
  std::function get_next_sample = [&wav_reader](double *sample) -> bool {
    if (wav_reader.IsEof()) {
      return false;
    }
    *CHECK_NOTNULL(sample) = wav_reader.GetSample().first;
    return true;
  };
  std::string decoded_string;
  std::function set_next_byte = [&decoded_string](char byte) {
    decoded_string += byte;
  };
  trival_encoder.Decode(get_next_sample, set_next_byte);
  wav_reader.Close();
  const std::string kCorrectAnswer = "kQ";
  EXPECT_EQ(decoded_string, kCorrectAnswer);
}

TEST(TrivalEncoderTest, DecodeMedium) {
  interface::EncoderParams encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("src/encoder/test_data/medium_encoder_params.txt",
                                            &encoder_params));
  wav::WavReader wav_reader("src/encoder/test_data/medium.wav");
  encoder::TrivalEncoder trival_encoder(wav_reader.GetWavHeader().sample_rate, std::move(encoder_params));

  // decode
  std::function get_next_sample = [&wav_reader](double *sample) -> bool {
    if (wav_reader.IsEof()) {
      return false;
    }
    *CHECK_NOTNULL(sample) = wav_reader.GetSample().first;
    return true;
  };
  std::string decoded_string;
  std::function set_next_byte = [&decoded_string](char byte) {
    decoded_string += byte;
  };
  trival_encoder.Decode(get_next_sample, set_next_byte);
  wav_reader.Close();
  const std::string kCorrectAnswer = "1f745684946ba0c5ccd19205003c387f637cfc736fe98af5c341c4c02bc54bb7";
  EXPECT_EQ(decoded_string, kCorrectAnswer);
}

TEST(TrivalEncoderTest, EncodeAndDecode) {
  interface::EncoderParams encoder_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/encoder_params.txt", &encoder_params));
  constexpr int kAudioSampleRate = 44100;
  encoder::TrivalEncoder trival_encoder(kAudioSampleRate, std::move(encoder_params));
  const std::string kStringToBeEncoded = "1f745684946ba0c5ccd19205003c387f637cfc736fe98af5c341c4c02bc54bb7";

  // encode
  std::vector<double> encoded_double;
  int encode_current_id = 0;
  std::function get_next_byte = [&kStringToBeEncoded, &encode_current_id](char *byte) -> bool {
    if (encode_current_id >= (int) kStringToBeEncoded.size()) {
      return false;
    }
    *CHECK_NOTNULL(byte) = kStringToBeEncoded[encode_current_id++];
    return true;
  };
  std::function set_next_sample = [&encoded_double](double sample) {
    encoded_double.push_back(sample);
  };
  trival_encoder.Encode(get_next_byte, set_next_sample);

  // decode
  std::string decoded_string;
  int decode_current_id = 0;
  std::function get_next_sample = [&encoded_double, &decode_current_id](double *sample) -> bool {
    if (decode_current_id >= (int) encoded_double.size()) {
      return false;
    }
    *CHECK_NOTNULL(sample) = encoded_double[decode_current_id++];
    return true;
  };
  std::function set_next_byte = [&decoded_string](char byte) {
    decoded_string += byte;
  };
  trival_encoder.Decode(get_next_sample, set_next_byte);
  EXPECT_EQ(kStringToBeEncoded, decoded_string);
}
