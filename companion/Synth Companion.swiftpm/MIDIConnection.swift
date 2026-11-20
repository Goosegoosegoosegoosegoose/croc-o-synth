// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
// Copyright 2026 goose
//
// The iPad's MIDI (CoreMIDI), both ways, for one synth at a time. Shared by every synth's page: it finds the synth by
// a word in its port name (the FM-1 calls itself "CROC-O-SYNTH FM-1"), connects by itself when it is plugged in, and
// hands every message it hears to the page on the main thread.

import Foundation
import CoreMIDI

enum MIDIMessage {
    case noteOn(channel: Int, note: Int, velocity: Int)
    case noteOff(channel: Int, note: Int)
    case cc(channel: Int, number: Int, value: Int)
    case sysex([UInt8])                       // the bytes between F0 and F7
}

final class MIDIConnection: ObservableObject {
    struct Port: Identifiable, Hashable {
        let id: MIDIUniqueID
        let name: String
        let ref: MIDIEndpointRef
    }

    @Published private(set) var sources: [Port] = []
    @Published private(set) var destinations: [Port] = []
    @Published private(set) var source: Port? = nil
    @Published private(set) var destination: Port? = nil

    var onMessage: ((MIDIMessage) -> Void)?   // on the main thread
    var onConnect: (() -> Void)?              // a new link to the synth: time to say hello

    let match: String                         // a port whose name has this in it is the synth
    private var client = MIDIClientRef()
    private var inPort = MIDIPortRef()
    private var outPort = MIDIPortRef()
    private var sysexBytes: [UInt8] = []      // a SysEx arriving in pieces (CoreMIDI's thread only)
    private var chosenByHand = false

    var isConnected: Bool { source != nil && destination != nil }

    init(match: String) {
        self.match = match.uppercased()
        MIDIClientCreateWithBlock("Synth Companion" as CFString, &client) { [weak self] note in
            if note.pointee.messageID == .msgSetupChanged {
                DispatchQueue.main.async { self?.refresh() }
            }
        }
        MIDIInputPortCreateWithProtocol(client, "Synth Companion In" as CFString, ._1_0, &inPort) { [weak self] list, _ in
            self?.receive(list)
        }
        MIDIOutputPortCreate(client, "Synth Companion Out" as CFString, &outPort)
        refresh()
    }

    // MARK: finding the synth

    func refresh() {
        sources = (0..<MIDIGetNumberOfSources()).map { Self.port(MIDIGetSource($0)) }
        destinations = (0..<MIDIGetNumberOfDestinations()).map { Self.port(MIDIGetDestination($0)) }
        // a port that went away (the cable out) is let go
        if let s = source, !sources.contains(where: { $0.id == s.id }) { disconnect() }
        if let d = destination, !destinations.contains(where: { $0.id == d.id }) { disconnect() }
        if !isConnected && !chosenByHand,
           let s = sources.first(where: { $0.name.uppercased().contains(match) }),
           let d = destinations.first(where: { $0.name.uppercased().contains(match) }) {
            connect(source: s, destination: d)
        }
    }

    func connect(source s: Port, destination d: Port, byHand: Bool = false) {
        if let old = source { MIDIPortDisconnectSource(inPort, old.ref) }
        chosenByHand = byHand
        MIDIPortConnectSource(inPort, s.ref, nil)
        source = s
        destination = d
        onConnect?()
    }

    func disconnect() {
        if let old = source { MIDIPortDisconnectSource(inPort, old.ref) }
        source = nil
        destination = nil
        chosenByHand = false
    }

    private static func port(_ ref: MIDIEndpointRef) -> Port {
        var name: Unmanaged<CFString>?
        var id: MIDIUniqueID = 0
        MIDIObjectGetIntegerProperty(ref, kMIDIPropertyUniqueID, &id)
        let text: String
        if MIDIObjectGetStringProperty(ref, kMIDIPropertyDisplayName, &name) == noErr, let name = name {
            text = name.takeRetainedValue() as String
        } else {
            text = "MIDI port"
        }
        return Port(id: id, name: text, ref: ref)
    }

    // MARK: sending (MIDI 1.0 as Universal MIDI Packets, group 0)

    func sendCC(_ number: Int, _ value: Int, channel: Int = 0) {
        send([0x2000_0000 | UInt32(0xB0 | (channel & 15)) << 16 | UInt32(number & 0x7F) << 8 | UInt32(value & 0x7F)])
    }

    func sendNote(_ note: Int, on: Bool, velocity: Int = 100, channel: Int = 0) {
        let status = UInt32((on ? 0x90 : 0x80) | (channel & 15))
        send([0x2000_0000 | status << 16 | UInt32(note & 0x7F) << 8 | UInt32(on ? velocity & 0x7F : 0)])
    }

    /// A SysEx frame: give the bytes between F0 and F7.
    func sendSysex(_ bytes: [UInt8]) {
        var words: [UInt32] = []
        let chunks = stride(from: 0, to: max(bytes.count, 1), by: 6).map { Array(bytes[$0..<min($0 + 6, bytes.count)]) }
        for (i, chunk) in chunks.enumerated() {
            let status: UInt32 = chunks.count == 1 ? 0 : i == 0 ? 1 : i == chunks.count - 1 ? 3 : 2
            var b = chunk.map { UInt32($0 & 0x7F) }
            while b.count < 6 { b.append(0) }
            words.append(0x3000_0000 | status << 20 | UInt32(chunk.count) << 16 | b[0] << 8 | b[1])
            words.append(b[2] << 24 | b[3] << 16 | b[4] << 8 | b[5])
        }
        send(words, messageSize: 2)
    }

    private func send(_ words: [UInt32], messageSize: Int = 1) {
        guard let d = destination, !words.isEmpty else { return }
        let size = MemoryLayout<MIDIEventList>.size
        let list = UnsafeMutablePointer<MIDIEventList>.allocate(capacity: 1)
        defer { list.deallocate() }
        var packet: UnsafeMutablePointer<MIDIEventPacket>? = MIDIEventListInit(list, ._1_0)
        words.withUnsafeBufferPointer { buf in
            guard let base = buf.baseAddress else { return }
            var i = 0
            while i < buf.count, let p = packet {
                packet = MIDIEventListAdd(list, size, p, 0, messageSize, base + i)
                i += messageSize
            }
        }
        MIDISendEventList(outPort, d.ref, list)
    }

    // MARK: receiving (CoreMIDI's thread)

    private func receive(_ list: UnsafePointer<MIDIEventList>) {
        var words: [UInt32] = []
        for packet in list.unsafeSequence() {
            let n = Int(packet.pointee.wordCount)
            withUnsafeBytes(of: packet.pointee.words) { raw in
                for i in 0..<min(n, raw.count / 4) {
                    words.append(raw.load(fromByteOffset: i * 4, as: UInt32.self))
                }
            }
        }
        var messages: [MIDIMessage] = []
        var i = 0
        while i < words.count {
            let w = words[i]
            let type = Int(w >> 28)
            let size = [1, 1, 1, 2, 2, 4, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4][type]
            if type == 0x2 {                  // a MIDI 1.0 channel message
                let status = Int(w >> 16) & 0xFF, d1 = Int(w >> 8) & 0x7F, d2 = Int(w) & 0x7F
                let ch = status & 15
                switch status & 0xF0 {
                case 0x90 where d2 > 0: messages.append(.noteOn(channel: ch, note: d1, velocity: d2))
                case 0x90, 0x80: messages.append(.noteOff(channel: ch, note: d1))
                case 0xB0: messages.append(.cc(channel: ch, number: d1, value: d2))
                default: break
                }
            } else if type == 0x3 && i + 1 < words.count {   // SysEx, six bytes at a time
                let w1 = words[i + 1]
                let status = Int(w >> 20) & 0xF, count = min(Int(w >> 16) & 0xF, 6)
                let six = [w >> 8, w, w1 >> 24, w1 >> 16, w1 >> 8, w1].map { UInt8($0 & 0x7F) }
                let bytes = Array(six[0..<count])
                switch status {
                case 0: messages.append(.sysex(bytes))
                case 1: sysexBytes = bytes
                case 2: sysexBytes += bytes
                default:
                    sysexBytes += bytes
                    messages.append(.sysex(sysexBytes))
                    sysexBytes = []
                }
            }
            i += size
        }
        guard !messages.isEmpty else { return }
        DispatchQueue.main.async { [weak self] in
            for m in messages { self?.onMessage?(m) }
        }
    }
}
