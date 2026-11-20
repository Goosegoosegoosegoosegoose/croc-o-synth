// swift-tools-version: 5.8

// Synth Companion: offline editors for goose's synths, starting with Croc-O-Synth.
// Open this folder in Swift Playgrounds on the iPad. SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
// Copyright 2026 goose

import PackageDescription
import AppleProductTypes

let package = Package(
    name: "Synth Companion",
    platforms: [
        .iOS("16.0")
    ],
    products: [
        .iOSApplication(
            name: "Synth Companion",
            targets: ["AppModule"],
            displayVersion: "1.0",
            bundleVersion: "1",
            appIcon: .placeholder(icon: .palette),
            accentColor: .presetColor(.green),
            supportedDeviceFamilies: [
                .pad,
                .phone
            ],
            supportedInterfaceOrientations: [
                .landscapeRight,
                .landscapeLeft,
                .portrait,
                .portraitUpsideDown(.when(deviceFamilies: [.pad]))
            ]
        )
    ],
    targets: [
        .executableTarget(
            name: "AppModule",
            path: "."
        )
    ]
)
