/* SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
 * Copyright 2026 goose */
/* The croc link: every setting as a MIDI CC, both ways, for an editor on a bigger screen (the iPad companion in
 * companion/). The whole map is in docs/MIDI.md; keep the two in step.
 *
 *   in   a CC from USB or the TRS jack sets that setting, as the knob would (a sound setting makes My Croc)
 *   out  whatever changed since the last frame goes out over USB as its CC, however it changed (a knob, a preset,
 *        the editor), so an editor always shows what the FM-1 has. A value from the editor is not sent back
 *        unless the FM-1 changed it (kept in range)
 *   SysEx F0 7D 43 52 01 F7 ("CR", hello): the FM-1 answers F0 7D 43 52 02 <protocol> <build> <name, 8 letters> F7,
 *        then sends every CC
 *   SysEx F0 7D 43 52 03 <name, up to 8 letters A-Z or space> F7: names the croc
 *
 * Values are the settings' own steps (0, 1, 2 ...), not stretched to 0..127: an editor shows them exactly.
 * Included from app.c after the settings and before input(), so it sees set, snd and their helpers. */

#define LINK_PROTOCOL 1
#define LINK_CH 0                        /* sent on channel 1; taken on any channel */

/* the CC numbers (docs/MIDI.md): 14..29 the sound, 102..118 the rest; all of them "undefined" in the MIDI spec */
enum {
    LCC_PRESET = 14, LCC_TEETH, LCC_ROWS, LCC_CROOK, LCC_SIZE, LCC_JAWS, LCC_ATK, LCC_DEC, LCC_SUS, LCC_REL,
    LCC_WAG, LCC_WAG_SIZE, LCC_WAG_TO, LCC_ECHO, LCC_ECHO_FAR, LCC_CAVE,
    LCC_TEMPO = 102, LCC_HOP, LCC_PADS, LCC_SONG, LCC_ARP, LCC_ARP_KEEP, LCC_SCALE, LCC_SCALE_HOME, LCC_PLAYING,
    LCC_PAD0, /* 111..118: each lily pad's note, 0 empty */
    LCC_STEP = 119                       /* out only: the pad the croc is on, 1..8 (0: not hopping) */
};
#define LINK_N 34                        /* 16 sound + 9 + 8 pads + the step */
static const uint8_t LINK_CC[LINK_N] = {
    LCC_PRESET, LCC_TEETH, LCC_ROWS, LCC_CROOK, LCC_SIZE, LCC_JAWS, LCC_ATK, LCC_DEC, LCC_SUS, LCC_REL,
    LCC_WAG, LCC_WAG_SIZE, LCC_WAG_TO, LCC_ECHO, LCC_ECHO_FAR, LCC_CAVE,
    LCC_TEMPO, LCC_HOP, LCC_PADS, LCC_SONG, LCC_ARP, LCC_ARP_KEEP, LCC_SCALE, LCC_SCALE_HOME, LCC_PLAYING,
    LCC_PAD0, LCC_PAD0 + 1, LCC_PAD0 + 2, LCC_PAD0 + 3, LCC_PAD0 + 4, LCC_PAD0 + 5, LCC_PAD0 + 6, LCC_PAD0 + 7, LCC_STEP};
static uint8_t link_sent[LINK_N];       /* what the editor was last told (0xFF: not yet) */
static uint32_t link_session;            /* the USB connection link_sent belongs to */

/* the CCs that came in (audio interrupt) for the main loop: one writer each side */
#define LINK_Q 64u
static volatile uint8_t linkq_cc[LINK_Q], linkq_v[LINK_Q];
static volatile uint32_t linkq_w, linkq_r;

static void link_cc_in(uint32_t cc, uint32_t v)   /* audio interrupt (midi_in, platglue.c) */
{
    uint32_t w = linkq_w;
    if (w - linkq_r >= LINK_Q)
        return;                              /* full: dropped (an editor sends far fewer) */
    linkq_cc[w % LINK_Q] = (uint8_t)cc;
    linkq_v[w % LINK_Q] = (uint8_t)v;
    linkq_w = w + 1u;
}

static uint8_t link_value(int i)         /* setting i as its CC value */
{
    uint8_t cc = LINK_CC[i];
    switch (cc) {
    case LCC_PRESET: return set.preset;
    case LCC_TEETH: return (uint8_t)snd.teeth;
    case LCC_ROWS: return (uint8_t)snd.rows;
    case LCC_CROOK: return (uint8_t)snd.crook;
    case LCC_SIZE: return (uint8_t)(snd.size + 1);      /* 0 small croc, 1 croc, 2 big croc */
    case LCC_JAWS: return (uint8_t)snd.jaws;
    case LCC_ATK: return (uint8_t)snd.atk;
    case LCC_DEC: return (uint8_t)snd.dec;
    case LCC_SUS: return (uint8_t)snd.sus;
    case LCC_REL: return (uint8_t)snd.rel;
    case LCC_WAG: return (uint8_t)snd.wag;
    case LCC_WAG_SIZE: return (uint8_t)snd.wag_size;
    case LCC_WAG_TO: return (uint8_t)snd.wag_to;
    case LCC_ECHO: return (uint8_t)snd.echo;
    case LCC_ECHO_FAR: return (uint8_t)snd.echo_far;
    case LCC_CAVE: return (uint8_t)snd.cave;
    case LCC_TEMPO: return set.seq_tempo;
    case LCC_HOP: return set.seq_gate;
    case LCC_PADS: return set.seq_pads;
    case LCC_SONG: return set.song;
    case LCC_ARP: return set.arp;
    case LCC_ARP_KEEP: return set.arp_keep;
    case LCC_SCALE: return set.scale;
    case LCC_SCALE_HOME: return set.scale_home;
    case LCC_PLAYING: return seq.playing ? 1u : 0u;
    case LCC_STEP: {
        int8_t st = seq_step;
        return seq.playing && st >= 0 ? (uint8_t)(st + 1) : 0u;
    }
    default: {
        int8_t n = set.seq_note[cc - LCC_PAD0];
        return n < 0 ? 0u : (uint8_t)n;
    }
    }
}

static void link_sound(int8_t *f, int32_t v, int32_t lo, int32_t hi)   /* a sound setting: as the knob, My Croc */
{
    v = clamp(v, lo, hi);
    if (v != *f) {
        *f = (int8_t)v;
        changed();
    }
}
static void link_seq(uint8_t *f, int32_t v, int32_t lo, int32_t hi)    /* the pads, hops and scale: kept, not My Croc */
{
    v = clamp(v, lo, hi);
    if (v != *f) {
        *f = (uint8_t)v;
        seq_changed();
    }
}

static void link_set(uint32_t cc, int32_t v)   /* main loop: one CC from the editor */
{
    int i;
    switch (cc) {
    case LCC_PRESET: {
        int32_t p = clamp(v, 0, NPRESET);
        if (p != set.preset) {
            set.preset = (uint8_t)p;
            snd = p < NPRESET ? PRESET[p] : set.mine;
            touched();
        }
        break;
    }
    case LCC_TEETH: link_sound(&snd.teeth, v, 0, 2); break;
    case LCC_ROWS: link_sound(&snd.rows, v, 1, 3); break;
    case LCC_CROOK: link_sound(&snd.crook, v, 0, 4); break;
    case LCC_SIZE: link_sound(&snd.size, v - 1, -1, 1); break;
    case LCC_JAWS: link_sound(&snd.jaws, v, 0, 12); break;
    case LCC_ATK: link_sound(&snd.atk, v, 0, 10); break;
    case LCC_DEC: link_sound(&snd.dec, v, 0, 10); break;
    case LCC_SUS: link_sound(&snd.sus, v, 0, 10); break;
    case LCC_REL: link_sound(&snd.rel, v, 0, 10); break;
    case LCC_WAG: link_sound(&snd.wag, v, 0, 8); break;
    case LCC_WAG_SIZE: link_sound(&snd.wag_size, v, 0, 4); break;
    case LCC_WAG_TO: link_sound(&snd.wag_to, v, 0, 2); break;
    case LCC_ECHO: link_sound(&snd.echo, v, 0, 6); break;
    case LCC_ECHO_FAR: link_sound(&snd.echo_far, v, 0, 4); break;
    case LCC_CAVE: link_sound(&snd.cave, v, 0, 6); break;
    case LCC_TEMPO: link_seq(&set.seq_tempo, v, 0, 8); break;
    case LCC_HOP: link_seq(&set.seq_gate, v, 0, 4); break;
    case LCC_PADS:
        link_seq(&set.seq_pads, v, 2, SEQ_N);
        if (ui.seq_cur >= set.seq_pads)
            ui.seq_cur = (uint8_t)(set.seq_pads - 1u);
        break;
    case LCC_SONG: link_seq(&set.song, v, 0, NSONG); break;
    case LCC_ARP: link_seq(&set.arp, v, 0, 4); break;
    case LCC_ARP_KEEP: link_seq(&set.arp_keep, v, 0, 1); break;
    case LCC_SCALE: link_seq(&set.scale, v, 0, NSCALE); break;
    case LCC_SCALE_HOME: link_seq(&set.scale_home, v, 0, 11); break;
    case LCC_PLAYING:
        if (ui.mode == M_PLAY)               /* only while she is playing (not asleep, not naming) */
            seq.playing = v ? 1u : 0u;
        break;
    default:
        if (cc >= LCC_PAD0 && cc < LCC_PAD0 + SEQ_N) {   /* a pad's note: one of the keys' (53..79), else empty */
            int8_t n = v >= 53 && v < 53 + NKEYS ? (int8_t)v : -1;
            if (n != set.seq_note[cc - LCC_PAD0]) {
                set.seq_note[cc - LCC_PAD0] = n;
                seq_changed();
            }
        } else {
            return;                          /* not ours: nothing changes, nothing is told */
        }
    }
    for (i = 0; i < LINK_N; i++)             /* the editor knows what it sent: only a value kept in range goes back */
        if (LINK_CC[i] == cc)
            link_sent[i] = (uint8_t)v;
    ui.last_touch = plat_ms();               /* the editor counts as playing: the croc does not fall asleep */
}

static void link_send_hello(void)        /* F0 7D 43 52 02 protocol build name[8] F7 */
{
    uint8_t f[16];
    int i, nb = 0;
    const char *b = CROC_BUILD;
    for (; *b; b++)                          /* the build's number, from "BUILD 13" */
        if (*b >= '0' && *b <= '9')
            nb = nb * 10 + (*b - '0');
    f[0] = 0xF0;
    f[1] = 0x7D;
    f[2] = 'C';
    f[3] = 'R';
    f[4] = 0x02;
    f[5] = LINK_PROTOCOL;
    f[6] = (uint8_t)(nb & 0x7F);
    for (i = 0; i < NAME_LEN; i++)
        f[7 + i] = (uint8_t)(set.name[i] & 0x7F);
    f[15] = 0xF7;
    plat_sysex_send(f, sizeof f);
}

/* a SysEx frame from USB (the platform's APP_SYSEX; p: the bytes between F0 and F7). 1 = ours */
int app_sysex(const uint8_t *p, uint32_t n)
{
    uint32_t i;
    if (n < 4 || p[0] != 0x7D || p[1] != 'C' || p[2] != 'R')
        return 0;
    if (p[3] == 0x01) {                      /* hello: who are you, and everything */
        link_send_hello();
        for (i = 0; i < LINK_N; i++)
            link_sent[i] = 0xFF;
        return 1;
    }
    if (p[3] == 0x03 && n <= 4u + NAME_LEN) {   /* a new name */
        char nm[NAME_LEN + 1];
        int k = 0, last = -1;
        for (i = 4; i < n; i++) {
            char c = (char)p[i];
            if (c >= 'a' && c <= 'z')
                c = (char)(c - 'a' + 'A');
            if (!((c >= 'A' && c <= 'Z') || c == ' '))
                return 1;                    /* not a croc's name: ignored */
            if (c != ' ' || k)               /* leading spaces dropped, as on the naming screen */
                nm[k++] = c;
        }
        for (i = 0; i < (uint32_t)k; i++)
            if (nm[i] != ' ')
                last = (int)i;
        if (last < 0)
            return 1;
        for (i = 0; i <= NAME_LEN; i++)
            set.name[i] = (int)i <= last ? nm[i] : 0;
        touched();
        link_send_hello();                   /* the editor hears the name as kept */
        return 1;
    }
    return 1;                                /* ours, but not known: dropped */
}

static void link_frame(void)             /* main loop, every frame: take what came in, tell what changed */
{
    uint32_t s = plat_usb_session(), room;
    int i;
    while (linkq_r != linkq_w) {
        uint32_t r = linkq_r;
        link_set(linkq_cc[r % LINK_Q], linkq_v[r % LINK_Q]);
        linkq_r = r + 1u;
    }
    if (s != link_session) {                 /* a new connection: it hears everything */
        link_session = s;
        for (i = 0; i < LINK_N; i++)
            link_sent[i] = 0xFF;
    }
    if (!s)
        return;
    room = plat_midi_out_room(TO_USB);
    for (i = 0; i < LINK_N && room > 8u; i++) {   /* room kept for the keys' notes */
        uint8_t v = link_value(i);
        if (v != link_sent[i]) {
            plat_midi_send(0x0Bu | (0xB0u | LINK_CH) << 8 | (uint32_t)LINK_CC[i] << 16 | (uint32_t)(v & 0x7F) << 24, TO_USB);
            link_sent[i] = v;
            room--;
        }
    }
}
