#include "../main/color_math.h"
#include "../main/led_calibration.h"
#include <cassert>
#include <algorithm>
#include <cstdlib>

int main() {
    auto r = hsv_rgb(0, 254, 254);
    assert(r.r == 255 && r.g == 0 && r.b == 0);
    auto b = hsv_rgb(169, 254, 254);
    assert(b.b > 250 && b.r < 5 && b.g < 5);
    auto z = hsv_rgb(200, 200, 0);
    assert(z.r == 0 && z.g == 0 && z.b == 0);
    auto dim = hsv_rgb(0, 254, 13);
    assert(dim.r == 13);
    // XY values for sRGB primaries: no unintended cross-channel color.
    auto red_xy = xy_rgb(41942, 21627, 254);
    assert(red_xy.r == 255 && red_xy.g <= 2 && red_xy.b <= 2);
    auto green_xy = xy_rgb(19661, 39321, 254);
    assert(green_xy.g == 255 && green_xy.r <= 2 && green_xy.b <= 2);
    auto blue_xy = xy_rgb(9830, 3932, 254);
    assert(blue_xy.b == 255 && blue_xy.r <= 2 && blue_xy.g <= 2);
    // Exact zero-saturation white is equal across channels at any hue/brightness.
    for (int l : {1, 13, 127, 254})
        for (int h : {0, 85, 169, 254}) {
            auto w = hsv_rgb(h, 0, l);
            assert(w.r == w.g && w.g == w.b);
            assert(w.r == calibrated_rgb(w, LedCalibration{}).r);
        }
    // Quantized D65 white may differ by one code step, never a visible blue boost.
    for (int l : {13, 127, 254}) {
        auto w = xy_rgb(20493, 21561, l);
        assert(std::abs(int(w.r) - int(w.g)) <= 1 && std::abs(int(w.r) - int(w.b)) <= 1);
        auto near = hsv_rgb(169, 1, l);
        assert(std::abs(int(near.r) - int(near.b)) <= 1);
    }
    // An HSV hue sweep at fixed saturation and a quantized XY red-to-green
    // sweep are continuous in the conversion itself. If the user sees an abrupt
    // white jump, inspect actual mode/paired attributes and BLE writes.
    RGB previous = hsv_rgb(0, 254, 254);
    for (int h = 1; h <= 85; h++) {
        RGB next = hsv_rgb(h, 254, 254);
        assert(std::abs(int(next.r) - int(previous.r)) <= 7);
        assert(std::abs(int(next.g) - int(previous.g)) <= 7);
        assert(next.b <= 3);
        previous = next;
    }
    previous = xy_rgb(41942, 21627, 254);
    for (int step = 1; step <= 100; step++) {
        auto x = static_cast<uint16_t>(std::lround(41942 + (19661 - 41942) * step / 100.0));
        auto y = static_cast<uint16_t>(std::lround(21627 + (39321 - 21627) * step / 100.0));
        RGB next = xy_rgb(x, y, 254);
        assert(std::abs(int(next.r) - int(previous.r)) <= 45);
        assert(std::abs(int(next.g) - int(previous.g)) <= 45);
        assert(std::abs(int(next.b) - int(previous.b)) <= 45);
        previous = next;
    }
    // Split Hue/Saturation callbacks could transiently produce white before green:
    // this is an event-order example, not proof that Apple sends this sequence.
    auto transient = hsv_rgb(85, 0, 254), final_green = hsv_rgb(85, 254, 254);
    assert(transient.r == 255 && transient.g == 255 && transient.b == 255);
    assert(final_green.r == 0 && final_green.g == 255 && final_green.b <= 3);
    z = xy_rgb(20000, 20000, 0);
    assert(z.r == 0 && z.g == 0 && z.b == 0);
    z = xy_rgb(100, 0, 100);
    assert(z.r == 0 && z.g == 0 && z.b == 0);
    // CCT is a separate path: increasing mireds (warmer) decreases G/B.
    auto hot = kelvin_rgb(500, 254), mellow = kelvin_rgb(370, 254);
    auto neutral_temp = kelvin_rgb(250, 254), cold = kelvin_rgb(153, 254);
    assert(hot.r == 255 && hot.g >= 130 && hot.g <= 145 && hot.b < 20);
    assert(mellow.r == 255 && mellow.g > hot.g && mellow.b > hot.b);
    assert(neutral_temp.g > mellow.g && neutral_temp.b > mellow.b);
    assert(cold.g >= 250 && cold.b >= 245);

    auto original = RGB{128, 64, 255};
    auto identity = calibrated_rgb(original, LedCalibration{});
    assert(identity.r == 128 && identity.g == 64 && identity.b == 255);
    LedCalibration sample{};
    sample.source[0] = 2;
    sample.source[2] = 0;
    sample.gain[0] = 0.5;
    sample.gamma[1] = 2.0;
    auto corrected = calibrated_rgb(original, sample);
    assert(corrected.r == 128 && corrected.g == 16 && corrected.b == 128);
    sample.gain[2] = 1.1;
    auto invalid = calibrated_rgb(original, sample);
    assert(invalid.r == 128 && invalid.g == 64 && invalid.b == 255);
    for (int h = 0; h <= 254; h++)
        for (int l = 0; l <= 254; l++) {
            auto c = hsv_rgb(h, 254, l);
            assert(std::max({c.r, c.g, c.b}) <= std::lround(l * 255.0 / 254.0));
        }
}
