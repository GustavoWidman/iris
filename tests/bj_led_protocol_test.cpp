#include "../main/bj_led_protocol.h"

#include <cassert>

int main() {
    using namespace bj_led_protocol;

    static_assert(kOff.size() == 5 && kOn.size() == 5);
    static_assert(kOff[0] == 0x69 && kOff[1] == 0x96 && kOff[2] == 0x02 && kOff[3] == 0x01 &&
                  kOff[4] == 0x00);
    static_assert(kOn[0] == 0x69 && kOn[1] == 0x96 && kOn[2] == 0x02 && kOn[3] == 0x01 &&
                  kOn[4] == 0x01);

    constexpr auto red = rgb(0xff, 0x00, 0x00);
    constexpr auto green = rgb(0x00, 0xff, 0x00);
    constexpr auto blue = rgb(0x00, 0x00, 0xff);
    static_assert(red[0] == 0x69 && red[1] == 0x96 && red[2] == 0x05 && red[3] == 0x02);
    static_assert(red[4] == 0xff && red[5] == 0x00 && red[6] == 0x00);
    static_assert(green[4] == 0x00 && green[5] == 0xff && green[6] == 0x00);
    static_assert(blue[4] == 0x00 && blue[5] == 0x00 && blue[6] == 0xff);

    const auto sample = rgb(128, 18, 8);
    assert(sample[4] == 128 && sample[5] == 18 && sample[6] == 8);
}
