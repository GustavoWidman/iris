#!/usr/bin/env bash
# Source from Bash. Existing SDK locations can be overridden without editing this file.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export IDF_PATH="${IDF_PATH:-$HOME/esp/esp-idf}"
export ESP_MATTER_PATH="${ESP_MATTER_PATH:-$ROOT/.tools/esp-matter}"
if [[ ! -f "$IDF_PATH/export.sh" || ! -f "$ESP_MATTER_PATH/export.sh" ]]; then
    printf '%s\n' 'Missing SDKs: set IDF_PATH and ESP_MATTER_PATH (see docs/setup.md).' >&2
    return 1 2>/dev/null || exit 1
fi
source "$IDF_PATH/export.sh" || return 1
source "$ESP_MATTER_PATH/export.sh" || return 1
