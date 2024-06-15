#pragma once
#include "SerialPort.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace smp {

class Channel final {
public:
  Channel(std::string_view portName, size_t baudRate);
  // handshake -> start word 4 times
  void handshake();        // start handshake word is 0xAE711707
  bool goodbye() noexcept; // start_handshake_word ^ start_word ^ id
  void peripheral(const uint8_t *buffer, size_t size);
  void load(const void *buffer, size_t size);
  ~Channel();

private:
  SerialPort port;
  uint32_t startWord;
  uint16_t maxPacketSize;
  uint8_t id;
};

} // namespace smp
