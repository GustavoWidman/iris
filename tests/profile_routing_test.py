#!/usr/bin/env python3
"""Static source contract, NOT vendor compilation or hardware verification."""
from pathlib import Path
root = Path(__file__).resolve().parent.parent
source = (root / 'main/app_main.cpp').read_text()
assert 'static const LedCalibration led_calibration=firmware_led_calibration();' in source
worker = source.split('static void worker(void*){', 1)[1].split('static void wifi_boot', 1)[0]
exact = worker.split('if(exact.active){', 1)[1].split('}else if(d.on&&d.level)', 1)[0]
assert 'const RGB rgb=exact.rgb;' in exact
assert 'calibrated_rgb' not in exact and 'experimental_relative_balance' not in exact
matter = worker.split('}else if(d.on&&d.level)', 1)[1].split('}else{ok=send_packet', 1)[0]
assert all(mode in matter for mode in ['hsv_rgb(', 'kelvin_rgb(', 'xy_rgb('])
assert matter.count('calibrated_rgb(rgb,led_calibration)') == 1
assert worker.count('calibrated_rgb(') == 1
assert 'experimental_relative_balance(' not in source
cmake = (root / 'main/CMakeLists.txt').read_text()
assert 'option(IRIS_EXPERIMENTAL_RELATIVE_BALANCE ' in cmake
assert 'identity otherwise)" OFF)' in cmake
assert 'PRIVATE IRIS_EXPERIMENTAL_RELATIVE_BALANCE=1' in cmake
