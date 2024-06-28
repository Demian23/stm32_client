#pragma once
#include "BinMsg.h"
#include "LocalStatusCode.h"
#include "Msg.h"
#include "Protocol.h"
#include "SerialPort.h"
#include <cstddef>
#include <cstdint>
#include <string>

namespace smp {

struct ReadResult final {
    LocalStatusCode localCode;
    uint16_t answerSize;
};

class Channel final {
public:
    Channel(std::string_view portName, uint32_t baudRate);
    // handshake -> start word 4 times
    void handshake(); // start handshake word is 0xAE711707
    LocalStatusCode handshakeAnswer();
    bool goodbye() noexcept;
    void peripheral(LedMsg msg);
    // assumed that outBuffer is big enough
    ReadResult getHeaderedMsg(uint8_t *outBuffer, uint16_t bufferSize,
                              uint16_t requestFlags);
    // rewrite as coroutine?
    LocalStatusCode load(BinMsg &msg);
    void startLoad(BinMsg& msg); // possible change of prototype in favour of return LocalStatusCode (espcially if timeout would be implemented)
    void boot();

    ~Channel();

    [[nodiscard]] std::string values() const;

private:
    SerialPort port;
    uint32_t startWord;
    uint16_t maxPacketSize;
    uint16_t id;

    LocalStatusCode headerCheck(const smp::header *headerView,
                                uint16_t requestedFlags,
                                uint16_t buffSize) const;
};

} // namespace smp
