# Real-Time Decoding Design

## Overview

Modify `receiver_main` to perform real-time decoding: as audio is being captured, decode and display text incrementally (typewriter effect) instead of decoding after capture completes.

## Architecture

### Components

1. **Capture Thread** (existing): Continuously captures audio samples into a shared buffer via `AudioCapturer`.
2. **Decode Thread** (new): Continuously reads from the shared buffer, calls `decoder_->Decode()`, and invokes callbacks for each decoded byte.
3. **Main Thread**: Registers callbacks and manages capture lifecycle.

### Data Flow

```
AudioCapturer -> shared_sample_buffer (lock-protected)
                    |
                    v
           Decode Thread -> decoder_->Decode()
                    |
                    v
           MessageCallback (per-byte, real-time)
```

### Thread Safety

- `sample_buffer_` is protected by a `std::mutex`.
- `std::condition_variable` signals when new samples are available.
- Decode thread waits on the condition variable when buffer is empty.

## Changes

### `src/receiver/receiver.h`

- Add `std::thread decode_thread_`
- Add `std::mutex buffer_mutex_`
- Add `std::condition_variable buffer_cv_`
- Add `bool stop_decode_` flag
- Add `bool buffer_has_data_` flag (to avoid spurious wakeups)

### `src/receiver/receiver.cpp`

**`StartCapture()` changes:**
- Spawn `decode_thread_` that runs `DecodeLoop()`.
- Start capture as before (capture thread fills buffer).

**New `DecodeLoop()` method:**
- Repeatedly:
  1. Lock mutex, wait if buffer empty until `stop_decode_` or data available.
  2. If `stop_decode_` and buffer empty, break.
  3. Copy samples out of buffer into a local vector.
  4. Unlock mutex.
  5. Call `decoder_->Decode()` with the copied samples.
  6. On each `set_next_byte` callback, invoke `message_callback_` immediately.

**`StopCapture()` changes:**
- Signal decode thread to stop (`stop_decode_ = true`, notify cv).
- Join decode thread.
- Clear remaining buffer.

### `src/encoder/chirp_encoder.cc`

**`Decode()` changes:**
- Add parameter `max_total_bits = 0` meaning unlimited (keep decoding until samples exhausted).
- When `max_total_bits == 0`, remove the `kMaxTotalBits` early-exit check (line 222-228).
- When samples exhausted, flush any incomplete byte (warn if incomplete).

### `src/receiver/receiver_main.cpp`

- Change `MessageCallback` to print each byte immediately with `std::cout << byte` and `std::cout.flush()`.
- Remove `sleep()` and duration-based capture; use blocking capture until user interrupts (Ctrl+C).

## Error Handling

- If `decoder_->Decode()` throws or returns early due to decode errors, log and continue (skip bad samples).
- If decode thread encounters an error, set `stop_decode_` and let main thread handle cleanup.

## Testing

- Run receiver with a live encoder transmitting. Verify bytes appear in terminal as they are decoded (typewriter effect).
- Verify no memory leaks or data races with `--config=asan` build.
