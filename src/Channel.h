#pragma once
#include "SerialPort.h"
#include "Protocol.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace smp {

class Channel final {
public:
    Channel(std::string_view portName, uint32_t baudRate);
    // handshake -> start word 4 times
    void handshake(); // start handshake word is 0xAE711707
    void handshakeAnswer();
    bool goodbye() noexcept;
    void peripheral(const uint8_t *buffer, uint16_t size);
    uint16_t getAnswer(uint8_t* buffer, uint16_t size, smp::action on);
    void load(const void *buffer, uint32_t size);
    ~Channel();

    [[nodiscard]] std::string values()const;

private:
    SerialPort port;
    uint32_t startWord;
    uint16_t maxPacketSize;
    uint16_t id;
};

} // namespace smp
