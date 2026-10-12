# protocol and state

```text
Matter attributes → desired light state → HSV / XY / CCT conversion
  → identity-default calibration → BJ_LED frames → BLE write without response
```

## device selection and frames

the bridge looks for the exact six-byte advertised name **`BJ_LED`**, then finds
16-bit characteristic **`0xEE01`** with write-without-response support. an optional
private `BRIDGE_LED_ADDR` narrows selection. `BJ_LED_M`, `ELK-BLEDOM` and other
similar controllers are not automatically treated as compatible.

| Operation | Hex frame |
| --- | --- |
| RGB | `69 96 05 02 RR GG BB` |
| On | `69 96 02 01 01` |
| Off | `69 96 02 01 00` |

frames are defined and host-tested in `main/bj_led_protocol.h`. Some published
captures use an on variant `69 96 06 01 01`; iris retains the variant observed to
work on the tested controller. an ordinary `off` request once updated Matter state while the physical LEDs
stayed lit. do not assume the off frame reliably darkens every device. exact-RGB
zero was physically confirmed on the tested unit, not a universal power-off fix.
there is no state feedback, acknowledgment of
physical output, effects or music implementation. Protocol references are in
[THIRD_PARTY.md](../THIRD_PARTY.md).

## conversion

Matter hue, saturation and level use 0–254 scales. ColorMode selects HSV (`0`),
XY (`1`) or color temperature (`2`, mired). XY uses the D65 XYZ-to-linear-sRGB
inverse matrix, clips negative channels, applies the sRGB transfer function,
normalizes peaks and scales brightness. Regression tests cover sRGB primaries and
neutral white; these test software math, not the strip's gamut.

Kelvin-to-RGB is an approximation, not a calibrated illuminant. the default
advertised color-temperature limits (1–65279 mired) are not a measured hardware
specification. `main/led_calibration.h` provides gain/gamma/channel hooks, **all
identity by default**. do not guess calibration from a named Home preset. the
[color investigation](color-investigation.md) separates the fixed XY matrix error
from still-unresolved nonlinear physical mixed colors.

## writes and persistence

one worker owns BLE writes. it samples desired state every 250 ms, with an 80 ms
pause between RGB and on frames. State changes can lag or step; separately
arriving H/S or X/Y attributes may produce intermediate combinations. these are
possible transient sources, not a proven explanation of physical color mismatch.

matter desired state is saved in NVS namespace `bj_state`, throttled to roughly
five seconds. Reconnects reapply saved desired state. A new board has no automatic
LED write until a state change; a board with saved state may apply it after boot.
IR/app changes do not update Home, and later bridge writes may overwrite them.

## exact-rgb diagnostics

`rgb R G B` bypasses conversion and calibration, queues through the same BLE
worker and holds a temporary override. `resume` or a real Matter attribute update
clears it. it is not saved to NVS. A reboot loses the override and can restore
saved Matter state, which may illuminate the strip.

the exact-RGB path sends the color frame **then the on frame**, including for
`rgb 0 0 0`. A zero-RGB test was physically dark on the tested controller, but is
not a universal power-off guarantee. observe output directly. Never autonomously
cycle colors or treat a successful BLE call as proof of what was displayed.
