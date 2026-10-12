#pragma once

#include <array>
#include <cstdint>

// BJ_LED is a reverse-engineered, device-family protocol.  These frames are
// deliberately kept here (rather than ad-hoc byte arrays in the worker) so the
// channel order, short RGB frame, and observed power frames are unit-tested.
// `69 96 05 02 R G B` is corroborated by the MohuanLED/BJ_LED integration and
// its captured GATT bytes.  This bridge's BJ_LED accepted the short RGB frame.
namespace bj_led_protocol {

using Packet5 = std::array<uint8_t, 5>;
using Packet7 = std::array<uint8_t, 7>;

constexpr Packet5 kOff{{0x69, 0x96, 0x02, 0x01, 0x00}};
// This is the power-on frame already used successfully by this device and by
// 8none1/bj_led's implementation. Some published captures show 0x06 instead;
// do not substitute it without a device-specific, observed test.
constexpr Packet5 kOn{{0x69, 0x96, 0x02, 0x01, 0x01}};

constexpr Packet7 rgb(uint8_t red, uint8_t green, uint8_t blue) {
    return {{0x69, 0x96, 0x05, 0x02, red, green, blue}};
}

} // namespace bj_led_protocol
