# Provenance: what Croc-O-Synth took, copied and renamed

Last updated: 2026-11-15 (fm1-base added as the planned foundation)

## Short answer

**No firmware code has been copied so far.** The only thing in this repository is the browser prototype (`index.html`), and every line of it was written new for this project. It contains no code from Felucca, from M-VAVE's stock firmware, or from any other FM-1 firmware.

What we did take is **facts about the hardware** and **a few layout conventions**, by reading the open-source Felucca project. Those are listed below. When the real firmware starts, this file is where every copied file gets logged.

## 1. Sources we read

| Project | Author | Licence | Version read | What it is |
| --- | --- | --- | --- | --- |
| [Felucca](https://github.com/hugelton/Felucca) | Leo Kuroshita, Hügelton Instruments | GPL-3.0-only | commit `ca1db43` (2026-11-15) | Custom multi-engine firmware for the FM-1 |
| [felucca-bsp](https://github.com/hugelton/felucca-bsp) | Leo Kuroshita and contributors | MPL-2.0 | commit `5942e68` | Hardware layer: chip drivers and FM-1 board code. No sound code, no user interface |
| [Felucca-Engines](https://github.com/hugelton/Felucca-Engines) | Hügelton Instruments | MIT | commit `d026007` | Six synthesis engines and shared DSP helpers |
| [fm1-base](https://github.com/charlesvestal/fm1-base) | Charles Vestal and contributors | MPL-2.0 | commit `c8e0599` (2026-11-13) | Platform layer on felucca-bsp (screen, drawing, fonts, panel, audio, MIDI, storage), a template app, a host simulator, a browser emulator, and build and install tools |

They were cloned only to read them. Nothing from them is in this repository.

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
| OCT- / OCT+ | Octave down and up | Big croc and baby croc (octave) | Same job, renamed |
| HOME | Home screen; hold for menu | Home croc; hold for goodbye | Press same, hold different |
| GLO | Mixer, global, system; hold for mutes | Grown-ups; hold to name the croc | Similar press, hold different |
| SCL, ARP, REC, SAVE | Scale, arpeggiator, record, save | Planned: Safe swamp, Croc hops, Croc remembers, Save | Planned, same areas |

## 4. Renamed synth terms (our names for standard ideas)

These are standard synthesizer concepts, not anything taken from a particular firmware. The left column is the usual term.

| Usual term | Croc-O-Synth name |
| --- | --- |
| Oscillator wave shape (sine, saw, square) | Teeth: round, pointy, block |
| Unison voices x1, x2, x3 | Rows of teeth |
| Detune between unison voices | Crooked |
| Octave | Baby croc to giant croc |
| Low-pass filter cutoff | Jaws |
| Attack | Open |
| Decay | Relax |
| Sustain | Hold |
| Release | Close |
| LFO rate / depth / destination | Tail wag: speed / size / what |
| Delay amount / delay time | Swamp echo / far |
| Reverb | Cave |
| Tempo | Hop speed |
| Sequencer step | Lily pad |
| Gate length | Hop |
| Preset | Surprise |
| User patch | My Croc |

## 5. What is original to this project

- The croc character (placeholder drawing, to be replaced by the owner's own art) and all screens
- "Been awhile, croco-dial!" and "See you later, oscillator!"
- The lesson structure, the kid names in section 4, the name-your-croc screen, My Croc
- All prototype code: panel, drawing, Web Audio sound engine, sequencer clock

Outside resources the prototype loads but does not copy: the Baloo 2 and Nunito fonts from Google Fonts (SIL Open Font Licence), by link.

## 6. What we plan to take for the real firmware

Nothing below has been done yet.

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
| | | Nothing yet | | |
