# Croc-O-Synth firmware builds

Every firmware build, newest first: what changed, what was checked, and what happened on the real FM-1. Each build is one commit in this repository and one **Build firmware** run on GitHub (Actions tab), where its install folder can be downloaded.

All of these are development builds, so the FM-1 reports the same identity for each (`FM-1_5800`). The commit is what tells them apart.

All dates are 2026-11-15.

## Build 4: three croc sizes

- Commit `45d99db`. Download: [run 43717276242](https://github.com/Goosegoosegoosegoosegoose/croc-o-synth/actions/runs/43717276242)
- **Changed:** the sizes went from five to three: small croc (F5 to G7), croc (F4 to G6), big croc (F3 to G5). Everything moved up one octave so no size goes below what the FM-1's speaker can play.
- **Includes** everything in build 3.
- **Checked:** simulator tests pass.
- **On the FM-1:** installed. It worked and sounds great: the crackling and the strange low notes are gone.

## Build 3: deep bass kept off the speaker

- Commit `df397d2`. Download: [run 43716268194](https://github.com/Goosegoosegoosegoosegoose/croc-o-synth/actions/runs/43716268194)
- **Why:** build 2 made low notes sound strange and crackle, worse at the lower sizes.
- **Changed:**
  - a high-pass filter at 170 Hz ahead of the speaker; 30 Hz when headphones are plugged in (the jack is now read)
  - the low-note level boost from build 2 removed
  - round teeth bend more the lower the note, so harmonics carry the pitch
  - the limiter releases slowly, so it no longer reshapes each cycle of a low note
  - overall level set between builds 1 and 2
- **Checked:** simulator tests pass; the row-change click stays fixed.
- **On the FM-1:** not installed on its own; its changes were heard as part of build 4.

## Build 2: first sound fixes

- Commit `21e12f0`. Download: [run 43715044774](https://github.com/Goosegoosegoosegoosegoose/croc-o-synth/actions/runs/43715044774)
- **Why:** on build 1, low notes were nearly silent at full volume and changing rows crackled.
- **Changed:**
  - round teeth are no longer a pure sine: they have a second and third harmonic
  - low notes boosted, and the whole output about three times louder
  - a limiter in place of the soft clip
  - changing rows or tooth shape fades over one audio block (5.8 ms)
  - the jaws glide; a note from silence starts its rows in phase
- **Checked:** simulator tests pass; the jump when going from one row to two fell from about 7 times the normal signal to about the same as it.
- **On the FM-1:** installed. Low notes sounded strange and crackled at random, more so at the Croc and Big Croc sizes. The boost and the level were too much for the speaker. Fixed in build 3.

## Build 1: first firmware

- Commits `4ff7df2` (the firmware), `28299ff` (first build that compiled for the device), `4f25a69` (installer bundled with the download; same firmware). Download: [run 43712734476](https://github.com/Goosegoosegoosegoosegoose/croc-o-synth/actions/runs/43712734476)
- **In it:** the start-up greeting, the home croc, the Teeth lesson on EDIT (tooth shape, rows, crooked), the jaws on ALGORITHM, five croc sizes on OCT, goodbye on a held HOME, four notes at once, the sound saved across power cycles, "coming soon" on the other buttons.
- **Checked:** 23 simulator checks; notes at the right pitch with no clicks.
- **On the FM-1:** installed over Melodee on the spare unit; first run on real hardware. The croc moved very smoothly. Low notes were nearly silent, middle notes quiet, and changing rows crackled. Fixed in builds 2 and 3.

## Failed build

- Commit `0698c60`: did not compile for the device. Two names in the firmware (`panel`, `FS`) clashed with the platform's on the real chip; the simulator had not caught it. Renamed in `28299ff`.
