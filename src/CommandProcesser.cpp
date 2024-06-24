#include "CommandProcesser.h"
#include "LocalStatusCode.h"
#include "Msg.h"
#include "Protocol.h"
#include <algorithm>
#include <charconv>
#include <string_view>
#include <unordered_map>

namespace {

bool checkAnswer(const smp::BufferedAnswer& answer, smp::ReadResult readResult, std::string& error) noexcept;
constexpr std::string_view localCodeToStr(LocalStatusCode code) noexcept;
constexpr std::string_view codeToStr(smp::StatusCode code) noexcept;
void fillLedCommand(std::string_view command, smp::LedMsg &msg) ;

}
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

std::string CommandProcesser::startCommand()
{
    comChannel.handshake();
    auto result = comChannel.handshakeAnswer();
    if (result == LocalStatusCode::Ok) {
        return "Values: " + comChannel.values();
    } else {
        return localCodeToStr(result).data();
    }
}

std::string CommandProcesser::ledCommand(std::string_view command)
{
    std::string resultString = "Led operation succeeded";

    smp::LedMsg msg{};
    smp::BufferedAnswer answer{};

    fillLedCommand(command, msg);

    comChannel.peripheral(msg);

    auto result = comChannel.getHeaderedMsg(
        answer.buffer.data(), answer.buffer.size(), smp::action::peripheral);
    checkAnswer(answer, result, resultString);
    return resultString;
}

// write as coroutine?
std::string CommandProcesser::loadCommand(std::string_view command)
{
    std::string resultStr{};
    smp::BufferedAnswer receiver{};
    smp::BinMsg msg(command);

    comChannel.startLoad(msg);

    auto readResult = comChannel.getHeaderedMsg(receiver.buffer.data(), receiver.buffer.size(), smp::action::startLoad);

    if(checkAnswer(receiver, readResult, resultStr)){
        
        LocalStatusCode result = comChannel.load(msg);
        while(result == LocalStatusCode::Ok){
            readResult = comChannel.getHeaderedMsg(receiver.buffer.data(), receiver.buffer.size(), smp::action::loading);
            if(checkAnswer(receiver, readResult, resultStr)){
                result = comChannel.load(msg);
            }
        }

        if(result == LocalStatusCode::NothingToWrite){
            // start write to memory?
            // wait written answer
            resultStr = "Loaded";
        } 
    }
    return resultStr;
}


namespace{

constexpr std::string_view localCodeToStr(LocalStatusCode code) noexcept
{
    using namespace std::string_view_literals;
    constexpr std::array localStatusCodeToString{
        std::pair{LocalStatusCode::WrongHash, "Wrong hash"sv},
        std::pair{LocalStatusCode::BufferToSmall, "Buffer too small"sv},
        std::pair{LocalStatusCode::WrongId, "Wrong response id"sv},
        std::pair{LocalStatusCode::WrongStartWord, "Wrong start word"sv},
        std::pair{LocalStatusCode::HandshakeAnswerHeaderNotEqual,
         "Handshake answer header not equal"sv},
        std::pair{LocalStatusCode::WrongFlags, "Header flags are different"sv}};
    auto res = std::find_if(localStatusCodeToString.cbegin(), localStatusCodeToString.cend(), [=](auto&& codeAndStr){return codeAndStr.first == code;});
    if(res != localStatusCodeToString.cend()){
        return res->second;
    } else {
        return "Unkonwn error";
    }
}

constexpr std::string_view codeToStr(smp::StatusCode code) noexcept
{
    using namespace std::string_view_literals;
    constexpr std::array statusCodeToStr{
        std::pair{smp::StatusCode::Invalid, "Invalid status code"sv},
        std::pair{smp::StatusCode::Ok, "Success"sv},
        std::pair{smp::StatusCode::InvalidId, "Invalid id"sv},
    };
    auto res = std::find_if(statusCodeToStr.cbegin(), statusCodeToStr.cend(), [=](auto&& codeAndStr){return codeAndStr.first == code;});
    if(res != statusCodeToStr.cend()){
        return res->second;
    } else {
        return "Unkonwn smp error";
    }
}

void fillLedCommand(std::string_view command, smp::LedMsg &msg) 
{
    using namespace std::string_view_literals;
    static const std::unordered_map strToLedOp{
        std::pair{"on"sv, smp::led_ops::ON},
        {"off"sv, smp::led_ops::OFF},
        {"toggle"sv, smp::led_ops::TOGGLE}};
    // static smp::BufferedLedPacket ledPacket;

    auto firstSpace = command.find_first_of(' ');
    if ("all" != command.substr(0, firstSpace)) {
        uint8_t val;
        auto [ptr, err] =
            std::from_chars(command.data(), command.data() + firstSpace, val);
        if (err != std::errc()) {
            throw std::logic_error(
                "Can't convert to number: " +
                std::string(command.cbegin(), command.cbegin() + firstSpace));
        }
        if (val > 0xF) {
            throw std::logic_error("Led number must be <= 0xF");
        }
        msg.ledDevice = val;
    } else {
        msg.ledDevice = 0xF;
    }
    auto ledOp = strToLedOp.find(
        command.substr(firstSpace + 1, command.size() - 1 - firstSpace));
    if (ledOp != strToLedOp.cend()) {
        msg.op = ledOp->second;
    } else {
        throw std::logic_error("No such led operation");
    }
}

bool checkAnswer(const smp::BufferedAnswer& answer, smp::ReadResult readResult, std::string& error) noexcept
{
    auto [code, received] = readResult;
    if(code == LocalStatusCode::Ok){
        if(received == answer.buffer.size()){
           if(answer.answer.code == smp::StatusCode::Ok){
               return true;
           } else{
                error = codeToStr(answer.answer.code);
           }
        } else {
            error = "Wrong size";
        }
    } else {
        error = localCodeToStr(code);
    }
    return false;
}
}
