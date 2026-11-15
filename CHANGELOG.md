# Croc-O-Synth firmware builds

Every firmware build, newest first: what changed, what was checked, and what happened on the real FM-1. Each build is one commit in this repository and one **Build firmware** run on GitHub (Actions tab), where its install folder can be downloaded.

All of these are development builds, so the FM-1 reports the same identity for each (`FM-1_5800`). The commit is what tells them apart.

All dates are 2026-11-15.

## Build 7: swamp echo and the cave

- Commit: the one that adds this entry. Download: double-click `update-croc.bat`, or the **latest** release.
- **Added, on FX:**
  - knob 1 **ECHO**: how many times the swamp answers (the delay's amount and feedback). Smaller crocs appear further off, one for each answer
  - knob 2 **FAR**: how far away the echo is (the delay time, 0.18 to 0.6 s). It glides like a tape when turned, so there are no clicks
  - knob 3 **CAVE**: how big the cave is (reverb). The sky darkens and rock walls close in from the sides
  - each knob has its line and readout, as on the other lessons
  - HOME's knob 4 is ECHO; the five presets have their echo and cave settings
- **Changed:** installing from now on is a double-click (`tools/update-croc.bat`); each build is also published as the repository's latest release.
- **Kept:** the croc's name and My Croc from builds 5 and 6 carry over (the saved record grew; the old one is read once and saved again in the new form).
- **Checked:** simulator checks pass, with echo and cave added. Measured: each echo is about half as loud as the one before, 0.34 s apart at the middle setting; the cave's tail fades smoothly over about 0.7 s. A build 6 save loaded into this build kept its name and sound.
- **Caught before release:** the first version fed the echo back into the filter and grew louder with each repeat. The recordings showed it; fixed.
- **On the FM-1:** not reported yet. This build uses the most memory and processing so far (a 0.6 s delay line and a small reverb), so it is the one to listen to for crackle.

## Build 6: tickle

- Commit: the one that adds this entry. Download: the matching **Build firmware** run on the Actions tab.
- **Changed, on the tail wag lesson (LFO):**
  - knob 3 is now **TICKLE**: tickle the **tail** (the note wobbles up and down: pitch), the **belly** (the sound goes loud and soft: volume, new), or the **snoot** (the sound goes wah wah: the filter). A feather moves along the croc from back to front as the knob turns
  - each knob has a line saying what it does and where it is set, as on the envelope lesson
  - **size** makes the tail grow
  - the croc reacts: it bobs for the tail, its belly swells for the belly, its jaw flaps for the snoot, and its eyes squeeze shut laughing whenever the wag is on
  - the home croc shows the same things
- **Note:** a My Croc sound saved in build 5 with the jaw wobble now tickles the belly; turn TICKLE one more step to get the snoot back.
- **Checked:** simulator checks pass. Measured: tickling the belly swings the loudness about 8 times at a big wag and leaves pitch and brightness alone; the tail moves the pitch; the snoot sweeps the brightness.
- **On the FM-1:** reported as great so far.

## Build 5: envelope, tail wag, presets, the croc's name

- Commit: the one that adds this entry. Download: the matching **Build firmware** run on the Actions tab.
- **Added:**
  - **ENV**, snap or yawn: Open, Relax, Hold, Close on knobs 1 to 4, each with a line saying what it does and where it is set; the shape of one note at the bottom, with the part being turned lit
  - **LFO**, tail wag: speed, size, and what wags (the voice or the jaws)
  - **PRESETS knob**: five ready-made sounds (Baby Croc, Swamp Bass, Chomper, Sleepy Yawn, Wiggle Tail), then My Croc. Changing any setting by hand makes the sound My Croc; the presets never overwrite it
  - **Name your croc** on the first power-on (knob 1 the letter, knob 2 or a key the next spot, HOME when done). The name shows on the greeting, the home screen and the goodbye. Hold GLO to rename
  - **GLO** shows the two hold actions
  - HOME's knobs: 1 teeth, 2 snap (open and close together), 3 wag
  - "BUILD 5" in the corner of the greeting, so the build in an FM-1 can be read off its screen
- **Checked:** the simulator checks pass, now 60 of them, including naming, presets, and everything kept across a power cycle. Measured in the simulator: the wag moves the pitch by about a semitone each way at its largest, and on the jaws it sweeps the brightness without moving the pitch.
- **Note:** the saved record changed, so the first start after installing asks for the croc's name and begins from Baby Croc.
- **On the FM-1:** installed; the copy on the tail wag lesson needed work ("What" explained nothing), which led to build 6.

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
