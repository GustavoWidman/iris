# color investigation

Apple Home control works on the tested setup. mixed colors are another matter.
calibration is still neutral by default; this is the record of what we know,
not a measured calibration. an optional [relative-balance experiment](experimental-relative-balance.md)
is opt-in. version 3 was built, app-only flashed and tried through Apple Home
on the same setup; see the [trial record](experimental-relative-balance.md#the-one-board-trial).

## software math vs. physical output

the old XY-to-RGB matrix was wrong. the current implementation uses the standard
D65 XYZ-to-sRGB inverse matrix, with host regressions for primaries and neutral
white. that is a reproducible software correction.

the remaining mixed-color problem also appeared in MohuanLED before the bridge.
exact-RGB diagnostics bypass HSV, XY, Kelvin and calibration entirely:

```text
rgb R G B → 69 96 05 02 R G B → 80 ms → 69 96 02 01 01
```

the raw bytes pass through the sole BLE worker unchanged. packet tests verify
channel order and frame length. private development traces recorded locally
accepted NimBLE writes; those logs are not distributed and do not establish the
controller's output.

## observed samples

these are subjective visual reports on one controller/strip, not radiometric
measurements or a portable calibration profile.

| requested RGB | reported appearance |
| --- | --- |
| `(128,0,0)`, `(255,0,0)` | red |
| `(128,0,1)`, `(128,0,4)` | bright pink |
| `(0,0,1)` | deep ocean blue |
| `(0,0,128)` | same blue hue, much brighter |
| `(128,21,0)` | slightly greenish yellow |
| `(128,19,0)` | yellow, very slightly green/light |
| `(120,18,0)` | decent yellow; preferred at this tested brightness |
| `(128,8,0)` | rich orange |
| `(128,16,1)` through `(128,16,8)` | warm / creamy white |
| `(128,16,10)` through `(128,16,12)` | pale rose-gold / washed pink |
| `(128,20,10)` | greenish, with rose-gold undertone |
| `(0,0,0)` | physically dark on the tested unit |

the blackout still used the follow-up on frame. darkness does not prove the
controller considered itself powered off. an ordinary `off` request previously
left the LEDs lit despite Matter reporting off. neither transport acceptance nor
Home's state is a substitute for looking at the strip.

## what this does—and doesn't—tell us

the blue-at-one jump is consistent with strongly nonlinear response. it doesn't
quantify gain or tell us whether the source is a controller lookup curve,
PWM/current quantization, channel imbalance, LED spectra or visual adaptation.
correct primaries and raw packet order argue against a simple channel swap, but
do not rule out every controller/strip fault.

separate H/S or X/Y updates and the worker's write cadence may create transient
intermediate colors. those are plausible bridge effects, not an explanation of
all symptoms; problems that existed before the bridge need a broader diagnosis.

the tested bridge now runs the opt-in version 3 red-context profile. existing
gain/gamma/permutation hooks and power frames are unchanged. the public build
still defaults to identity, not the experimental profile.
there is no measured white point, gamut or calibrated color-temperature range.
an RGB strip cannot produce a dedicated high-quality white emitter in software.

## if you investigate further

use a deliberate, one-at-a-time observed session at fixed brightness. keep
calibration identity for a baseline. record the input mode, computed RGB, packet
and physical appearance separately. stop when the intended test is finished;
there is no automatic color sweep.

for actual curve fitting, collect repeated stable per-channel samples across
codes such as 0, 1, 2, 4, 8, 16, 32, 64, 128 and 255. a camera with locked exposure
and white balance can give relative comparisons, not colorimeter measurements.
validate a proposed profile against held-out swatches, make it opt-in and retain
identity as the rollback. color names alone aren't measurements.

protocol research and algorithm attribution are in [THIRD_PARTY.md](../THIRD_PARTY.md).
serial precautions and the temporary override lifecycle are in
[troubleshooting](troubleshooting.md#one-at-a-time-exact-rgb-session).
