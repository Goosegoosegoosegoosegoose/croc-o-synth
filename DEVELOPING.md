# Working on Croc-O-Synth

Read this first if you are picking the project up, in a new conversation or as a new person. It says how the firmware is put together, how to add a lesson, and how to work alongside someone else without colliding.

## What this is

A firmware for the M-VAVE FM-1 pocket synth that teaches a seven-year-old synth basics. A crocodile is the instrument: its teeth are the oscillator, its jaws the filter, its tail the LFO. Every button opens one lesson. The owner (goose) designs the lessons and the words; they will also draw the final croc, so the current croc is a placeholder.

Tone of the copy: capitals, short, plain words a child can read. Each knob gets a line saying what it does and a readout saying where it is set. Synth terms stay out of the screen; they are listed for grown-ups in `README.md` and `PROVENANCE.md`.

## Where things are

| Path | What |
| --- | --- |
| `app/app.c` | Screens, controls, saved settings. One `picture()` draws whatever the current mode and page need |
| `app/synth.c` | The sound, run from the audio interrupt (`app_render`) |
| `app/draw.c` | Shapes through a small transform, text helpers, the croc |
| `app/platglue.c`, `app/app.h`, `app/fm1.toml`, `app/words.tsv` | Adapted from fm1-base's template; MPL-2.0 |
| `app/tests/*.txt` | Scripts for the simulator |
| `base/` | fm1-base (git submodule): the platform, the simulator, the build tools. Do not edit |
| `prototype/index.html` | The browser mock where lessons are designed first |
| `CHANGELOG.md` | One entry per firmware build, with what was heard on the real FM-1 |
| `PROVENANCE.md` | What was taken from where. Has a copy log |
| `docs/FLASHING.md`, `tools/update-croc.bat` | Getting a build onto an FM-1 from Windows |

## Build and test

    git clone --recursive https://github.com/Goosegoosegoosegoosegoose/croc-o-synth
    tests/run_tests.sh         # builds the simulator, runs every script in it
    ./build.sh --host          # the simulator only: app/build/host/crocosynth_host

The simulator takes a script of presses and turns and can save screenshots (`shot`), record audio (`wav`), and check state (`expect`). Its commands are listed at the top of `base/host/plat_host.c`. Look at the screenshots and measure the recordings before calling something done: two real bugs (a name clash and an echo that grew louder) were only visible that way.

The device build needs JieLi's toolchain, which most sandboxes cannot download. **GitHub builds it:** every push to `main` that touches the firmware runs the *Build firmware* workflow, which runs the tests, builds for the FM-1 and publishes the result as the repository's **latest** release. A branch is built too (as a downloadable artifact) but is not published as the latest release.

Things the simulator does not catch, learned the hard way:

- **Names.** The device build compiles more platform code than the simulator. `panel` and `FS` clashed. Give new file-scope names a distinctive word.
- **The speaker.** The FM-1's speaker plays almost nothing below about 170 Hz and rattles if driven hard there. The output is high-passed (unless headphones are in) and the keys sound an octave above their printed pitch. Keep new sounds in that range.
- **Memory.** Small RAM is about 98 KB; put anything big in the pool: `__attribute__((section(".pool")))`. The pool is not cleared at power-on (see `fx_clear`).
- **Speed.** Unknown until it runs on the device. So far there has been headroom.

## How a lesson is built

Follow an existing one; the tail wag lesson (`PG_LFO`) is the fullest example.

1. **A page**: add it to the `PG_*` enum in `app.c`.
2. **Its button**: in `input()`, the loop over buttons sends the button to `go_page(...)`. Remove its entry from `SOON_NAME`.
3. **Its knobs**: in `input()`, the loop over KNOB 1 to 4 has a block per page. Change settings with `step()`, which also marks the sound as My Croc and schedules the save.
4. **Its picture**: a block in `picture()`. Title at the top, the line and readout under it, the croc, a panel or picture at the bottom, then `strip()` with the four knob names.
5. **Its light**: the `plat_leds` call at the end of `input()`.
6. **Its sound**: fields in `sound_t` (`synth.c`), read in `app_render`. Anything that changes while a note sounds must glide or fade, never jump.
7. **Tests**: add to `app/tests/basic.txt` (screens and settings) and `sound.txt` (recordings).
8. **Prototype**: keep `prototype/index.html` in step when the design changes.
9. **Build number and changelog**: bump `CROC_BUILD` in `app.c` and add an entry at the top of `CHANGELOG.md`.

### Saved settings

`settings_t` in `app.c` is written to flash. It has spare bytes (`rsv`), and so does `sound_t`: take new fields from those so the record keeps its size, and make 0 a valid value for a new field, since older saves read as 0 there. If the record must change size, bump the magic and carry the old record over as `app_init` does for `settings_v2_t`; the croc's name must survive an update.

### The lights

Each key and button has one light with brightness only (`plat_leds`, `plat_lights`, `plat_lights_mid` in `base/include/plat.h`). PLAY is the only one with two colours (green and red). The key lights' colour cannot be changed by firmware.

## Rules

- **No code from Felucca.** It is GPL-3.0-only and would force that licence on everything. Reading it to learn how the hardware behaves is fine. fm1-base and felucca-bsp are MPL-2.0: use them as they are, and if you copy lines out of their files, put those lines in an MPL-2.0 file (as `app/platglue.c` does) and add a row to the copy log in `PROVENANCE.md`.
- **Our own code** is PolyForm Noncommercial 1.0.0; art, screens and words are CC BY-NC-SA 4.0. The copyright name is `goose`, lower case.
- **Commits** say what changed and why. Do not put anyone's email address in them.
- **Say what is not verified.** Nobody working in a sandbox can hear the device. Report what was measured and what still needs the owner's ears.

## Working in parallel

`app/app.c` is one file, so two people editing it at once will collide. To keep that small:

- Work on a **branch** named for the lesson (`scale-mode`, `arp`), not on `main`. Open a pull request when the tests pass.
- Keep a lesson's code together: its knob block, its picture block. Put new sound code in its own clearly marked section of `synth.c`, or in a new file included from `app.c`.
- Before opening the pull request, merge `main` into the branch and rerun the tests.
- The build number and the changelog entry are added when the branch is merged, so two branches do not both claim the same number.

### Who is doing what

| Work | Where | Status |
| --- | --- | --- |
| Lily pad sequencer (SEQ, PLAY), and the shared clock with the hop speed on SELECT | `main`, the original conversation | Next: build 8 |
| Safe swamp, the scale mode (SCL) | branch `scale-mode`, a second conversation | Free to start now |
| Croc hops, the arpeggiator (ARP) | branch `arp`, a second conversation | Start after build 8 is on `main`: it uses the same clock |
| Croc remembers (REC), SAVE, challenges and stars | not started | |
| The croc and background art | the owner | In progress |

### Notes for the two lessons

**Safe swamp (SCL).** Hooks are in place: `key_to_note()` in `app.c` is the one place a key becomes a note (return -1 for silence; each key remembers the note it started, so its note-off matches), and the saved record already has `scale`, `scale_home` and `scale_unsafe` (all 0 must mean every key as it is). Only the keys in a scale are "safe". Ideas agreed so far: the safe keys glow; a happy scale and a spooky one (major and minor). To decide with the owner: which scales, whether the root can move, and what an unsafe key does (stay silent, or slide to the nearest safe note). The lights can show three levels, so safe keys can glow dimly, the home note a little brighter, and held keys fully.

**Croc hops (ARP).** Hold a few keys and the croc plays them one at a time: up, down, or random. It needs a clock; build 8 adds one for the sequencer, with the hop speed on the SELECT knob. Build on that clock, do not add a second one.
