// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/encoder/chirp_encoder.h"

#include <cmath>
#include <algorithm>
#include <numeric>

#include "glog/logging.h"

namespace encoder {

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
  reference_down_chirp_ = GenerateChirp(ChirpType::kDownChirp);
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

double ChirpEncoder::ComputeChirpCorrelation(const std::vector<double>& signal,
                                             const std::vector<double>& reference) const {
  if (signal.size() != reference.size()) {
    return -1.0;
  }
  double sum_product = 0.0;
  double sum_sq_signal = 0.0;
  double sum_sq_reference = 0.0;
  for (size_t i = 0; i < signal.size(); ++i) {
    sum_product += signal[i] * reference[i];
    sum_sq_signal += signal[i] * signal[i];
    sum_sq_reference += reference[i] * reference[i];
  }
  double denom = std::sqrt(sum_sq_signal * sum_sq_reference);
  if (denom < 1e-10) {
    return 0.0;
  }
  return sum_product / denom;
}

int ChirpEncoder::DetectChirpType(const std::vector<double>& samples) const {
  double corr_up = ComputeChirpCorrelation(samples, reference_up_chirp_);
  double corr_down = ComputeChirpCorrelation(samples, reference_down_chirp_);

  static int debug_counter = 0;
  if (debug_counter++ % 100 == 0) {
    LOG(INFO) << "Correlation: up=" << corr_up << ", down=" << corr_down
              << ", threshold=" << detection_threshold_;
  }

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

int ChirpEncoder::DecodeBitFromChirp(const std::vector<double>& samples) const {
  int type = DetectChirpType(samples);
  if (type < 0) {
    return -1;
  }
  return type;
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
  std::deque<double> sample_buffer;
  double next_sample;

  int sync_matched = 0;
  int consecutive_failures = 0;
  int samples_until_next_chirp = 0;
  int current_bit_count = 0;
  int last_byte = 0;
  int last_byte_bit_count = 0;

  const int kSyncConsecutiveFailuresLimit = 5;

  enum class State {
    kWaitingForSync,
    kInSync,
  };
  State state = State::kWaitingForSync;

  while (get_next_audio_sample(&next_sample)) {
    sample_buffer.push_back(next_sample);

    if (state == State::kWaitingForSync) {
      if ((int)sample_buffer.size() > chirp_samples_) {
        sample_buffer.pop_front();
      }
      if ((int)sample_buffer.size() < chirp_samples_) {
        continue;
      }

      std::vector<double> window_samples(sample_buffer.begin(), sample_buffer.end());
      int detected_type = DetectChirpType(window_samples);

      if (detected_type == 0) {
        LOG(INFO) << "Sync chirp " << sync_matched + 1 << " detected";
        ++sync_matched;
        consecutive_failures = 0;
        sample_buffer.clear();
        if (sync_matched >= sync_chirp_count_) {
          state = State::kInSync;
          samples_until_next_chirp = 0;
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
      if (samples_until_next_chirp > 0) {
        --samples_until_next_chirp;
        if ((int)sample_buffer.size() >= chirp_samples_) {
          sample_buffer.pop_front();
        }
        continue;
      }

      if ((int)sample_buffer.size() < chirp_samples_) {
        continue;
      }

      std::vector<double> window_samples(sample_buffer.begin(), sample_buffer.end());
      int detected_type = DetectChirpType(window_samples);

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

        sample_buffer.clear();
        samples_until_next_chirp = chirp_samples_ - 1;
      } else {
        sample_buffer.pop_front();
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
