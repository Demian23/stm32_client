#include "SerialPort.h"
#include "ErrnoException.h"

#ifdef _WIN32

SerialPort::SerialPort(std::string_view port, uint32_t baudRate) 
    : portDescriptor(INVALID_HANDLE_VALUE)
{
    portDescriptor = CreateFile(port.data(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (portDescriptor == INVALID_HANDLE_VALUE) {
        throw ErrnoException("Can't open port, WinAPI error", GetLastError());
    }
    DCB portParams{};
    if (!GetCommState(portDescriptor, &portParams)) {
        throw ErrnoException("Can't get port params, WinAPI error", GetLastError());
    }
    portParams.BaudRate = baudRate; // TODO check
    portParams.ByteSize = 8;
    portParams.StopBits = ONESTOPBIT;
    portParams.Parity = NOPARITY;
    portParams.fDtrControl = DTR_CONTROL_ENABLE; // TODO should enable?
    portParams.fBinary = TRUE;
    portParams.fRtsControl = RTS_CONTROL_ENABLE;
    portParams.fOutxCtsFlow = FALSE;
    portParams.fOutxDsrFlow = FALSE;
    portParams.fOutX = FALSE;
    portParams.fInX = FALSE;

    if (!SetCommState(portDescriptor, &portParams)) {
        throw ErrnoException("Can't set port params, WinAPI error", GetLastError());
    }
    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = MAXDWORD; // blocking 
    if (!SetCommTimeouts(portDescriptor, &timeouts)) {
        throw ErrnoException("Can't set port timeouts, WinAPI error", GetLastError());
    }
    PurgeComm(portDescriptor, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

SerialPort::~SerialPort()
{
    if (portDescriptor != INVALID_HANDLE_VALUE)
        CloseHandle(portDescriptor);
}

uint32_t SerialPort::write(const void *buffer, uint32_t size)
{
    DWORD result{};
    if(!WriteFile(portDescriptor, buffer, size, &result, nullptr))
        throw ErrnoException("Port write error, WinAPI error", GetLastError());
    return static_cast<uint32_t>(result);
}

uint32_t SerialPort::read(void *buffer, uint32_t size)
{
    DWORD result{};
    if(!ReadFile(portDescriptor, buffer, size, &result, nullptr))
        throw ErrnoException("Port read error, WinAPI error", GetLastError());
    return static_cast<uint32_t>(result);
}



#else

#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace {

speed_t speedCheck(size_t baudRate)
{
    speed_t speed;
    switch (baudRate) {
    case 230400:
        speed = B230400;
        break;

    case 115200:
        speed = B115200;
        break;

    case 57600:
        speed = B57600;
        break;

    case 38400:
        speed = B38400;
        break;

    case 19200:
        speed = B19200;
        break;

    case 9600:
        speed = B9600;
        break;

    case 4800:
        speed = B4800;
        break;

    case 2400:
        speed = B2400;
        break;

    case 1800:
        speed = B1800;
        break;

    case 1200:
        speed = B1200;
        break;

    case 600:
        speed = B600;
        break;

    case 300:
        speed = B300;
        break;
    default:
        throw std::logic_error("No such baud rate");
    }
    return speed;
}

void turnOffSpecialCharacters(termios &termiosStruct)
{
    termiosStruct.c_cc[VEOF] = _POSIX_VDISABLE;
    termiosStruct.c_cc[VEOL] = _POSIX_VDISABLE;
    termiosStruct.c_cc[VEOL2] = _POSIX_VDISABLE;
    termiosStruct.c_cc[VERASE] = _POSIX_VDISABLE;
    termiosStruct.c_cc[VWERASE] = _POSIX_VDISABLE;
    termiosStruct.c_cc[VKILL] = _POSIX_VDISABLE;
    termiosStruct.c_cc[VREPRINT] = _POSIX_VDISABLE;
    termiosStruct.c_cc[VINTR] = _POSIX_VDISABLE;
    termiosStruct.c_cc[VQUIT] = _POSIX_VDISABLE;
    termiosStruct.c_cc[VSUSP] = _POSIX_VDISABLE;
    termiosStruct.c_cc[VDSUSP] = _POSIX_VDISABLE;
    termiosStruct.c_cc[VSTART] = _POSIX_VDISABLE;
    termiosStruct.c_cc[VSTOP] = _POSIX_VDISABLE;
    termiosStruct.c_cc[VLNEXT] = _POSIX_VDISABLE;
    termiosStruct.c_cc[VDISCARD] = _POSIX_VDISABLE;
    termiosStruct.c_cc[VSTATUS] = _POSIX_VDISABLE;
}

} // namespace

SerialPort::SerialPort(std::string_view port, uint32_t baudRate)
    : portDescriptor(-1)
{
    // rd/wr, no control tty for process, blocking
    int descriptor = open(port.data(), O_RDWR | O_NOCTTY | O_NDELAY);

    if (descriptor != -1) {
        auto res = fcntl(descriptor, F_SETFL, 0); // blocking read
        if (res == -1)
            throw ErrnoException("Can't set blocking read");
        res = ioctl(descriptor, TIOCEXCL);
        if (res == -1)
            throw ErrnoException("Can't set exclusive use");
        termios options{};
        options.c_cflag |= (CREAD | CLOCAL);
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;
        speed_t speed = speedCheck(baudRate);
        res = cfsetspeed(&options, speed);
        if (res == -1)
            throw ErrnoException("Can't set output speed");
        res = cfsetispeed(&options, speed);
        if (res == -1)
            throw ErrnoException("Can't set input speed");
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_oflag &= ~OPOST;
        turnOffSpecialCharacters(options);
        options.c_cc[VMIN] = 0;  // set to header size?
        options.c_cc[VTIME] = 0; // timeout in 0.1 sec
        res = tcsetattr(descriptor, TCSANOW, &options);
        if (res == -1)
            throw ErrnoException("Can't set options");
        res = tcflush(descriptor, TCIOFLUSH);
        if (res == -1)
            throw ErrnoException("Can't flush");

        portDescriptor = descriptor;
    } else {
        throw ErrnoException("Can't open port");
    }
}

uint32_t SerialPort::write(const void *buffer, uint32_t size)
{
    auto result = ::write(portDescriptor, buffer, size);
    if(result != -1)
        return static_cast<uint32_t>(result);
    else
        throw ErrnoException("Can't write port");
}

uint32_t SerialPort::read(void *buffer, uint32_t size)
{
    auto result = ::read(portDescriptor, buffer, size);
    if(result != -1)
        return static_cast<uint32_t>(result);
    else
        throw ErrnoException("Can't write port");
}

SerialPort::~SerialPort()
{
    if (portDescriptor != -1) {
        // tcflush(portDescriptor, TCIOFLUSH);
        close(portDescriptor);
    }
}

SerialPort::SerialPort(SerialPort &&rhs) noexcept
    : portDescriptor(rhs.portDescriptor)
{
    rhs.portDescriptor = -1;
}

#endif
