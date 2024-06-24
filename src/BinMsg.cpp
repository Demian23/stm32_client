#include "BinMsg.h"
#include <filesystem>
#include <fstream>

namespace smp {

BinMsg::BinMsg(std::string_view binFilePath)
    : buffer{}, written{}, nextPacketId{}, hash{}
{
    using namespace std::filesystem;

    path pathToBinFile{binFilePath.cbegin(), binFilePath.cend()};
    if (exists(pathToBinFile) && is_regular_file(pathToBinFile)) {
        auto fileSize = static_cast<std::streamsize>(file_size(pathToBinFile));
        std::vector<char> tempBuffer(fileSize);
        std::ifstream binFile{pathToBinFile, std::ios::binary};
        binFile.read(tempBuffer.data(), fileSize);
        if (binFile) {
            buffer = std::move(tempBuffer);
        } else {
            throw std::logic_error("Can't read from file");
        }
    } else {
        throw std::logic_error("Wrong file");
    }
}
uint32_t BinMsg::getWrittenBytes() const noexcept { return written; };
uint32_t BinMsg::getMsgSize() const noexcept { return buffer.size(); }


}; // namespace smp
