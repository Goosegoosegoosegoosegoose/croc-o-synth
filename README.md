# Crocosynth

A kid-friendly firmware idea for the M-VAVE FM-1 pocket synth: a crocodile teaches synth basics, one lesson per button.

This repository currently holds the **browser prototype** (`index.html`). It is a click-through mock of the FM-1 panel with a stand-in sound engine. It is not firmware yet.

## Try it

Open `index.html` in a browser. Tap the screen to wake the croc.

- Drag a knob, or tap its right or left side.
- Hold HOME to say bye bye. Hold GLO to name the croc.
- On a computer, the keys A to ; play notes.

## Lessons

| Control | Lesson | Synth idea |
| --- | --- | --- |
| EDIT | Teeth | Wave shape; rows of teeth are unison voices (x1 to x3); crooked is detune |
| ENV | Snap or yawn | Open, Relax, Hold, Close: attack, decay, sustain, release |
| LFO | Tail wag | LFO speed, depth, target (pitch or filter) |
| FX | Swamp echo and cave | Delay amount and time, reverb |
| SEQ + PLAY | Lily pad steps | Eight-step sequencer |
| OCT- / OCT+ | Baby or big croc | Octave |
| PRESETS knob | Surprise | Five sounds plus her own, kept as My Croc |
| ALGORITHM knob | Jaws | Filter cutoff |
| SELECT knob | Hop speed | Tempo |
| MASTER knob | Volume | Capped |

Screens: "Been awhile, croco-dial!" on start-up, "See you later, oscillator!" on goodbye, and a name-your-croc screen on first power-on.

## Not built yet

- SCL (safe swamp: scales), ARP (croc hops), REC (croc remembers), SAVE
- Challenges and stars for each lesson
- Final croc and background art (the current drawing is a placeholder)
- The real firmware

## Plan for the firmware

Build on the open hardware layer and engines from the Felucca project:

- Hardware layer: https://github.com/hugelton/felucca-bsp (MPL-2.0)
- Engines: https://github.com/hugelton/Felucca-Engines (MIT)
- Felucca itself, for reference: https://github.com/hugelton/Felucca (GPL-3.0-only)

The FM-1 has a 240 x 240 colour screen, 7 encoders plus a volume pot, 14 buttons and 27 keys (F3 to G5). Art should be drawn for 240 x 240, with the croc in separate parts (body, upper jaw, lower jaw, tail, teeth, eyes, waving arm) so it can move.

Unofficial. Not affiliated with M-VAVE.
