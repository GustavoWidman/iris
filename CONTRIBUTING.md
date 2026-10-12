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
