#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#ifdef _WIN32
#include <windows.h>
#endif

class SerialPort final {
public:
    SerialPort(std::string_view port, uint32_t baudRate);
    SerialPort(SerialPort &&rhs) noexcept;

    SerialPort &operator=(SerialPort &&rhs) = delete;
    SerialPort(const SerialPort &) = delete;
    SerialPort &operator=(const SerialPort &) = delete;

    int32_t write(const void *buffer, int32_t size);
    int32_t read(void *buffer, int32_t size);
    ~SerialPort();

private:
#ifdef _WIN32
    HANDLE portDescriptor;
#else
    int portDescriptor;
#endif
};
