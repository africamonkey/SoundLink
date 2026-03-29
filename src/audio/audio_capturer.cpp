// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/audio/audio_capturer.h"

#include "third_party/portaudio/portaudio.h"
#include <glog/logging.h>

namespace audio {

namespace {

constexpr int kFramesPerBuffer = 512;

}  // namespace

int CaptureCallback(const void* input_buffer, void* output_buffer,
                    unsigned long frames_per_buffer,
                    const PaStreamCallbackTimeInfo* time_info,
                    PaStreamCallbackFlags status_flags, void* user_data) {
  auto* capturer = static_cast<AudioCapturer*>(user_data);
  const float* input = static_cast<const float*>(input_buffer);

  if (capturer && input && capturer->impl_->callback) {
    capturer->impl_->callback(input, frames_per_buffer);
  }

  return paContinue;
}

AudioCapturer::AudioCapturer(int sample_rate, int channels)
    : sample_rate_(sample_rate), channels_(channels), impl_(new Impl) {}

AudioCapturer::~AudioCapturer() { Close(); }

bool AudioCapturer::Initialize() {
  PaError err = Pa_Initialize();
  if (err != paNoError) {
    LOG(ERROR) << "PortAudio initialization failed: " << Pa_GetErrorText(err);
    return false;
  }

  return true;
}

bool AudioCapturer::StartCapture(AudioCallback callback) {
  impl_->callback = callback;

  PaError err = Pa_OpenDefaultStream(&impl_->stream, channels_, 0,
                                     paFloat32, sample_rate_, kFramesPerBuffer,
                                     CaptureCallback, this);
  if (err != paNoError) {
    LOG(ERROR) << "Failed to open PortAudio capture stream: "
               << Pa_GetErrorText(err);
    return false;
  }

  err = Pa_StartStream(impl_->stream);
  if (err != paNoError) {
    LOG(ERROR) << "Failed to start PortAudio capture stream: "
               << Pa_GetErrorText(err);
    Pa_CloseStream(impl_->stream);
    impl_->stream = nullptr;
    return false;
  }

  return true;
}

void AudioCapturer::StopCapture() {
  if (impl_->stream) {
    Pa_StopStream(impl_->stream);
  }
}

void AudioCapturer::Close() {
  if (impl_->stream) {
    Pa_CloseStream(impl_->stream);
    impl_->stream = nullptr;
  }
  Pa_Terminate();
}

}  // namespace audio
