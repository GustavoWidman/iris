#pragma once
#include <cstdint>
#include <string_view>
#include "color_math.h"

enum class TestCommandKind : uint8_t { None, Rgb, Resume, InvalidRgb };

struct TestCommand {
    TestCommandKind kind = TestCommandKind::None;
    RGB rgb{};
};

// Deliberately accepts only ASCII decimal RGB triples. It is used for a
// temporary physical-output test path, never configuration or pairing data.
inline TestCommand parse_test_command(std::string_view line) {
    auto is_space = [](char c) { return c == ' ' || c == '\t'; };
    size_t begin = 0, end = line.size();
    while (begin < end && is_space(line[begin])) ++begin;
    while (end > begin && is_space(line[end - 1])) --end;
    line = line.substr(begin, end - begin);
    if (line == "resume") return {TestCommandKind::Resume, {}};
    if (line.size() < 3 || line.substr(0, 3) != "rgb") return {};
    // Keep unrelated console words such as "rgbstatus" on the old unknown path.
    if (line.size() > 3 && !is_space(line[3])) return {};
    size_t i = 3;
    uint8_t values[3]{};
    for (int component = 0; component < 3; ++component) {
        while (i < line.size() && is_space(line[i])) ++i;
        if (i == line.size() || line[i] < '0' || line[i] > '9') return {TestCommandKind::InvalidRgb, {}};
        unsigned value = 0;
        while (i < line.size() && line[i] >= '0' && line[i] <= '9') {
            value = value * 10u + static_cast<unsigned>(line[i++] - '0');
            if (value > 255u) return {TestCommandKind::InvalidRgb, {}};
        }
        values[component] = static_cast<uint8_t>(value);
        if (component < 2 && (i == line.size() || !is_space(line[i]))) return {TestCommandKind::InvalidRgb, {}};
    }
    while (i < line.size() && is_space(line[i])) ++i;
    if (i != line.size()) return {TestCommandKind::InvalidRgb, {}};
    return {TestCommandKind::Rgb, {values[0], values[1], values[2]}};
}

struct ExactRgbOverride {
    bool active = false;
    RGB rgb{};
    void set(RGB value) { rgb = value; active = true; }
    bool clear() { const bool was_active = active; active = false; return was_active; }
    // A real Matter attribute update always wins over a temporary console test.
    bool supersede_by_matter() { return clear(); }
};
