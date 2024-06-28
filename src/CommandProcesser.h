#include "Channel.h"
#include <string>

class CommandProcesser final {
public:
    CommandProcesser(std::string_view portName, size_t baudRate);
    std::string process(std::string_view command); // with answer return
    ~CommandProcesser() = default;

private:
    smp::Channel comChannel;

    std::string loadCommand(std::string_view command);
    std::string startCommand();
    std::string bootCommand();
    std::string ledCommand(std::string_view command);
};
