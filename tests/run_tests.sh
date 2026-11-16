#!/bin/sh
# Build the simulator and run Croc-O-Synth's checks in it.
set -e
cd "$(dirname "$0")/.."
./build.sh --host >/dev/null
app/build/host/crocosynth_host app/tests/basic.txt app/build/host
app/build/host/crocosynth_host app/tests/sound.txt app/build/host
app/build/host/crocosynth_host app/tests/hops.txt app/build/host
app/build/host/crocosynth_host app/tests/remembers.txt app/build/host
app/build/host/crocosynth_host app/tests/stars.txt app/build/host
echo "tests: ok"
