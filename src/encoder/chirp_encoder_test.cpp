// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/encoder/chirp_encoder.h"

#include "glog/logging.h"
#include "gtest/gtest.h"

#include "src/common/file/io.h"
#include "src/common/interface/proto/encoder_params.pb.h"
#include "src/common/interface/proto/wav_params.pb.h"
#include "src/wav/wav_reader.h"
#include "src/wav/wav_writer.h"
#include "src/wav/wav_noise_producer.h"

namespace encoder {

namespace {

void EncodeString(const std::string& str,
                 const interface::EncoderParams& encoder_params,
                 const std::string& temp_filename,
                 int audio_sample_rate) {
  ChirpEncoder encoder(audio_sample_rate, encoder_params);

  int encode_current_id = 0;
  std::function get_next_byte = [&str, &encode_current_id](char *byte) -> bool {
    if (encode_current_id >= (int) str.size()) {
      return false;
    }
    *CHECK_NOTNULL(byte) = str[encode_current_id++];
    return true;
  };

  interface::WavParams wav_params;
  wav_params.set_num_channels(1);
  wav_params.set_sample_rate(audio_sample_rate);
  wav_params.set_bit_depth(16);

  wav::WavWriter wav_writer(temp_filename, wav_params);
  std::function set_next_sample = [&wav_writer](double sample) {
    wav_writer.AddSample(sample);
  };

  encoder.Encode(get_next_byte, set_next_sample);
  wav_writer.Write();
}

std::string DecodeFile(const std::string& temp_filename,
                       const interface::EncoderParams& encoder_params,
                       int audio_sample_rate) {
  wav::WavReader reader(temp_filename);
  ChirpEncoder decoder(audio_sample_rate, encoder_params);

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

  return std::string(decoded_bytes.begin(), decoded_bytes.end());
}

std::string RoundTripWithNoise(const std::string& str,
                               double noise_level,
                               const std::function<void(const std::string&, const std::string&, double)>& add_noise_func) {
  interface::WavParams wav_params;
  io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params);
  const int audio_sample_rate = wav_params.sample_rate();

  interface::EncoderParams encoder_params;
  io::ReadFromProtoInTextFormat("params/encoder_params.txt", &encoder_params);

  const std::string temp_filename = io::GenerateTestFolder() + "/chirp_noisy_test.wav";
  const std::string noisy_filename = io::GenerateTestFolder() + "/chirp_noisy_test_noisy.wav";

  EncodeString(str, encoder_params, temp_filename, audio_sample_rate);

  add_noise_func(temp_filename, noisy_filename, noise_level);

  std::string result = DecodeFile(noisy_filename, encoder_params, audio_sample_rate);

  io::DeleteFileIfExists(temp_filename);
  io::DeleteFileIfExists(noisy_filename);

  return result;
}

}  // namespace

TEST(ChirpEncoderTest, EncodeAndDecodeRoundTrip) {
  interface::EncoderParams encoder_params;
  io::ReadFromProtoInTextFormat("params/encoder_params.txt", &encoder_params);

  interface::WavParams wav_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params));
  const int audio_sample_rate = wav_params.sample_rate();

  const std::string kStringToBeEncoded = "Hello, world!";

  int encode_current_id = 0;
  std::function get_next_byte = [&kStringToBeEncoded, &encode_current_id](char *byte) -> bool {
    if (encode_current_id >= (int) kStringToBeEncoded.size()) {
      return false;
    }
    *CHECK_NOTNULL(byte) = kStringToBeEncoded[encode_current_id++];
    return true;
  };

  const std::string temp_filename = io::GenerateTestFolder() + "/chirp_encoded.wav";
  wav::WavWriter wav_writer(temp_filename, wav_params);
  std::function set_next_sample = [&wav_writer](double sample) {
    wav_writer.AddSample(sample);
  };

  ChirpEncoder encoder(audio_sample_rate, encoder_params);
  encoder.Encode(get_next_byte, set_next_sample);
  wav_writer.Write();

  wav::WavReader reader(temp_filename);
  ChirpEncoder decoder(reader.GetWavHeader().sample_rate, encoder_params);

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
  EXPECT_EQ(decoded_message, kStringToBeEncoded);

  io::DeleteFileIfExists(temp_filename);
}

TEST(ChirpEncoderTest, EncodeAndDecodeHexString) {
  interface::EncoderParams encoder_params;
  io::ReadFromProtoInTextFormat("params/encoder_params.txt", &encoder_params);

  interface::WavParams wav_params;
  ASSERT_TRUE(io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params));
  const int audio_sample_rate = wav_params.sample_rate();

  const std::string kStringToBeEncoded = "1f745684946ba0c5ccd19205003c387f637cfc736fe98af5c341c4c02bc54bb7";

  int encode_current_id = 0;
  std::function get_next_byte = [&kStringToBeEncoded, &encode_current_id](char *byte) -> bool {
    if (encode_current_id >= (int) kStringToBeEncoded.size()) {
      return false;
    }
    *CHECK_NOTNULL(byte) = kStringToBeEncoded[encode_current_id++];
    return true;
  };

  const std::string temp_filename = io::GenerateTestFolder() + "/chirp_hex_encoded.wav";
  wav::WavWriter wav_writer(temp_filename, wav_params);
  std::function set_next_sample = [&wav_writer](double sample) {
    wav_writer.AddSample(sample);
  };

  ChirpEncoder encoder(audio_sample_rate, encoder_params);
  encoder.Encode(get_next_byte, set_next_sample);
  wav_writer.Write();

  wav::WavReader reader(temp_filename);
  ChirpEncoder decoder(reader.GetWavHeader().sample_rate, encoder_params);

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
  EXPECT_EQ(decoded_message, kStringToBeEncoded);

  io::DeleteFileIfExists(temp_filename);
}

TEST(ChirpEncoderTest, RoundTripWithWhiteNoise10dBSNR) {
  const std::vector<std::string> test_strings = {
    "Hello, Chirp!",
    "Test123",
    "ABCxyz789"
  };

  double noise_level = 0.316;

  for (const auto& str : test_strings) {
    std::string result = RoundTripWithNoise(
        str, noise_level,
        [](const std::string& in, const std::string& out, double level) {
          wav::AddWhiteNoise(in, out, level);
        });
    EXPECT_EQ(result, str) << "White noise 10dB SNR failed for: " << str;
  }
}

TEST(ChirpEncoderTest, RoundTripWithUniformNoise10dBSNR) {
  const std::vector<std::string> test_strings = {
    "MessageOne",
    "DataTest2",
    "AudioChirp"
  };

  double noise_level = 0.316;

  for (const auto& str : test_strings) {
    std::string result = RoundTripWithNoise(
        str, noise_level,
        [](const std::string& in, const std::string& out, double level) {
          wav::AddUniformNoise(in, out, level);
        });
    EXPECT_EQ(result, str) << "Uniform noise 10dB SNR failed for: " << str;
  }
}

TEST(ChirpEncoderTest, RoundTripWithPinkNoise10dBSNR) {
  const std::vector<std::string> test_strings = {
    "PinkNoiseTest",
    "LowFreqNoise",
    "AudioSignalQ"
  };

  double noise_level = 0.316;

  for (const auto& str : test_strings) {
    std::string result = RoundTripWithNoise(
        str, noise_level,
        [](const std::string& in, const std::string& out, double level) {
          wav::AddPinkNoise(in, out, level);
        });
    EXPECT_EQ(result, str) << "Pink noise 10dB SNR failed for: " << str;
  }
}

TEST(ChirpEncoderTest, RoundTripWithBrownNoise10dBSNR) {
  const std::vector<std::string> test_strings = {
    "BrownNoiseChk",
    "DeepBassTest",
    "LowPassAudio"
  };

  double noise_level = 0.316;

  for (const auto& str : test_strings) {
    std::string result = RoundTripWithNoise(
        str, noise_level,
        [](const std::string& in, const std::string& out, double level) {
          wav::AddBrownNoise(in, out, level);
        });
    EXPECT_EQ(result, str) << "Brown noise 10dB SNR failed for: " << str;
  }
}

TEST(ChirpEncoderTest, RoundTripWithSilenceBeforeAndAfter) {
  interface::WavParams wav_params;
  io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params);
  const int audio_sample_rate = wav_params.sample_rate();

  interface::EncoderParams encoder_params;
  io::ReadFromProtoInTextFormat("params/encoder_params.txt", &encoder_params);

  const std::vector<std::string> test_strings = {
    "Short",
    "MediumText",
    "LongerStringTest"
  };

  for (const auto& str : test_strings) {
    const std::string temp_filename = io::GenerateTestFolder() + "/chirp_silence_test.wav";
    const std::string silence_filename = io::GenerateTestFolder() + "/chirp_silence_test_silence.wav";
    const std::string noisy_filename = io::GenerateTestFolder() + "/chirp_silence_test_noisy.wav";

    EncodeString(str, encoder_params, temp_filename, audio_sample_rate);

    wav::WavReader reader(temp_filename);
    interface::WavParams output_wav_params;
    output_wav_params.set_num_channels(1);
    output_wav_params.set_sample_rate(audio_sample_rate);
    output_wav_params.set_bit_depth(16);

    wav::WavWriter silence_writer(silence_filename, output_wav_params);

    for (int i = 0; i < audio_sample_rate * 2; ++i) {
      silence_writer.AddSample(0.0);
    }

    while (!reader.IsEof()) {
      auto sample = reader.GetSample();
      silence_writer.AddSample(sample.first);
    }

    for (int i = 0; i < audio_sample_rate * 3; ++i) {
      silence_writer.AddSample(0.0);
    }

    silence_writer.Write();
    reader.Close();

    wav::AddWhiteNoise(silence_filename, noisy_filename, 0.1);

    std::string result = DecodeFile(noisy_filename, encoder_params, audio_sample_rate);

    EXPECT_EQ(result, str) << "Silence handling failed for: " << str;

    io::DeleteFileIfExists(temp_filename);
    io::DeleteFileIfExists(silence_filename);
    io::DeleteFileIfExists(noisy_filename);
  }
}

TEST(ChirpEncoderTest, RoundTripWithVariableSilenceAndNoise10dB) {
  interface::WavParams wav_params;
  io::ReadFromProtoInTextFormat("params/wav_params.txt", &wav_params);
  const int audio_sample_rate = wav_params.sample_rate();

  interface::EncoderParams encoder_params;
  io::ReadFromProtoInTextFormat("params/encoder_params.txt", &encoder_params);

  const std::vector<std::string> test_strings = {
    "TestA",
    "TestB",
    "TestC"
  };

  const std::vector<std::pair<int, int>> silence_configs = {
    {0, 0},
    {500, 0},
    {0, 500},
    {500, 500},
    {1000, 1000},
  };

  for (const auto& str : test_strings) {
    for (const auto& [silence_before_ms, silence_after_ms] : silence_configs) {
      const std::string temp_filename = io::GenerateTestFolder() + "/chirp_var_silence.wav";
      const std::string silence_filename = io::GenerateTestFolder() + "/chirp_var_silence_silence.wav";
      const std::string noisy_filename = io::GenerateTestFolder() + "/chirp_var_silence_noisy.wav";

      EncodeString(str, encoder_params, temp_filename, audio_sample_rate);

      wav::WavReader reader(temp_filename);
      interface::WavParams output_wav_params;
      output_wav_params.set_num_channels(1);
      output_wav_params.set_sample_rate(audio_sample_rate);
      output_wav_params.set_bit_depth(16);

      wav::WavWriter writer(silence_filename, output_wav_params);

      for (int i = 0; i < silence_before_ms * audio_sample_rate / 1000; ++i) {
        writer.AddSample(0.0);
      }

      while (!reader.IsEof()) {
        auto sample = reader.GetSample();
        writer.AddSample(sample.first);
      }

      for (int i = 0; i < silence_after_ms * audio_sample_rate / 1000; ++i) {
        writer.AddSample(0.0);
      }

      writer.Write();
      reader.Close();

      wav::AddWhiteNoise(silence_filename, noisy_filename, 0.316);

      std::string result = DecodeFile(noisy_filename, encoder_params, audio_sample_rate);

      EXPECT_EQ(result, str) << "Variable silence (before=" << silence_before_ms
                             << "ms, after=" << silence_after_ms
                             << "ms) failed for: " << str;

      io::DeleteFileIfExists(temp_filename);
      io::DeleteFileIfExists(silence_filename);
      io::DeleteFileIfExists(noisy_filename);
    }
  }
}

}  // namespace encoder
