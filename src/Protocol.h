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


struct header {
    uint32_t startWord;    // smth as 0xFCD1A612
    uint32_t packetLength; // header + data
    uint16_t connectionId;
    uint16_t flags; // action only now
    uint32_t hash; // of header + data
    // data -> byte array that depends on flags
};

inline constexpr uint32_t djb2(const uint8_t* buffer, size_t size, uint32_t start = 5381) noexcept
{
    for(int i = 0; i < size; i++){
        start = ((start << 5) + start) + buffer[size];
    }
    return start;
}

} // namespace smp