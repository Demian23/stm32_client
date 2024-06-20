#include "Channel.h"
#include "LocalStatusCode.h"
#include "Protocol.h"
#include <algorithm>
#include <cstdint>

namespace smp {

static std::array<uint8_t, 16> handshakeBuffer{
    0xAE, 0x71, 0x17, 0x07, 0xAE, 0x71, 0x17, 0x07,
    0xAE, 0x71, 0x17, 0x07, 0xAE, 0x71, 0x17, 0x07};

void Channel::handshake()
{
    auto res = port.write(handshakeBuffer.data(), handshakeBuffer.size());
    // 16 bytes should write at once
    if (res != handshakeBuffer.size()) {
        throw std::logic_error(
            "Can't write serial port whole message, partly not implemented");
    }
}

LocalStatusCode Channel::handshakeAnswer()
{
    constexpr auto buffSize = handshakeBuffer.size() + sizeof(startWord) +
                              sizeof(maxPacketSize) + sizeof(id);
    std::array<uint8_t, buffSize> buffer{}; // format handshake_start_word * 4,
                                            // start_word, packet_size, id
    uint32_t size = 0;
    // TODO add timeouts for read and write
    while (size != buffSize) {
        size += port.read(static_cast<void *>(buffer.data() + size),
                          buffer.size() - size);
    }

    if (std::equal(buffer.begin(), buffer.begin() + handshakeBuffer.size(),
                   handshakeBuffer.begin())) {
        // TODO byte ordering can fail here?
        // little endian assumed
        startWord = *reinterpret_cast<uint32_t *>(
            (buffer.data() + handshakeBuffer.size())); // no cast from begin to
                                                       // pointer here in msvc
        maxPacketSize = *reinterpret_cast<uint16_t *>(
            (buffer.data() + handshakeBuffer.size() + sizeof(startWord)));
        id = *reinterpret_cast<uint16_t *>(
            (buffer.data() + handshakeBuffer.size() + sizeof(startWord) +
             sizeof(maxPacketSize)));
        return LocalStatusCode::Ok;
    } else {
        return LocalStatusCode::HandshakeAnswerHeaderNotEqual;
    }
}

Channel::Channel(std::string_view portName, uint32_t baudRate)
    : port{portName, baudRate}, startWord{}, maxPacketSize{}, id{}
{}

void Channel::peripheral(LedMsg msg)
{
    BufferedLedPacket ledPacket;
    ledPacket.packet = {.baseHeader{.startWord = startWord,
                                    .packetLength = ledPacket.buffer.size(),
                                    .connectionId = id,
                                    .flags = action::peripheral},
                        .dev = peripheral_devices::LED,
                        .msg = msg};

    auto hash = djb2(ledPacket.buffer.data(), sizeBeforeHashField);
    hash = djb2(ledPacket.buffer.data() + sizeof(header),
                ledPacket.buffer.size() - sizeof(header), hash);
    ledPacket.packet.baseHeader.hash = hash;

    auto res = port.write(ledPacket.buffer.data(), ledPacket.buffer.size());
    if (res != ledPacket.buffer.size()) {
        throw std::logic_error(
            "Can't write serial port whole message, partly not implemented");
    }
}

ReadResult Channel::getHeaderedMsg(uint8_t *outBuffer, uint16_t bufferSize,
                                   uint16_t requestFlags)
{
    // wrong implementation
    ReadResult result{};
    uint16_t answerSize = 0;
    bool done = false;
    uint16_t readSize = sizeof(smp::header);
    const smp::header *headerView = nullptr;

    requestFlags |= 0x8000;

    if (outBuffer == nullptr || bufferSize == 0 || bufferSize < readSize) {
        throw std::logic_error("Nullptr, 0 or too small sized outBuffer");
    }

    while (!done) {
        answerSize += port.read(outBuffer + answerSize, readSize - answerSize);
        if (headerView == nullptr && answerSize == 16) {
            headerView = reinterpret_cast<const smp::header *>(outBuffer);
            auto statusCode = headerCheck(headerView, requestFlags, bufferSize);
            if (statusCode != LocalStatusCode::Ok) {
                result = {.localCode = statusCode, .answerSize = answerSize};
                readSize = headerView->packetLength;
                done = true;
            }
        }

        if (!done && headerView != nullptr && answerSize == readSize) {
            auto hash = djb2(outBuffer, sizeBeforeHashField);
            hash = djb2(outBuffer + sizeof(smp::header),
                        answerSize - sizeof(smp::header), hash);
            if (headerView->hash == hash) {
                result = {.localCode = LocalStatusCode::Ok,
                          .answerSize = answerSize};
                done = true;
            } else {
                result = {.localCode = LocalStatusCode::WrongHash,
                          .answerSize = answerSize};
            }
        }
    }
    return result;
}

bool Channel::goodbye() noexcept
{
    BufferedHeader packet;
    packet.header = {.startWord = startWord,
                     .packetLength = sizeof(header),
                     .connectionId = id,
                     .flags = action::goodbye};
    auto hash =
        djb2(packet.buffer.data(), sizeBeforeHashField); // header before hash
    packet.header.hash = hash;

    auto res = port.write(packet.buffer.data(), packet.buffer.size());
    return res == packet.buffer.size();
}

Channel::~Channel()
{
    goodbye(); // check return value and log?
}

std::string Channel::values() const
{
    return std::to_string(startWord) + ' ' + std::to_string(maxPacketSize) +
           ' ' + std::to_string(id);
}

LocalStatusCode Channel::headerCheck(const smp::header *headerView,
                                     uint16_t requestedFlags,
                                     uint16_t buffSize) const
{
    LocalStatusCode result;
    if (headerView->startWord == startWord) {
        if (headerView->connectionId == id) {
            if (headerView->flags == requestedFlags) {
                if (buffSize >= headerView->packetLength) {
                    result = LocalStatusCode::Ok;
                } else {
                    result = LocalStatusCode::BufferToSmall;
                }
            } else {
                result = LocalStatusCode::WrongFlags;
            }
        } else {
            result = LocalStatusCode::WrongId;
        }
    } else {
        result = LocalStatusCode::WrongStartWord;
    }
    return result;
}

LocalStatusCode Channel::load(BinMsg &msg)
{
    auto leftToWrite = msg.buffer.size() - msg.written;
    if (leftToWrite > 0) {
        BufferedLoadHeader packet;

        std::array<uint8_t, sizeof(LoadHeader)> answer;

        leftToWrite = leftToWrite > maxPacketSize - sizeof(LoadHeader)
                          ? maxPacketSize - sizeof(LoadHeader)
                          : leftToWrite;
        packet.header = {
            .baseHeader{.startWord = startWord,
                        .packetLength = static_cast<uint16_t>(
                            leftToWrite + sizeof(LoadHeader)),
                        .connectionId = id,
                        .flags = action::load},
            .msg = {.packetId = msg.nextPacketId,
                    .wholeMsgSize = static_cast<uint32_t>(msg.buffer.size())}};
        auto hash = djb2(packet.buffer.data(), sizeBeforeHashField);
        hash =
            djb2(packet.buffer.data() + sizeof(header), sizeof(LoadMsg), hash);
        hash = djb2(reinterpret_cast<const uint8_t *>(msg.buffer.data()) +
                        msg.written,
                    leftToWrite, hash);
        packet.header.baseHeader.hash = hash;

        auto headerLeft = sizeof(LoadHeader);
        auto currentPos = packet.buffer.data();
        while (headerLeft) {
            auto written = port.write(currentPos, headerLeft);
            currentPos += written;
            headerLeft -= written;
        }
        while (leftToWrite) {
            auto written =
                port.write(msg.buffer.data() + msg.written, leftToWrite);
            msg.written += written;
            leftToWrite -= written;
        }
        msg.nextPacketId += 1;

        // if success send header with flags high set and new hash
        // and if not send common header with StatusCode (size 18)
        // TODO currently not implemented
        auto result =
            getHeaderedMsg(answer.data(), answer.size(), action::load);
        if (result.localCode == LocalStatusCode::Ok &&
            result.answerSize == answer.size()) {
            packet.header.baseHeader.flags = 0x8000 + action::load;
            hash = djb2(packet.buffer.data(),
                        sizeBeforeHashField); // header before hash
            hash = djb2(packet.buffer.data() + sizeof(header), sizeof(LoadMsg),
                        hash);
            packet.header.baseHeader.hash = hash;
            if (std::equal(packet.buffer.cbegin(), packet.buffer.cend(),
                           answer.cbegin(), answer.cend())) {
                return LocalStatusCode::Ok;
            } else {
                return LocalStatusCode::LoadAnswerNotEqual;
            }
        } else {
            return result.localCode;
        }
    } else {
        return LocalStatusCode::NothingToWrite;
    }
}

} // namespace smp
