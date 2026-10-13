#!/usr/bin/env python3
"""Private-config regression checks. All generated values live in a temp directory."""

import importlib.util
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("configure", ROOT / "tools/configure.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
for _ in range(100):
    pin = module.generate_pin()
    assert 1 <= pin <= 99999998 and pin not in module.INVALID_PINS

for ssid, password in [("a" * 33, ""), ("", "a" * 64), ("nul\0ssid", ""), ("", "\0")]:
    try:
        module.render_config(ssid, password)
        raise AssertionError("invalid configuration accepted")
    except ValueError:
        pass

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    content = module.render_config('quote"\\\n café', 'example"\\password')
    header = root / ".private/private_config.h"
    module.write_new_config(header, content)
    assert header.stat().st_mode & 0o777 == 0o600
    assert header.parent.stat().st_mode & 0o777 == 0o700
    assert len(re.findall(r"0x[0-9a-f]{2}", content)) == 16
    discriminator = int(re.search(r"BRIDGE_DISCRIMINATOR (\d+)", content)[1])
    assert 0 <= discriminator <= 4095
    try:
        module.write_new_config(header, "overwrite")
        raise AssertionError("existing keys overwritten")
    except FileExistsError:
        assert header.read_text() == content
    link = root / "link.h"
    link.symlink_to(header)
    try:
        module.write_new_config(link, "overwrite")
        raise AssertionError("symlink overwritten")
    except FileExistsError:
        pass
    # Validate generated C++ literals, lengths, setup values and the actual 16-byte salt.
    unit = root / "config.cpp"
    unit.write_text(
        '#include "' + str(header) + '"\n'
        "static_assert(sizeof(BRIDGE_WIFI_SSID) == 15);\n"
        "static_assert(sizeof(BRIDGE_WIFI_PASS) == 18);\n"
        "static_assert(BRIDGE_SETUP_PIN >= 1 && BRIDGE_SETUP_PIN <= 99999998);\n"
        "static_assert(BRIDGE_DISCRIMINATOR < 4096);\n"
        "constexpr unsigned char salt[] = BRIDGE_PAIR_SALT;\n"
        "static_assert(sizeof(salt) == 16);\nint main() {}\n"
    )
    subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++17",
            "-Wall",
            "-Wextra",
            "-Werror",
            str(unit),
            "-o",
            str(root / "config"),
        ],
        check=True,
    )
    # Run the documented fresh-user entrypoint in a private temporary checkout.
    (root / "tools").mkdir()
    shutil.copy(ROOT / "tools/configure.py", root / "tools/configure.py")
    header.unlink()
    result = subprocess.run(
        ["python3", str(root / "tools/configure.py"), "--no-wifi"],
        capture_output=True,
        text=True,
        check=True,
    )
    assert "BRIDGE_SETUP_PIN" not in result.stdout and "BRIDGE_PAIR_SALT" not in result.stdout
    assert header.exists()
    second = subprocess.run(
        ["python3", str(root / "tools/configure.py"), "--no-wifi"], capture_output=True, text=True
    )
    assert second.returncode == 1 and "File exists" in second.stderr
