#include "CommandProcesser.h"
#include "ErrnoException.h"
#include <iostream>

void exceptionHandler();

int main(int argc, char **argv)
{
    using namespace std::chrono_literals;

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << "<port_name> <baud_rate>\n";
    }
    std::cin.exceptions(std::ios::badbit | std::ios::failbit);
    std::cerr.exceptions(std::ios::badbit | std::ios::failbit);
    std::cout.exceptions(std::ios::badbit | std::ios::failbit);

    try {
        auto baudRate = std::stoul(argv[2]);
        CommandProcesser processer{argv[1], baudRate};
        std::string command;
        while (std::getline(std::cin, command)) {
            auto answer = processer.process(command);
            if(answer.empty())
                break;
            std::cout << answer << std::endl;
        }
    } catch (...) {
        exceptionHandler();
    }
    return 0;
}

void exceptionHandler()
{
    try {
        throw;
    } catch (const ErrnoException &error) {
        std::cerr << error.what() << '\t' << "Errno: " << error.errno_code()
                  << std::endl;
    } catch (const std::logic_error &error) {
        std::cerr << error.what() << std::endl;
    } catch (const std::exception &error) {
        std::cerr << error.what() << std::endl;
    }
}
