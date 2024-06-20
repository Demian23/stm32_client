#pragma once

#include <string_view>
#include <vector>

namespace smp {

class BinMsg {
public:
    friend class Channel;
    explicit BinMsg(std::string_view binFilePath);
    BinMsg(BinMsg &&) noexcept = default;
    BinMsg &operator=(BinMsg &&) noexcept = default;
    BinMsg(const BinMsg &) = delete;
    BinMsg &operator=(const BinMsg &) = delete;

    uint32_t getWrittenBytes() const noexcept;
    uint32_t getMsgSize() const noexcept;

    ~BinMsg() = default;

private:
    std::vector<char> buffer;
    uint32_t written;
    uint32_t nextPacketId;
};

}; // namespace smp
