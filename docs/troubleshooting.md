# troubleshooting

## home works but the strip doesn't respond

- keep both devices powered and within Bluetooth range. close MohuanLED; many
  controllers accept only one BLE connection.
- Confirm the advertised name is exactly `BJ_LED` and the writable `EE01`
  characteristic exists. Matching a similar name does not establish compatibility.
- `BLE ready=1` confirms a connection/characteristic, not light output. A write
  counter records local API acceptance only. observe one deliberate command.
- with several `BJ_LED` devices nearby, add the optional address filter in your
  private configuration; see [setup](setup.md).

## home says off, but it is still lit

this happened during development: an ordinary `off` command updated Matter but
did not darken the strip. exact `rgb 0 0 0` was later physically confirmed dark on
the tested controller, even with the follow-up on frame. that does not establish
controller power-off or universal blackout reliability. use the physical remote
if you need an immediate dependable stop, and observe the LEDs directly.

## color is surprising

primaries and mixed colors need separate observations. Low blue values caused
large visual changes on the development controller; similar symptoms existed in
MohuanLED before iris. Calibration remains neutral. the XY conversion fix is
mathematically tested, but cannot repair an unknown LED/controller response.
See [color investigation](color-investigation.md) before changing gains or gamma.
white from an RGB strip is not a dedicated, high-CRI white emitter.

## bounded serial capture

opening a serial port can reset an ESP32 even when modem lines are configured
before opening. stop other serial tools first and check Wi-Fi, Matter and BLE
reconnect afterward. do not run diagnostics unattended or while preserving a
sensitive lighting state. Python `pyserial` is needed only for actual captures:

```sh
# in the active esp-idf python environment:
python3 -m pip install pyserial
PORT='/dev/your-serial-device' # replace with the actual device
python3 tools/serial_session.py --port "$PORT" --seconds 30 --output evidence/status.log --command '5:status'
```

the output is owner-only, but raw SDK logs may contain network or pairing
information. review and redact before sharing; logs are ignored by Git. the tool
has no daemon, remote endpoint or automatic port discovery. Sessions are finite,
up to 3600 seconds; scheduled command delays must lie inside that window.
`--command SECONDS:COMMAND` sends an arbitrary **native console command**; it is
not the restricted queue. do not schedule reset, reboot or pairing inadvertently.

## one-at-a-time exact-rgb session

this is an opt-in physical test, not part of setup. Start one persistent, bounded
session instead of reopening serial for every color:

```sh
umask 077
mkdir -p evidence
: > evidence/rgb.queue
chmod 600 evidence/rgb.queue
python3 tools/serial_session.py --port "$PORT" --seconds 300 --output evidence/rgb.log --command-queue evidence/rgb.queue
```

while that session runs, append a single request from another terminal:

```sh
printf 'rgb 128 0 0\n' >> evidence/rgb.queue
# observe first. when deliberately ready to return to matter's desired state:
printf 'resume\n' >> evidence/rgb.queue
```

the queue accepts only byte-range `rgb R G B`, `resume` and `status`; it rejects
non-regular files, symlinks, foreign owners and group/other permissions. use a
trusted, private local directory; this is not an adversarial multi-user sandbox.
the serial program exits at its deadline, but **the RGB override remains in
firmware** until resumed, superseded by Matter or rebooted. ensure your intended
end state before the session ends. A local write is not physical confirmation.

## native console commands

| Command | Effect |
| --- | --- |
| `status` | Connection, optimistic write counts and memory |
| `on`, `off` | Update Matter on/off desired state |
| `dim` | Request very dim neutral HSV light |
| `rgb R G B`, `resume` | Temporary raw override / return to Matter |
| `disconnect` | Drop BLE; the worker will reconnect |
| `reboot` | Restart; saved desired state may be reapplied |
| `pair` | Open a 900-second commissioning window |
| `factory-reset` | **Destructive:** erase Matter setup |

avoid factory reset for ordinary BLE problems. on NVS initialization errors, the
firmware can erase NVS as a recovery measure; this is a prototype, not a
pairing-preservation guarantee under all failures.

## build or pairing problems

- run commands in Bash and source `tools/env.sh` in the current terminal.
- Confirm ESP-IDF v5.5, ESP-Matter release/v1.5 and target `esp32s3`; record exact
  SDK commits before reporting API/build compatibility issues.
- use `tools/configure.py` before building. the example header intentionally
  cannot compile. do not replace unique keys with a shared public passcode.
- check local Wi-Fi/mDNS reachability. A successful compiler run or QR generation
  does not establish successful commissioning.
- preserve current partitions, NVS and backups on a commissioned board; see the
  [recovery guidance](setup.md#updates-and-recovery).

## separate diagnostic firmware

`diagnostic/` contains a legacy BLE-only scanner/controller, not iris's Matter
application. it logs nearby Bluetooth addresses and can send raw commands.
Flashing it replaces the running app; do not deploy it to diagnose a working,
commissioned bridge casually. it has not been revalidated as part of the public
host test suite.
