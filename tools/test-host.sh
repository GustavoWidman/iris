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
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -DIRIS_EXPERIMENTAL_RELATIVE_BALANCE=1 \
    tests/relative_balance_test.cpp -o "$BUILD/relative_balance_enabled"
"$BUILD/relative_balance_enabled"
printf 'PASS relative_balance_enabled\n'
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror tools/show-color-profile.cpp -o "$BUILD/show_color_profile"
"$BUILD/show_color_profile" > "$BUILD/mappings.csv"
test "$(wc -l < "$BUILD/mappings.csv")" -eq 43
grep -qx 'yellow_source,120,120,0,120,18,0' "$BUILD/mappings.csv"
grep -qx 'white_source,128,128,128,128,19,4' "$BUILD/mappings.csv"
printf 'PASS hardware-free profile mapping report\n'
# Reject invalid flags before any vendor SDK/build/hardware access.
if "${CXX:-c++}" -std=c++17 -DIRIS_EXPERIMENTAL_RELATIVE_BALANCE=2 \
    tests/relative_balance_test.cpp -o "$BUILD/invalid_profile" 2>"$BUILD/invalid_profile.txt"; then
    printf 'Invalid profile flag unexpectedly compiled\n' >&2; exit 1
fi
grep -q 'must be 0 or 1' "$BUILD/invalid_profile.txt"
printf 'PASS invalid profile flag rejected\n'
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
