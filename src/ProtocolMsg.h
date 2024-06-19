#pragma once
#include <cstdint>

namespace smp {
enum peripheral_devices : uint8_t { LED = 0 };
enum led_ops : uint8_t { ON, OFF, TOGGLE };
/*
 * Led msg: uint8_t number (0xFF -> all), led_ops
 */


} // namespace smp
