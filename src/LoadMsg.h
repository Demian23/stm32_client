#pragma once

#include "Protocol.h"

namespace smp {

struct LoadHeader
{
    header header;
    uint32_t packetId;
    uint32_t wholeMsgSize;
};

}
