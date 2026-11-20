# Synth Companion (iPad)

An offline editor for the iPad: every setting of the synth on one page, following the synth as it is played and changing it from the screen. Croc-O-Synth is the first synth in it; the Dreadbox Nymphes and the Micromonsta 2 are meant to join as more pages, made the same way.

It is a Swift Playgrounds app project, so it is made and run on the iPad itself: no Mac needed, no internet once it is on the iPad.

## Put it on the iPad

1. Install **Swift Playgrounds** (free, App Store) on the iPad.
2. Get `Synth Companion.swiftpm` onto the iPad: download this repository as a ZIP from GitHub (Code → Download ZIP) in Safari on the iPad, or copy the folder over with iCloud Drive or OneDrive. In the Files app, tap the ZIP to unpack it.
3. Tap `companion/Synth Companion.swiftpm` in Files. It opens in Swift Playgrounds.
4. Tap ▶ (Run). The first build takes a moment. Use it full screen with the button at the top right.

The app lives inside Swift Playgrounds. Open Swift Playgrounds and pick it from My Playgrounds to use it again.

## Use it with the FM-1

1. Install Croc-O-Synth **build 14 or later** on the FM-1 (older builds only send notes).
2. Plug the FM-1 into the iPad with a USB-C cable.
3. The dot at the top right turns yellow when the iPad finds the FM-1, then green once the FM-1 has answered. The croc's name appears, and every knob jumps to where the FM-1 is.

Then:

- Turn a knob on either one, and the other follows.
- Drag a knob up or down, or tap its left or right half for one step.
- Play the keys on the screen. Notes played on the FM-1 light up here.
- Lily pads: tap a pad, then a key, to plant that note (the next pad is picked after it, as on the FM-1). Hold a pad to empty it. ▶ starts the croc hopping, and the croc shows which pad it is on.
- Tap the croc's name to rename it.
- The dot's menu: pick another MIDI device by hand, or ask the FM-1 for everything again.

## Files

| File | What |
| --- | --- |
| `Package.swift` | The app's settings for Swift Playgrounds |
| `SynthCompanionApp.swift` | The app: which pages it has |
| `MIDIConnection.swift` | The iPad's MIDI, both ways: finds the synth by name, connects by itself, SysEx included. Shared by every synth |
| `SynthModel.swift` | A synth's settings (`Param`: CC, range, names) and their values (`SynthState`), and the colours |
| `Controls.swift` | Knob, row of choices, menu, card |
| `CrocOSynth.swift` | Croc-O-Synth's settings and its page |

## Adding a synth

A new synth is a file like `CrocOSynth.swift`: its list of `Param`s (CC number, range, names), the word its MIDI port's name contains, and a page laid out from the shared controls. Then a tab for it in `SynthCompanionApp.swift`. Synths that answer a SysEx request (a dump) get it in `onSysex`. Synths that only listen are sent the editor's values and show what was last sent.

Licence: PolyForm Noncommercial 1.0.0, as the firmware. Copyright goose.
