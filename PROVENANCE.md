# Provenance: what Croc-O-Synth took, copied and renamed

Last updated: 2026-11-15 (first firmware on fm1-base)

## Short answer

**No code from Felucca or from M-VAVE's stock firmware has been copied.** The browser prototype (`prototype/index.html`) was written new for this project.

The firmware (`app/`) is built on **fm1-base**, which is included whole as a git submodule (`base/`), and four small files in `app/` are adapted from fm1-base's template. Those are listed in the copy log at the end. The rest of the firmware (`app/app.c`, `app/synth.c`, `app/draw.c`) was written new, against the interface fm1-base defines.

From Felucca we took only **facts about the hardware** and **a few layout conventions**, by reading it. Those are listed below.

## 1. Sources we read

| Project | Author | Licence | Version read | What it is |
| --- | --- | --- | --- | --- |
| [Felucca](https://github.com/hugelton/Felucca) | Leo Kuroshita, Hügelton Instruments | GPL-3.0-only | commit `ca1db43` (2026-11-15) | Custom multi-engine firmware for the FM-1 |
| [felucca-bsp](https://github.com/hugelton/felucca-bsp) | Leo Kuroshita and contributors | MPL-2.0 | commit `5942e68` | Hardware layer: chip drivers and FM-1 board code. No sound code, no user interface |
| [Felucca-Engines](https://github.com/hugelton/Felucca-Engines) | Hügelton Instruments | MIT | commit `d026007` | Six synthesis engines and shared DSP helpers |
| [fm1-base](https://github.com/charlesvestal/fm1-base) | Charles Vestal and contributors | MPL-2.0 | commit `c8e0599` (2026-11-13) | Platform layer on felucca-bsp (screen, drawing, fonts, panel, audio, MIDI, storage), a template app, a host simulator, a browser emulator, and build and install tools |

Felucca and Felucca-Engines were cloned only to read them; nothing from them is in this repository. fm1-base (with felucca-bsp inside it) is in the repository as the submodule `base/`, at the commit above (felucca-bsp at `18eadad`).

## 2. Facts taken (hardware knowledge, not code)

| Fact | Where we learned it | How we use it |
| --- | --- | --- |
| Screen is 240 x 240, 16-bit colour (ST7789-class) | `Felucca/firmware/src/lcd.c`, header comment | Prototype screen is drawn on a 240 x 240 grid; art spec is 240 x 240 |
| 7 encoders, 41 scanned keys: 14 buttons and 27 note keys, each with an LED | `felucca-bsp/boards/fm1/fm1_input.h` (`FM1_NENC`, `FM1_NKEY`), `examples/hwtest/README.md` | Prototype panel has the same counts |
| MASTER is a pot, not an encoder | `felucca-bsp/boards/fm1/fm1_adc.h` | MASTER is the volume knob |
| Printed labels and positions of knobs and buttons; note keys run F3 to G5 | `Felucca/docs/controls.jpg` | Prototype panel layout and labels; keyboard starts on F |
| Two failed boots in a row drop the device into update mode | `felucca-bsp/examples/hwtest/README.md` | Safety note for flashing; not used in code yet |
| How the build works (JieLi toolchain, AC79 SDK, package format) | `Felucca/BUILDING.md` | Planning only |

The printed labels themselves (MASTER, SELECT, PRESETS, ALGORITHM, KNOB 1 to 4, FX, SCL, ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ, PLAY, REC, OCT-, OCT+) are M-VAVE's panel printing on the FM-1, not Felucca's.

## 3. Conventions that match Felucca (ideas, not code)

These are places where our design does the same job with the same control as Felucca does. We arrived at several of them because the printed label says so, but they are listed here so nothing is hidden.

| Control | Felucca | Croc-O-Synth | Same or different |
| --- | --- | --- | --- |
| MASTER | Volume | Volume, capped | Same |
| SELECT | Tempo | Hop speed (tempo) | Same job, renamed |
| PRESETS | The track's sound | Surprise: five sounds plus My Croc | Same job, renamed |
| ALGORITHM | Track 1 to 4 | Jaws (filter cutoff) | Different |
| KNOB 1 to 4 | The four columns of the current page | The four settings of the current lesson, named along the bottom of the screen | Same convention |
| ENV / LFO / FX | Envelope / LFO and mod matrix / effects and slicer | Snap or yawn / Tail wag / Swamp echo and cave | Same area, our own content |
| EDIT | Engine parameters | Teeth (wave shape, rows, crooked) | Similar area |
| SEQ | Sequencer: steps, pattern | Lily pad steps | Same area, our own design |
| Note keys on the SEQ page | Enter notes into steps | Plant a note on the ringed lily pad | Same idea |
| PLAY | Start and stop | Start and stop the croc hopping | Same |
| OCT- / OCT+ | Octave down and up | Big croc and small croc (octave, three steps) | Same job, renamed |
| HOME | Home screen; hold for menu | Home croc; hold for goodbye | Press same, hold different |
| GLO | Mixer, global, system; hold for mutes | Grown-ups; hold to name the croc | Similar press, hold different |
| SCL | Scale | Safe swamp: a scale and its home note; the scale's keys stay lit on every page, and every key still plays | Same area, our own design and code |
| ARP, REC | Arpeggiator, record | Croc hops, Croc remembers | Same areas |
| SEQ pressed again | (nothing) | Croc songs: eight nursery tunes | Traditional melodies, out of copyright; written out by hand as note lists in `app/synth.c`, not taken from any file or firmware |
| SAVE | Save a preset | Challenges and stars (settings save themselves, so the button was free) | Own idea; nothing taken |

## 4. Renamed synth terms (our names for standard ideas)

These are standard synthesizer concepts, not anything taken from a particular firmware. The left column is the usual term.

| Usual term | Croc-O-Synth name |
| --- | --- |
| Oscillator wave shape (sine, saw, square) | Teeth: round, pointy, block |
| Unison voices x1, x2, x3 | Rows of teeth |
| Detune between unison voices | Crooked |
| Octave | Small croc, croc, big croc |
| Low-pass filter cutoff | Jaws |
| Attack | Open |
| Decay | Relax |
| Sustain | Hold |
| Release | Close |
| LFO rate / depth / destination | Tail wag: speed / size / tickle |
| LFO to pitch (vibrato) / to volume (tremolo) / to filter cutoff | Tickle the tail / the belly / the snoot |
| Delay amount / delay time | Swamp echo / far |
| Reverb | Cave |
| Tempo | Hop speed |
| Sequencer step | Lily pad |
| Arpeggiator / direction / latch | Croc hops / way / keep |
| Note recorder and looper | Croc remembers |
| Gate length | Hop |
| Preset | Surprise |
| User patch | My Croc |

## 5. What is original to this project

- The croc character (placeholder drawing, to be replaced by the owner's own art) and all screens
- "Been awhile, croco-dial!" and "See you later, oscillator!"
- The lesson structure, the kid names in section 4, the name-your-croc screen, My Croc
- All prototype code: panel, drawing, Web Audio sound engine, sequencer clock

Outside resources the prototype loads but does not copy: the Baloo 2 and Nunito fonts from Google Fonts (SIL Open Font Licence), by link.

## 6. What the firmware takes, and what it may take later

The first two rows are done (see the copy log). The others have not been done.

| Planned | From | Licence | What that obliges |
| --- | --- | --- | --- |
| Platform layer, build tools, simulator and installer, used as a git submodule; our app starts from its `template/` | fm1-base | MPL-2.0 | Its files stay MPL-2.0; publish any changes we make to those files. Its licensing notes say an app built on it may use any licence |
| Hardware layer (clock, flash, USB, keys, LEDs, screen, audio), which fm1-base includes | felucca-bsp | MPL-2.0 | Keep those files under MPL-2.0 with their notices; publish any changes we make to those files |
| A simple subtractive voice, probably the ANALOG engine and its DSP helpers | Felucca-Engines | MIT | Keep the copyright and licence notice |
| Update loader and install tools | felucca-bsp | MPL-2.0 | As above |
| A few binary files the package needs | JieLi AC79 SDK | Apache-2.0 | Ship the licence text with the package |

Rule we are following: **do not copy code from Felucca's own tree** (its user interface, sequencer, arpeggiator and so on). That code is GPL-3.0-only, and copying any of it would make the whole Croc-O-Synth firmware GPL-3.0-only. felucca-bsp's own licensing notes say the same: to stay free to choose a licence, start a new project, import the hardware layer, and write the rest yourself. We can read Felucca to understand how something works, then write our own.

If we ever decide to copy Felucca code after all, that is allowed, but the firmware must then be released under GPL-3.0 with full source, and it gets logged here first.

## 7. Copy log

Every file copied or adapted from another project gets a row here before it is committed.

| Date | Our file | Copied or adapted from (project, path, commit) | Licence | What we changed or renamed |
| --- | --- | --- | --- | --- |
| 2026-11-15 | `base/` (submodule, unmodified) | fm1-base, whole repository, commit `c8e0599`; it includes felucca-bsp at `18eadad` | MPL-2.0 | Nothing changed |
| 2026-11-15 | `app/app.h` | fm1-base `template/app.h`, commit `c8e0599` | MPL-2.0 | `APP_NAME` TEMPLATE to CROC-O-SYNTH; splash colour; the touch and Bluetooth options and the Bluetooth store object removed; the list of simulator checks |
| 2026-11-15 | `app/platglue.c` | fm1-base `template/app.c`, commit `c8e0599`: the `midi_in` loop and the two note messages from its key scan | MPL-2.0 | The template's MIDI display and velocity removed; the two sends made into one helper, `key_midi`, USB only, fixed velocity |
| 2026-11-15 | `app/fm1.toml` | fm1-base `template/fm1.toml`, commit `c8e0599` | MPL-2.0 | Every value replaced with Croc-O-Synth's; Bluetooth off; keys kept |
| 2026-11-15 | `app/words.tsv` | fm1-base `template/words.tsv`, commit `c8e0599` | MPL-2.0 | The file format and header comment; the words are ours |
| 2026-11-15 | `.github/workflows/build.yml` | [zvenson/fm1-build](https://github.com/zvenson/fm1-build) `.github/workflows/build.yml`, tag `v1` (commit `85a003a`): the toolchain and SDK download steps and the pinned action versions | MIT | Its attestation and release steps left out; our test and build commands; submodules checked out |
| 2026-11-15 | `build.sh`, `tests/run_tests.sh`, `app/tests/*.txt` | Written new, in the layout fm1-base's template README describes; the test scripts use the simulator's commands | PolyForm Noncommercial | Not copied |

Things in our own files that follow fm1-base's template without copying its lines: the function names the platform calls (`app_init`, `app_render`, `app_frame` and the others in `app.h`), the settings record with a magic number that is checked on load, and the `app_expect` hook for simulator tests. The platform's drawing calls (`cv_begin`, `cv_rect`, `cv_line`, `cv_text`, `cv_blit`) and fonts are used as an interface. `app/draw.c` sets the platform's canvas offset (`cv_oy`) directly to draw the screen in two bands.
