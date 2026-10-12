#!/usr/bin/env bash
# No SDK downloads, serial access, credentials or firmware artifacts.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
BUILD="$(mktemp -d "${TMPDIR:-/tmp}/bj-led-host.XXXXXXXX")"
trap 'rm -rf "$BUILD"' EXIT
for test in tests/*_test.cpp; do
    name="$(basename "$test" .cpp)"
    "${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror "$test" -o "$BUILD/$name"
    "$BUILD/$name"
    printf 'PASS %s\n' "$name"
done
export PYTHONDONTWRITEBYTECODE=1
for test in tests/*_test.py; do
    python3 "$test"
    printf 'PASS %s\n' "$(basename "$test" .py)"
done
bash -n tools/env.sh tools/test-host.sh
python3 tools/configure.py --help >/dev/null
python3 tools/generate_pairing.py --help >/dev/null
python3 tools/serial_session.py --help >/dev/null
printf 'PASS shell syntax and tool help\n'
