#include "../main/console_command.h"
#include <cassert>

int main() {
    auto rgb = parse_test_command("rgb 0 127 255");
    assert(rgb.kind == TestCommandKind::Rgb && rgb.rgb.r == 0 && rgb.rgb.g == 127 &&
           rgb.rgb.b == 255);
    assert(parse_test_command("\trgb 1 2 3  ").kind == TestCommandKind::Rgb);
    assert(parse_test_command("resume").kind == TestCommandKind::Resume);
    assert(parse_test_command(" status ").kind == TestCommandKind::None);
    for (const char *bad : {"rgb",
                            "rgb 1",
                            "rgb 1 2",
                            "rgb -1 2 3",
                            "rgb 256 0 0",
                            "rgb 1 2 3 x",
                            "rgb 1,2,3",
                            "rgb 1 2 3 4",
                            "rgb 01 02 003x"})
        assert(parse_test_command(bad).kind == TestCommandKind::InvalidRgb);
    assert(parse_test_command("rgbstatus").kind == TestCommandKind::None);
    ExactRgbOverride state;
    assert(!state.active && !state.clear());
    state.set({12, 34, 56});
    assert(state.active && state.rgb.g == 34);
    assert(state.supersede_by_matter() && !state.active);
    state.set({1, 2, 3});
    assert(state.clear() && !state.active);
}
