#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

class SerialPort final {
public:
  SerialPort(std::string_view port, size_t baudRate);

  SerialPort(SerialPort &&) = delete;
  SerialPort &operator=(SerialPort &&) = delete;
  SerialPort(const SerialPort &) = delete;
  SerialPort &operator=(const SerialPort &) = delete;

  ssize_t write(const void *buffer, size_t size);
  ssize_t read(void *buffer, size_t size);
  ~SerialPort();

private:
  int portDescriptor;
};
