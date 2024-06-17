#include "Channel.h"
#include "ErrnoException.h"
#include "Protocol.h"
#include <algorithm>

namespace smp {

static std::array<uint32_t, 4> handshakeBuffer{0xAE711707, 0xAE711707,
                                               0xAE711707, 0xAE711707};

constexpr auto handshakeBufferSizeInBytes =
    handshakeBuffer.size() * sizeof(uint32_t);

void Channel::handshake()
{
    auto res = port.write(handshakeBuffer.begin(), handshakeBufferSizeInBytes);
    if (res == -1) {
        throw ErrnoException("Can't write handshake");
    }
    if (res != handshakeBufferSizeInBytes) {
        throw std::logic_error(
            "Can't write serial port whole message, partly not implemented");
    }
}

void Channel::handshakeAnswer()
{
    constexpr auto buffSize = handshakeBufferSizeInBytes + sizeof(startWord) + sizeof(maxPacketSize) + sizeof(id);
    std::array<uint8_t, buffSize> buffer{}; // format handshake_start_word * 4,
                                      // start_word, packet_size, id
    auto res = port.read(buffer.begin(), buffer.size());
    if (res == -1) {
        throw ErrnoException("Can't read handshake answer");
    }
    if (res != buffSize) {
        throw std::logic_error(
            "Can't read serial port whole message, partly not implemented");
    }

    if (std::equal(buffer.begin(), buffer.begin() + handshakeBufferSizeInBytes,
                   handshakeBuffer.begin())) {
        // TODO byte ordering can fail here?
        startWord = *reinterpret_cast<uint32_t *>(
            (buffer.begin() + handshakeBufferSizeInBytes));
        maxPacketSize = *reinterpret_cast<uint16_t *>(
            (buffer.begin() + handshakeBufferSizeInBytes + sizeof(startWord)));
        id = *reinterpret_cast<uint16_t *>(
            (buffer.begin() + handshakeBufferSizeInBytes + sizeof(startWord) +
             sizeof(maxPacketSize)));
    } else {
        throw std::logic_error("Handshake answer first 16 bytes not equal");
    }
}

Channel::Channel(std::string_view portName, size_t baudRate)
    : port{portName, baudRate}, startWord{}, maxPacketSize{}, id{}
{}

void Channel::load(const void *buffer, size_t size)
{
    port.write(buffer, size);
    throw std::logic_error("Load action on channel not implemented");
}

void Channel::peripheral(const uint8_t *buffer, size_t size)
{
    static union {
        enum { static_buffer_size = 128 };
        std::array<uint8_t, static_buffer_size> buffer;
        struct {
            header packetHeader;
            std::array<uint8_t, static_buffer_size - sizeof(header)> data;
        } headerAndData;
    } packet;

    uint32_t packetLength = size + sizeof(header);
    packet.headerAndData.packetHeader = {.startWord = startWord,
                                         .packetLength = packetLength,
                                         .connectionId = id,
                                         .flags = action::peripheral
    };

    auto hash = djb2(packet.buffer.cbegin(), 10); // header before hash
    hash = djb2(buffer, size, hash);
    packet.headerAndData.packetHeader.hash = hash;

    std::copy(buffer, buffer + size, packet.headerAndData.data.begin());

    auto res = port.write(packet.buffer.begin(), packetLength);
    if (res == -1) {
        throw ErrnoException("Can't write serial port");
    }
    if (res != packetLength) {
        throw std::logic_error(
            "Can't write serial port whole message, partly not implemented");
    }
}

bool Channel::goodbye() noexcept
{
    union{
        header header;
        std::array<uint8_t, sizeof(header)> buffer{};
    }packet;
    packet.header = {.startWord = startWord, .packetLength = sizeof(header), .connectionId = id, .flags = action::goodbye};
    auto hash = djb2(packet.buffer.cbegin(), 10); // header before hash
    packet.header.hash = hash;

    auto res = port.write(packet.buffer.cbegin(), packet.buffer.size());
    return res == packet.buffer.size();
}

Channel::~Channel()
{
    goodbye(); // check return value and log?
}

std::string Channel::values()const{
    return std::to_string(startWord) + ' ' + std::to_string(maxPacketSize) + ' ' + std::to_string(id);
}

} // namespace smp