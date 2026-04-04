// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <deque>

#include "src/encoder/encoder_base.h"

namespace encoder {

class SimpleEncoder final : public EncoderBase {
 public:
  SimpleEncoder(int audio_sample_rate, interface::EncoderParams encoder_params);

  void Encode(const std::function<bool(char *)> &get_next_byte,
              const std::function<void(double)> &set_next_audio_sample) const override;

  void Decode(const std::function<bool(double*)>& get_next_audio_sample,
              const std::function<void(char)>& set_next_byte,
              int max_total_bits = 0) const override;

 private:
  class SampleWindow {
   public:
    explicit SampleWindow(int audio_sample_rate, int max_window_size);
    ~SampleWindow() = default;
    size_t Size() const;
    double GetAverageAmplitude() const;
    double GetAverageFrequency() const;
    void Clear();
    void PushBack(double sample);
    void PopFront();
   private:
    int audio_sample_rate_ = 0;
    int max_window_size_ = 0;
    std::deque<double> samples_{};
    double amplitude_sum_ = 0.0;
    int sign_diff_sum_ = 0;
  };
  double encoder_rate_ = 0.0;
  double encode_frequency_for_bit_0_ = 0.0;
  double encode_frequency_for_bit_1_ = 0.0;
  double encode_frequency_for_rest_ = 0.0;
  double minimum_absolute_amplitude_ = 0.0;
  double maximum_standard_error_ = 0.0;
  double maximum_standard_error_for_half_window_ = 0.0;
};

} // encoder
