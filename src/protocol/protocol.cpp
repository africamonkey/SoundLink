// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/protocol/protocol.h"

#include <cstring>

#include "glog/logging.h"

namespace protocol {

Protocol::Protocol() {}

Protocol::~Protocol() {}

std::vector<char> Protocol::BuildFrame(const char* data, size_t length) {
  std::vector<char> frame;

  FrameHeader header;
  std::memset(&header, 0, sizeof(header));

  for (size_t i = 0; i < kPreambleSize; ++i) {
    header.preamble[i] = kPreambleByte;
  }
  header.delimiter = kFrameDelimiter;
  header.length = static_cast<uint16_t>(length);
  header.checksum = CalculateChecksum(data, length);

  frame.resize(sizeof(FrameHeader));
  std::memcpy(frame.data(), &header, sizeof(FrameHeader));

  frame.insert(frame.end(), data, data + length);

  return frame;
}

bool Protocol::ValidateFrame(const char* buffer, size_t buffer_size, std::vector<char>* payload) {
  if (buffer_size < sizeof(FrameHeader)) {
    return false;
  }

  FrameHeader header;
  std::memcpy(&header, buffer, sizeof(FrameHeader));

  for (size_t i = 0; i < kPreambleSize; ++i) {
    if (header.preamble[i] != kPreambleByte) {
      return false;
    }
  }

  if (header.delimiter != kFrameDelimiter) {
    return false;
  }

  size_t total_frame_size = sizeof(FrameHeader) + header.length;
  if (buffer_size < total_frame_size) {
    return false;
  }

  const char* payload_data = buffer + sizeof(FrameHeader);
  uint32_t expected_checksum = CalculateChecksum(payload_data, header.length);

  if (header.checksum != expected_checksum) {
    LOG(ERROR) << "Checksum mismatch: expected " << expected_checksum
               << ", got " << header.checksum;
    return false;
  }

  payload->resize(header.length);
  std::memcpy(payload->data(), payload_data, header.length);

  return true;
}

uint32_t Protocol::CalculateChecksum(const char* data, size_t length) {
  uint32_t checksum = 0;
  for (size_t i = 0; i < length; ++i) {
    checksum = (checksum << 7) | (checksum >> 25);
    checksum += static_cast<uint8_t>(data[i]);
  }
  return checksum;
}

bool Protocol::FindPreamble(const char* buffer, size_t buffer_size, size_t* preamble_pos) {
  if (buffer_size < kPreambleSize) {
    return false;
  }

  for (size_t i = 0; i <= buffer_size - kPreambleSize; ++i) {
    bool found = true;
    for (size_t j = 0; j < kPreambleSize; ++j) {
      if (static_cast<uint8_t>(buffer[i + j]) != kPreambleByte) {
        found = false;
        break;
      }
    }
    if (found) {
      *preamble_pos = i;
      return true;
    }
  }
  return false;
}

bool Protocol::ParseHeader(const char* buffer, size_t buffer_size, FrameHeader* header) {
  if (buffer_size < sizeof(FrameHeader)) {
    return false;
  }
  std::memcpy(header, buffer, sizeof(FrameHeader));
  return true;
}

}  // namespace protocol
