#pragma once

#include "Msg.h"
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

// on success send header only

struct header {
    uint32_t startWord;    // smth as 0xFCD1A612
    uint32_t packetLength; // header + data
    uint16_t connectionId;
    uint16_t flags; // action only now, if success sender set high bit
    uint32_t hash;  // of header + data
    // data -> byte array that depends on flags
};

static_assert(sizeof(header) == 16, "No packing required");

constexpr auto sizeBeforeHashField = sizeof(header) - sizeof(uint32_t);

inline constexpr uint32_t djb2(const uint8_t *buffer, size_t size,
                               uint32_t start = 5381) noexcept
{
    for (int i = 0; i < size; i++) {
        start = ((start << 5) + start) + buffer[i];
    }
    return start;
}

enum StatusCode : uint16_t { Invalid, Ok, NoSuchCommand, NoSuchDevice };

#pragma pack(push, 2)
struct LedPacket {
    header baseHeader;
    peripheral_devices dev;
    LedMsg msg;
};
#pragma pack(pop)

static_assert(sizeof(LedPacket) ==
              sizeof(header) + sizeof(LedMsg) + sizeof(peripheral_devices));

struct LoadHeader {
    header baseHeader;
    LoadMsg msg;
};

static_assert(sizeof(LoadHeader) == sizeof(header) + sizeof(LoadMsg));

union BufferedHeader {
    header header;
    std::array<uint8_t, sizeof(header)> buffer;
};

union BufferedLedPacket {
    LedPacket packet;
    std::array<uint8_t, sizeof(LedPacket)> buffer;
};

union BufferedLoadHeader {
    LoadHeader header;
    std::array<uint8_t, sizeof(header)> buffer;
};

} // namespace smp
