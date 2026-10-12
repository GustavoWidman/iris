# setup

these instructions target the tested **ESP32-S3 / 16 MB flash** configuration.
check your module's flash size before proceeding; the partition table reserves a
5 MB factory application. A 4 MB board cannot use this layout. Other ESP32
variants, including the original DevKit V1, have not been validated.

use Bash for the examples. do not flash, open serial or erase a board just to
run the host tests. on an already commissioned board, read **updates and recovery**
below first.

## 1. check the hardware-free code

```sh
bash tools/test-host.sh
# optional, when installed:
CXX=clang++ bash tools/test-host.sh
```

no ESP SDK, Python package, credentials or connected device is required. CI runs
these same checks with GCC and Clang; it does not build or upload firmware.

## 2. install the sdks

the development firmware was built with **ESP-IDF v5.5** and **ESP-Matter
release/v1.5**. the known tested source revisions are:

| SDK | Revision |
| --- | --- |
| ESP-IDF v5.5 | `8c750b088c7cd857d079c0eeb495da199b359461` |
| ESP-Matter release/v1.5 | `aeefd294b55ad9209da9bcc1757be73c8bad3cf3` |
| connectedhomeip submodule | `9c2a63634e880b92e29b25a49543b70a4baf1572` |

`dependencies.lock` records managed components. these revisions come from the
working development checkout; the public packaging changes have not been rebuilt
with the embedded SDK. do not assume the newest SDK is source-compatible.

install the host prerequisites in the [ESP-IDF guide](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/get-started/linux-macos-setup.html)
and [ESP-Matter guide](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html).
then, from the iris repository:

```sh
mkdir -p "$HOME/esp"
git clone --branch v5.5 --recursive https://github.com/espressif/esp-idf.git "$HOME/esp/esp-idf"
( cd "$HOME/esp/esp-idf" && git checkout 8c750b088c7cd857d079c0eeb495da199b359461 && git submodule update --init --recursive && ./install.sh esp32s3 )
source "$HOME/esp/esp-idf/export.sh"
mkdir -p .tools
git clone --branch release/v1.5 --recursive https://github.com/espressif/esp-matter.git .tools/esp-matter
( cd .tools/esp-matter && git checkout aeefd294b55ad9209da9bcc1757be73c8bad3cf3 && git submodule update --init --recursive && ./install.sh )
source tools/env.sh
```

recursive SDK checkout and installation are large downloads. if already
installed elsewhere, `tools/env.sh` respects `IDF_PATH` and `ESP_MATTER_PATH`:

```sh
export IDF_PATH="$HOME/esp/esp-idf"
export ESP_MATTER_PATH="$HOME/esp/esp-matter"
source tools/env.sh
```

do this in each new terminal. keep SDK trees out of the repository.

## 3. create private settings

```sh
umask 077
python3 tools/configure.py
```

both bootstrap Wi-Fi fields are prompted without echo. the tool generates a
unique Matter passcode, 12-bit discriminator and 16-byte salt, then writes
`.private/private_config.h` as mode `0600`. it refuses to overwrite existing
settings (including symlinks). no secret is printed or passed on the command line.
an SSID is limited to 32 UTF-8 bytes, password to 63 bytes.

if you prefer to supply Wi-Fi through the commissioner, use
`python3 tools/configure.py --no-wifi`. this generates empty bootstrap Wi-Fi
fields, not a different firmware mode; the initial empty bootstrap attempt can
fail until the commissioner supplies a network. the actual project was tested
with bootstrap Wi-Fi; the no-bootstrap onboarding path needs device validation.

[examples/private_config.h.example](../examples/private_config.h.example) shows
all consumed defines but deliberately will not compile. the optional
`BRIDGE_LED_ADDR` restricts selection when several controllers advertise `BJ_LED`.
use six bytes in **NimBLE internal order** (reverse of the usual displayed
address), in the private header only. without it, the first matching named device
may be selected. neither device address nor BLE name is cryptographic identity.

## 4. build locally

```sh
umask 077
mkdir -p build && chmod 700 build
source tools/env.sh
idf.py set-target esp32s3
idf.py build
python3 tools/generate_pairing.py
```

onboarding codes go to owner-only `.private/pairing.txt`; view them locally to add
the accessory, never in a public issue or CI log. the firmware uses development
VID `0xFFF1` / PID `0x8000`; it is not Matter-certified. Firmware embeds the
bootstrap Wi-Fi password and onboarding material. do not upload `build/`, release
binaries, private headers, flash backups or pairing QR codes.

## 5. flash a new board and pair

**Only for a board you deliberately intend to provision.** Set the actual serial
device path; iris does not choose one automatically.

```sh
PORT='/dev/your-serial-device' # replace with the actual device
idf.py -p "$PORT" flash
```

keep the controller and bridge powered and nearby. close MohuanLED to release
its BLE connection. In Apple Home, add the Matter accessory using the locally
generated manual code. the bridge exposes one extended-color light. Matter
commissioning and BLE central control share the ESP-Matter/NimBLE host.

after pairing, test one deliberate on/off action and observe the strip. A
`BLE ready` log or accepted write alone is not end-to-end proof. for diagnostics,
see [troubleshooting](troubleshooting.md). Opening serial can reset the board.

local Apple Home pairing/control was observed without a HomePod or Apple TV;
this is a tested setup, not a promise for all iOS versions. Remote access and
hub-dependent automation requirements still follow Apple's guidance.

## updates and recovery

on a commissioned device, do **not** run `erase-flash`, `factory-reset`, switch
partition tables or blindly restore a full image. keep private backups of the
current flash, application and configuration before making a deliberate update.
a full-flash restore replaces Wi-Fi, NVS and Matter fabrics; an old backup can
undo later pairings. SDK migrations may also make older application images
incompatible with current NVS.

the supplied factory app starts at `0x20000` and has size `0x500000`. an app-only
update can preserve pairing **only if** the image and existing partition/SDK
layout are compatible. Verify board size, partition table, image size and checksum
before choosing a write command. no automatic app-only flashing or backup
restore command is included: copying a board-specific command is not a safe
recovery plan. save backups locally with restrictive permissions; never publish.

the console `pair` command opens a 900-second basic commissioning window for
multi-admin use. `factory-reset` erases Matter setup. neither is required for a
normal firmware build or host test.
