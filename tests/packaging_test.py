#!/usr/bin/env python3
"""Docs/art/tooling contracts without SDKs, credentials or hardware."""

from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent.parent
for asset in (ROOT / "assets").glob("*.svg"):
    tree = ET.parse(asset)
    assert tree.getroot().attrib.get("role") == "img"
    assert tree.find("{http://www.w3.org/2000/svg}title") is not None
    for element in tree.iter():
        assert element.tag.split("}")[-1] not in {"script", "foreignObject", "image"}

# Every authored relative Markdown link resolves, including directory anchors.
for document in [
    ROOT / "README.md",
    ROOT / "CONTRIBUTING.md",
    ROOT / "SECURITY.md",
    ROOT / "THIRD_PARTY.md",
    *list((ROOT / "docs").glob("*.md")),
]:
    for target in re.findall(r"\[[^\]]*\]\(([^)]+)\)", document.read_text()):
        if not target.startswith(("http:", "https:", "#")):
            assert (document.parent / target.split("#")[0]).exists(), (document.name, target)

# Fresh reference config matches actual required defines without shipping usable secrets.
example = (ROOT / "examples/private_config.h.example").read_text()
assert "#error" in example
for define in [
    "BRIDGE_WIFI_SSID",
    "BRIDGE_WIFI_PASS",
    "BRIDGE_SETUP_PIN",
    "BRIDGE_DISCRIMINATOR",
    "BRIDGE_PAIR_SALT",
]:
    assert f"#define {define}" in example
assert not (ROOT / "tools/upload.py").exists()

# Explicit port + bounded lifetime rejected before pyserial import or any hardware use.
for arguments in [
    [],
    ["--port", "unused", "--seconds", "nan"],
    ["--port", "unused", "--seconds", "inf"],
    ["--port", "unused", "--seconds", "3601"],
    ["--port", "unused", "--seconds", "1", "--command", "inf:status"],
]:
    result = subprocess.run(
        ["python3", str(ROOT / "tools/serial_session.py"), *arguments],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 2, result.stderr
    assert "Traceback" not in result.stderr

# Portable env honors override paths (including spaces), exports both SDKs, fails early if absent.
with tempfile.TemporaryDirectory(prefix="iris env ") as temporary:
    root = Path(temporary)
    for name, variable in [("idf", "FAKE_IDF"), ("matter", "FAKE_MATTER")]:
        (root / name).mkdir()
        (root / name / "export.sh").write_text(f"export {variable}=yes\n")
    command = (
        f'export IDF_PATH="{root}/idf" ESP_MATTER_PATH="{root}/matter"; '
        f'source "{ROOT}/tools/env.sh"; '
        'test "$FAKE_IDF:$FAKE_MATTER" = yes:yes'
    )
    subprocess.run(["bash", "-e", "-c", command], check=True)
    result = subprocess.run(
        ["bash", "-c", f'export IDF_PATH="{root}/missing"; source "{ROOT}/tools/env.sh"'],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 1 and "Missing SDKs" in result.stderr

license_text = (ROOT / "licenses/Tanner-Helland-BSD-2-Clause.txt").read_text()
assert "2018" in license_text and "Tanner Helland" in license_text
assert "BSD-2-Clause" in (ROOT / "main/color_math.h").read_text()

# Pairing output honors external SDK location and never prints onboarding values.
# The stub avoids depending on vendor packages while exercising local file permissions.
import os
import shutil

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    (root / "tools").mkdir()
    (root / ".private").mkdir(mode=0o700)
    shutil.copy(ROOT / "tools/generate_pairing.py", root / "tools/generate_pairing.py")
    (root / ".private/private_config.h").write_text(
        "#define BRIDGE_SETUP_PIN 24681012\n#define BRIDGE_DISCRIMINATOR 123\n"
    )
    sdk = root / "external-sdk"
    library = sdk / "connectedhomeip/connectedhomeip/src/setup_payload/python"
    library.mkdir(parents=True)
    (library / "SetupPayload.py").write_text(
        "class SetupPayload:\n"
        " def __init__(self, discriminator, pin, rendezvous, vid, pid):\n"
        "  assert (discriminator, pin, rendezvous, vid, pid) == (123, 24681012, 2, 0xFFF1, 0x8000)\n"
        ' def generate_manualcode(self): return "test-manual-only"\n'
        ' def generate_qrcode(self): return "test-qr-only"\n'
    )
    output = root / ".private/pairing.txt"
    output.write_text("old")
    output.chmod(0o644)
    environment = dict(os.environ, ESP_MATTER_PATH=str(sdk))
    result = subprocess.run(
        ["python3", str(root / "tools/generate_pairing.py")],
        env=environment,
        capture_output=True,
        text=True,
        check=True,
    )
    assert output.stat().st_mode & 0o777 == 0o600
    assert "test-manual-only" in output.read_text() and "test-qr-only" in output.read_text()
    assert "test-manual-only" not in result.stdout and "test-qr-only" not in result.stdout
    output.unlink()
    untouched = root / "untouched.txt"
    untouched.write_text("preserve")
    output.symlink_to(untouched)
    result = subprocess.run(
        ["python3", str(root / "tools/generate_pairing.py")],
        env=environment,
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0 and untouched.read_text() == "preserve"
