# SoundLink

[![unit-test](https://github.com/africamonkey/ThruAudioXfer/actions/workflows/unit-test.yml/badge.svg?branch=master)](https://github.com/africamonkey/ThruAudioXfer/actions/workflows/unit-test.yml)

SoundLink enables data transfer through audio signals. It encodes messages into audio waveforms and decodes them back, using the speaker and microphone as the transmission medium.

## Demo

https://github.com/user-attachments/assets/e479d578-9a5a-4c79-8ab0-255910f1575a

## Architecture

```
Sender                          Receiver
  |                                ^
  |  Encode + Play                 |  Capture + Decode
  v                                |
[Encoder] ---- audio -----> [Encoder]
```

## Quick Start

### Dependencies

Make sure that all the required packages are installed (on Debian):
```bash
sudo apt-get update
sudo apt-get install -y portaudio19-dev
```

### Sender
```bash
bazel run //src/sender:sender_main
```

### Receiver
```bash
bazel run //src/receiver:receiver_main
```

### Configuration

Parameters are defined in `params/encoder_params_prod.txt`.

## Build

```bash
# Build all targets
bazel build //...

# Build specific target
bazel build //src/demo

# Build with AddressSanitizer
bazel build --config=asan //...
```

## Test

```bash
# Run all tests
bazel test //...

# Run specific test
bazel test //src/wav:wav_writer_test

# Run with full output
bazel test --test_output=all //...

# Run with AddressSanitizer
bazel test --config=asan --test_output=errors //...
```

## Components

| Component | Description |
|-----------|-------------|
| **Encoder** | Encodes/decodes data to/from audio samples. Supports multiple algorithms (simple, Goertzel, chirp). |
| **Audio** | Cross-platform audio playback and capture using PortAudio. |
| **Sender** | High-level module that encodes and transmits messages via speakers. |
| **Receiver** | High-level module that captures audio and decodes messages via microphone. |
| **Protocol** | Frame structure with preamble, length, checksum for reliable transmission. |
| **WAV** | WAV file read/write utilities for audio file handling. |

## Dependencies

- **Bazel** - Build system
- **PortAudio** - Cross-platform audio I/O (`sudo apt-get install portaudio19-dev`)
- **Google Test** - Unit testing
- **Google Logging** - Logging
- **Protocol Buffers** - Data serialization


