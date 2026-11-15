# Putting Croc-O-Synth on an FM-1 (Windows)

**Status: not yet tried on a real FM-1.** The firmware builds for the device and passes its checks in the simulator. The first install is itself the test. Use a spare unit.

The steps below come from reading fm1-base's tools and installer text. They have not been run on Windows by this project yet; correct this page after the first install.

## Before you start

1. Download M-VAVE's official FM-1 firmware file (m-vave.com, Downloads, PC Firmware) and keep it. It is how you go back.
2. Use a USB **data** cable, plugged straight into the computer.
3. Close every other program and browser tab that uses MIDI (DAWs, editors, other installers).

## Get the firmware package

1. On GitHub, open the repository's **Actions** tab and pick the latest green **Build firmware** run.
2. Under **Artifacts**, download `croc-o-synth-firmware` and unzip it. The file you need is `crocosynth.fwsc`.

To make a fresh build, press **Run workflow** on the same page.

## Install

One-time setup, in a terminal (PowerShell):

    git clone --recursive https://github.com/Goosegoosegoosegoosegoose/croc-o-synth
    cd croc-o-synth
    pip install mido python-rtmidi

Then, with the FM-1 switched on and running whatever firmware it has (no special mode):

    python base\tools\fm1_install.py --info
    python base\tools\fm1_install.py path\to\crocosynth.fwsc

`--info` only asks the FM-1 what it is; run it first to see that it is found. The install shows UPDATING and a bar on the FM-1, or a dark screen. Do not unplug or switch off until the tool says it is done. It can take several minutes.

Expected afterwards: the name CROC-O-SYNTH, then "BEEN AWHILE, CROCO-DIAL!", then the home croc. Keys make sound; EDIT opens the Teeth lesson.

## What to report back

- Did `--info` find the FM-1, and what did it print?
- Did the croc appear? Does it move smoothly or in jerks?
- Does it sound right? Any crackle when holding a chord with three rows of teeth?
- Do the knobs and buttons do what the screen says?

## If something goes wrong

In order, from mild to severe:

1. **It runs, but you want the original back.** Use M-VAVE's own updater, M-UPGRADE (m-vave.com, Downloads, PC Software), with the official firmware file.
2. **It crashes while starting.** After two failed starts the platform shows **CROC-O-SYNTH SAFE MODE**. USB still works there: run the install again with a fixed package, or go back to the original as in 1.
3. **Safe mode fails too.** After four failed starts the chip falls into its own update mode. Getting out needs felucca-bsp's rescue tool (`base/bsp/tools/fm1_rescue.sh`), which runs on a Mac or on Linux, not on Windows. On a Windows-only desk that means a Linux live USB stick, or WSL with the USB device passed through. Neither has been tried by this project.

Installing the open m-bootloader beforehand adds a recovery menu (hold PLAY while switching on). fm1-base's web installer pages offer it; this page's Python route does not install it.
