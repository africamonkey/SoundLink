// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/encoder/goertzel_encoder.h"

#include <cmath>
#include <algorithm>
#include <array>

#include "glog/logging.h"
#include "src/common/math/math_utils.h"

namespace encoder {

GoertzelEncoder::GoertzelEncoder(int audio_sample_rate, interface::EncoderParams encoder_params)
    : EncoderBase(audio_sample_rate, std::move(encoder_params)) {
  CHECK(encoder_params_.has_goertzel_encoder_params());
  const auto& params = encoder_params_.goertzel_encoder_params();
  CHECK_GT(params.encoder_rate(), 0.0);
  CHECK_GT(params.encode_frequency_for_bit_0(), 0.0);
  CHECK_GT(params.encode_frequency_for_bit_1(), 0.0);
  CHECK_GT(params.encode_frequency_for_bit_1(), params.encode_frequency_for_bit_0());
  encoder_rate_ = params.encoder_rate();
  encode_frequency_for_bit_0_ = params.encode_frequency_for_bit_0();
  encode_frequency_for_bit_1_ = params.encode_frequency_for_bit_1();
  encode_frequency_for_rest_ = params.encode_frequency_for_rest();
  goertzel_window_size_ = params.goertzel_window_size() > 0 ? params.goertzel_window_size() : 256;
  frequency_tolerance_ = params.frequency_tolerance() > 0 ? params.frequency_tolerance() : 0.15;
  minimum_energy_ratio_ = params.minimum_energy_ratio() > 0 ? params.minimum_energy_ratio() : 2.0;
  voting_window_size_ = params.voting_window_size() > 0 ? params.voting_window_size() : 3;
  window_size_ = static_cast<int>(audio_sample_rate_ / encoder_rate_);
  CHECK_GT(audio_sample_rate_, encoder_rate_ * 2);
}

void GoertzelEncoder::InitGoertzel(GoertzelState* state, double target_freq) const {
  state->target_freq = target_freq;
  state->s1 = 0.0;
  state->s2 = 0.0;
  state->sample_count = 0;
  const double normalized_freq = target_freq / audio_sample_rate_;
  state->coeff = 2.0 * std::cos(2.0 * M_PI * normalized_freq);
}

void GoertzelEncoder::ProcessGoertzelSample(GoertzelState* state, double sample) const {
  const double s0 = state->coeff * state->s1 - state->s2 + sample;
  state->s2 = state->s1;
  state->s1 = s0;
  ++state->sample_count;
}

double GoertzelEncoder::ComputeGoertzelEnergy(const GoertzelState& state) const {
  if (state.sample_count == 0) return 0.0;
  return state.s1 * state.s1 + state.s2 * state.s2 - state.s1 * state.s2 * state.coeff;
}

GoertzelEncoder::DetectionResult GoertzelEncoder::AnalyzeWindow(const std::vector<double>& samples) const {
  DetectionResult result = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  if (samples.empty()) return result;
  GoertzelState states[3];
  InitGoertzel(&states[0], encode_frequency_for_bit_0_);
  InitGoertzel(&states[1], encode_frequency_for_bit_1_);
  InitGoertzel(&states[2], encode_frequency_for_rest_);
  double amplitude_sum = 0.0;
  double rms_sum = 0.0;
  for (double sample : samples) {
    double abs_sample = std::abs(sample);
    amplitude_sum += abs_sample;
    rms_sum += sample * sample;
  }
  result.amplitude = amplitude_sum * M_PI_2 / samples.size();
  double rms = std::sqrt(rms_sum / samples.size());
  double normalize_factor = (rms > math::kEpsilon) ? (1.0 / rms) : 1.0;
  for (double sample : samples) {
    double normalized_sample = sample * normalize_factor;
    ProcessGoertzelSample(&states[0], normalized_sample);
    ProcessGoertzelSample(&states[1], normalized_sample);
    ProcessGoertzelSample(&states[2], normalized_sample);
  }
  result.energy_bit_0 = ComputeGoertzelEnergy(states[0]);
  result.energy_bit_1 = ComputeGoertzelEnergy(states[1]);
  result.energy_rest = ComputeGoertzelEnergy(states[2]);
  result.total_energy = result.energy_bit_0 + result.energy_bit_1 + result.energy_rest;
  double max_energy = std::max({result.energy_bit_0, result.energy_bit_1, result.energy_rest});
  if (max_energy > 0) {
    result.energy_bit_0 /= max_energy;
    result.energy_bit_1 /= max_energy;
    result.energy_rest /= max_energy;
  }
  if (result.energy_bit_0 > result.energy_bit_1) {
    result.frequency = encode_frequency_for_bit_0_;
  } else {
    result.frequency = encode_frequency_for_bit_1_;
  }
  return result;
}

int GoertzelEncoder::VotingDecision(const std::deque<int>& recent_decisions) const {
  if (recent_decisions.empty()) return -1;
  int count_0 = 0, count_1 = 0;
  for (int d : recent_decisions) {
    if (d == 0) ++count_0;
    else if (d == 1) ++count_1;
  }
  if (count_1 > count_0) return 1;
  if (count_0 > count_1) return 0;
  return recent_decisions.back();
}

void GoertzelEncoder::Encode(const std::function<bool(char *)> &get_next_byte,
                           const std::function<void(double)> &set_next_audio_sample) const {
  char next_byte;
  int current_bit_count = 0;
  for (int i = 0; i < audio_sample_rate_; ++i) {
    set_next_audio_sample(0.0);
  }
  while (get_next_byte(&next_byte)) {
    int next_byte_to_int = (int)static_cast<unsigned char>(next_byte);
    for (int j = 0; j < 8; ++j) {
      int next_count = static_cast<int>((current_bit_count + 1) * audio_sample_rate_ / encoder_rate_);
      int curr_count = static_cast<int>(current_bit_count * audio_sample_rate_ / encoder_rate_);
      int delta_count = next_count - curr_count;
      double frequency =
          (next_byte_to_int & (1 << j)) ? encode_frequency_for_bit_1_ : encode_frequency_for_bit_0_;
      for (int sample = 0; sample < delta_count; ++sample) {
        double amplitude = std::sin(2 * M_PI * frequency * (double)sample / audio_sample_rate_);
        set_next_audio_sample(amplitude);
      }
      ++current_bit_count;
      for (int sample = 0; sample < delta_count; ++sample) {
        double amplitude = std::sin(2 * M_PI * encode_frequency_for_rest_ * (double)sample / audio_sample_rate_);
        set_next_audio_sample(amplitude);
      }
    }
  }
  for (int i = 0; i < audio_sample_rate_; ++i) {
    set_next_audio_sample(0.0);
  }
}

void GoertzelEncoder::Decode(const std::function<bool(double*)>& get_next_audio_sample,
                             const std::function<void(char)>& set_next_byte,
                             int max_total_bits) const {
  std::deque<double> sample_buffer;
  std::deque<int> recent_decisions;
  double next_sample;
  int last_byte = 0;
  int last_byte_bit_count = 0;
  int current_bit_count = 0;
  int samples_in_bit = 0;
  const int samples_per_bit = static_cast<int>(audio_sample_rate_ / encoder_rate_);
  while (get_next_audio_sample(&next_sample)) {
    sample_buffer.push_back(next_sample);
    ++samples_in_bit;
    if ((int)sample_buffer.size() > window_size_) {
      sample_buffer.pop_front();
    }
    if (samples_in_bit < samples_per_bit) {
      continue;
    }
    std::vector<double> window_samples(sample_buffer.begin(), sample_buffer.end());
    DetectionResult result = AnalyzeWindow(window_samples);
    const double energy_ratio = result.energy_bit_1 / (result.energy_bit_0 + math::kEpsilon);
    const double freq_error_0 = std::abs(result.energy_bit_0 - 1.0);
    const double freq_error_1 = std::abs(result.energy_bit_1 - 1.0);
    int raw_decision = -1;
    double max_energy = std::max({result.energy_bit_0, result.energy_bit_1, result.energy_rest});
    double second_energy = 0.0;
    if (max_energy == result.energy_bit_0) {
      second_energy = std::max(result.energy_bit_1, result.energy_rest);
    } else if (max_energy == result.energy_bit_1) {
      second_energy = std::max(result.energy_bit_0, result.energy_rest);
    } else {
      second_energy = std::max(result.energy_bit_0, result.energy_bit_1);
    }
    if (result.amplitude < 0.03) {
      raw_decision = -1;
    } else if (result.energy_rest > 0.2 * max_energy || second_energy > 0.6 * max_energy) {
      raw_decision = -1;
    } else if (result.energy_bit_0 > result.energy_bit_1 && freq_error_0 < frequency_tolerance_) {
      raw_decision = 0;
    } else if (result.energy_bit_1 > result.energy_bit_0 && freq_error_1 < frequency_tolerance_) {
      raw_decision = 1;
    } else if (energy_ratio > minimum_energy_ratio_) {
      raw_decision = 1;
    } else if (energy_ratio < 1.0 / minimum_energy_ratio_) {
      raw_decision = 0;
    }
    if (raw_decision < 0) {
      LOG_EVERY_N(INFO, 100) << "rejected window: amp=" << result.amplitude
          << " E0=" << result.energy_bit_0 << " E1=" << result.energy_bit_1
          << " Erest=" << result.energy_rest << " maxE=" << max_energy
          << " ratio=" << energy_ratio << " err0=" << freq_error_0 << " err1=" << freq_error_1;
    }
    if (raw_decision >= 0) {
      recent_decisions.push_back(raw_decision);
      if ((int)recent_decisions.size() > voting_window_size_) {
        recent_decisions.pop_front();
      }
      int current_bit = VotingDecision(recent_decisions);
      if (current_bit >= 0) {
        LOG(ERROR) << "valid bit " << current_bit << " amp=" << result.amplitude
                   << " E0=" << result.energy_bit_0 << " E1=" << result.energy_bit_1
                   << " Erest=" << result.energy_rest;
        last_byte |= (1 << last_byte_bit_count) * current_bit;
        ++last_byte_bit_count;
        ++current_bit_count;
        if (max_total_bits > 0 && current_bit_count >= max_total_bits) {
          LOG(INFO) << "Stopping decode after " << current_bit_count << " bits (limit)";
          if (last_byte_bit_count > 0) {
            set_next_byte(static_cast<char>(last_byte));
          }
          break;
        }
        if (last_byte_bit_count == 8) {
          set_next_byte(static_cast<char>(last_byte));
          LOG(ERROR) << "submit " << static_cast<char>(last_byte);
          last_byte = 0;
          last_byte_bit_count = 0;
        }
        recent_decisions.clear();
      }
    } else {
      if ((int)recent_decisions.size() >= voting_window_size_ / 2) {
        recent_decisions.pop_back();
        if (recent_decisions.empty()) {
          recent_decisions.push_back(-1);
        }
      }
    }
    samples_in_bit = 0;
    sample_buffer.clear();
  }
  LOG(INFO) << "Total bit count: " << current_bit_count;
  if (last_byte_bit_count > 0) {
    LOG(WARNING) << "Flushing incomplete byte at end: " << last_byte_bit_count << " bits";
    set_next_byte(static_cast<char>(last_byte));
  }
}

} // encoder