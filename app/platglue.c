/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal
 * Taken from fm1-base's template/app.c and adapted for Croc-O-Synth: the incoming-MIDI loop (without the template's
 * message display and velocity) and the note messages its key scan sends (as one helper, USB only, a fixed
 * velocity). Kept in this file so the lines that came from the template stay under its licence (see PROVENANCE.md). */

static void link_cc_in(uint32_t cc, uint32_t v);   /* link.c (Croc-O-Synth's own): a CC for the editor link */

static void midi_in(void)                    /* audio interrupt: the packets that came in */
{
    uint32_t pkt;
    int port;
    while (plat_midi_in(&pkt, &port)) {
        uint32_t st = (pkt >> 8) & 0xFFu, d1 = (pkt >> 16) & 0x7Fu, d2 = pkt >> 24;
        if ((st & 0xF0u) == 0x90u && d2)
            note_on((int32_t)d1);
        else if ((st & 0xF0u) == 0x80u || (st & 0xF0u) == 0x90u)
            note_off((int32_t)d1);
        else if ((st & 0xF0u) == 0xB0u)
            link_cc_in(d1, d2);
    }
}

static void key_midi(int32_t note, int on)   /* a key's note out over USB-MIDI */
{
    if (on)
        plat_midi_send(0x09u | 0x90u << 8 | (uint32_t)note << 16 | 100u << 24, TO_USB);
    else
        plat_midi_send(0x08u | 0x80u << 8 | (uint32_t)note << 16, TO_USB);
}
