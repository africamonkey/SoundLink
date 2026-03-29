// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "denoiser_base.h"

#include "glog/logging.h"

namespace denoiser {

DenoiserBase::DenoiserBase(int audio_sample_rate) : audio_sample_rate_(audio_sample_rate) {
  CHECK_GT(audio_sample_rate_, 0);
}

}  // namespace denoiser
