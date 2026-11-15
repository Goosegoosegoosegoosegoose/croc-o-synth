#!/bin/sh
# Build Croc-O-Synth with fm1-base's build tool.
#   ./build.sh            the firmware package for the FM-1 (needs the JieLi toolchain and SDK)
#   ./build.sh --host     the simulator: app/build/host/crocosynth_host
#   ./build.sh --web      the browser emulator: app/build/emu/
set -e
cd "$(dirname "$0")"
[ -f base/tools/build.py ] || { echo "build.sh: base/ is empty. Run: git submodule update --init --recursive"; exit 1; }
exec python3 base/tools/build.py app "$@"
