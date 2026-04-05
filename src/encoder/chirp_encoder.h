// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <vector>
#include <complex>
#include <functional>

#include "src/encoder/encoder_base.h"

namespace encoder {

class ChirpEncoder final : public EncoderBase {
 public:
  ChirpEncoder(int audio_sample_rate, interface::EncoderParams encoder_params);

  void Encode(const std::function<bool(char *)> &get_next_byte,
              const std::function<void(double)> &set_next_audio_sample) const override;

  void Decode(const std::function<bool(double*)>& get_next_audio_sample,
              const std::function<void(char)>& set_next_byte) const override;

 private:
  enum class ChirpType {
    kUpChirp,
    kDownChirp,
  };

  bool GetBatchAudioSamples(const std::function<bool(double*)>& get_next_audio_sample,
                            std::vector<double>* batch) const;

  void GenerateReferenceChirps();
  std::vector<double> GenerateChirp(ChirpType type) const;
  double ComputeChirpCorrelation(const std::vector<double>& signal,
                                 const std::vector<double>& reference) const;
  int DetectChirpType(const std::vector<double>& samples) const;
  bool DetectSyncHeader() const;
  int DecodeBitFromChirp(const std::vector<double>& samples) const;

  double chirp_duration_ms_;
  double frequency_low_;
  double frequency_high_;
  int sync_chirp_count_;
  double detection_threshold_;
  int correlation_window_size_;
  int chirp_samples_;
  int audio_sample_rate_;

  std::vector<double> reference_up_chirp_;
  std::vector<double> reference_down_chirp_;
};

}  // namespace encoder
