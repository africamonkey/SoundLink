// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <cstdint>
#include <vector>

namespace protocol {

constexpr uint8_t kPreambleByte = 0xAA;
constexpr uint8_t kFrameDelimiter = 0x55;
constexpr uint8_t kAckByte = 0x06;
constexpr uint8_t kNackByte = 0x15;

constexpr size_t kPreambleSize = 4;
constexpr size_t kLengthFieldSize = 2;
constexpr size_t kChecksumSize = 4;

struct FrameHeader {
  uint8_t preamble[kPreambleSize];
  uint8_t delimiter;
  uint16_t length;
  uint32_t checksum;
};

struct Frame {
  FrameHeader header;
  std::vector<char> payload;
};

class Protocol {
 public:
  Protocol();
  ~Protocol();

  std::vector<char> BuildFrame(const char* data, size_t length);
  bool ValidateFrame(const char* buffer, size_t buffer_size, std::vector<char>* payload);

  static uint32_t CalculateChecksum(const char* data, size_t length);

 private:
  bool FindPreamble(const char* buffer, size_t buffer_size, size_t* preamble_pos);
  bool ParseHeader(const char* buffer, size_t buffer_size, FrameHeader* header);
};

}  // namespace protocol
