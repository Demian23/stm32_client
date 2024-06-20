#pragma once

#include <cstdint>

namespace smp {

enum peripheral_devices : uint8_t { LED = 0 };
enum led_ops : uint8_t { ON, OFF, TOGGLE };
/*
 * Led msg: uint8_t number (0xFF -> all), led_ops
 */

struct LoadMsg {
    uint32_t packetId;
    uint32_t wholeMsgSize;
};

struct LedMsg {
    uint8_t ledDevice : 4;
    uint8_t op : 4;
};

static_assert(sizeof(LoadMsg) == 8);
static_assert(sizeof(LedMsg) == 1);

} // namespace smp
