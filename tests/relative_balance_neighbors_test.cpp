// Exhaustive unsigned-byte neighborhood report. Every +1 edge represents the
// corresponding reverse -1 edge too. No camera, SDK or hardware accesses.
#include "../main/led_calibration.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vector>

struct Worst {
    int jump = -1;
    RGB from{}, to{}, out_from{}, out_to{};
};

static RGB input_at(int r, int g, int b) {
    return {static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)};
}

static size_t index(int r, int g, int b) {
    return (size_t(r) * 256 + g) * 256 + b;
}

static int component(RGB rgb, int channel) {
    return channel == 0 ? rgb.r : channel == 1 ? rgb.g : rgb.b;
}

int main() {
    std::vector<RGB> table(256u * 256u * 256u);
    for (int r = 0; r < 256; r++)
        for (int g = 0; g < 256; g++)
            for (int b = 0; b < 256; b++)
                table[index(r, g, b)] = experimental_relative_balance(input_at(r, g, b));
    Worst worst[3][3];
    size_t edges = 0;
    for (int r = 0; r < 256; r++)
        for (int g = 0; g < 256; g++)
            for (int b = 0; b < 256; b++) {
                const int components[3] = {r, g, b};
                const RGB from = input_at(r, g, b), a = table[index(r, g, b)];
                for (int axis = 0; axis < 3; axis++) {
                    if (components[axis] == 255)
                        continue;
                    int next[3] = {r, g, b};
                    ++next[axis];
                    const RGB to = input_at(next[0], next[1], next[2]),
                              z = table[index(next[0], next[1], next[2])];
                    ++edges;
                    for (int channel = 0; channel < 3; channel++) {
                        const int jump = std::abs(component(z, channel) - component(a, channel));
                        assert(jump <=
                               1); // Single-code input changes must not cause large output jumps.
                        auto &w = worst[axis][channel];
                        if (jump > w.jump)
                            w = {jump, from, to, a, z};
                    }
                }
            }
    assert(edges == 50135040u);
    std::printf("input_triples=16777216 undirected_edges=%zu directed_plus_minus_edges=%zu\n",
                edges,
                2 * edges);
    std::puts(
        "input_axis,output_channel,max_abs_jump,from_r,from_g,from_b,to_r,to_g,to_b,out_from_r,out_from_g,out_from_b,out_to_r,out_to_g,out_to_b");
    for (int axis = 0; axis < 3; axis++)
        for (int channel = 0; channel < 3; channel++) {
            const auto &w = worst[axis][channel];
            std::printf("%c,%c,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                        "RGB"[axis],
                        "RGB"[channel],
                        w.jump,
                        w.from.r,
                        w.from.g,
                        w.from.b,
                        w.to.r,
                        w.to.g,
                        w.to.b,
                        w.out_from.r,
                        w.out_from.g,
                        w.out_from.b,
                        w.out_to.r,
                        w.out_to.g,
                        w.out_to.b);
        }
}
