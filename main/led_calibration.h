#pragma once
#include "color_math.h"

// A heuristic opt-in experiment, not measured colorimetry. Identity is the default.
enum class LedColorProfile { Identity, ExperimentalRelativeBalance };

// Red-context attenuation: identity on the entire R=0 plane; never amplify
// contamination. No peak renormalization: red mixtures can lose peak code.
// Heuristic strengths (17/20,31/32), not a measured perceptual device model.
inline RGB experimental_relative_balance(RGB input) {
    const uint32_t peak = std::max({input.r, input.g, input.b});
    if (peak == 0)
        return input;
    auto attenuate = [&](uint32_t channel, uint32_t other, uint32_t divisor, uint32_t strength) {
        const uint32_t denominator = divisor * peak;
        const uint32_t numerator = channel * (denominator - strength * other);
        // Integer half-up rounding; largest doubled numerator < 4.2 million.
        return static_cast<uint8_t>((2 * numerator + denominator) / (2 * denominator));
    };
    return {input.r, attenuate(input.g, input.r, 20, 17), attenuate(input.b, input.r, 32, 31)};
}

// Destination R/G/B output selects an input channel. Identity until physical measurements.
// Gains only reduce channel maxima; gamma adjusts encoded channel values (1 = unchanged).
struct LedCalibration {
    uint8_t source[3] = {0, 1, 2};
    double gain[3] = {1.0, 1.0, 1.0};
    double gamma[3] = {1.0, 1.0, 1.0};
    LedColorProfile profile = LedColorProfile::Identity;
};

inline RGB calibrated_rgb(RGB input, const LedCalibration &c) {
    if (c.profile != LedColorProfile::Identity &&
        c.profile != LedColorProfile::ExperimentalRelativeBalance)
        return input;
    const uint8_t src[3] = {input.r, input.g, input.b};
    uint8_t out[3] = {};
    for (int i = 0; i < 3; i++) {
        if (c.source[i] > 2 || !std::isfinite(c.gain[i]) || !std::isfinite(c.gamma[i]) ||
            c.gain[i] < 0.0 || c.gain[i] > 1.0 || c.gamma[i] < 0.2 || c.gamma[i] > 5.0)
            return input; // Invalid calibration: never send an unexpectedly altered color.
        if (c.gain[i] == 1.0 && c.gamma[i] == 1.0)
            out[i] = src[c.source[i]];
        else
            out[i] = byte_clamp(255.0 * c.gain[i] * std::pow(src[c.source[i]] / 255.0, c.gamma[i]));
    }
    const RGB mapped = {out[0], out[1], out[2]};
    return c.profile == LedColorProfile::ExperimentalRelativeBalance
               ? experimental_relative_balance(mapped)
               : mapped;
}
