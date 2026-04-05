# AGENTS.md - SoundLink

Guidelines for agentic coding agents working on the SoundLink project.

## Permissions

### Allowed without prompting
- Read files, list directories
- Single file linting, type checking, formatting
- Unit tests on specific files

### Require approval first
- Package installations (`uv add`, `npm install`)
- Git operations (`git push`, `git commit`)
- File deletion
- Running full build or E2E test suites
- Terraform apply/destroy operations

## Build System

SoundLink uses Bazel as its build system with C++17 standard.

### Build Commands

```bash
bazel build //...
bazel build //path/to:target_name
bazel build --config=asan //...
```

### Test Commands

```bash
bazel test //...
bazel test //src/wav:wav_writer_test
bazel test --test_output=all //...
bazel test --config=asan --test_output=errors //...
```

### Environment Variables

- `TEST_FOLDER`: Test output directory (default: `/var/tmp/africamonkey`)

## Component Overview

The ThruAudioXfer project implements two main components:

### Encoder
- Encodes messages into audio signals
- Outputs to WAV files
- Plays encoded audio through speakers
- Located in `src/encoder/` directory
- Contains both Encode and Decode functionality (merged decoder into encoder)

### Audio Module
- Cross-platform audio playback and capture using PortAudio
- Located in `src/audio/` directory
- `AudioPlayer`: Plays audio samples through speakers
- `AudioCapturer`: Captures audio samples from microphone

### Sender
- High-level module for encoding and transmitting messages
- Located in `src/sender/` directory

### Receiver
- High-level module for receiving and decoding messages
- Located in `src/receiver/` directory

### Protocol
- Frame structure definitions for data transfer
- Located in `src/protocol/` directory

## Code Style Guidelines

### File Organization

- Headers: `#pragma once` for header guards
- Copyright notice: `// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved`
- Test files: `*_test.cpp` or `*_test.cc` in same directory as implementation
- Protocol buffer files: `*.proto` in `src/common/interface/proto/`
- Third-party headers: `third_party/<library>/<header>.h`

### Naming Conventions

- **Namespaces**: `lowercase` (e.g., `namespace wav`, `namespace io`)
- **Classes**: `PascalCase` (e.g., `WavWriter`, `WavHeaderBuilder`, `EncoderBase`)
- **Functions**: `PascalCase` (e.g., `AddSample`, `Write`)
- **Member variables**: `trailing_underscore_` (e.g., `filename_`, `sample_count_`)
- **Constants**: `kPascalCase` (e.g., `kEpsilon`, `kFrequencyDo`)
- **Template parameters**: `PascalCase` (e.g., `T`)
- **Builder methods**: Return `*this` for method chaining

### Imports

```cpp
// Standard library
#include <string>
#include <fstream>

// Third-party
#include "glog/logging.h"
#include "google/protobuf/message.h"
#include "gtest/gtest.h"

// Local headers (use 'src/' prefix)
#include "src/wav/wav_writer.h"
#include "src/common/file/io.h"
#include "src/common/math/math_utils.h"
```

Order: standard library, third-party, local headers (alphabetical within each group).

### Error Handling

Use glog CHECK macros for fatal errors:
```cpp
CHECK(outfile_.is_open()) << filename_;
CHECK_GT(sample_double, -1 - math::kEpsilon) << "sample out of range";
CHECK_EQ(wav_header_.num_channels, 2);
```

Return bool for non-fatal errors:
```cpp
bool ReadFromProtoInTextFormat(const std::string &text_file,
                                google::protobuf::Message *proto) {
  std::ifstream input(text_file.data());
  if (input.fail()) return false;
  // ... implementation
}
```

Use `MUST_USE_RESULT` attribute for bool return values that should be checked:
```cpp
MUST_USE_RESULT bool ReadFromProtoInTextFormat(const std::string &text_file,
                                                google::protobuf::Message *proto);
```

### Formatting

- No external formatting tools configured (no clang-format, clang-tidy, pre-commit hooks)
- Use existing code style as reference
- Ensure files end with a newline character

### Testing Guidelines

```cpp
#include "src/wav/wav_writer.h"
#include "gtest/gtest.h"
#include "src/common/file/io.h"

namespace wav {

TEST(ClassName, TestCaseDescription) {
  const std::string temp_filename = io::GenerateTestFolder() + "/test.wav";
  // Act and Assert
  EXPECT_EQ(actual, expected);
  ASSERT_TRUE(condition);
  // Cleanup
  io::DeleteFileIfExists(temp_filename);
}

}  // namespace wav
```

- Use `TEST` macro, `EXPECT_*` for non-fatal assertions, `ASSERT_*` for fatal
- Clean up temporary files using `io::DeleteFileIfExists()`
- Use `io::GenerateTestFolder()` for test file paths

### Dependencies

- **Google Test (gtest)**: Unit testing framework
- **Google Logging (glog)**: Logging library (CHECK macros)
- **Protocol Buffers**: Configuration/data serialization
- **PortAudio**: Cross-platform audio library
- **C++ filesystem**: File operations
- **Bazel rules_cc, rules_proto**: Build rules

### Testing Setup

All tests use `google_test_main` which initializes:
```cpp
::testing::InitGoogleTest(&argc, argv);
google::ParseCommandLineFlags(&argc, &argv, true);
google::InitGoogleLogging(argv[0]);
google::InstallFailureSignalHandler();
```

### Special Notes

- Sample data for audio is in range [-1, 1] for floating point representations
- WAV files support bit depths: 16, 24, 32 bits
- Supported channels: 1 (mono) or 2 (stereo)
- Use `std::clamp()` for value clamping, `constexpr` for compile-time constants
- Use `auto` judiciously when type is clear from context

### Build Files (BUILD)

Use `cc_library` for libraries, `cc_binary` for executables, `cc_test` for tests:
```python
cc_library(
    name = "wav_writer",
    srcs = ["wav_writer.cpp"],
    hdrs = ["wav_writer.h"],
    deps = [
        ":wav_header",
        "//src/common/interface/proto:cc_wav_params_proto",
        "//src/common/math:math_utils",
    ],
)

cc_test(
    name = "wav_writer_test",
    srcs = ["wav_writer_test.cpp"],
    data = ["//params:wav_params"],
    deps = [
        ":wav_writer",
        "//src/common/file:io",
        "//src/common/testing:google_test_main",
    ],
)
```

### Cross-Platform Considerations

- Use `select()` with `@platforms//os:macos` and `@platforms//os:linux` for OS-specific settings
- Third-party library paths differ between macOS (Homebrew) and Linux (system)
- Ensure all files end with a newline character
