# contributing

small, well-evidenced improvements are welcome. run `bash tools/test-host.sh`
with GCC or Clang before sending a change. Host tests must not open hardware,
fetch SDKs, embed credentials or upload firmware artifacts.

preserve the separation between Matter state, conversion, protocol writes and
physical output. Add regression coverage for conversion math, packet order and
console/override behavior. Calibration stays identity unless an explicit profile
has repeatable measured evidence; do not silently adjust channel gains based on
color names. A no-response BLE write is never a physical-output assertion.

for hardware findings, report board/controller family, SDK versions, input mode,
computed RGB and observed response. keep device addresses, Wi-Fi information,
setup codes, logs and flash backups private. test one deliberate command at a
time, preserve existing commissioning and document disruptive steps. do not
introduce autonomous color cycling, hidden serial access or resets into tests.

firmware integration changes need a local SDK build and deliberate hardware
verification beyond host CI. note what was not tested. There is no claim of
Matter certification or support for every `BJ_LED`-named controller.

## formatting

keep code readable rather than compressed: four-space indentation, a 100-column
limit, and expanded functions, branches and loops. `.clang-format` keeps include
order and attribution comments intact. Python uses Ruff, shell scripts use
shfmt, and CMake uses cmake-format; their settings are checked in alongside the
source. workflow YAML uses Prettier with two-space indentation. from the repository root, with these formatters installed:

```sh
clang-format -i main/*.h main/*.cpp diagnostic/main/*.c tests/*.cpp tools/*.cpp
clang-format -i examples/private_config.h.example
ruff format tools tests
shfmt -w -i 4 tools/*.sh
cmake-format -i CMakeLists.txt main/CMakeLists.txt diagnostic/CMakeLists.txt diagnostic/main/CMakeLists.txt
prettier --write --tab-width 2 --print-width 100 .github/workflows/*.yml
```

format first-party code only, not fetched SDKs or generated/private settings.
run the host tests after formatting; source-contract tests should check routing,
not depend on a particular choice of whitespace.
