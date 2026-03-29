// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/audio/audio_player.h"

#include "third_party/portaudio/portaudio.h"
#include <glog/logging.h>

namespace audio {

namespace {

constexpr int kFramesPerBuffer = 512;

}  // namespace

struct AudioPlayer::Impl {
  Impl() : float_buffer(nullptr) {}
  ~Impl() { delete[] float_buffer; }
  PaStream* stream = nullptr;
  const double* playback_buffer = nullptr;
  float* float_buffer = nullptr;
  size_t playback_index = 0;
  size_t playback_size = 0;
  bool is_playing = false;
};

AudioPlayer::AudioPlayer(int sample_rate, int channels)
    : sample_rate_(sample_rate), channels_(channels), impl_(new Impl) {}

AudioPlayer::~AudioPlayer() { Close(); }

bool AudioPlayer::Initialize() {
  PaError err = Pa_Initialize();
  if (err != paNoError) {
    LOG(ERROR) << "PortAudio initialization failed: " << Pa_GetErrorText(err);
    return false;
  }

  err = Pa_OpenDefaultStream(&impl_->stream, 0, channels_,
                             paFloat32, sample_rate_, kFramesPerBuffer,
                             nullptr, nullptr);
  if (err != paNoError) {
    LOG(ERROR) << "Failed to open PortAudio stream: " << Pa_GetErrorText(err);
    Pa_Terminate();
    return false;
  }

  return true;
}

bool AudioPlayer::Play(const std::vector<double>& samples) {
  return Play(samples.data(), samples.size());
}

bool AudioPlayer::Play(const double* samples, size_t num_samples) {
  if (!impl_->stream) {
    LOG(ERROR) << "AudioPlayer not initialized";
    return false;
  }

  delete[] impl_->float_buffer;
  impl_->float_buffer = new float[num_samples];
  for (size_t i = 0; i < num_samples; ++i) {
    impl_->float_buffer[i] = static_cast<float>(samples[i]);
  }

  impl_->playback_buffer = samples;
  impl_->playback_index = 0;
  impl_->playback_size = num_samples;

  PaError err = Pa_StartStream(impl_->stream);
  if (err != paNoError) {
    LOG(ERROR) << "Failed to start PortAudio stream: " << Pa_GetErrorText(err);
    return false;
  }

  while (Pa_IsStreamActive(impl_->stream)) {
    Pa_Sleep(10);
  }

  err = Pa_StopStream(impl_->stream);
  if (err != paNoError) {
    LOG(ERROR) << "Failed to stop PortAudio stream: " << Pa_GetErrorText(err);
    return false;
  }

  return true;
}

void AudioPlayer::Close() {
  if (impl_->stream) {
    Pa_CloseStream(impl_->stream);
    impl_->stream = nullptr;
  }
  Pa_Terminate();
}

}  // namespace audio
