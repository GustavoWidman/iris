#include "../main/led_profile_config.h"
#include <cassert>
#include <cmath>

static bool equal(RGB a, RGB b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

static int peak(RGB c) {
    return std::max({c.r, c.g, c.b});
}

int main() {
    LedCalibration c{};
    c.profile = LedColorProfile::ExperimentalRelativeBalance;
    assert(equal(calibrated_rgb({0, 0, 0}, c), {0, 0, 0}));
    for (int level = 0; level < 256; level++) {
        const auto l = static_cast<uint8_t>(level);
        for (RGB pure : {RGB{l, 0, 0}, RGB{0, l, 0}, RGB{0, 0, l}})
            assert(equal(calibrated_rgb(pure, c), pure));
        assert(equal(calibrated_rgb({l, l, l}, c),
                     {l, byte_clamp(level * 0.15), byte_clamp(level / 32.0)}));
        assert(equal(calibrated_rgb({0, l, l}, c), {0, l, l}));
    }
    assert(equal(calibrated_rgb({120, 120, 0}, c), {120, 18, 0}));
    assert(equal(calibrated_rgb({128, 128, 128}, c), {128, 19, 4}));
    assert(equal(calibrated_rgb({128, 0, 128}, c), {128, 0, 4}));
    assert(equal(calibrated_rgb({128, 64, 0}, c), {128, 10, 0}));
    assert(equal(calibrated_rgb({128, 0, 1}, c), {128, 0, 0})); // No dithering.
    // Regress the rejected v1 normalization: no near-blue contamination amplification.
    assert(equal(calibrated_rgb({0, 0, 32}, c), {0, 0, 32}));
    assert(equal(calibrated_rgb({1, 0, 32}, c), {1, 0, 31}));
    assert(equal(calibrated_rgb({0, 1, 32}, c), {0, 1, 32}));
    assert(equal(calibrated_rgb({0, 128, 128}, c), {0, 128, 128}));
    assert(equal(calibrated_rgb({128, 19, 0}, c), {128, 3, 0})); // Do not double-correct presets.
    // Exhaustive byte-space properties: identity exactly, channelwise non-amplifying,
    // bounded output; zeros stay zero. These are numeric, not physical properties.
    for (int r = 0; r < 256; r++)
        for (int g = 0; g < 256; g++)
            for (int b = 0; b < 256; b++) {
                RGB input{
                    static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)};
                assert(equal(calibrated_rgb(input, LedCalibration{}), input));
                const auto output = calibrated_rgb(input, c);
                assert(output.r == input.r && output.g <= input.g && output.b <= input.b);
                assert(output.r <= peak(input) && output.g <= peak(input) &&
                       output.b <= peak(input));
                if (!r)
                    assert(equal(output, input)); // Entire green/blue plane is identity.
                if (!g)
                    assert(!output.g);
                if (!b)
                    assert(!output.b);
            }
    // Along exactly representable rays, attenuated output is homogeneous before
    // rounding. Adjacent brightness steps stay within the input peak step + 1.
    for (RGB ray :
         {RGB{1, 1, 1}, RGB{1, 1, 0}, RGB{0, 1, 1}, RGB{1, 0, 1}, RGB{4, 1, 2}, RGB{1, 4, 8}}) {
        RGB previous{0, 0, 0};
        for (int n = 1; n <= 255 / peak(ray); n++) {
            RGB input{static_cast<uint8_t>(ray.r * n),
                      static_cast<uint8_t>(ray.g * n),
                      static_cast<uint8_t>(ray.b * n)};
            const auto next = calibrated_rgb(input, c);
            assert(next.r >= previous.r && next.g >= previous.g && next.b >= previous.b);
            assert(int(next.r) - previous.r <= peak(ray) + 1);
            assert(int(next.g) - previous.g <= peak(ray) + 1);
            assert(int(next.b) - previous.b <= peak(ray) + 1);
            previous = next;
        }
    }
    // Existing invalid-gain/gamma behavior remains fail-safe even with opt-in.
    for (double bad : {-1.0, 1.01, double(INFINITY), double(NAN)}) {
        auto invalid = c;
        invalid.gain[1] = bad;
        assert(equal(calibrated_rgb({120, 120, 0}, invalid), {120, 120, 0}));
    }
    auto invalid = c;
    invalid.profile = static_cast<LedColorProfile>(999);
    assert(equal(calibrated_rgb({120, 120, 0}, invalid), {120, 120, 0}));
    const auto firmware = firmware_led_calibration();
#if IRIS_EXPERIMENTAL_RELATIVE_BALANCE
    assert(firmware.profile == LedColorProfile::ExperimentalRelativeBalance);
    assert(equal(calibrated_rgb({120, 120, 0}, firmware), {120, 18, 0}));
#else
    assert(firmware.profile == LedColorProfile::Identity);
    assert(equal(calibrated_rgb({120, 120, 0}, firmware), {120, 120, 0}));
#endif
}
