#pragma once
#include <cerrno>
#include <stdexcept>

class ErrnoException : public std::logic_error {
public:
    ErrnoException(const ErrnoException &) noexcept = default;
    ErrnoException &operator=(const ErrnoException &) noexcept = default;
    ErrnoException(const std::string &msg, int errno_code) noexcept
        : std::logic_error(msg), _errno(errno_code)
    {}
    ErrnoException(const char *msg, int errno_code) noexcept
        : std::logic_error(msg), _errno(errno_code)
    {}
    explicit ErrnoException(const std::string &msg) noexcept
        : std::logic_error(msg), _errno(
#ifdef _WIN32
            GetLastError()
#else
            errno
#endif
        )
    {}
    explicit ErrnoException(const char *msg) noexcept
        : std::logic_error(msg), _errno(
#ifdef _WIN32
            GetLastError()
#else
            errno
#endif
        )
    {}
    [[nodiscard]] inline int errno_code() const noexcept { return _errno; }
    ~ErrnoException() noexcept override = default;

private:
    int _errno;
};