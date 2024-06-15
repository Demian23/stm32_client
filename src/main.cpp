#include "Channel.h"
#include "SerialPort.h"
#include <iostream>
#include <thread>

int main(int argc, char **argv) {
  using namespace std::chrono_literals;
  if (argc == 3) {
    try {
      std::array<uint8_t, 5> msg{"Just"};
      smp::Channel channel(argv[1], std::stoi(argv[2]));
      channel.peripheral(msg.begin(), msg.size() - 1);
      std::this_thread::sleep_for(200ms);
    } catch (const std::logic_error &error) {
      std::cout << error.what() << std::endl;
    }
  } else {
    std::cout << "Usage: " << argv[0] << "<port_name> <baud_rate>\n";
  }
}