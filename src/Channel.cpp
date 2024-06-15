#include "Channel.h"
#include "ErrnoException.h"
#include "Protocol.h"
#include <algorithm>

namespace smp {

static std::array<uint32_t, 4> handshakeBuffer{0xAE711707, 0xAE711707,
                                               0xAE711707, 0xAE711707};

void Channel::handshake() {
  constexpr auto bufferSize = handshakeBuffer.size() * sizeof(uint32_t);
  auto res = port.write(handshakeBuffer.begin(), bufferSize);
  if (res == -1) {
    throw ErrnoException("Can't write handshake");
  }
  if (res != bufferSize) {
    throw std::logic_error(
        "Can't write serial port whole message, partly not implemented");
  }
}

Channel::Channel(std::string_view portName, size_t baudRate)
    : port{portName, baudRate}, startWord{}, maxPacketSize{}, id{} {}

void Channel::load(const void *buffer, size_t size) {
  port.write(buffer, size);
  throw std::logic_error("Load action on channel not implemented");
}

void Channel::peripheral(const uint8_t *buffer, size_t size) {
  static union {
    enum { static_buffer_size = 128 };
    std::array<uint8_t, static_buffer_size> buffer;
    struct {
      header packetHeader;
      std::array<uint8_t, static_buffer_size - sizeof(header)> data;
    } headerAndData;
  } packet;
  uint32_t packetLength = size + sizeof(header);
  // TODO hash calculation
  packet.headerAndData.packetHeader = {.startWord = startWord,
                                       .packetLength = packetLength,
                                       .connectionId = id,
                                       .flags = action::peripheral};
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

bool Channel::goodbye() noexcept {
  std::array<uint32_t, 4> goodbyeBuffer{handshakeBuffer};
  std::transform(goodbyeBuffer.begin(), goodbyeBuffer.end(),
                 goodbyeBuffer.begin(),
                 [start = this->startWord, clientId = this->id](uint32_t word) {
                   return word ^ start ^ clientId;
                 });

  constexpr auto bufferSize = goodbyeBuffer.size() * sizeof(uint32_t);
  auto res = port.write(handshakeBuffer.begin(), bufferSize);
  return res == bufferSize;
}

Channel::~Channel() {
  goodbye(); // check return value and log?
}

} // namespace smp