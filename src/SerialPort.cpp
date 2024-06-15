#include "SerialPort.h"
#include "ErrnoException.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace {

speed_t speedCheck(size_t baudRate) {
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

void turnOffSpecialCharacters(termios &termiosStruct) {
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

SerialPort::SerialPort(std::string_view port, size_t baudRate)
    : portDescriptor(-1) {
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

ssize_t SerialPort::write(const void *buffer, size_t size) {
  return ::write(portDescriptor, buffer, size);
}

ssize_t SerialPort::read(void *buffer, size_t size) {
  return ::read(portDescriptor, buffer, size);
}

SerialPort::~SerialPort() {
  if (portDescriptor != -1) {
    // tcflush(portDescriptor, TCIOFLUSH);
    close(portDescriptor);
  }
}
