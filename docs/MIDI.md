# Croc-O-Synth over MIDI

From build 14 the FM-1 tells an editor every setting over USB-MIDI and takes every setting back, so a bigger screen (the iPad app in `companion/`, or a controller such as the Electra One) can show and change the whole synth while she plays it. The code is `app/link.c`.

## Notes

- The keys send note on and note off on channel 1 (velocity 100), as before.
- Notes coming in (USB or the TRS jack, any channel) play the croc.

## Settings as CCs

Sent on channel 1; taken on any channel. Values are the setting's own steps (0, 1, 2 ...), not stretched to 0..127. A value out of range is kept in range, and the FM-1 then sends back where it really is.

Whenever a setting changes, however it changed (a knob, a preset, the editor), the FM-1 sends its CC. A value the editor sent is not sent back unless the FM-1 changed it. At a new USB connection, and after a hello (below), it sends every CC.

| CC | Setting | Values | Synth term |
| --- | --- | --- | --- |
| 14 | Surprise (preset) | 0 Baby Croc, 1 Swamp Bass, 2 Chomper, 3 Sleepy Yawn, 4 Wiggle Tail, 5 My Croc | Preset |
| 15 | Teeth | 0 round, 1 pointy, 2 block | Wave: sine, saw, square |
| 16 | Rows of teeth | 1..3 | Unison voices |
| 17 | Crooked | 0..4 | Unison detune |
| 18 | Croc size | 0 small croc, 1 croc, 2 big croc | Octave |
| 19 | Jaws | 0..12 | Filter cutoff |
| 20 | Open | 0..10 | Attack |
| 21 | Relax | 0..10 | Decay |
| 22 | Hold | 0..10 | Sustain |
| 23 | Close | 0..10 | Release |
| 24 | Tail wag speed | 0 still, 1..8 | LFO rate |
| 25 | Tail wag size | 0..4 | LFO depth |
| 26 | Tickle | 0 the tail, 1 the belly, 2 the snoot | LFO to pitch, volume, filter |
| 27 | Swamp echo | 0..6 | Delay feedback and level |
| 28 | How far | 0..4 | Delay time |
| 29 | Cave | 0..6 | Reverb |
| 102 | Hop speed | 0..8 (60 to 180 beats a minute) | Tempo |
| 103 | Hop | 0 tiny hops .. 4 sliding | Gate length |
| 104 | Lily pads | 2..8 | Sequence length |
| 105 | Song | 0 her own pads, 1..8 a croc song | |
| 106 | Croc hops | 0 off, 1 up, 2 down, 3 up and down, 4 any way | Arpeggiator |
| 107 | Keep hopping | 0, 1 | Arp latch |
| 108 | Safe swamp | 0 every key, 1..10 the scales | Scale |
| 109 | Home note | 0..11 (C..B) | Scale root |
| 110 | Playing | 0 stopped, 1 hopping | Sequencer run |
| 111..118 | Pad 1..8 | the note (53..79, F3..G5), 0 empty | Step notes |
| 119 | (out only) the pad the croc is on | 1..8, 0 not hopping | Playhead |

Changing a sound setting (15..29) makes the sound My Croc, as turning a knob does. Everything is saved a few seconds later, as it is from the knobs.

## SysEx

Manufacturer ID 7D (non-commercial), then `43 52` ("CR").

| Frame | Direction | What |
| --- | --- | --- |
| `F0 7D 43 52 01 F7` | to the FM-1 | Hello: the FM-1 answers with its hello, then sends every CC |
| `F0 7D 43 52 02 pp bb n1 .. n8 F7` | from the FM-1 | Protocol (1), build number, the croc's name in 8 ASCII letters (0 after the end) |
| `F0 7D 43 52 03 n1 .. F7` | to the FM-1 | A new name: up to 8 letters A-Z or spaces (lower case is taken as upper). The FM-1 answers with its hello |
