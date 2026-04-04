# Real-Time Decoding Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Modify receiver to decode audio in real-time during capture, displaying decoded bytes immediately via typewriter effect.

**Architecture:** A decode thread runs concurrently with audio capture, reading from a lock-protected shared sample buffer and invoking `decoder_->Decode()` continuously. Each decoded byte triggers the `MessageCallback` immediately. The `ChirpEncoder::Decode()` is modified to support unlimited decoding (no `kMaxTotalBits` cap) and flush incomplete bytes at end.

**Tech Stack:** C++17, `std::thread`, `std::mutex`, `std::condition_variable`, Bazel

---

## File Map

| File | Change |
|------|--------|
| `src/receiver/receiver.h` | Add decode thread, mutex, cv, stop flag |
| `src/receiver/receiver.cpp` | Implement decode loop, thread-safe buffer access |
| `src/encoder/chirp_encoder.cc` | Remove `kMaxTotalBits` cap, add flush-incomplete-byte logic |
| `src/encoder/chirp_encoder.h` | Add `max_total_bits` parameter to `Decode()` |
| `src/receiver/receiver_main.cpp` | Typewriter-style message callback, blocking capture |

---

## Task 1: Add `max_total_bits` param to `EncoderBase::Decode()`

**Files:**
- Modify: `src/encoder/encoder_base.h:24-25`
- Modify: `src/encoder/chirp_encoder.h:20-21`
- Modify: `src/encoder/chirp_encoder.cpp:141-249`

- [ ] **Step 1: Modify `EncoderBase::Decode()` signature**

`src/encoder/encoder_base.h:24-25` — change:
```cpp
virtual void Decode(const std::function<bool(double*)>& get_next_audio_sample,
                    const std::function<void(char)>& set_next_byte) const = 0;
```
to:
```cpp
virtual void Decode(const std::function<bool(double*)>& get_next_audio_sample,
                    const std::function<void(char)>& set_next_byte,
                    int max_total_bits = 0) const = 0;
```
`max_total_bits = 0` means unlimited.

- [ ] **Step 2: Modify `ChirpEncoder::Decode()` signature in header**

`src/encoder/chirp_encoder.h:20-21` — change:
```cpp
void Decode(const std::function<bool(double*)>& get_next_audio_sample,
            const std::function<void(char)>& set_next_byte) const override;
```
to:
```cpp
void Decode(const std::function<bool(double*)>& get_next_audio_sample,
            const std::function<void(char)>& set_next_byte,
            int max_total_bits = 0) const override;
```

- [ ] **Step 3: Modify `ChirpEncoder::Decode()` implementation**

`src/encoder/chirp_encoder.cpp:141-249` — apply these changes:

1. Change signature to add `int max_total_bits` parameter.
2. In the `kInSync` state block, change:
   ```cpp
   if (current_bit_count >= kMaxTotalBits) {
     LOG(INFO) << "Stopping decode after " << current_bit_count << " bits";
     if (last_byte_bit_count > 0) {
       set_next_byte(static_cast<char>(last_byte));
     }
     break;
   }
   ```
   to:
   ```cpp
   if (max_total_bits > 0 && current_bit_count >= max_total_bits) {
     LOG(INFO) << "Stopping decode after " << current_bit_count << " bits (limit)";
     if (last_byte_bit_count > 0) {
       set_next_byte(static_cast<char>(last_byte));
     }
     break;
   }
   ```
3. At the end of function (before `LOG(INFO) << "Total bit count"`), change:
   ```cpp
   if (last_byte_bit_count > 0) {
     LOG(WARNING) << "Incomplete byte at end, discarding " << last_byte_bit_count << " bits";
   }
   ```
   to:
   ```cpp
   if (last_byte_bit_count > 0) {
     LOG(WARNING) << "Flushing incomplete byte at end: " << last_byte_bit_count << " bits";
     set_next_byte(static_cast<char>(last_byte));
   }
   ```
4. Remove `#include <deque>` if only used by `Decode` — check if other code uses it.

---

## Task 2: Add decode thread infrastructure to `Receiver`

**Files:**
- Modify: `src/receiver/receiver.h:1-38`
- Modify: `src/receiver/receiver.cpp:1-96`

- [ ] **Step 1: Modify `receiver.h` — add thread and sync primitives**

`src/receiver/receiver.h` — add to includes:
```cpp
#include <condition_variable>
#include <thread>
```

Add to private members of `Receiver` class:
```cpp
std::thread decode_thread_;
std::mutex buffer_mutex_;
std::condition_variable buffer_cv_;
bool stop_decode_ = false;
bool buffer_has_data_ = false;
```

Add new private method declaration:
```cpp
void DecodeLoop();
```

- [ ] **Step 2: Modify `receiver.cpp` — implement decode thread**

In `Receiver::Receiver()`, initialize new members:
```cpp
Receiver::Receiver(int audio_sample_rate, std::shared_ptr<encoder::EncoderBase> decoder)
    : decoder_(decoder),
      capturer_(new audio::AudioCapturer(audio_sample_rate)),
      stop_decode_(false),
      buffer_has_data_(false) {}
```

- [ ] **Step 3: Modify `StartCapture()` — spawn decode thread**

Change `src/receiver/receiver.cpp:33-43` to:
```cpp
bool Receiver::StartCapture() {
  auto audio_callback = [this](const float* samples, size_t num_samples) {
    {
      std::lock_guard<std::mutex> lock(buffer_mutex_);
      for (size_t i = 0; i < num_samples; ++i) {
        sample_buffer_.push_back(static_cast<double>(samples[i]));
      }
      buffer_has_data_ = true;
    }
    buffer_cv_.notify_one();
    LOG_EVERY_N(INFO, 100) << "Captured " << num_samples << " samples, buffer size: " << sample_buffer_.size();
  };

  LOG(INFO) << "Starting audio capture...";

  stop_decode_ = false;
  buffer_has_data_ = false;
  decode_thread_ = std::thread(&Receiver::DecodeLoop, this);

  capturer_->StartCapture(audio_callback);
  return true;
}
```

- [ ] **Step 4: Implement `DecodeLoop()`**

Add before `Close()` in `receiver.cpp`:
```cpp
void Receiver::DecodeLoop() {
  std::vector<double> local_buffer;

  auto get_next_audio_sample = [&local_buffer, this](double* next_sample) -> bool {
    std::unique_lock<std::mutex> lock(buffer_mutex_);
    while (!stop_decode_ && local_buffer.empty()) {
      buffer_cv_.wait(lock);
    }
    if (stop_decode_ && local_buffer.empty()) {
      return false;
    }
    *next_sample = local_buffer.back();
    local_buffer.pop_back();
    return true;
  };

  auto set_next_byte = [this](char byte) {
    if (message_callback_) {
      message_callback_(std::string(1, byte));
    }
  };

  while (!stop_decode_) {
    {
      std::lock_guard<std::mutex> lock(buffer_mutex_);
      if (!sample_buffer_.empty()) {
        local_buffer.insert(local_buffer.end(),
                            sample_buffer_.begin(),
                            sample_buffer_.end());
        sample_buffer_.clear();
        buffer_has_data_ = false;
      }
    }

    if (local_buffer.empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    std::vector<double> decode_buffer;
    decode_buffer.swap(local_buffer);

    std::reverse(decode_buffer.begin(), decode_buffer.end());

    size_t sample_index = 0;
    auto get_sample = [&decode_buffer, &sample_index](double* next_sample) -> bool {
      if (sample_index >= decode_buffer.size()) return false;
      *next_sample = decode_buffer[sample_index++];
      return true;
    };

    decoder_->Decode(get_sample, set_next_byte, 0);
  }

  LOG(INFO) << "Decode thread exiting";
}
```

**Note:** The `std::reverse` and `push_back` in lambda is a workaround so that `Decode()` — which consumes samples from back to front — gets samples in the correct order. The capture callback pushes to the back; we reverse before decoding so that the first captured sample is the first consumed.

Alternatively, restructure so decode reads from front of buffer (more natural):
- In `DecodeLoop`, copy samples and clear buffer, then use a forward iterator:
  ```cpp
  size_t sample_index = 0;
  auto get_sample = [&decode_buffer, &sample_index](double* next_sample) -> bool {
    if (sample_index >= decode_buffer.size()) return false;
    *next_sample = decode_buffer[sample_index++];
    return true;
  };
  ```
  (No reverse needed.) Adjust `DecodeLoop` accordingly.

- [ ] **Step 5: Modify `StopCapture()` — stop and join decode thread**

Replace `src/receiver/receiver.cpp:46-88` with:
```cpp
void Receiver::StopCapture() {
  capturer_->StopCapture();

  LOG(INFO) << "Capture stopped.";

  {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    stop_decode_ = true;
  }
  buffer_cv_.notify_one();

  if (decode_thread_.joinable()) {
    decode_thread_.join();
  }

  LOG(INFO) << "Total samples captured: " << sample_buffer_.size();

  double min_sample = *std::min_element(sample_buffer_.begin(), sample_buffer_.end());
  double max_sample = *std::max_element(sample_buffer_.begin(), sample_buffer_.end());
  LOG(INFO) << "Sample range: [" << min_sample << ", " << max_sample << "]";
}
```

- [ ] **Step 6: Modify `Close()`**

`src/receiver/receiver.cpp:90-94` — ensure decode thread is stopped:
```cpp
void Receiver::Close() {
  {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    stop_decode_ = true;
  }
  buffer_cv_.notify_one();
  if (decode_thread_.joinable()) {
    decode_thread_.join();
  }
  if (capturer_) {
    capturer_->Close();
  }
}
```

---

## Task 3: Update `receiver_main.cpp` for typewriter effect and blocking capture

**Files:**
- Modify: `src/receiver/receiver_main.cpp:1-60`

- [ ] **Step 1: Modify `receiver_main.cpp`**

Replace lines 38-44 with:
```cpp
receiver.SetMessageCallback([](const std::string& byte) {
    std::cout << byte << std::flush;
  });

  receiver.SetDataCallback([](const std::vector<char>& data) {
    // No-op in real-time mode; data is already displayed byte-by-byte
  });
```

Change lines 46-53 from:
```cpp
std::cout << "Starting capture for " << FLAGS_capture_duration_seconds << " seconds..." << std::endl;

  if (!receiver.StartCapture()) {
    std::cerr << "Failed to start capture" << std::endl;
    return 1;
  }

  sleep(FLAGS_capture_duration_seconds);

  receiver.StopCapture();
```
to:
```cpp
std::cout << "Starting real-time capture (Ctrl+C to stop)..." << std::endl;

  if (!receiver.StartCapture()) {
    std::cerr << "Failed to start capture" << std::endl;
    return 1;
  }

  std::cout << std::endl;

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
```

- [ ] **Step 2: Add `thread` header**

Add to includes in `receiver_main.cpp`:
```cpp
#include <thread>
```

- [ ] **Step 3: Remove `sleep()` and duration flag (optional cleanup)**

Keep `FLAGS_capture_duration_seconds` for backward compatibility but the loop above ignores it.

---

## Task 4: Build and verify

- [ ] **Step 1: Build**

```bash
bazel build //src/receiver:receiver_main
bazel build //src/receiver:receiver
bazel build //src/encoder:chirp_encoder
```

Expected: all pass.

- [ ] **Step 2: Run receiver test**

```bash
bazel test //src/receiver:receiver_test
bazel test //src/encoder:chirp_encoder_test
bazel test --config=asan //src/receiver:receiver_test
bazel test --config=asan //src/encoder:chirp_encoder_test
```

- [ ] **Step 3: Manual test (optional — requires encoder sending audio)**

Run encoder sending "Hello" repeatedly, then run receiver. Verify characters appear one-by-one in terminal during capture.

---

## Self-Review Checklist

1. **Spec coverage:** Real-time decode via background thread ✓, thread-safe buffer ✓, unlimited decode ✓, typewriter effect ✓, blocking capture until Ctrl+C ✓
2. **Placeholder scan:** No TBD/TODO in plan ✓, all code snippets are concrete ✓
3. **Type consistency:** `Decoder()` method signatures match between base and derived ✓, `Receiver` constructor initializes new members ✓
