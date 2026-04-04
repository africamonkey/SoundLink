// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <deque>
#include <vector>

#include "src/encoder/encoder_base.h"

namespace encoder {

class GoertzelEncoder final : public EncoderBase {
 public:
  GoertzelEncoder(int audio_sample_rate, interface::EncoderParams encoder_params);

  void Encode(const std::function<bool(char *)> &get_next_byte,
              const std::function<void(double)> &set_next_audio_sample) const override;

  void Decode(const std::function<bool(double*)>& get_next_audio_sample,
              const std::function<void(char)>& set_next_byte,
              int max_total_bits = 0) const override;

 private:
  struct GoertzelState {
    double s1 = 0.0;
    double s2 = 0.0;
    double target_freq = 0.0;
    double coeff = 0.0;
    int sample_count = 0;
  };

  struct DetectionResult {
    double frequency;
    double energy_bit_0;
    double energy_bit_1;
    double energy_rest;
    double total_energy;
    double amplitude;
  };

  void InitGoertzel(GoertzelState* state, double target_freq) const;
  void ProcessGoertzelSample(GoertzelState* state, double sample) const;
  double ComputeGoertzelEnergy(const GoertzelState& state) const;
  DetectionResult AnalyzeWindow(const std::vector<double>& samples) const;
  int VotingDecision(const std::deque<int>& recent_decisions) const;

  double encoder_rate_ = 0.0;
  double encode_frequency_for_bit_0_ = 0.0;
  double encode_frequency_for_bit_1_ = 0.0;
  double encode_frequency_for_rest_ = 0.0;
  int goertzel_window_size_ = 0;
  double frequency_tolerance_ = 0.0;
  double minimum_energy_ratio_ = 0.0;
  int voting_window_size_ = 0;
  int window_size_ = 0;
};

} // encoder