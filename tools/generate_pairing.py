#!/usr/bin/env python3
"""Generate private Matter onboarding codes from local settings; no hardware access."""
from pathlib import Path
import argparse
import os
import re
import sys

ROOT = Path(__file__).resolve().parent.parent


def numeric_define(source, name):
    match = re.search(r'^#define\s+' + re.escape(name) + r'\s+(\d+)\s*$', source, re.M)
    if not match:
        raise ValueError('Missing numeric pairing setting: ' + name)
    return int(match.group(1))


def main():
    argparse.ArgumentParser(description=__doc__).parse_args()
    source = (ROOT / '.private/private_config.h').read_text()
    matter = Path(os.environ.get('ESP_MATTER_PATH', ROOT / '.tools/esp-matter'))
    sys.path.insert(0, str(matter / 'connectedhomeip/connectedhomeip/src/setup_payload/python'))
    from SetupPayload import SetupPayload
    payload = SetupPayload(numeric_define(source, 'BRIDGE_DISCRIMINATOR'),
                           numeric_define(source, 'BRIDGE_SETUP_PIN'),
                           rendezvous=2, vid=0xFFF1, pid=0x8000)
    output = ROOT / '.private/pairing.txt'
    flags = os.O_WRONLY | os.O_CREAT | os.O_TRUNC | getattr(os, 'O_NOFOLLOW', 0)
    fd = os.open(output, flags, 0o600)
    os.fchmod(fd, 0o600)
    with os.fdopen(fd, 'w') as handle:
        handle.write('Apple Home manual setup code: ' + payload.generate_manualcode() + '\n')
        handle.write('Matter QR payload: ' + payload.generate_qrcode() + '\n')
    print('Wrote private onboarding codes to .private/pairing.txt')


if __name__ == '__main__':
    main()
