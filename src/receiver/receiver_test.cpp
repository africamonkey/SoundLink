// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/receiver/receiver.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "src/encoder/encoder_base.h"
#include "src/common/interface/proto/encoder_params.pb.h"

namespace receiver {

namespace {

constexpr int kDefaultSampleRate = 44100;

class MockEncoder : public encoder::EncoderBase {
 public:
  MockEncoder()
      : EncoderBase(kDefaultSampleRate, interface::EncoderParams()) {}

  void Encode(const std::function<bool(char*)>&,
              const std::function<void(double)>&) const override {}

  void Decode(const std::function<bool(double*)>& get_next_audio_sample,
              const std::function<void(char)>&) const override {
    double sample;
    while (get_next_audio_sample(&sample)) {
      decoded_samples_.push_back(sample);
    }
  }

  const std::vector<double>& decoded_samples() const { return decoded_samples_; }

 private:
  mutable std::vector<double> decoded_samples_;
};

class FakeAudioCapturer : public audio::AudioCapturer {
 public:
  FakeAudioCapturer() : AudioCapturer(kDefaultSampleRate, 1) {}

  bool Initialize() override { return true; }
  bool StartCapture(AudioCallback callback) override {
    callback_ = callback;
    return true;
  }
  void StopCapture() override {}
  void Close() override {}

  void InjectSamples(const float* samples, size_t num_samples) {
    if (callback_) {
      callback_(samples, num_samples);
    }
  }

 private:
  AudioCallback callback_;
};

}  // namespace

class ReceiverTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(ReceiverTest, StartAndStopWithoutCrash) {
  auto mock_encoder = std::make_shared<MockEncoder>();
  auto fake_capturer = std::make_unique<FakeAudioCapturer>();

  Receiver receiver(kDefaultSampleRate, mock_encoder, std::move(fake_capturer));
  receiver.SetMessageCallback([](const std::string&) {});
  receiver.SetDataCallback([](const std::vector<char>&) {});

  EXPECT_TRUE(receiver.Initialize());
  EXPECT_TRUE(receiver.StartCapture());
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  receiver.StopCapture();
}

TEST_F(ReceiverTest, ReceivesInjectedSamples) {
  auto mock_encoder = std::make_shared<MockEncoder>();
  auto fake_capturer = std::make_unique<FakeAudioCapturer>();
  auto* fake_ptr = fake_capturer.get();

  Receiver receiver(kDefaultSampleRate, mock_encoder, std::move(fake_capturer));
  receiver.SetMessageCallback([](const std::string&) {});
  receiver.SetDataCallback([](const std::vector<char>&) {});

  EXPECT_TRUE(receiver.Initialize());
  EXPECT_TRUE(receiver.StartCapture());

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::vector<float> samples(512, 0.5f);
  fake_ptr->InjectSamples(samples.data(), samples.size());

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  receiver.StopCapture();

  EXPECT_EQ(mock_encoder->decoded_samples().size(), 512);
}

TEST_F(ReceiverTest, DecoderReceivesAllInjectedSamples) {
  auto mock_encoder = std::make_shared<MockEncoder>();
  auto fake_capturer = std::make_unique<FakeAudioCapturer>();
  auto* fake_ptr = fake_capturer.get();

  Receiver receiver(kDefaultSampleRate, mock_encoder, std::move(fake_capturer));
  receiver.SetMessageCallback([](const std::string&) {});
  receiver.SetDataCallback([](const std::vector<char>&) {});

  EXPECT_TRUE(receiver.Initialize());
  EXPECT_TRUE(receiver.StartCapture());

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  constexpr size_t kNumSamples = 2048;
  std::vector<float> samples(kNumSamples, 0.123f);
  fake_ptr->InjectSamples(samples.data(), samples.size());

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  receiver.StopCapture();

  EXPECT_EQ(mock_encoder->decoded_samples().size(), kNumSamples);
}

TEST_F(ReceiverTest, SmallSampleBatch) {
  auto mock_encoder = std::make_shared<MockEncoder>();
  auto fake_capturer = std::make_unique<FakeAudioCapturer>();
  auto* fake_ptr = fake_capturer.get();

  Receiver receiver(kDefaultSampleRate, mock_encoder, std::move(fake_capturer));
  receiver.SetMessageCallback([](const std::string&) {});
  receiver.SetDataCallback([](const std::vector<char>&) {});

  EXPECT_TRUE(receiver.Initialize());
  EXPECT_TRUE(receiver.StartCapture());

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::vector<float> samples(10, 0.5f);
  fake_ptr->InjectSamples(samples.data(), samples.size());

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  receiver.StopCapture();

  EXPECT_EQ(mock_encoder->decoded_samples().size(), 10);
}

TEST_F(ReceiverTest, SampleValuesPreserved) {
  auto mock_encoder = std::make_shared<MockEncoder>();
  auto fake_capturer = std::make_unique<FakeAudioCapturer>();
  auto* fake_ptr = fake_capturer.get();

  Receiver receiver(kDefaultSampleRate, mock_encoder, std::move(fake_capturer));
  receiver.SetMessageCallback([](const std::string&) {});
  receiver.SetDataCallback([](const std::vector<char>&) {});

  EXPECT_TRUE(receiver.Initialize());
  EXPECT_TRUE(receiver.StartCapture());

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::vector<float> samples = {0.1f, -0.2f, 0.3f, -0.4f, 0.5f};
  fake_ptr->InjectSamples(samples.data(), samples.size());

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  receiver.StopCapture();

  const auto& decoded = mock_encoder->decoded_samples();
  ASSERT_EQ(decoded.size(), 5);
  EXPECT_NEAR(decoded[0], 0.1, 0.001);
  EXPECT_NEAR(decoded[1], -0.2, 0.001);
  EXPECT_NEAR(decoded[2], 0.3, 0.001);
  EXPECT_NEAR(decoded[3], -0.4, 0.001);
  EXPECT_NEAR(decoded[4], 0.5, 0.001);
}

TEST_F(ReceiverTest, NoSamplesBeforeInjection) {
  auto mock_encoder = std::make_shared<MockEncoder>();
  auto fake_capturer = std::make_unique<FakeAudioCapturer>();
  auto* fake_ptr = fake_capturer.get();

  Receiver receiver(kDefaultSampleRate, mock_encoder, std::move(fake_capturer));
  receiver.SetMessageCallback([](const std::string&) {});
  receiver.SetDataCallback([](const std::vector<char>&) {});

  EXPECT_TRUE(receiver.Initialize());
  EXPECT_TRUE(receiver.StartCapture());

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  receiver.StopCapture();

  EXPECT_EQ(mock_encoder->decoded_samples().size(), 0);
}

TEST_F(ReceiverTest, CloseWithoutStart) {
  auto mock_encoder = std::make_shared<MockEncoder>();

  Receiver receiver(kDefaultSampleRate, mock_encoder);
  receiver.SetMessageCallback([](const std::string&) {});
  receiver.SetDataCallback([](const std::vector<char>&) {});

  EXPECT_TRUE(receiver.Initialize());
  receiver.Close();
}

TEST_F(ReceiverTest, MixedBatchSizes) {
  auto mock_encoder = std::make_shared<MockEncoder>();
  auto fake_capturer = std::make_unique<FakeAudioCapturer>();
  auto* fake_ptr = fake_capturer.get();

  Receiver receiver(kDefaultSampleRate, mock_encoder, std::move(fake_capturer));
  receiver.SetMessageCallback([](const std::string&) {});
  receiver.SetDataCallback([](const std::vector<char>&) {});

  EXPECT_TRUE(receiver.Initialize());
  EXPECT_TRUE(receiver.StartCapture());

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::vector<size_t> batch_sizes = {1, 10, 100, 50, 200, 25, 75, 512, 33, 256};
  size_t total_expected = 0;
  for (size_t batch_size : batch_sizes) {
    std::vector<float> samples(batch_size, 0.5f);
    fake_ptr->InjectSamples(samples.data(), samples.size());
    total_expected += batch_size;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  receiver.StopCapture();

  EXPECT_EQ(mock_encoder->decoded_samples().size(), total_expected);
}

TEST_F(ReceiverTest, AlternatingPositiveAndNegativeSamples) {
  auto mock_encoder = std::make_shared<MockEncoder>();
  auto fake_capturer = std::make_unique<FakeAudioCapturer>();
  auto* fake_ptr = fake_capturer.get();

  Receiver receiver(kDefaultSampleRate, mock_encoder, std::move(fake_capturer));
  receiver.SetMessageCallback([](const std::string&) {});
  receiver.SetDataCallback([](const std::vector<char>&) {});

  EXPECT_TRUE(receiver.Initialize());
  EXPECT_TRUE(receiver.StartCapture());

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  constexpr size_t kNumSamples = 100;
  std::vector<float> samples(kNumSamples);
  for (size_t i = 0; i < kNumSamples; ++i) {
    samples[i] = (i % 2 == 0) ? 0.5f : -0.5f;
  }
  fake_ptr->InjectSamples(samples.data(), samples.size());

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  receiver.StopCapture();

  const auto& decoded = mock_encoder->decoded_samples();
  ASSERT_EQ(decoded.size(), kNumSamples);
  for (size_t i = 0; i < kNumSamples; ++i) {
    float expected = (i % 2 == 0) ? 0.5f : -0.5f;
    EXPECT_NEAR(decoded[i], expected, 0.001) << "Mismatch at index " << i;
  }
}

}  // namespace receiver
