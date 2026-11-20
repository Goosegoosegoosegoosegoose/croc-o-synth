// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
// Copyright 2026 goose
//
// What a synth's page needs to know about it, written once per synth (CrocOSynth.swift now; the Nymphes and the
// Micromonsta 2 later): its settings as CCs with their ranges and names, grouped the way its panel is. SynthState
// keeps the values the synth last told us and sends what the page changes.

import SwiftUI

struct Param: Identifiable {
    enum Style { case knob, choice, toggle }

    let cc: Int
    let name: String                          // the synth's own word ("JAWS")
    let term: String                          // the usual synth word ("filter cutoff")
    let range: ClosedRange<Int>
    var labels: [String]? = nil               // one per value, from range.lowerBound
    var style: Style = .knob
    var initial: Int? = nil
    var format: ((Int) -> String)? = nil

    var id: Int { cc }

    func text(_ v: Int) -> String {
        if let labels = labels, labels.indices.contains(v - range.lowerBound) { return labels[v - range.lowerBound] }
        if let format = format { return format(v) }
        return "\(v)"
    }
}

final class SynthState: ObservableObject {
    @Published var values: [Int: Int] = [:]
    @Published var notesOn: Set<Int> = []     // notes sounding: from the synth's keys, or touched here
    @Published var deviceName: String = ""    // what the synth calls itself (the croc's name)
    @Published var deviceInfo: String = ""
    @Published var heardFromSynth = false     // the synth has answered since we connected

    let midi: MIDIConnection
    let params: [Int: Param]
    var onSysex: ((SynthState, [UInt8]) -> Void)?
    var hello: ((MIDIConnection) -> Void)?

    init(match: String, params: [Param]) {
        midi = MIDIConnection(match: match)
        var p: [Int: Param] = [:]
        var v: [Int: Int] = [:]
        for param in params {
            p[param.cc] = param
            v[param.cc] = param.initial ?? param.range.lowerBound
        }
        self.params = p
        self.values = v
        midi.onMessage = { [weak self] m in self?.heard(m) }
        midi.onConnect = { [weak self] in
            guard let self = self else { return }
            self.heardFromSynth = false
            self.hello?(self.midi)
        }
    }

    /// Call once hello is set: a synth already plugged in when the app opened is asked for everything.
    func start() {
        if midi.isConnected { sayHello() } else { midi.refresh() }
    }

    func value(_ cc: Int) -> Int { values[cc] ?? params[cc]?.range.lowerBound ?? 0 }

    /// Change a setting here: kept in range, and sent if it changed.
    func set(_ cc: Int, _ v: Int) {
        let kept = params[cc].map { min(max(v, $0.range.lowerBound), $0.range.upperBound) } ?? v
        guard values[cc] != kept else { return }
        values[cc] = kept
        midi.sendCC(cc, kept)
    }

    func binding(_ cc: Int) -> Binding<Int> {
        Binding(get: { self.value(cc) }, set: { self.set(cc, $0) })
    }

    func play(_ note: Int, on: Bool) {
        if on { notesOn.insert(note) } else { notesOn.remove(note) }
        midi.sendNote(note, on: on)
    }

    func sayHello() { hello?(midi) }

    private func heard(_ m: MIDIMessage) {
        switch m {
        case let .cc(_, number, value):
            values[number] = value
            heardFromSynth = true
        case let .noteOn(_, note, _):
            notesOn.insert(note)
        case let .noteOff(_, note):
            notesOn.remove(note)
        case let .sysex(bytes):
            heardFromSynth = true
            onSysex?(self, bytes)
        }
    }
}

// MARK: shared look

enum Swamp {
    static let night = Color(red: 0.06, green: 0.13, blue: 0.10)
    static let card = Color(red: 0.10, green: 0.20, blue: 0.15)
    static let line = Color(red: 0.20, green: 0.34, blue: 0.26)
    static let croc = Color(red: 0.48, green: 0.83, blue: 0.54)
    static let sun = Color(red: 0.95, green: 0.79, blue: 0.30)
    static let blush = Color(red: 0.96, green: 0.65, blue: 0.65)
    static let water = Color(red: 0.42, green: 0.74, blue: 0.86)
    static let cream = Color(red: 0.97, green: 0.95, blue: 0.86)
    static let dim = Color(red: 0.62, green: 0.72, blue: 0.65)
}
