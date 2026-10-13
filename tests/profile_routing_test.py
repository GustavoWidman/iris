#!/usr/bin/env python3
"""Static source contract, NOT vendor compilation or hardware verification."""

from pathlib import Path
import re


def compact(text):
    # Whitespace is not part of this routing contract: formatters may change it.
    return re.sub(r"\s+", "", text)


root = Path(__file__).resolve().parent.parent
source = compact((root / "main/app_main.cpp").read_text())
assert (
    compact("static const LedCalibration led_calibration = firmware_led_calibration();") in source
)
worker = source.split(compact("static void worker(void *) {"), 1)[1].split(
    compact("static void wifi_boot"), 1
)[0]
exact = worker.split(compact("if (exact.active) {"), 1)[1].split(
    compact("} else if (d.on && d.level)"), 1
)[0]
assert compact("const RGB rgb = exact.rgb;") in exact
assert "calibrated_rgb" not in exact and "experimental_relative_balance" not in exact
matter = worker.split(compact("} else if (d.on && d.level)"), 1)[1].split(
    compact("} else { ok = send_packet"), 1
)[0]
assert all(mode in matter for mode in ["hsv_rgb(", "kelvin_rgb(", "xy_rgb("])
assert matter.count(compact("calibrated_rgb(rgb, led_calibration)")) == 1
assert worker.count("calibrated_rgb(") == 1
assert "experimental_relative_balance(" not in source
cmake = compact((root / "main/CMakeLists.txt").read_text())
assert compact("option(IRIS_EXPERIMENTAL_RELATIVE_BALANCE ") in cmake
assert compact('identity otherwise)" OFF)') in cmake
assert compact("PRIVATE IRIS_EXPERIMENTAL_RELATIVE_BALANCE=1") in cmake
