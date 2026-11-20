// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
// Copyright 2026 goose
//
// The controls every synth's page is built from: a stepped knob (drag up or down, or tap its left or right half),
// a row of choices, a switch, and a card to group them.

import SwiftUI

struct Knob: View {
    let param: Param
    @Binding var value: Int
    var color: Color = Swamp.croc
    var size: CGFloat = 76

    @State private var dragStart: Int? = nil

    private var fraction: Double {
        let r = param.range
        return r.upperBound == r.lowerBound ? 0 : Double(value - r.lowerBound) / Double(r.upperBound - r.lowerBound)
    }

    var body: some View {
        VStack(spacing: 4) {
            Text(param.name)
                .font(.system(size: 13, weight: .heavy, design: .rounded))
                .foregroundColor(Swamp.cream)
                .lineLimit(1)
            ZStack {
                Circle().fill(Swamp.night)
                Circle()
                    .trim(from: 0, to: 0.75)
                    .stroke(Swamp.line, style: StrokeStyle(lineWidth: 6, lineCap: .round))
                    .rotationEffect(.degrees(135))
                Circle()
                    .trim(from: 0, to: 0.75 * fraction)
                    .stroke(color, style: StrokeStyle(lineWidth: 6, lineCap: .round))
                    .rotationEffect(.degrees(135))
                Capsule()
                    .fill(color)
                    .frame(width: 4, height: size * 0.26)
                    .offset(y: -size * 0.2)
                    .rotationEffect(.degrees(-135 + 270 * fraction))
                Text(param.text(value))
                    .font(.system(size: 11, weight: .bold, design: .rounded))
                    .foregroundColor(Swamp.cream)
                    .minimumScaleFactor(0.6)
                    .lineLimit(1)
                    .frame(width: size * 0.62)
                    .offset(y: size * 0.18)
            }
            .frame(width: size, height: size)
            .contentShape(Circle())
            .gesture(
                DragGesture(minimumDistance: 0)
                    .onChanged { g in
                        let start = dragStart ?? value
                        if dragStart == nil { dragStart = value }
                        let steps = Int((-g.translation.height + g.translation.width * 0.5) / 14)
                        value = min(max(start + steps, param.range.lowerBound), param.range.upperBound)
                    }
                    .onEnded { g in
                        if abs(g.translation.height) < 4 && abs(g.translation.width) < 4 {   // a tap: one step
                            let step = g.location.x < size / 2 ? -1 : 1
                            value = min(max((dragStart ?? value) + step, param.range.lowerBound), param.range.upperBound)
                        }
                        dragStart = nil
                    }
            )
            Text(param.term)
                .font(.system(size: 10, weight: .medium))
                .foregroundColor(Swamp.dim)
                .lineLimit(1)
        }
        .frame(minWidth: size + 10)
        .accessibilityElement()
        .accessibilityLabel("\(param.name), \(param.term)")
        .accessibilityValue(param.text(value))
        .accessibilityAdjustableAction { dir in
            switch dir {
            case .increment: value = min(value + 1, param.range.upperBound)
            case .decrement: value = max(value - 1, param.range.lowerBound)
            @unknown default: break
            }
        }
    }
}

struct Choice: View {
    let param: Param
    @Binding var value: Int
    var color: Color = Swamp.croc

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack(alignment: .firstTextBaseline, spacing: 6) {
                Text(param.name)
                    .font(.system(size: 13, weight: .heavy, design: .rounded))
                    .foregroundColor(Swamp.cream)
                Text(param.term)
                    .font(.system(size: 10, weight: .medium))
                    .foregroundColor(Swamp.dim)
            }
            HStack(spacing: 4) {
                ForEach(Array(param.range), id: \.self) { v in
                    Button {
                        value = v
                    } label: {
                        Text(param.text(v))
                            .font(.system(size: 12, weight: .bold, design: .rounded))
                            .lineLimit(1)
                            .minimumScaleFactor(0.6)
                            .padding(.vertical, 7)
                            .padding(.horizontal, 6)
                            .frame(maxWidth: .infinity)
                            .foregroundColor(v == value ? Swamp.night : Swamp.cream)
                            .background(RoundedRectangle(cornerRadius: 8).fill(v == value ? color : Swamp.night))
                    }
                    .buttonStyle(.plain)
                }
            }
        }
    }
}

/// A long list of choices (scales, songs): a menu instead of a row.
struct PickerMenu: View {
    let param: Param
    @Binding var value: Int
    var color: Color = Swamp.croc

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack(alignment: .firstTextBaseline, spacing: 6) {
                Text(param.name)
                    .font(.system(size: 13, weight: .heavy, design: .rounded))
                    .foregroundColor(Swamp.cream)
                Text(param.term)
                    .font(.system(size: 10, weight: .medium))
                    .foregroundColor(Swamp.dim)
            }
            Menu {
                ForEach(Array(param.range), id: \.self) { v in
                    Button(param.text(v)) { value = v }
                }
            } label: {
                HStack {
                    Text(param.text(value))
                        .font(.system(size: 14, weight: .bold, design: .rounded))
                        .lineLimit(1)
                    Spacer(minLength: 4)
                    Image(systemName: "chevron.up.chevron.down").font(.system(size: 11, weight: .bold))
                }
                .foregroundColor(Swamp.night)
                .padding(.vertical, 7)
                .padding(.horizontal, 10)
                .background(RoundedRectangle(cornerRadius: 8).fill(color))
            }
        }
    }
}

struct Card<Content: View>: View {
    let title: String
    let subtitle: String
    var color: Color = Swamp.croc
    @ViewBuilder var content: Content

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack(alignment: .firstTextBaseline) {
                Text(title)
                    .font(.system(size: 17, weight: .black, design: .rounded))
                    .foregroundColor(color)
                Spacer()
                Text(subtitle)
                    .font(.system(size: 11, weight: .semibold))
                    .foregroundColor(Swamp.dim)
            }
            content
        }
        .padding(12)
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
        .background(RoundedRectangle(cornerRadius: 16).fill(Swamp.card))
        .overlay(RoundedRectangle(cornerRadius: 16).stroke(Swamp.line, lineWidth: 1))
    }
}
