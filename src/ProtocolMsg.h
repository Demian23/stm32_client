#pragma once
#include <cstdint>

namespace smp {
enum peripheral_devices : uint8_t { LED = 0 };

enum led_ops : uint8_t { ON, OFF, TOGGLE };
} // namespace smp