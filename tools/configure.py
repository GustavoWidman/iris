#!/usr/bin/env python3
"""Generate owner-only bootstrap settings; no hardware access or secret output."""
import argparse
import getpass
import os
from pathlib import Path
import secrets

ROOT = Path(__file__).resolve().parent.parent
INVALID_PINS = {0, 11111111, 22222222, 33333333, 44444444, 55555555,
                66666666, 77777777, 88888888, 99999999, 12345678, 87654321}


def generate_pin():
    while True:
        pin = secrets.randbelow(99999998) + 1
        if pin not in INVALID_PINS:
            return pin


def c_string(value):
    # Encode UTF-8 as fixed-width octal escapes: portable C++ literal, no injection.
    return '"' + ''.join(f'\\{byte:03o}' for byte in value.encode('utf-8')) + '"'


def render_config(ssid, password):
    if len(ssid.encode('utf-8')) > 32 or len(password.encode('utf-8')) > 63:
        raise ValueError('Wi-Fi SSID/password exceed supported byte lengths')
    if '\0' in ssid or '\0' in password:
        raise ValueError('Wi-Fi fields must not contain NUL')
    salt = ', '.join(f'0x{byte:02x}' for byte in secrets.token_bytes(16))
    return (f'#pragma once\n// Private generated settings. Never publish.\n'
            f'#define BRIDGE_WIFI_SSID {c_string(ssid)}\n'
            f'#define BRIDGE_WIFI_PASS {c_string(password)}\n'
            f'#define BRIDGE_SETUP_PIN {generate_pin()}\n'
            f'#define BRIDGE_DISCRIMINATOR {secrets.randbelow(4096)}\n'
            f'#define BRIDGE_PAIR_SALT {{{salt}}}\n')


def write_new_config(path, content):
    path.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    # Exclusive creation refuses existing files and symlinks; no accidental key rotation.
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, 'w') as handle:
        handle.write(content)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--no-wifi', action='store_true',
                        help='leave bootstrap Wi-Fi empty; provision it via Matter commissioning')
    args = parser.parse_args()
    ssid = '' if args.no_wifi else getpass.getpass('Bootstrap Wi-Fi SSID (hidden): ')
    password = '' if args.no_wifi else getpass.getpass('Bootstrap Wi-Fi password (hidden): ')
    try:
        write_new_config(ROOT / '.private/private_config.h', render_config(ssid, password))
    except (ValueError, OSError) as error:
        parser.exit(1, f'Configuration not written: {error}\n')
    print('Created .private/private_config.h (owner-only); values are not printed.')


if __name__ == '__main__':
    main()
