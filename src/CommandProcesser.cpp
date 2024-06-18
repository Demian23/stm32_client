#include "CommandProcesser.h"
#include "ErrnoException.h"
#include "ProtocolMsg.h"
#include <unordered_map>
#include <charconv>

namespace {
    std::array<uint8_t, 3> generateLedCommand(std::string_view command);
}

CommandProcesser::CommandProcesser(std::string_view portName, size_t baudRate)
    : comChannel(portName, baudRate)
{}

enum class commands { START, LED, LOAD, STOP};

std::string CommandProcesser::process(std::string_view command)
{
    using namespace std::string_view_literals;
    static const std::unordered_map strToCommand{
        std::pair{"start"sv, commands::START},
        {"LED"sv, commands::LED},
        {"stop"sv, commands::STOP},
        {"load"sv, commands::LOAD}};

    std::array<uint8_t, 0x40> answerBuffer{};

    auto commandIndex = command.find_first_of(' ');

    auto value = strToCommand.find(command.substr(0, commandIndex));
    if (value != strToCommand.end()) {
        switch (value->second) {
        case commands::STOP:
            return "";
        case commands::START:
            comChannel.handshake();
            comChannel.handshakeAnswer();
            return comChannel.values();
        case commands::LED:{
            auto commandBuffer = generateLedCommand(command.substr(commandIndex + 1, command.size() - commandIndex -1));
            comChannel.peripheral(commandBuffer.data(), commandBuffer.size());
            auto resultSize = comChannel.getAnswer(answerBuffer.data(), answerBuffer.size(), smp::action::peripheral);
            return {answerBuffer.data() + sizeof(smp::header), answerBuffer.data() + sizeof(smp::header) + resultSize};
        }
        case commands::LOAD:
            throw std::logic_error("Command not implemented");
        }
    } else {
        return "No such command";
    }
}

namespace {
    std::array<uint8_t, 3> generateLedCommand(std::string_view command)
    {
        using namespace std::string_view_literals;
        static const std::unordered_map strToLedOp{
            std::pair{"on"sv, smp::led_ops::ON},
            {"off"sv, smp::led_ops::OFF},
            {"toggle"sv, smp::led_ops::TOGGLE}
        };
        static union{
            std::array<uint8_t, 3> buffer;
            struct{
                smp::peripheral_devices device;
                uint8_t controlFlags;
                smp::led_ops op;
            } comp;
        } packet;

        auto firstSpace = command.find_first_of(' ');
        packet.comp.device = smp::peripheral_devices::LED;
        auto [ptr, err] = std::from_chars(command.data(), command.data() + firstSpace, packet.comp.controlFlags);
        if(err != std::errc()){
            throw std::logic_error("Can't convert to number: " +
                std::string(command.cbegin(), command.cbegin() + firstSpace));
        }
        auto ledOp = strToLedOp.find(command.substr(firstSpace + 1, command.size() - 1 - firstSpace));
        if(ledOp != strToLedOp.cend()){
            packet.comp.op = ledOp->second;
        } else {
            throw std::logic_error("No such led operation");
        }
        return packet.buffer;
    }
}
