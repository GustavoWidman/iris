# experimental relative balance

this is an **opt-in heuristic**, not measured colorimetry. identity is still the
default. nothing here flashes a board, changes pairing or runs a color sweep.
a one-board Apple Home trial was subjectively encouraging. purple and white
temperature might still get worse. the R=0 green/blue plane is now unchanged. host tests don't have
eyeballs.

## why version 1 was rejected

fixed channel weights followed by peak normalization amplified tiny contaminants:
`(0,0,32)` stayed blue, but `(1,0,32)` became `(32,0,32)`. a one-code input step
caused a 32-code red jump. that version is **not a hardware-trial candidate**.

version 2 removed normalization but attenuated green/blue against each other.
that sent cyan `(0,128,128)` to `(0,19,4)` without an observed cyan anchor. it is
also rejected. version 3 only uses red to trigger attenuation, preserving the
entire R=0 green/blue plane. for nonblack input:

```text
p = max(R, G, B)
R' = R
G' = round(G * (1 - (17/20) * R / p))
B' = round(B * (1 - (31/32) * R / p))
black -> black
```

`experimental_relative_balance` implements this with exact integer half-up
rounding. pure primaries and all 65,536 codes with R=0 are unchanged; no
channel can exceed its own input. tiny contamination is never normalized upward.

**peak code is NOT preserved for mixtures.** neither is perceived brightness,
luminance, power or color temperature. near-primary paths lose dominant-channel
code gradually. avoiding the normalization jump means accepting that tradeoff.
no optional renormalization is hidden in the implementation.

## anchors and guesses

- `(120,120,0)` becomes the user's preferred yellow code `(120,18,0)`.
- `(128,128,128)` becomes `(128,19,4)`, not the observed creamy-white
  `(128,16,4)`. this favors the yellow anchor rather than inventing another fit.
- suppressing blue in red mixtures is a hypothesis based on reported sensitivity,
  not evidence that any resulting code produces perceptually correct purple.
- the red-context dependencies outside those anchors are **heuristic choices**,
  not inferred controller physics or a camera-data fit.

fixed-camera measurements rendered the user's bright yellow orange. one yellow
write left the previous blue visible despite BLE success, so that sample was
excluded. low-signal samples were excluded too. don't fit camera orange to a
human yellow target, or treat transport acceptance as physical-output proof.

## numeric examples, not displayed-color promises

names describe **source RGB**. channels are 8-bit encoded codes.

| source | input | output |
| --- | --- | --- |
| red | `(128,0,0)` | `(128,0,0)` |
| green | `(0,128,0)` | `(0,128,0)` |
| blue | `(0,0,128)` | `(0,0,128)` |
| yellow | `(120,120,0)` | `(120,18,0)` |
| white | `(128,128,128)` | `(128,19,4)` |
| orange | `(128,64,0)` | `(128,10,0)` |
| magenta | `(128,0,128)` | `(128,0,4)` |
| cyan | `(0,128,128)` | `(0,128,128)` |
| near blue | `(1,0,32)` | `(1,0,31)` |
| near blue | `(0,1,32)` | `(0,1,32)` |
| near green | `(1,32,0)` | `(1,31,0)` |
| near green | `(0,32,1)` | `(0,32,1)` |

cyan is unchanged, not calibrated: there is no observed cyan anchor. red-present
mixtures can still lose their peak and dim substantially; `(1,1,1)` becomes
`(1,0,0)`. the limiting red=0 plane does not mean every near-primary mixture
is visually preserved. the user liked the Home trial overall, but did not
validate every corrected source color or brightness level.

reproduce multiple levels without hardware:

```sh
c++ -std=c++17 -Wall -Wextra -Werror tools/show-color-profile.cpp -o /tmp/iris-profile
/tmp/iris-profile
```

## smooth byte steps aren't smooth perception

all 16,777,216 inputs and 50,135,040 positive-neighbor edges were checked.
reverse edges give the same absolute jumps, covering 100,270,080 directed
plus/minus transitions. each output channel changes by **at most one code** for
any one-code change to any input channel. this fixes version 1's numeric jump,
not the controller's physical transfer function.

rounding still eats small channels. `(128,0,1)` becomes `(128,0,0)`; level-8 white
becomes `(8,1,0)`; `(0,1,1)` stays `(0,1,1)`, but `(1,1,1)` becomes `(1,0,0)`.
one-code steps can therefore be large relative changes near black, even though their absolute size is bounded. no
dithering or invented fractional BLE codes are used. on exactly representable
RGB rays the unrounded function scales linearly; arbitrary rounded hue or
brightness paths remain subject to quantization. perceived output might not
scale at all. brightness, CCT, magenta/purple and cyan remain unverified.

## integration and hardware-free checks

Matter HSV, XY and Kelvin meet the existing single `calibrated_rgb` call in the
BLE worker. the profile runs there once after the existing permutation/gain/gamma
hook (identity in the factory). protocol encoding does not run it again. exact-RGB
console diagnostics still bypass conversion and calibration: do not feed an
already-corrected preset through the profile twice.

```sh
CXX=g++ bash tools/test-host.sh
CXX=clang++ bash tools/test-host.sh
```

checks cover pure-primary levels, anchors, near-primary regressions, invalid
configuration, both firmware flag settings, identity, the full R=0 plane,
non-amplification, bounds, fixed-ray brightness steps and the exhaustive neighborhood bound. the continuity
bound is for the profile function with identity gain/gamma/permutation, not every
arbitrary custom calibration. static source contracts check shared routing and
exact-RGB bypass. they are NOT vendor compilation or live Matter validation.

## build and identity rollback

use the SDK pins and **existing private configuration** in [setup](setup.md).
the local trial built with those pinned SDKs. a fresh-clone embedded build with
new private settings is a separate check; host CI doesn't run the vendor SDKs.
do not generate replacement commissioning material for an already-paired board.

```sh
source tools/env.sh
# Build only, no flash or serial access.
idf.py -D IRIS_EXPERIMENTAL_RELATIVE_BALANCE=ON reconfigure
idf.py build
```

`main/CMakeLists.txt` has a default-OFF option; the profile is not persisted in
Matter/NVS. a running board changes only after a deliberate compatible
application-only update. the trial below performed one; these commands only
build. no upload or sweep happens automatically.

identity rollback explicitly sets OFF because CMake retains cached options:

```sh
source tools/env.sh
idf.py -D IRIS_EXPERIMENTAL_RELATIVE_BALANCE=OFF reconfigure
idf.py build
```

this only rebuilds. an actual rollback needs a deliberate compatible app-only
update preserving current NVS/commissioning, not a stale full-flash restore.
retain the known-good application privately and verify SDK/partition compatibility.
for the trial, a neutral app was read from the actual device and its embedded
image hash verified before updating. a cached build was NOT assumed to match
what was installed. both images embed private configuration; neither is published.

## the one-board trial

version 3 was built with the pinned ESP-IDF v5.5 / ESP-Matter setup, enabled
explicitly and written to the existing app partition only. the write's data hash
and the boot-time `experimental-relative-balance-v3` flag were verified.
existing Matter fabrics loaded; the node reported already commissioned, Wi-Fi
returned, Matter advertised and `BJ_LED` reconnected. no erase, bootloader,
partition-table, NVS or commissioning writes were commanded. private config
and partition settings were unchanged, and a neutral app-only rollback was kept.

an exact `(120,18,0)` diagnostic override was restored first. that bypasses the
profile; reconnecting or looking at that preset alone would prove nothing about
the correction. the next Apple Home color update releases the override and
exercises the profile. after trying it in Home, the user reported:

> it's not perfect, but it's pretty damn good.

that's subjective acceptance on **one controller/strip**, not a measured global
calibration, a per-color validation matrix or proof all brightness levels work.
no specific purple, cyan, CCT or dim-color result was recorded in that feedback.
no automatic sweep followed the flash. optimistic BLE writes remain optimistic.

the profile and dispatch headers in this tree match the built trial source.
the local build used existing private settings; no fresh-clone vendor build is
claimed by publication or host CI. raw logs, camera captures, device/account
data, private headers and rollback binaries stay out of the repository.

## status

version 3: opt-in, host-tested heuristic with a successful embedded build and
app-only trial on one setup, plus the user's encouraging Home feedback.
identity remains the default and rollback option. keep the limitations above;
we haven't outsmarted cheap LED hardware into being a colorimeter.
