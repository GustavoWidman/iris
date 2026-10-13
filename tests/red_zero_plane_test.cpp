// Regression: v2 changed cyan without any supporting observation.
#include "../main/led_calibration.h"
#include <cassert>

int main() {
    for (int g = 0; g < 256; g++)
        for (int b = 0; b < 256; b++) {
            RGB in{0, static_cast<uint8_t>(g), static_cast<uint8_t>(b)};
            const auto out = experimental_relative_balance(in);
            assert(out.r == 0 && out.g == in.g && out.b == in.b);
        }
}
