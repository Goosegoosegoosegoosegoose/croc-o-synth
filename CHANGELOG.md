# Croc-O-Synth firmware builds

Every firmware build, newest first: what changed, what was checked, and what happened on the real FM-1. Each build is one commit in this repository and one **Build firmware** run on GitHub (Actions tab), where its install folder can be downloaded.

All of these are development builds, so the FM-1 reports the same identity for each (`FM-1_5800`). The commit is what tells them apart.

Builds 1 to 8 are dated 2026-11-15; builds 9 to 13 are 2026-11-16.

## Web app (no new firmware build)

- The firmware is now also built for the browser on every build and published at <https://goosegoosegoosegoosegoose.github.io/croc-o-synth/>. It is the same code as the FM-1 runs, inside fm1-base's emulator page.
- **Checked:** the published page opened in a headless Chrome: it starts and shows the name-your-croc screen. Sound was not listened to there.

## Build 13: croc songs

- Commit: the one that adds this entry. Download: double-click `update-croc.bat`, or the **latest** release.
- **Added, on SEQ:** eight ready-made songs. Press **SEQ** again on the lily pads page to switch between her own pads and the croc's songs.
  - knob 1 **SONG** picks: Twinkle Twinkle, Itsy Bitsy Spider, Mary Had a Lamb, Row Your Boat, Hot Cross Buns, Old MacDonald, Are You Sleeping, London Bridge
  - **PLAY** sings it, round and round with a short breath at the end; a newly picked song starts from its beginning
  - the song's notes show as lily pads, eight at a time, with the croc on the one it is singing; the key for each note lights up, to play along
  - knob 3 **HOP** (note length) and the SELECT knob (speed) work as on the lily pads. The song is sung in the croc's current sound
  - her own eight pads are left untouched, and keys do not plant notes while the songs are up
- **Saved:** which song (or her own pads), in one of the spare bytes.
- **Changed:** the four-lily-pads challenge needs her own pads, not a song.
- **Checked:** simulator checks pass, with a new script: the first notes of Twinkle Twinkle and Itsy Bitsy Spider at the right times, a new song starting at once, her pads untouched, the song kept through a power cycle. Screens looked at.
- **Not checked:** the tunes were written out by hand from memory and have not been listened to: a wrong note or rhythm is possible in any of them, the six not in the script especially.
- **On the FM-1:** not installed yet.

## Build 12: challenges and stars

- Commit: the one that adds this entry. Download: double-click `update-croc.bat`, or the **latest** release.
- **Added, on SAVE:** twelve challenges, one or two for each lesson, and a star for each one done.
  - knob 1 **PICK** chooses a challenge. The screen shows it in two lines, where to start, and the row of stars
  - the button of the lesson to go to blinks
  - the picked challenge is checked on every page. The star is won when it goes from not done to done, with a "YOU DID IT!" screen for three seconds
  - a challenge that is already done when it is picked says "UNDO IT, THEN DO IT AGAIN"
  - a star stays once won. All twelve: the title reads "EVERY STAR!"
  - hold **SAVE** for two seconds to give all the stars back (listed on the GLO page)
- **The challenges:** make the croc yawn (open 7+, close 7+); pointy teeth; three rows of crooked teeth (crook 3+); shut the jaws tight (1 or less); a big croc; tickle the belly (speed above 0); a huge, super fast wag (speed 6+, size 4); lots of echoes in a giant cave (both 5+); four lily pads, hopping; up and down on three keys; teach the croc a tune (4 notes, sung back); light up a scale.
- **Saved:** the stars and the picked challenge, in three of the spare bytes, so the record keeps its size and nothing else is lost in the update.
- **Checked:** simulator checks pass, with a new script: five stars won in different ways, no star for something already done, stars kept through a power cycle, stars given back by holding SAVE. Screens looked at.
- **Not checked:** challenges 3, 4, 6, 7, 8, 10 and 12 were not each played through in the script (their conditions are one line each). The blinking lesson button, on the device.
- **Not done:** the browser prototype has no challenges.
- **On the FM-1:** not installed yet.

## Build 11: croc remembers

- Commit: the one that adds this entry. Download: double-click `update-croc.bat`, or the **latest** release.
- **Added, on REC:** croc remembers. The croc listens to a tune on the keys and sings it back.
  - **REC** once: the croc waits, listening. The tune starts with the first key. PLAY turns red while it listens
  - **REC** again: it sings the tune back, again and again. The sound can be changed and other pages opened while it sings
  - **REC** a third time: quiet. The tune is kept: on this page **PLAY** sings it again, and stops it. On every other page PLAY is still the lily pads
  - knob 1 **SPEED**: how fast the croc sings it. Slow (half speed), the same, fast (double)
  - knob 2 **AGAIN**: sing it again and again, or just once
  - the tune shows as dots, left to right in time, higher notes higher, with a line where the singing is; the keys it sings light up
  - up to 120 notes or half a minute; then it starts singing by itself
- **Limits:** the tune lasts until the power goes off (it is not saved). It keeps what the keys did, not the sound, so it is sung in whatever sound the croc has now.
- **Checked:** simulator checks pass, with a new script. Read back from a recording: three notes played at 0, 0.40 and 1.10 s came back at 0, 0.39 and 1.09 s, and the tune came round again every 1.60 s.
- **Not checked:** the red PLAY light. The simulator has no colours; on the device it relies on PLAY's first light being the red one.
- **Not done:** the browser prototype still shows "coming soon" for REC.
- **On the FM-1:** installed. Looks great; no issues found so far, with more testing still to come.

## Build 10: croc hops

- Commit: the one that adds this entry. Download: double-click `update-croc.bat`, or the **latest** release.
- **Added, on ARP:** croc hops, the arpeggiator. Hold down a few keys and the croc plays them one at a time, on the same beat as the lily pads.
  - knob 1 **WAY**: which way the croc hops. No hops (keys sound together, as before), Up, Down, Up and down, Any way
  - knob 2 **HOP**: how long each note lasts (the same setting as on the lily pads)
  - knob 3 **KEEP**: keep hopping when you let go. A new key then starts a new handful
  - the held keys show as stepping stones, low to high like stairs, with the croc hopping along them; the key being played lights up
  - the speed is the SELECT knob's hop speed; it works on every page, and together with PLAY
  - a goodbye, or turning the way to No hops, lets go of a kept handful
- **Changed:** a change is now saved after a quarter of a minute even if something is still playing. Before, the save waited for silence, which never came while the lily pads (build 8) or a kept handful of hops was playing.
- **Checked:** simulator checks pass, with a new script for the hops. Read back from recordings: three held keys came out C, E, G going up and G, E, C going down; four keys up and down came out C, E, G, C, G, E, C; the notes were 0.28 to 0.29 s apart at the default speed (0.286 expected).
- **With safe swamp (build 9):** the key lights still show the scale while the croc hops; the key being played is fully lit.
- **Not done:** the browser prototype still shows "coming soon" for ARP.
- **On the FM-1:** not reported yet.

## Build 9: safe swamp (scales)

- Commit: the one that adds this entry. Download: double-click `update-croc.bat`, or the **latest** release.
- **Added, on SCL:**
  - knob 1 **SCALE**: every key (no scale), major, minor, harmonic minor, melodic minor, harmonic major, Dorian, Phrygian, Lydian, Mixolydian, Locrian. Major is the Ionian mode and minor the Aeolian, so all seven modes are there. The screen shows the scale's name, a word for how it feels, and a keyboard with a dot on every key in the scale
  - knob 2 **HOME**: the home note (the root), C to B
  - every key still plays its own note: the scale only shows which keys are in it
  - **the key lights show the scale**: its keys glow, its home notes are a step brighter, and a held key or the note a lily pad is playing is fully lit. The lights stay on every page, so the scale shows on the lily pads and while the croc hops. SCL is half lit on other pages while a scale is set
  - the scale and its home note are saved. They belong to the instrument, not to the sound: changing them does not switch to My Croc, and the PRESETS knob does not change them
- **Checked:** simulator checks pass: the lit keys for C major and D harmonic minor match the scales note for note, they stay lit on the lily pad and teeth pages and while the croc hops, a key outside the scale still plays its own note, all of it is kept across a power cycle, and no key stays lit once the croc is asleep.
- **Not checked:** how the three brightness levels look on the real keys. The simulator only knows lit and glowing.
- **On the FM-1:** installed; the scales work great.

## Build 8: lily pad steps

- Commit: the one that adds this entry. Download: double-click `update-croc.bat`, or the **latest** release.
- **Added:**
  - **SEQ**, lily pad steps: eight lily pads, a note on each. Press a key to plant its note on the ringed pad; the ring moves on. Knob 1 picks the pad, knob 2 how many pads the croc hops (2 to 8), knob 3 how long each note lasts (tiny hops to sliding), knob 4 turned left empties the pad. Each knob has its line and readout
  - **PLAY**: the croc hops across the pads, round and round, on any page; PLAY lights while it hops and the key of the note being played lights up. It starts with a little tune already planted
  - **SELECT knob**: hop speed (60 to 180 beats a minute), on any page
  - the pads, their count, the hop and the speed are saved
- **Under the hood:** a clock in the audio interrupt (for the arpeggiator to share); every note-on and note-off now happens in that interrupt, so keys and pads can no longer grab the same voice at once; the saved record has room for the lessons still to come, and saves from builds 5, 6 and 7 carry over.
- **Checked:** simulator checks pass (planting, emptying, pad count, PLAY across pages, speed, all kept across a power cycle). Measured: at the default speed the notes land 0.285 s apart against 0.286 s expected, within 4 ms of each other. Saves made by builds 6 and 7 load with the croc's name and sound intact.
- **On the FM-1:** installed; the sequencer works great.

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
