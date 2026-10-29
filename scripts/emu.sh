#!/usr/bin/env bash
# Small helper around the Pebble emulator, so that iterating on the UI is quick.
#
#   scripts/emu.sh install [platform]   build, install and launch the app
#   scripts/emu.sh shot <file>          save a screenshot of the emulator
#   scripts/emu.sh press <button>       click a button (back, up, select, down)
#   scripts/emu.sh launch [platform]    launch the app from the app list
set -euo pipefail

export PATH="$HOME/.local/bin:$PATH"
PLATFORM="${PEBBLE_PLATFORM:-emery}"
cd "$(dirname "$0")/.."

case "${1:-}" in
  install)
    platform="${2:-$PLATFORM}"
    pebble build
    pebble install --emulator "$platform"
    sleep 2
    scripts/emu.sh launch "$platform"
    ;;
  launch)
    platform="${2:-$PLATFORM}"
    # BACK leaves the app (or the dictation UI) and lands on the app list, where
    # the app stays highlighted, so a single SELECT starts it again.
    pebble emu-button click back --emulator "$platform" >/dev/null
    sleep 1
    pebble emu-button click select --emulator "$platform" >/dev/null
    sleep 2
    ;;
  shot)
    out="${2:?usage: emu.sh shot <file>}"
    mkdir -p "$(dirname "$out")"
    pebble screenshot --no-open --emulator "$PLATFORM" "$out"
    ;;
  press)
    pebble emu-button click "${2:?usage: emu.sh press <button>}" --emulator "$PLATFORM" >/dev/null
    sleep 1
    ;;
  *)
    echo "usage: $0 {install|launch|shot|press} [arg]" >&2
    exit 2
    ;;
esac
