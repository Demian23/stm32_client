#include "CommandProcesser.h"
#include "ErrnoException.h"
#include "ProtocolMsg.h"
#include <charconv>
#include <format>
#include <unordered_map>

static std::array<uint8_t, 3> generateLedCommand(std::string_view command);
static const std::unordered_map localStatusCodeToString{
    std::pair{LocalStatusCode::WrongHash, "Wrong hash"},
    {LocalStatusCode::BufferToSmall, "Buffer too small"},
    {LocalStatusCode::WrongId, "Wrong response id"},
    {LocalStatusCode::WrongStartWord, "Wrong start word"},
    {LocalStatusCode::HandshakeAnswerHeaderNotEqual, "Handshake answer header not equal"},
    {LocalStatusCode::WrongFlags, "Header flags are different"}
};

CommandProcesser::CommandProcesser(std::string_view portName, size_t baudRate)
    : comChannel(portName, baudRate)
{}

enum class commands { START, LED, LOAD, STOP };

std::string CommandProcesser::process(std::string_view command)
{
    using namespace std::string_view_literals;
    static const std::unordered_map strToCommand{
        std::pair{"start"sv, commands::START},
        {"LED"sv, commands::LED},
        {"stop"sv, commands::STOP},
        {"load"sv, commands::LOAD}};

    auto commandIndex = command.find_first_of(' ');

    auto value = strToCommand.find(command.substr(0, commandIndex));
    if (value != strToCommand.end()) {
        std::array<uint8_t, 0x40> answerBuffer{};
        switch (value->second) {
        case commands::STOP:
            break;
        case commands::START:
            return startCommand();
        case commands::LED: {
            return ledCommand(command.substr(
                commandIndex + 1, command.size() - 1 - commandIndex));
        }
        case commands::LOAD:
            return loadCommand(command.substr(
                commandIndex + 1, command.size() - 1 - commandIndex));
        }
    } else {
        return "No such command";
    }
    return "";
}

static std::array<uint8_t, 3> generateLedCommand(std::string_view command)
{
    using namespace std::string_view_literals;
    static const std::unordered_map strToLedOp{
        std::pair{"on"sv, smp::led_ops::ON},
        {"off"sv, smp::led_ops::OFF},
        {"toggle"sv, smp::led_ops::TOGGLE}};
    static union {
        std::array<uint8_t, 3> buffer;
        struct {
            smp::peripheral_devices device;
            uint8_t controlFlags;
            smp::led_ops op;
        } comp;
    } packet;

    packet.comp.device = smp::peripheral_devices::LED;
    auto firstSpace = command.find_first_of(' ');
    if ("all" != command.substr(0, firstSpace)) {
        auto [ptr, err] =
            std::from_chars(command.data(), command.data() + firstSpace,
                            packet.comp.controlFlags);
        if (err != std::errc()) {
            throw std::logic_error(
                "Can't convert to number: " +
                std::string(command.cbegin(), command.cbegin() + firstSpace));
        }
    } else {
        packet.comp.controlFlags = 0xFF;
    }
    auto ledOp = strToLedOp.find(
        command.substr(firstSpace + 1, command.size() - 1 - firstSpace));
    if (ledOp != strToLedOp.cend()) {
        packet.comp.op = ledOp->second;
    } else {
        throw std::logic_error("No such led operation");
    }
    return packet.buffer;
}

std::string CommandProcesser::startCommand()
{
    comChannel.handshake();
    auto result = comChannel.handshakeAnswer();
    if (result == LocalStatusCode::Ok) {
        return std::format("Values: {}", comChannel.values());
    }
    auto translate = localStatusCodeToString.find(result);
    if(translate != localStatusCodeToString.cend()){
        return translate->second;
    } else {
        return "Unknown error";
    }
}

std::string CommandProcesser::ledCommand(std::string_view command)
{
    union{
        struct {
            smp::header header;
            smp::StatusCode code;
        } expectedAnswer;
        std::array<uint8_t, sizeof(expectedAnswer)> buffer{};
    } answer;
    std::string resultString;
    auto msg = generateLedCommand(command);
    comChannel.peripheral(msg.data(), msg.size());
    uint16_t expectedFlags = smp::action::peripheral; // maybe led include too
    auto result = comChannel.getHeaderedMsg(answer.buffer.data(), answer.buffer.size(), expectedFlags);
    if(result.localCode == LocalStatusCode::Ok){
        switch(answer.expectedAnswer.code){
            case smp::StatusCode::Ok:
                resultString = "Led operation succeed";
                break;
            default:
                resultString = "Error with code" + std::to_string(answer.expectedAnswer.code);
        }
    } else {
        auto translate = localStatusCodeToString.find(result.localCode);
        if(translate != localStatusCodeToString.cend()){
            return translate->second;
        } else {
            return "Unknown error";
        }
    }
    return resultString;
}
