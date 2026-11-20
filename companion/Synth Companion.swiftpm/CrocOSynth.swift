// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
// Copyright 2026 goose
//
// Croc-O-Synth (the M-VAVE FM-1 running goose's firmware, build 14 or later): every setting on one page. The CC map
// is the firmware's docs/MIDI.md; keep the two in step.

import SwiftUI

enum Croc {
    static let noteNames = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
    static func noteName(_ n: Int) -> String { "\(noteNames[n % 12])\(n / 12 - 1)" }

    static let firstKey = 53                  // F3: the FM-1's lowest key
    static let keyCount = 27                  // to G5

    // the CCs (docs/MIDI.md)
    static let preset = 14, teeth = 15, rows = 16, crook = 17, size = 18, jaws = 19
    static let open = 20, relax = 21, hold = 22, close = 23
    static let wag = 24, wagSize = 25, tickle = 26, echo = 27, far = 28, cave = 29
    static let tempo = 102, hop = 103, pads = 104, song = 105, arp = 106, keep = 107
    static let scale = 108, home = 109, playing = 110, pad0 = 111, step = 119

    static let scaleNames = ["Every key", "Major", "Minor", "Harmonic minor", "Melodic minor", "Harmonic major",
                             "Dorian", "Phrygian", "Lydian", "Mixolydian", "Locrian"]
    // the scales' notes above the home note, as the firmware's SWAMP_SCALE
    static let scaleSteps: [[Int]] = [[0, 2, 4, 5, 7, 9, 11], [0, 2, 3, 5, 7, 8, 10], [0, 2, 3, 5, 7, 8, 11],
                                      [0, 2, 3, 5, 7, 9, 11], [0, 2, 4, 5, 7, 8, 11], [0, 2, 3, 5, 7, 9, 10],
                                      [0, 1, 3, 5, 7, 8, 10], [0, 2, 4, 6, 7, 9, 11], [0, 2, 4, 5, 7, 9, 10],
                                      [0, 1, 3, 5, 6, 8, 10]]

    static let params: [Param] = mainParams + padParams

    private static let mainParams: [Param] = [
        Param(cc: preset, name: "SURPRISE", term: "preset", range: 0...5,
              labels: ["Baby Croc", "Swamp Bass", "Chomper", "Sleepy Yawn", "Wiggle Tail", "My Croc"], style: .choice),
        Param(cc: teeth, name: "TEETH", term: "wave", range: 0...2, labels: ["Round", "Pointy", "Block"], style: .choice),
        Param(cc: rows, name: "ROWS", term: "unison voices", range: 1...3, labels: ["x1", "x2", "x3"], style: .choice, initial: 1),
        Param(cc: crook, name: "CROOKED", term: "detune", range: 0...4),
        Param(cc: size, name: "CROC SIZE", term: "octave", range: 0...2, labels: ["Small croc", "Croc", "Big croc"],
              style: .choice, initial: 1),
        Param(cc: jaws, name: "JAWS", term: "filter cutoff", range: 0...12),
        Param(cc: open, name: "OPEN", term: "attack", range: 0...10),
        Param(cc: relax, name: "RELAX", term: "decay", range: 0...10),
        Param(cc: hold, name: "HOLD", term: "sustain", range: 0...10),
        Param(cc: close, name: "CLOSE", term: "release", range: 0...10),
        Param(cc: wag, name: "SPEED", term: "LFO rate", range: 0...8, format: { $0 == 0 ? "still" : "\($0)" }),
        Param(cc: wagSize, name: "SIZE", term: "LFO depth", range: 0...4),
        Param(cc: tickle, name: "TICKLE", term: "LFO goes to", range: 0...2,
              labels: ["Tail · pitch", "Belly · volume", "Snoot · filter"], style: .choice),
        Param(cc: echo, name: "ECHO", term: "delay amount", range: 0...6),
        Param(cc: far, name: "HOW FAR", term: "delay time", range: 0...4),
        Param(cc: cave, name: "CAVE", term: "reverb", range: 0...6),
        Param(cc: tempo, name: "HOP SPEED", term: "tempo", range: 0...8, initial: 3, format: { "\(60 + 15 * $0) bpm" }),
        Param(cc: hop, name: "HOP", term: "gate length", range: 0...4,
              labels: ["Tiny", "Short", "Hops", "Long", "Sliding"], style: .choice, initial: 2),
        Param(cc: pads, name: "LILY PADS", term: "steps", range: 2...8, initial: 8),
        Param(cc: song, name: "SONG", term: "built-in tunes", range: 0...8,
              labels: ["Her own pads", "Twinkle Twinkle", "Itsy Bitsy Spider", "Mary Had a Lamb", "Row Your Boat",
                       "Hot Cross Buns", "Old MacDonald", "Are You Sleeping", "London Bridge"]),
        Param(cc: arp, name: "WAY", term: "arpeggiator", range: 0...4,
              labels: ["Off", "Up", "Down", "Up & down", "Any way"], style: .choice),
        Param(cc: keep, name: "KEEP HOPPING", term: "latch", range: 0...1, labels: ["Off", "On"], style: .toggle),
        Param(cc: scale, name: "SCALE", term: "lights the keys", range: 0...10, labels: scaleNames),
        Param(cc: home, name: "HOME NOTE", term: "root", range: 0...11, labels: noteNames),
        Param(cc: playing, name: "PLAY", term: "run", range: 0...1, style: .toggle),
        Param(cc: step, name: "STEP", term: "playhead", range: 0...8),
    ]

    private static let padParams: [Param] = (0..<8).map { i in
        Param(cc: pad0 + i, name: "PAD \(i + 1)", term: "step note", range: 0...79,
              format: { $0 == 0 ? "—" : Croc.noteName($0) })
    }

    static func makeState() -> SynthState {
        let s = SynthState(match: "CROC", params: params)
        s.hello = { midi in midi.sendSysex([0x7D, 0x43, 0x52, 0x01]) }
        s.onSysex = { state, b in
            // F0 7D 43 52 02 protocol build name[8] F7
            guard b.count >= 6, b[0] == 0x7D, b[1] == 0x43, b[2] == 0x52, b[3] == 0x02 else { return }
            let letters = b.dropFirst(6).prefix(8).prefix(while: { $0 != 0 })
            state.deviceName = String(decoding: letters, as: UTF8.self)
            state.deviceInfo = "build \(b[5])"
        }
        s.start()
        return s
    }

    static func rename(_ state: SynthState, to name: String) {
        let letters = name.uppercased().filter { ($0 >= "A" && $0 <= "Z") || $0 == " " }.prefix(8)
        guard !letters.trimmingCharacters(in: .whitespaces).isEmpty else { return }
        state.midi.sendSysex([0x7D, 0x43, 0x52, 0x03] + letters.utf8.map { $0 })
    }
}

// MARK: the page

struct CrocOSynthPage: View {
    @ObservedObject var synth: SynthState
    @ObservedObject var midi: MIDIConnection
    @State private var plantingPad: Int? = nil
    @State private var naming = false
    @State private var newName = ""

    init(synth: SynthState) {
        self.synth = synth
        self.midi = synth.midi
    }

    private func p(_ cc: Int) -> Param { synth.params[cc]! }
    private func knob(_ cc: Int, _ color: Color = Swamp.croc) -> some View {
        Knob(param: p(cc), value: synth.binding(cc), color: color)
    }

    var body: some View {
        GeometryReader { geo in
            let wide = geo.size.width > geo.size.height
            ScrollView {
                VStack(spacing: 10) {
                    header
                    if wide {
                        HStack(alignment: .top, spacing: 10) {
                            VStack(spacing: 10) { teethCard; jawsCard }
                            VStack(spacing: 10) { envelopeCard; wagCard }
                            VStack(spacing: 10) { fxCard; swampCard; hopsCard }
                        }
                    } else {
                        HStack(alignment: .top, spacing: 10) { teethCard; jawsCard }
                        HStack(alignment: .top, spacing: 10) { envelopeCard; wagCard }
                        HStack(alignment: .top, spacing: 10) { fxCard; VStack(spacing: 10) { swampCard; hopsCard } }
                    }
                    lilyPadsCard
                    Keyboard(synth: synth, onKey: plant)
                        .frame(height: wide ? 150 : 180)
                }
                .padding(12)
            }
        }
        .background(Swamp.night.ignoresSafeArea())
        .alert("Name the croc", isPresented: $naming) {
            TextField("Up to 8 letters", text: $newName)
                .textInputAutocapitalization(.characters)
                .autocorrectionDisabled()
            Button("Name it") { Croc.rename(synth, to: newName) }
            Button("Cancel", role: .cancel) {}
        } message: {
            Text("Letters A to Z and spaces, up to 8.")
        }
    }

    // MARK: header: who, connection, the presets

    private var header: some View {
        VStack(spacing: 10) {
            HStack(spacing: 12) {
                VStack(alignment: .leading, spacing: 2) {
                    Text("CROC-O-SYNTH")
                        .font(.system(size: 13, weight: .black, design: .rounded))
                        .foregroundColor(Swamp.dim)
                    Button {
                        newName = synth.deviceName
                        naming = true
                    } label: {
                        HStack(spacing: 6) {
                            Text(synth.deviceName.isEmpty ? "Croc" : synth.deviceName)
                                .font(.system(size: 30, weight: .black, design: .rounded))
                                .foregroundColor(Swamp.croc)
                            Image(systemName: "pencil").foregroundColor(Swamp.dim)
                        }
                    }
                    .buttonStyle(.plain)
                    .disabled(!midi.isConnected)
                }
                Spacer()
                Choice(param: p(Croc.size), value: synth.binding(Croc.size), color: Swamp.sun)
                    .frame(maxWidth: 300)
                connection
            }
            Choice(param: p(Croc.preset), value: synth.binding(Croc.preset), color: Swamp.blush)
        }
        .padding(12)
        .background(RoundedRectangle(cornerRadius: 16).fill(Swamp.card))
    }

    private var connection: some View {
        Menu {
            if midi.sources.isEmpty { Text("No MIDI devices") }
            ForEach(midi.sources) { s in
                Button(s.name) {
                    if let d = midi.destinations.first(where: { $0.name == s.name }) ?? midi.destinations.first {
                        midi.connect(source: s, destination: d, byHand: true)
                    }
                }
            }
            Divider()
            Button("Ask the synth for everything") { synth.sayHello() }
            Button("Look for devices again") { midi.refresh() }
        } label: {
            HStack(spacing: 6) {
                Circle()
                    .fill(midi.isConnected ? (synth.heardFromSynth ? Swamp.croc : Swamp.sun) : Swamp.blush)
                    .frame(width: 10, height: 10)
                VStack(alignment: .leading, spacing: 0) {
                    Text(midi.isConnected ? (midi.source?.name ?? "") : "Plug in the FM-1")
                        .font(.system(size: 12, weight: .bold))
                    Text(midi.isConnected ? (synth.heardFromSynth ? synth.deviceInfo : "waiting for it to answer")
                                          : "USB-C cable to the iPad")
                        .font(.system(size: 10))
                        .foregroundColor(Swamp.dim)
                }
            }
            .foregroundColor(Swamp.cream)
            .padding(.vertical, 6)
            .padding(.horizontal, 10)
            .background(RoundedRectangle(cornerRadius: 10).fill(Swamp.night))
        }
    }

    // MARK: the lessons, as cards

    private var teethCard: some View {
        Card(title: "Teeth", subtitle: "EDIT · oscillator") {
            Choice(param: p(Croc.teeth), value: synth.binding(Croc.teeth))
            Choice(param: p(Croc.rows), value: synth.binding(Croc.rows))
            HStack { knob(Croc.crook); Spacer() }
        }
    }

    private var jawsCard: some View {
        Card(title: "Jaws", subtitle: "ALGORITHM knob · filter", color: Swamp.blush) {
            HStack(alignment: .center, spacing: 14) {
                Knob(param: p(Croc.jaws), value: synth.binding(Croc.jaws), color: Swamp.blush, size: 96)
                JawsPicture(open: Double(synth.value(Croc.jaws)) / 12)
                    .frame(maxWidth: .infinity, minHeight: 80)
            }
        }
    }

    private var envelopeCard: some View {
        Card(title: "Snap or yawn", subtitle: "ENV · envelope", color: Swamp.sun) {
            EnvelopePicture(a: synth.value(Croc.open), d: synth.value(Croc.relax), s: synth.value(Croc.hold),
                            r: synth.value(Croc.close))
                .frame(height: 60)
            HStack(spacing: 4) {
                knob(Croc.open, Swamp.sun); knob(Croc.relax, Swamp.sun)
                knob(Croc.hold, Swamp.sun); knob(Croc.close, Swamp.sun)
            }
        }
    }

    private var wagCard: some View {
        Card(title: "Tail wag", subtitle: "LFO", color: Swamp.water) {
            HStack { knob(Croc.wag, Swamp.water); knob(Croc.wagSize, Swamp.water); Spacer() }
            Choice(param: p(Croc.tickle), value: synth.binding(Croc.tickle), color: Swamp.water)
        }
    }

    private var fxCard: some View {
        Card(title: "Swamp echo & cave", subtitle: "FX · delay, reverb", color: Swamp.water) {
            HStack(spacing: 4) {
                knob(Croc.echo, Swamp.water); knob(Croc.far, Swamp.water); knob(Croc.cave, Swamp.water)
            }
        }
    }

    private var swampCard: some View {
        Card(title: "Safe swamp", subtitle: "SCL · scale lights") {
            HStack(spacing: 8) {
                PickerMenu(param: p(Croc.scale), value: synth.binding(Croc.scale))
                PickerMenu(param: p(Croc.home), value: synth.binding(Croc.home))
                    .frame(maxWidth: 110)
            }
        }
    }

    private var hopsCard: some View {
        Card(title: "Croc hops", subtitle: "ARP · arpeggiator", color: Swamp.sun) {
            Choice(param: p(Croc.arp), value: synth.binding(Croc.arp), color: Swamp.sun)
            Toggle(isOn: Binding(get: { synth.value(Croc.keep) == 1 }, set: { synth.set(Croc.keep, $0 ? 1 : 0) })) {
                Text("KEEP HOPPING  ").font(.system(size: 13, weight: .heavy, design: .rounded)).foregroundColor(Swamp.cream)
                    + Text("latch").font(.system(size: 10)).foregroundColor(Swamp.dim)
            }
            .tint(Swamp.sun)
        }
    }

    private var lilyPadsCard: some View {
        let count = synth.value(Croc.pads)
        let ownPads = synth.value(Croc.song) == 0
        let playing = synth.value(Croc.playing) == 1
        let at = synth.value(Croc.step) - 1
        return Card(title: "Lily pads", subtitle: "SEQ + PLAY · step sequencer") {
            HStack(alignment: .center, spacing: 12) {
                Button {
                    synth.set(Croc.playing, playing ? 0 : 1)
                } label: {
                    Image(systemName: playing ? "stop.fill" : "play.fill")
                        .font(.system(size: 26, weight: .bold))
                        .foregroundColor(Swamp.night)
                        .frame(width: 64, height: 64)
                        .background(Circle().fill(playing ? Swamp.blush : Swamp.croc))
                }
                .buttonStyle(.plain)
                HStack(spacing: 6) {
                    ForEach(0..<8, id: \.self) { i in
                        LilyPad(index: i, note: synth.value(Croc.pad0 + i), active: i < count, crocHere: playing && i == at,
                                planting: plantingPad == i, editable: ownPads)
                            .onTapGesture {
                                guard ownPads, i < count else { return }
                                plantingPad = plantingPad == i ? nil : i
                            }
                            .contextMenu {
                                Button("Empty this pad", role: .destructive) { synth.set(Croc.pad0 + i, 0) }
                            }
                    }
                }
                .frame(maxWidth: .infinity)
                knob(Croc.pads)
                knob(Croc.tempo, Swamp.sun)
            }
            HStack(alignment: .bottom, spacing: 12) {
                Choice(param: p(Croc.hop), value: synth.binding(Croc.hop))
                PickerMenu(param: p(Croc.song), value: synth.binding(Croc.song), color: Swamp.sun)
                    .frame(maxWidth: 240)
            }
            Text(ownPads ? (plantingPad == nil ? "Tap a pad, then a key, to plant a note. Hold a pad to empty it."
                                               : "Now tap a key: it goes on pad \(plantingPad! + 1).")
                         : "Playing a croc song. Pick “Her own pads” to edit the pads.")
                .font(.system(size: 11))
                .foregroundColor(Swamp.dim)
        }
    }

    private func plant(_ note: Int) {
        guard let i = plantingPad, synth.value(Croc.song) == 0 else { return }
        synth.set(Croc.pad0 + i, note)
        let count = synth.value(Croc.pads)
        plantingPad = (i + 1) % max(count, 1)
    }
}

// MARK: pictures

struct LilyPad: View {
    let index: Int
    let note: Int
    let active: Bool
    let crocHere: Bool
    let planting: Bool
    let editable: Bool

    var body: some View {
        VStack(spacing: 2) {
            ZStack {
                Circle().fill(active ? Swamp.croc.opacity(note == 0 ? 0.35 : 0.85) : Swamp.night)
                Circle().stroke(planting ? Swamp.sun : Swamp.line, lineWidth: planting ? 4 : 1)
                if crocHere {
                    Text("🐊").font(.system(size: 26))
                } else {
                    Text(note == 0 ? "" : Croc.noteName(note))
                        .font(.system(size: 14, weight: .black, design: .rounded))
                        .foregroundColor(Swamp.night)
                }
            }
            .frame(width: 54, height: 54)
            Text("\(index + 1)")
                .font(.system(size: 10, weight: .bold))
                .foregroundColor(active ? Swamp.dim : Swamp.line)
        }
        .opacity(editable || crocHere ? 1 : 0.7)
        .accessibilityElement()
        .accessibilityLabel("Pad \(index + 1), \(note == 0 ? "empty" : Croc.noteName(note))")
    }
}

struct EnvelopePicture: View {
    let a: Int, d: Int, s: Int, r: Int

    var body: some View {
        GeometryReader { g in
            let w = g.size.width, h = g.size.height
            let seg = w / 4.4
            let xa = seg * (0.1 + Double(a) / 10)
            let xd = xa + seg * (0.1 + Double(d) / 10)
            let xs = xd + seg * 0.8
            let xr = xs + seg * (0.1 + Double(r) / 10)
            let ys = h - h * Double(s) / 10
            Path { p in
                p.move(to: CGPoint(x: 0, y: h))
                p.addLine(to: CGPoint(x: xa, y: 2))
                p.addLine(to: CGPoint(x: xd, y: ys))
                p.addLine(to: CGPoint(x: xs, y: ys))
                p.addLine(to: CGPoint(x: xr, y: h))
            }
            .stroke(Swamp.sun, style: StrokeStyle(lineWidth: 3, lineCap: .round, lineJoin: .round))
        }
        .background(RoundedRectangle(cornerRadius: 8).fill(Swamp.night))
        .accessibilityHidden(true)
    }
}

struct JawsPicture: View {
    let open: Double                          // 0 closed .. 1 wide open

    var body: some View {
        GeometryReader { g in
            let w = g.size.width, h = g.size.height, mid = h / 2
            let gap = h * 0.42 * open
            ZStack {
                Path { p in                       // upper jaw
                    p.move(to: CGPoint(x: 0, y: mid - 4))
                    p.addLine(to: CGPoint(x: w, y: mid - 4 - gap))
                    p.addLine(to: CGPoint(x: w, y: mid - 22 - gap))
                    p.addLine(to: CGPoint(x: 0, y: mid - 26))
                    p.closeSubpath()
                }.fill(Swamp.croc)
                Path { p in                       // lower jaw
                    p.move(to: CGPoint(x: 0, y: mid + 4))
                    p.addLine(to: CGPoint(x: w, y: mid + 4 + gap * 0.6))
                    p.addLine(to: CGPoint(x: w, y: mid + 18 + gap * 0.6))
                    p.addLine(to: CGPoint(x: 0, y: mid + 22))
                    p.closeSubpath()
                }.fill(Swamp.croc.opacity(0.8))
                Path { p in                       // teeth along the upper jaw
                    let n = 7
                    for i in 0..<n {
                        let x = w * (Double(i) + 0.5) / Double(n)
                        let y = mid - 4 - gap * x / w
                        p.move(to: CGPoint(x: x - 5, y: y))
                        p.addLine(to: CGPoint(x: x, y: y + 8))
                        p.addLine(to: CGPoint(x: x + 5, y: y))
                    }
                }.fill(Swamp.cream)
            }
        }
        .accessibilityHidden(true)
    }
}

// MARK: the keys, F3 to G5 as on the FM-1

struct Keyboard: View {
    @ObservedObject var synth: SynthState
    var onKey: (Int) -> Void = { _ in }

    private static let blackPC: Set<Int> = [1, 3, 6, 8, 10]
    private var whites: [Int] { (Croc.firstKey..<Croc.firstKey + Croc.keyCount).filter { !Self.blackPC.contains($0 % 12) } }

    private var inScale: Set<Int> {
        let sc = synth.value(Croc.scale)
        guard sc > 0, sc <= Croc.scaleSteps.count else { return [] }
        let home = synth.value(Croc.home)
        return Set(Croc.scaleSteps[sc - 1].map { ($0 + home) % 12 })
    }

    var body: some View {
        GeometryReader { g in
            let whiteW = g.size.width / Double(whites.count)
            let lit = inScale
            let home = synth.value(Croc.home)
            let scaleOn = synth.value(Croc.scale) > 0
            ZStack(alignment: .topLeading) {
                HStack(spacing: 0) {
                    ForEach(whites, id: \.self) { n in
                        PianoKey(note: n, black: false, down: synth.notesOn.contains(n),
                                 glow: lit.contains(n % 12), home: scaleOn && n % 12 == home, synth: synth, onKey: onKey)
                            .frame(width: whiteW, height: g.size.height)
                    }
                }
                ForEach(Array(whites.enumerated()), id: \.element) { i, n in
                    let b = n + 1
                    if Self.blackPC.contains(b % 12) && b < Croc.firstKey + Croc.keyCount {
                        PianoKey(note: b, black: true, down: synth.notesOn.contains(b),
                                 glow: lit.contains(b % 12), home: scaleOn && b % 12 == home, synth: synth, onKey: onKey)
                            .frame(width: whiteW * 0.62, height: g.size.height * 0.6)
                            .offset(x: whiteW * Double(i + 1) - whiteW * 0.31)
                    }
                }
            }
        }
    }
}

struct PianoKey: View {
    let note: Int
    let black: Bool
    let down: Bool
    let glow: Bool
    let home: Bool
    let synth: SynthState
    let onKey: (Int) -> Void
    @State private var pressed = false

    var body: some View {
        let fill: Color = down ? Swamp.croc
            : black ? (glow ? Color(red: 0.25, green: 0.45, blue: 0.32) : Color(red: 0.08, green: 0.09, blue: 0.08))
            : (glow ? Color(red: 0.80, green: 0.93, blue: 0.80) : Swamp.cream)
        RoundedRectangle(cornerRadius: black ? 4 : 6)
            .fill(fill)
            .overlay(RoundedRectangle(cornerRadius: black ? 4 : 6).stroke(Swamp.night, lineWidth: 1))
            .overlay(alignment: .bottom) {
                VStack(spacing: 2) {
                    if home { Circle().fill(Swamp.sun).frame(width: 8, height: 8) }
                    if !black && note % 12 == 0 {
                        Text(Croc.noteName(note)).font(.system(size: 10, weight: .bold)).foregroundColor(Swamp.night)
                    }
                }
                .padding(.bottom, 6)
            }
            .gesture(
                DragGesture(minimumDistance: 0)
                    .onChanged { _ in
                        if !pressed {
                            pressed = true
                            synth.play(note, on: true)
                            onKey(note)
                        }
                    }
                    .onEnded { _ in
                        pressed = false
                        synth.play(note, on: false)
                    }
            )
            .accessibilityLabel(Croc.noteName(note))
            .accessibilityAddTraits(.isButton)
    }
}
