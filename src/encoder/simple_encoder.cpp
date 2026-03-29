// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "simple_encoder.h"

#include <cmath>

#include "glog/logging.h"
#include "src/common/math/math_utils.h"

namespace encoder {

SimpleEncoder::SimpleEncoder(int audio_sample_rate, interface::EncoderParams encoder_params) : EncoderBase(
    audio_sample_rate,
    std::move(encoder_params)) {
  CHECK(encoder_params_.has_simple_encoder_params());
  CHECK(encoder_params_.simple_encoder_params().has_encoder_rate());
  CHECK_GT(encoder_params_.simple_encoder_params().encoder_rate(), 0.0);
  encoder_rate_ = encoder_params_.simple_encoder_params().encoder_rate();
  encode_frequency_for_bit_0_ = encoder_params_.simple_encoder_params().encode_frequency_for_bit_0();
  encode_frequency_for_bit_1_ = encoder_params_.simple_encoder_params().encode_frequency_for_bit_1();
  encode_frequency_for_rest_ = encoder_params_.simple_encoder_params().encode_frequency_for_rest();
  minimum_absolute_amplitude_ = encoder_params_.simple_encoder_params().minimum_absolute_amplitude();
  maximum_standard_error_ = encoder_params_.simple_encoder_params().maximum_standard_error();
  maximum_standard_error_for_half_window_ =
      encoder_params_.simple_encoder_params().maximum_standard_error_for_half_window();
  CHECK_GT(audio_sample_rate_, encoder_rate_ * 2);
}

void SimpleEncoder::Encode(const std::function<bool(char *)> &get_next_byte,
                           const std::function<void(double)> &set_next_audio_sample) const {
  char next_byte;
  int current_bit_count = 0;
  // +1s
  for (int i = 0; i < audio_sample_rate_; ++i) {
    set_next_audio_sample(0.0);
  }
  while (get_next_byte(&next_byte)) {
    int next_byte_to_int = (int) static_cast<unsigned char>(next_byte);
    // little endian
    for (int j = 0; j < 8; ++j) {
      int next_count = static_cast<int>((current_bit_count + 1) * audio_sample_rate_ / encoder_rate_);
      int curr_count = static_cast<int>(current_bit_count * audio_sample_rate_ / encoder_rate_);
      int delta_count = next_count - curr_count;
      double frequency =
          (next_byte_to_int & (1 << j)) ? encode_frequency_for_bit_1_ : encode_frequency_for_bit_0_;
      for (int sample = 0; sample < delta_count; ++sample) {
        double amplitude = std::sin(2 * M_PI * frequency * (double) sample / audio_sample_rate_);
        set_next_audio_sample(amplitude);
      }
      ++current_bit_count;
      for (int sample = 0; sample < delta_count; ++sample) {
        double amplitude = std::sin(2 * M_PI * encode_frequency_for_rest_ * (double) sample / audio_sample_rate_);
        set_next_audio_sample(amplitude);
      }
    }
  }
  // +1s
  for (int i = 0; i < audio_sample_rate_; ++i) {
    set_next_audio_sample(0.0);
  }
}

void SimpleEncoder::Decode(const std::function<bool(double*)>& get_next_audio_sample,
                           const std::function<void(char)>& set_next_byte) const {
  const int window_size = static_cast<int>(audio_sample_rate_ / encoder_rate_);
  SampleWindow sample_window(audio_sample_rate_, window_size);
  SampleWindow sample_window_half_size(audio_sample_rate_, window_size / 2);
  double next_sample;
  int last_byte = 0;
  int last_byte_bit_count = 0;
  int current_bit_count = 0;
  while (get_next_audio_sample(&next_sample)) {
    sample_window.PushBack(next_sample);
    sample_window_half_size.PushBack(next_sample);
    if ((int) sample_window.Size() != window_size) {
      continue;
    }
    const double amplitude_in_window = sample_window.GetAverageAmplitude();
    const double frequency_in_window = sample_window.GetAverageFrequency();
    const double std_error_for_half_window = std::abs(
        (frequency_in_window + math::kEpsilon) / (sample_window_half_size.GetAverageFrequency() + math::kEpsilon) - 1);
    const double std_error_for_bit_0 = std::abs(frequency_in_window / encode_frequency_for_bit_0_ - 1);
    const double std_error_for_bit_1 = std::abs(frequency_in_window / encode_frequency_for_bit_1_ - 1);
    if (std::abs(amplitude_in_window) < minimum_absolute_amplitude_) {
      continue;
    }
    if (std_error_for_half_window > maximum_standard_error_for_half_window_) {
      continue;
    }
    if (std::min(std_error_for_bit_0, std_error_for_bit_1) > maximum_standard_error_) {
      continue;
    }
    const int current_bit = static_cast<int>(std_error_for_bit_1 < std_error_for_bit_0);
    LOG(ERROR) << "valid bit " << current_bit;
    last_byte |= (1 << last_byte_bit_count) * static_cast<int>(std_error_for_bit_1 < std_error_for_bit_0);
    ++last_byte_bit_count;
    ++current_bit_count;
    if (last_byte_bit_count == 8) {
      set_next_byte(static_cast<char>(last_byte));
      LOG(ERROR) << "submit " << static_cast<char>(last_byte);
      last_byte = 0;
      last_byte_bit_count = 0;
    }
    sample_window.Clear();
  }
  LOG(INFO) << "Total bit count: " << current_bit_count;
  CHECK_EQ(last_byte_bit_count, 0) << "Decode error: Incomplete data.";
}

SimpleEncoder::SampleWindow::SampleWindow(int audio_sample_rate, int max_window_size)
    : audio_sample_rate_(audio_sample_rate),
      max_window_size_(max_window_size),
      samples_(),
      amplitude_sum_(0.0),
      sign_diff_sum_(0) {}

size_t SimpleEncoder::SampleWindow::Size() const {
  return samples_.size();
}

double SimpleEncoder::SampleWindow::GetAverageAmplitude() const {
  CHECK_GT(Size(), 0);
  return amplitude_sum_ * M_PI_2 / (double) Size();
}

double SimpleEncoder::SampleWindow::GetAverageFrequency() const {
  CHECK_GT(Size(), 0);
  if (sign_diff_sum_ == 0) {
    return 0.0;
  }
  return (double) sign_diff_sum_ * audio_sample_rate_ / (double) samples_.size() / 2.0;
}

void SimpleEncoder::SampleWindow::Clear() {
  while (!samples_.empty()) {
    samples_.pop_front();
  }
  amplitude_sum_ = 0.0;
  sign_diff_sum_ = 0;
}

void SimpleEncoder::SampleWindow::PushBack(double sample) {
  amplitude_sum_ += std::abs(sample);
  if (!samples_.empty()) {
    if (samples_.back() <= 0.0 && sample > 0.0) {
      sign_diff_sum_ += 1;
    } else if (samples_.back() >= 0.0 && sample < 0.0) {
      sign_diff_sum_ += 1;
    }
  }
  samples_.push_back(sample);
  while ((int) samples_.size() > max_window_size_) {
    PopFront();
  }
}

void SimpleEncoder::SampleWindow::PopFront() {
  CHECK(!samples_.empty());
  const double last_sample_front = samples_.front();
  amplitude_sum_ -= std::abs(samples_.front());
  samples_.pop_front();
  if (!samples_.empty()) {
    if (last_sample_front <= 0.0 && samples_.front() > 0.0) {
      sign_diff_sum_ -= 1;
    } else if (last_sample_front >= 0.0 && samples_.front() < 0.0) {
      sign_diff_sum_ -= 1;
    }
  }
}

} // encoder
