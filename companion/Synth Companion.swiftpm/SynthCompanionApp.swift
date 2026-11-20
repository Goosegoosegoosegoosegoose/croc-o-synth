// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
// Copyright 2026 goose
//
// Synth Companion: one page per synth. Croc-O-Synth first; the Nymphes and the Micromonsta 2 join as tabs, each a
// list of Params (like CrocOSynth.swift) and a page made of the shared controls.

import SwiftUI

@main
struct SynthCompanionApp: App {
    @StateObject private var croc = Croc.makeState()

    var body: some Scene {
        WindowGroup {
            CrocOSynthPage(synth: croc)
                .preferredColorScheme(.dark)
        }
    }
}
