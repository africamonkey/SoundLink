// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/encoder/chirp_encoder.h"

#include <cmath>
#include <algorithm>
#include <numeric>

#include "glog/logging.h"

#include "src/common/math/fft.h"
#include "src/common/math/math_utils.h"

namespace encoder {

bool ChirpEncoder::GetBatchAudioSamples(const std::function<bool(double*)>& get_next_audio_sample,
                                       std::vector<double>* batch) const {
  batch->resize(chirp_samples_);
  for (int i = 0; i < chirp_samples_; ++i) {
    if (!get_next_audio_sample(&(*batch)[i])) {
      batch->resize(i);
      return false;
    }
  }
  return true;
}

ChirpEncoder::ChirpEncoder(int audio_sample_rate, interface::EncoderParams encoder_params)
    : EncoderBase(audio_sample_rate, std::move(encoder_params)),
      audio_sample_rate_(audio_sample_rate) {
  CHECK(encoder_params_.has_chirp_encoder_params());
  const auto& params = encoder_params_.chirp_encoder_params();

  chirp_duration_ms_ = params.chirp_duration_ms() > 0 ? params.chirp_duration_ms() : 15.0;
  frequency_low_ = params.frequency_low() > 0 ? params.frequency_low() : 500.0;
  frequency_high_ = params.frequency_high() > 0 ? params.frequency_high() : 2000.0;
  sync_chirp_count_ = params.sync_chirp_count() > 0 ? params.sync_chirp_count() : 2;
  detection_threshold_ = params.detection_threshold() > 0 ? params.detection_threshold() : 0.4;
  correlation_window_size_ = params.correlation_window_size() > 0 ? params.correlation_window_size() : 256;

  chirp_samples_ = static_cast<int>(audio_sample_rate_ * chirp_duration_ms_ / 1000.0);

  CHECK_GT(chirp_samples_, 0);
  CHECK_GT(frequency_high_, frequency_low_);

  GenerateReferenceChirps();
  LOG(INFO) << "ChirpEncoder initialized: duration=" << chirp_duration_ms_
            << "ms, samples=" << chirp_samples_
            << ", freq_range=[" << frequency_low_ << ", " << frequency_high_ << "]";
}

void ChirpEncoder::GenerateReferenceChirps() {
  reference_up_chirp_ = GenerateChirp(ChirpType::kUpChirp);
  reversed_reference_up_chirp_ = reference_up_chirp_;
  std::reverse(reversed_reference_up_chirp_.begin(), reversed_reference_up_chirp_.end());
  sum_sq_reference_up_chirp_ = 0.0;
  for (int i = 0; i < reference_up_chirp_.size(); ++i) {
    sum_sq_reference_up_chirp_ += math::Sqr(reference_up_chirp_[i]);
  }

  reference_down_chirp_ = GenerateChirp(ChirpType::kDownChirp);
  reversed_reference_down_chirp_ = reference_down_chirp_;
  std::reverse(reversed_reference_down_chirp_.begin(), reversed_reference_down_chirp_.end());
  sum_sq_reference_down_chirp_ = 0.0;
  for (int i = 0; i < reference_down_chirp_.size(); ++i) {
    sum_sq_reference_down_chirp_ += math::Sqr(reference_down_chirp_[i]);
  }
}

std::vector<double> ChirpEncoder::GenerateChirp(ChirpType type) const {
  std::vector<double> chirp(chirp_samples_);
  double duration = static_cast<double>(chirp_samples_) / audio_sample_rate_;
  double freq_range = frequency_high_ - frequency_low_;
  double freq_rate = freq_range / duration;

  for (int i = 0; i < chirp_samples_; ++i) {
    double t = static_cast<double>(i) / audio_sample_rate_;
    double phase;
    if (type == ChirpType::kUpChirp) {
      phase = 2.0 * M_PI * (frequency_low_ * t + 0.5 * freq_rate * t * t);
    } else {
      phase = 2.0 * M_PI * (frequency_high_ * t - 0.5 * freq_rate * t * t);
    }
    chirp[i] = std::sin(phase);
  }
  return chirp;
}

int ChirpEncoder::DetectChirpType(double corr_up, double corr_down) const {
  if (corr_up > corr_down && corr_up > detection_threshold_) {
    return 0;
  } else if (corr_down > corr_up && corr_down > detection_threshold_) {
    return 1;
  }
  return -1;
}

bool ChirpEncoder::DetectSyncHeader() const {
  return true;
}

void ChirpEncoder::Encode(const std::function<bool(char *)> &get_next_byte,
                          const std::function<void(double)> &set_next_audio_sample) const {
  char next_byte;

  for (int i = 0; i < audio_sample_rate_; ++i) {
    set_next_audio_sample(0.0);
  }

  for (int sync_idx = 0; sync_idx < sync_chirp_count_; ++sync_idx) {
    for (double sample : reference_up_chirp_) {
      set_next_audio_sample(sample);
    }
  }

  while (get_next_byte(&next_byte)) {
    int byte_val = static_cast<int>(static_cast<unsigned char>(next_byte));
    for (int bit_pos = 0; bit_pos < 8; ++bit_pos) {
      int bit = (byte_val >> bit_pos) & 1;
      const std::vector<double>& chirp = (bit == 0) ? reference_up_chirp_ : reference_down_chirp_;
      for (double sample : chirp) {
        set_next_audio_sample(sample);
      }
    }
  }

  for (int i = 0; i < audio_sample_rate_ / 2; ++i) {
    set_next_audio_sample(0.0);
  }
}

void ChirpEncoder::Decode(const std::function<bool(double*)>& get_next_audio_sample,
                           const std::function<void(char)>& set_next_byte) const {
  std::vector<double> sample_buffer;
  std::vector<double> batch;

  int sync_matched = 0;
  int consecutive_failures = 0;
  int current_bit_count = 0;
  int last_byte = 0;
  int last_byte_bit_count = 0;

  const int kSyncConsecutiveFailuresLimit = 5;

  enum class State {
    kWaitingForSync,
    kInSync,
  };
  State state = State::kWaitingForSync;

  while (GetBatchAudioSamples(get_next_audio_sample, &batch)) {
    sample_buffer.insert(sample_buffer.end(), batch.begin(), batch.end());
    while (sample_buffer.size() >= chirp_samples_) {
      std::vector<double> conv_up = math::ComputeConvolution(sample_buffer, reversed_reference_up_chirp_);
      std::vector<double> conv_down = math::ComputeConvolution(sample_buffer, reversed_reference_down_chirp_);
      double sum_sq_sample_buffer = 0.0;
      for (int i = 0; i + 1 < chirp_samples_; ++i) {
        sum_sq_sample_buffer += math::Sqr(sample_buffer[i]);
      }
      int should_clean_buffer_to_idx = -1;
      for (int i = chirp_samples_ - 1; i < sample_buffer.size(); ++i) {
        sum_sq_sample_buffer += math::Sqr(sample_buffer[i]);

        double denom_up = std::sqrt(sum_sq_sample_buffer * sum_sq_reference_up_chirp_);
        double denom_down = std::sqrt(sum_sq_sample_buffer * sum_sq_reference_down_chirp_);
        double corr_up = denom_up < math::kEpsilon ? 0.0 : conv_up[i] / denom_up;
        double corr_down = denom_down < math::kEpsilon ? 0.0 : conv_down[i] / denom_down;
        int detected_type = DetectChirpType(corr_up, corr_down);

        if (state == State::kWaitingForSync) {
          if (detected_type == 0) {
            LOG(INFO) << "Sync chirp " << sync_matched + 1 << " detected";
            ++sync_matched;
            consecutive_failures = 0;
            should_clean_buffer_to_idx = i;
            if (sync_matched >= sync_chirp_count_) {
              state = State::kInSync;
              LOG(INFO) << "Sync complete, starting data decode";
            }
          } else {
            if (sync_matched > 0) {
              ++consecutive_failures;
              if (consecutive_failures >= kSyncConsecutiveFailuresLimit) {
                LOG(WARNING) << "Sync lost after " << consecutive_failures << " failures, resetting";
                sync_matched = 0;
                consecutive_failures = 0;
              }
            }
          }
        } else if (state == State::kInSync) {
          if (detected_type >= 0) {
            int bit = detected_type;
            last_byte |= (bit << last_byte_bit_count);
            ++last_byte_bit_count;
            ++current_bit_count;
            LOG(INFO) << "Decoded bit " << bit << " (total: " << current_bit_count
                      << "), byte progress: " << last_byte_bit_count;
            if (last_byte_bit_count == 8) {
              set_next_byte(static_cast<char>(last_byte));
              LOG(INFO) << "Output byte: " << static_cast<char>(last_byte);
              last_byte = 0;
              last_byte_bit_count = 0;
            }
            should_clean_buffer_to_idx = i;
          }
        }
        if (should_clean_buffer_to_idx != -1) {
          break;
        }

        sum_sq_sample_buffer -= math::Sqr(sample_buffer[i - chirp_samples_ + 1]);
      }
      if (should_clean_buffer_to_idx != -1) {
        sample_buffer.erase(sample_buffer.begin(), sample_buffer.begin() + should_clean_buffer_to_idx);
      } else {
        sample_buffer.erase(sample_buffer.begin(), sample_buffer.end() - (chirp_samples_ - 1));
      }
    }
  }

  LOG(INFO) << "Total bit count: " << current_bit_count;
  if (last_byte_bit_count > 0) {
    LOG(WARNING) << "Flushing incomplete byte at end: " << last_byte_bit_count << " bits";
    set_next_byte(static_cast<char>(last_byte));
  }
}

}  // namespace encoder
