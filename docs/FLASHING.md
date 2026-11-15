# Putting Croc-O-Synth on an FM-1 (Windows)

**Status: these steps have been used on a real FM-1 from Windows** (2026-11-15, over the Melodee firmware). The recovery steps at the end have not been needed or tried.

## Before you start

1. Download M-VAVE's official FM-1 firmware file (m-vave.com, Downloads, PC Firmware) and keep it. It is how you go back.
2. Use a USB **data** cable, plugged straight into the computer.
3. Close every other program and browser tab that uses MIDI (DAWs, editors, other installers).

## Get the install folder

1. On GitHub, open the repository's **Actions** tab and pick the latest green **Build firmware** run.
2. Under **Artifacts**, download `croc-o-synth-install` and unzip it. It holds the firmware (`crocosynth.fwsc`), the installer tool and a short READ-ME-FIRST.

To make a fresh build, press **Run workflow** on the same page. Git is not needed on the PC.

## Install

Open PowerShell in the unzipped folder (Shift + right-click the folder, "Open PowerShell window here"). With the FM-1 switched on and running whatever firmware it has (no special mode):

    py -m pip install mido python-rtmidi
    py fm1_install.py --info
    py fm1_install.py crocosynth.fwsc

If `py` is not found, use the full path to `python.exe` in its place. `--info` only asks the FM-1 what it is; run it first to see that it is found. The install shows UPDATING and a bar on the FM-1, or a dark screen. Do not unplug or switch off until the tool says it is done. It can take several minutes.

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
