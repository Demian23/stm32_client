#pragma once

#include <array>
#include <cstdint>

// stm32_manage_protocol
namespace smp {
enum action : uint8_t {
  handshake = 0, // return id, startWord, desirable size of packet
  peripheral,
  load,
  goodbye
};

enum peripheral : uint8_t { LED };

struct header {
  uint32_t startWord;    // smth as 0xFCD1A612
  uint32_t packetLength; // header + data
  uint8_t connectionId;
  uint8_t flags; // action only now
  std::array<uint8_t, 2>
      hash; // hash for header + data, maybe 6 bytes for header of 16 bytes
            // data -> byte array that depends on flags
};
} // namespace smp