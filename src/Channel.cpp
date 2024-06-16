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
    std::array<uint8_t, 23> buffer{}; // format handshake_start_word * 4,
                                      // start_word, packet_size, id
    auto res = port.read(buffer.begin(), buffer.size());
    if (res == -1) {
        throw ErrnoException("Can't read handshake answer");
    }
    if (res != 23) {
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
        id = *reinterpret_cast<uint8_t *>(
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
    std::array<uint32_t, 4> goodbyeBuffer{handshakeBuffer};
    std::transform(goodbyeBuffer.begin(), goodbyeBuffer.end(),
                   goodbyeBuffer.begin(),
                   [start = this->startWord, clientId = this->id](
                       uint32_t word) { return word ^ start ^ clientId; });

    constexpr auto bufferSize = goodbyeBuffer.size() * sizeof(uint32_t);
    auto res = port.write(handshakeBuffer.begin(), bufferSize);
    return res == bufferSize;
}

Channel::~Channel()
{
    goodbye(); // check return value and log?
}

std::string Channel::values()const{
    return std::to_string(startWord) + ' ' + std::to_string(maxPacketSize) + ' ' + std::to_string(id);
}

} // namespace smp