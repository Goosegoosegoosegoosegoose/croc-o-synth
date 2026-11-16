/* SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
 * Copyright 2026 goose */
/* Croc-O-Synth: a crocodile teaches synth basics, one lesson a button.
 *   HOME       the croc, wearing the sound          EDIT   teeth: shape, rows, crooked
 *   ENV        snap or yawn: open, relax, hold, close      LFO    tail wag: speed, size, tickle
 *   FX         swamp echo, how far, the cave        PRESETS     five sounds and My Croc
 *   SEQ        lily pad steps: eight pads, a note on each     PLAY   the croc hops across them
 *   SELECT     hop speed                            ALGORITHM   the jaws, on every page
 *   SCL        safe swamp: a scale, its home note; the keys in it stay lit on every page
 *   OCT- OCT+  big croc, croc, small croc           HOME held: goodbye      GLO held: name the croc
 * Part of the platform's unity build (base/device/unity.c, base/host/plat_host.c), after the platform: gfx.c and
 * announce.c are in scope. Written against the contract in app.h; see PROVENANCE.md for what came from where. */
#include "fastmath.h"
#include "synth.c"
#include "platglue.c"
#include "draw.c"

#define CROC_BUILD "BUILD 11"             /* on the greeting: which build is in this FM-1 (CHANGELOG.md) */

/* --------------------------------------------------------------- settings --- */
#define NAME_LEN 8
#define NPRESET 5                        /* then one more: My Croc */
#define SET_MAGIC 0x43524F34u            /* "CRO4": the saved record; another magic loads the defaults */
typedef struct {
    uint32_t magic;
    sound_t mine;                        /* My Croc: the last sound she made herself */
    uint8_t preset;                      /* 0..NPRESET-1 a ready-made sound, NPRESET My Croc */
    char name[NAME_LEN + 1];             /* the croc's name, A-Z and spaces; empty: not named yet */
    int8_t seq_note[SEQ_N];              /* the lily pads' notes (-1 empty) */
    uint8_t seq_pads, seq_gate, seq_tempo;
    uint8_t scale, scale_home, scale_unsafe;   /* safe swamp (the scale mode): 0 no scale or 1..NSCALE; the home note
                                              * 0..11 (C..B). The scale only lights its keys: every key still plays.
                                              * scale_unsafe is not used (kept 0, so the record keeps its shape) */
    uint8_t arp;                         /* croc hops (the arpeggiator): 0 off, 1 up, 2 down, 3 up and down, 4 any way */
    uint8_t arp_keep;                    /* 1: keep hopping when the keys are let go */
    uint8_t rsv[14];                     /* room for more: take from here, so the record keeps its size */
} settings_t;
/* older records, read once and carried over, so an update keeps the croc's name and her sound */
#define SET_MAGIC_V3 0x43524F33u         /* "CRO3": build 7 */
typedef struct {
    uint32_t magic;
    sound_t mine;
    uint8_t preset;
    char name[9];
    uint8_t rsv[2];
} settings_v3_t;
#define SET_MAGIC_V2 0x43524F32u         /* "CRO2": builds 5 and 6 */
typedef struct {
    uint32_t magic;
    int8_t s[12];                        /* that sound: teeth .. wag_to */
    uint8_t preset;
    char name[9];
    uint8_t rsv[2];
} settings_v2_t;
static settings_t set;
static uint32_t dirty_at;                /* something changed at this time and is not saved yet (0: saved) */

static const char *const PRESET_NAME[NPRESET] = {"BABY CROC", "SWAMP BASS", "CHOMPER", "SLEEPY YAWN", "WIGGLE TAIL"};
/*                                      teeth rows crook size jaws  atk dec sus rel  wag size to  echo far cave */
static const sound_t PRESET[NPRESET] = {{0, 1, 2, -1, 9, 0, 4, 7, 2, 0, 2, 0, 1, 2, 1, {0}},
                                        {2, 2, 1, 1, 4, 0, 5, 6, 2, 0, 2, 0, 0, 2, 0, {0}},
                                        {1, 1, 2, 0, 10, 0, 2, 2, 1, 0, 2, 0, 2, 1, 0, {0}},
                                        {0, 3, 2, 0, 5, 8, 5, 8, 8, 2, 1, 0, 3, 3, 4, {0}},
                                        {1, 1, 2, -1, 7, 1, 4, 7, 3, 6, 3, 2, 2, 2, 1, {0}}};

static int sound_ok(const sound_t *s)
{
    return s->teeth >= 0 && s->teeth <= 2 && s->rows >= 1 && s->rows <= 3 && s->crook >= 0 && s->crook <= 4 &&
           s->size >= -1 && s->size <= 1 && s->jaws >= 0 && s->jaws <= 12 && s->atk >= 0 && s->atk <= 10 &&
           s->dec >= 0 && s->dec <= 10 && s->sus >= 0 && s->sus <= 10 && s->rel >= 0 && s->rel <= 10 &&
           s->wag >= 0 && s->wag <= 8 && s->wag_size >= 0 && s->wag_size <= 4 && s->wag_to >= 0 && s->wag_to <= 2 &&
           s->echo >= 0 && s->echo <= 6 && s->echo_far >= 0 && s->echo_far <= 4 && s->cave >= 0 && s->cave <= 6;
}
static int name_ok(const char *n)
{
    int i;
    for (i = 0; i < NAME_LEN && n[i]; i++)
        if (!((n[i] >= 'A' && n[i] <= 'Z') || n[i] == ' '))
            return 0;
    return n[i] == 0;
}

/* ------------------------------------------------------- safe swamp (scales) --- */
/* Each scale: the semitones above its home note, as bits (bit 0 the home note itself). The seven modes of the major
 * scale (major is Ionian, minor is Aeolian), then the harmonic and melodic ones. */
#define SWAMP_BITS(a, b, c, d, e, f, g) (uint16_t)(1u << a | 1u << b | 1u << c | 1u << d | 1u << e | 1u << f | 1u << g)
#define NSCALE 10
static const uint16_t SWAMP_SCALE[NSCALE] = {
    SWAMP_BITS(0, 2, 4, 5, 7, 9, 11),    /* major (Ionian) */
    SWAMP_BITS(0, 2, 3, 5, 7, 8, 10),    /* minor (Aeolian) */
    SWAMP_BITS(0, 2, 3, 5, 7, 8, 11),    /* harmonic minor */
    SWAMP_BITS(0, 2, 3, 5, 7, 9, 11),    /* melodic minor */
    SWAMP_BITS(0, 2, 4, 5, 7, 8, 11),    /* harmonic major */
    SWAMP_BITS(0, 2, 3, 5, 7, 9, 10),    /* Dorian */
    SWAMP_BITS(0, 1, 3, 5, 7, 8, 10),    /* Phrygian */
    SWAMP_BITS(0, 2, 4, 6, 7, 9, 11),    /* Lydian */
    SWAMP_BITS(0, 2, 4, 5, 7, 9, 10),    /* Mixolydian */
    SWAMP_BITS(0, 1, 3, 5, 6, 8, 10)};   /* Locrian */
static const char *const SWAMP_NAME[NSCALE + 1] = {"EVERY KEY", "MAJOR", "MINOR", "HARMONIC MINOR", "MELODIC MINOR",
                                                   "HARMONIC MAJOR", "DORIAN", "PHRYGIAN", "LYDIAN", "MIXOLYDIAN", "LOCRIAN"};
static const char *const SWAMP_FEEL[NSCALE + 1] = {"TURN KNOB 1 TO PICK A SCALE", "HAPPY (IONIAN MODE)", "SAD (AEOLIAN MODE)",
                                                   "SPOOKY", "MYSTERY", "SUNNY WITH ONE CLOUD", "COOL", "DESERT NIGHT",
                                                   "DREAMY", "ROCKING", "WOBBLY"};
static const char *const NOTE_NAME[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

static const int8_t SEQ_FIRST[SEQ_N] = {60, 64, 67, 64, 60, 64, 67, 72};   /* a little tune to start from */

static void seq_from_settings(void)          /* the saved pads and speed into the clock's copy */
{
    int i;
    for (i = 0; i < SEQ_N; i++)
        seq.note[i] = set.seq_note[i];
    seq.pads = set.seq_pads;
    seq.gate = set.seq_gate;
    seq.tempo = set.seq_tempo;
    crochop.way = set.arp;
    crochop.keep = set.arp_keep;
}

static void crocmem_ask(int what)             /* croc remembers: listen, sing, be quiet (synth.c does it) */
{
    crocmem.cmd = (uint8_t)what;
    crocmem.cmd_n++;
}

void app_init(void)
{
    static union {
        settings_t now;
        settings_v3_t v3;
        settings_v2_t v2;
    } ld;
    int n = plat_store_load(OBJ_SETTINGS, &ld, sizeof ld), i, ok = 0, old = 0;
    fx_clear();
    for (i = 0; i < (int)sizeof set; i++)
        ((uint8_t *)&set)[i] = 0;
    for (i = 0; i < SEQ_N; i++)
        set.seq_note[i] = SEQ_FIRST[i];
    set.seq_pads = SEQ_N;
    set.seq_gate = 2;
    set.seq_tempo = 3;
    set.mine = PRESET[0];
    if (n == (int)sizeof(settings_t) && ld.now.magic == SET_MAGIC) {
        set = ld.now;
        ok = 1;
    } else if (n == (int)sizeof(settings_v3_t) && ld.v3.magic == SET_MAGIC_V3) {   /* build 7 */
        set.mine = ld.v3.mine;
        set.preset = ld.v3.preset;
        for (i = 0; i < NAME_LEN; i++)
            set.name[i] = ld.v3.name[i];
        ok = old = 1;
    } else if (n == (int)sizeof(settings_v2_t) && ld.v2.magic == SET_MAGIC_V2) {   /* builds 5 and 6 */
        int8_t *d = (int8_t *)&set.mine;
        for (i = 0; i < 12; i++)
            d[i] = ld.v2.s[i];
        set.mine.echo = 0;
        set.mine.echo_far = 2;
        set.mine.cave = 0;
        set.preset = ld.v2.preset;
        for (i = 0; i < NAME_LEN; i++)
            set.name[i] = ld.v2.name[i];
        ok = old = 1;
    }
    for (i = 0; i < SEQ_N && ok; i++)
        if (set.seq_note[i] < -1 || (set.seq_note[i] >= 0 && (set.seq_note[i] < 53 || set.seq_note[i] > 79)))
            ok = 0;
    if (!ok || !sound_ok(&set.mine) || set.preset > NPRESET || !name_ok(set.name) || set.seq_pads < 2 ||
        set.seq_pads > SEQ_N || set.seq_gate > 4 || set.seq_tempo > 8) {
        set.mine = PRESET[0];
        set.preset = 0;
        for (i = 0; i <= NAME_LEN; i++)
            set.name[i] = 0;
        for (i = 0; i < SEQ_N; i++)
            set.seq_note[i] = SEQ_FIRST[i];
        set.seq_pads = SEQ_N;
        set.seq_gate = 2;
        set.seq_tempo = 3;
        set.scale = set.scale_home = set.scale_unsafe = set.arp = 0;
    }
    if (set.scale > NSCALE || set.scale_home > 11)   /* only the scale is put back, not her sound */
        set.scale = set.scale_home = 0;
    set.scale_unsafe = 0;
    if (set.arp > 4 || set.arp_keep > 1)     /* croc hops alone: back to off */
        set.arp = set.arp_keep = 0;
    set.magic = SET_MAGIC;
    if (old)
        dirty_at = 1u;                       /* saved again in the new form */
    seq_from_settings();
    crocmem.rate = 1;                        /* sung back at the speed it was played, again and again */
    crocmem.again = 1;
    snd = set.preset < NPRESET ? PRESET[set.preset] : set.mine;
}

/* --------------------------------------------------------------------- UI --- */
enum { M_BOOT, M_PLAY, M_BYE, M_OFF, M_NAME };
enum { PG_HOME, PG_EDIT, PG_SOON, PG_ENV, PG_LFO, PG_GLO, PG_FX, PG_SEQ, PG_SCALE, PG_ARP, PG_REC };
enum { OV_NONE, OV_SIZE, OV_JAWS, OV_TEMPO };
enum { F_NONE = -1, F_K1, F_K2, F_K3, F_K4 };   /* which of KNOB 1 to 4 was turned last on this page */

static struct {
    uint8_t mode, page, ov, soon;        /* soon: the button whose lesson is still to come */
    int8_t focus;
    uint32_t t0;                         /* when this mode began */
    uint32_t ov_until, home_t, glo_t, last_draw, last_touch;
    uint32_t keys, btn;
    uint8_t key_note[NKEYS];             /* the note each held key started (its note-off must match) */
    uint8_t letter[NAME_LEN];            /* the name being spelled: 0 a space, 1..26 A..Z */
    uint8_t ncur;
    uint8_t seq_cur;                     /* the lily pad a key plants its note on */
    uint32_t lit_scale, lit_home;        /* the keys glowing for the scale, and its home notes (brighter) */
    uint8_t play_red;                    /* PLAY's red light, not its green one: the croc is listening */
} ui;

static const char *const SOON_NAME[NB] = {"", "", "", "", "", "", "", "SAVE", "",
                                          "", "", "", "", ""};

/* is this note in the scale that is set? (every note is, with no scale) */
static int swamp_has(int32_t note)
{
    return !set.scale || (SWAMP_SCALE[set.scale - 1] >> ((note - (int32_t)set.scale_home + 120) % 12) & 1u);
}
/* The note a key plays: key 0 is F3 (MIDI 53). -1: the key stays silent. The scale does not change this: it only
 * lights its keys, and every key plays its own note. */
static int32_t key_to_note(uint32_t k) { return 53 + (int32_t)k; }
/* The lights. Buttons: lit, the rest glowing (PLAY green), SCL half lit while a scale is set. Keys: lit, the scale's
 * home notes half lit, its other notes glowing. The scale's lights stay on whatever page is up. */
static void swamp_lights(uint32_t buttons, uint32_t keys)
{
    uint32_t play = 1u << B_PLAY, all = (1u << NB) - 1u, b = buttons & all, dim = all & ~b, k, sc = 0, home = 0;
    for (k = 0; k < NKEYS && set.scale; k++) {
        if ((53 + k - set.scale_home) % 12u == 0)
            home |= 1u << k;
        else if (swamp_has(53 + (int32_t)k))
            sc |= 1u << k;
    }
    ui.lit_scale = sc;
    ui.lit_home = home;
    b = (b & ~play) | (b & play ? (ui.play_red ? play : LED_PLAY_GREEN) : 0u);
    dim = (dim & ~play) | (dim & play ? LED_PLAY_GREEN : 0u);
    plat_lights(b, keys, dim, sc, 2);
    plat_lights_mid(set.scale ? 1u << B_SEL : 0u, home);
}

void app_say(const char *a, const char *b)
{
    if (ann_on())
        ann_say(a ? a : "", b && b[0] ? b : 0, ANN_NOW);
}

static void text_join(char *d, const char *a, const char *b, const char *c)
{
    while (*a)
        *d++ = *a++;
    while (*b)
        *d++ = *b++;
    while (*c)
        *d++ = *c++;
    *d = 0;
}

static void touched(void) { dirty_at = plat_ms() | 1u; }
/* she changed the sound herself: it becomes My Croc */
static void changed(void)
{
    set.mine = snd;
    set.preset = NPRESET;
    touched();
}
static void set_mode(int m)
{
    ui.mode = (uint8_t)m;
    ui.t0 = plat_ms();
    ui.ov = OV_NONE;
    ui.home_t = ui.glo_t = 0;
    if (m != M_PLAY) {
        seq.playing = 0;                     /* the croc stops hopping, and lets go of the keys it kept */
        crochop.flush++;
        crocmem_ask(CM_DO_QUIET);
        notes_off();
    }
}
static void step(int8_t *v, int32_t d, int32_t lo, int32_t hi)
{
    int32_t n = clamp(*v + d, lo, hi);
    if (n != *v) {
        *v = (int8_t)n;
        changed();
    }
}
static void overlay(int o)
{
    ui.ov = (uint8_t)o;
    ui.ov_until = plat_ms() + 1800u;
}
static void go_page(int p)
{
    ui.page = (uint8_t)p;
    ui.focus = F_NONE;
}

static void seq_changed(void)                /* a pad, the pad count, the hop or the speed changed: to the clock, and kept */
{
    seq_from_settings();
    touched();
}

static void naming_start(void)
{
    int i, end = 0;
    for (i = 0; i < NAME_LEN; i++) {
        char c = end ? 0 : set.name[i];
        if (!c)
            end = 1;
        ui.letter[i] = c >= 'A' && c <= 'Z' ? (uint8_t)(c - 'A' + 1) : 0;
    }
    ui.ncur = 0;
    set_mode(M_NAME);
}
static void naming_done(void)
{
    int i, n = 0, last = -1;
    for (i = 0; i < NAME_LEN; i++)
        if (ui.letter[i])
            last = i;
    for (i = 0; i <= last; i++)              /* leading spaces dropped, the rest as spelled */
        if (n || ui.letter[i])
            set.name[n++] = ui.letter[i] ? (char)('A' + ui.letter[i] - 1) : ' ';
    if (!n) {                                /* nothing spelled: plain CROC */
        set.name[0] = 'C';
        set.name[1] = 'R';
        set.name[2] = 'O';
        set.name[3] = 'C';
        n = 4;
    }
    for (; n <= NAME_LEN; n++)
        set.name[n] = 0;
    touched();
    go_page(PG_HOME);
    set_mode(M_PLAY);
}

static void input(void)
{
    uint32_t keys = plat_keys(), btn = plat_buttons(), k, now = plat_ms();
    uint32_t key_dn = keys & ~ui.keys, btn_dn = btn & ~ui.btn;
    int32_t e[NE];
    int any = keys != ui.keys || btn != ui.btn;
    for (k = 0; k < NE; k++) {
        e[k] = plat_enc((int)k);
        any |= e[k] != 0;
    }
    if (any)
        ui.last_touch = now;
    if (ui.mode == M_NAME) {                 /* KNOB 1 the letter, KNOB 2 or a key the next spot, HOME done */
        if (e[EN_K1])
            ui.letter[ui.ncur] = (uint8_t)(((int32_t)ui.letter[ui.ncur] + e[EN_K1] % 27 + 27) % 27);
        if (e[EN_K2])
            ui.ncur = (uint8_t)clamp((int32_t)ui.ncur + e[EN_K2], 0, NAME_LEN - 1);
        if (key_dn && ui.ncur < NAME_LEN - 1)
            ui.ncur++;
        ui.keys = keys;
        ui.btn = btn;
        plat_glow(1);
        plat_leds(1u << B_HOME, 0);
        plat_lights_mid(0, 0);               /* the scale's lights are for playing only */
        if (btn_dn >> B_HOME & 1u)
            naming_done();
        return;
    }
    if (ui.mode != M_PLAY) {
        /* the greeting ends at any touch; the goodbye and the sleeping croc wake at a new press */
        if (ui.mode == M_BOOT) {
            if (any && now - ui.t0 > 400u) {
                if (set.name[0])
                    set_mode(M_PLAY);
                else
                    naming_start();
            }
        } else if (key_dn || btn_dn) {
            set_mode(M_BOOT);
        }
        ui.keys = keys;
        ui.btn = btn;
        plat_leds(0, 0);
        plat_lights_mid(0, 0);
        return;
    }
    for (k = 0; k < NKEYS; k++) {
        uint32_t bit = 1u << k;
        if (key_dn & bit) {
            int32_t note = key_to_note(k);
            ui.key_note[k] = note < 0 ? 0 : (uint8_t)note;
            if (note >= 0 && ui.page == PG_SEQ && !ui.ov) {   /* on the lily pads a key also plants its note */
                set.seq_note[ui.seq_cur] = (int8_t)note;
                ui.seq_cur = (uint8_t)((ui.seq_cur + 1u) % set.seq_pads);
                seq_changed();
            }
            if (note >= 0) {
                key_event(note, 1);
                key_midi(note, 1);
            }
        } else if (!(keys & bit) && (ui.keys & bit) && ui.key_note[k]) {
            key_event(ui.key_note[k], 0);
            key_midi(ui.key_note[k], 0);
            ui.key_note[k] = 0;
        }
    }
    for (k = 0; k < NB; k++) {
        if (!(btn_dn >> k & 1u))
            continue;
        ui.ov = OV_NONE;
        if (k == B_OCTDN || k == B_OCTUP) {  /* OCT- : a bigger croc, lower notes */
            step(&snd.size, k == B_OCTDN ? 1 : -1, -1, 1);
            overlay(OV_SIZE);
        } else if (k == B_HOME) {
            go_page(PG_HOME);
            ui.home_t = now | 1u;
        } else if (k == B_EDIT) {
            go_page(PG_EDIT);
        } else if (k == B_ENV) {
            go_page(PG_ENV);
        } else if (k == B_LFO) {
            go_page(PG_LFO);
        } else if (k == B_FX) {
            go_page(PG_FX);
        } else if (k == B_SEQ) {
            go_page(PG_SEQ);
        } else if (k == B_SEL) {
            go_page(PG_SCALE);
        } else if (k == B_ARP) {
            go_page(PG_ARP);
        } else if (k == B_REC) {             /* croc remembers: listen; then sing it back; then quiet */
            uint8_t st = crocmem_state;
            go_page(PG_REC);
            crocmem_ask(st == CM_QUIET ? CM_DO_LISTEN : st == CM_LISTEN ? CM_DO_SING_NOW : CM_DO_QUIET);
        } else if (k == B_PLAY) {            /* the croc hops, or stops, whatever page is up; on the remembering
                                              * page it sings the tune again, or stops singing */
            uint8_t st = crocmem_state;
            if (seq.playing)
                seq.playing = 0;
            else if (ui.page == PG_REC && st == CM_SING)
                crocmem_ask(CM_DO_QUIET);
            else if (ui.page == PG_REC && st == CM_QUIET && crocmem_n)
                crocmem_ask(CM_DO_SING);
            else if (ui.page == PG_REC && st != CM_QUIET)
                ;                            /* listening: PLAY waits */
            else
                seq.playing = 1;
        } else if (k == B_GLO) {
            go_page(PG_GLO);
            ui.glo_t = now | 1u;
        } else {
            go_page(PG_SOON);
            ui.soon = (uint8_t)k;
        }
    }
    if (ui.home_t) {                         /* HOME held: goodbye */
        if (!(btn >> B_HOME & 1u)) {
            ui.home_t = 0;
        } else if (now - ui.home_t >= 900u) {
            set_mode(M_BYE);
        }
    }
    if (ui.glo_t) {                          /* GLO held: name the croc */
        if (!(btn >> B_GLO & 1u)) {
            ui.glo_t = 0;
        } else if (now - ui.glo_t >= 900u) {
            naming_start();
        }
    }
    if (ui.mode == M_PLAY) {
        if (e[EN_ALGO]) {                    /* the jaws, on every page */
            step(&snd.jaws, e[EN_ALGO], 0, 12);
            overlay(OV_JAWS);
        }
        if (e[EN_SELECT]) {                  /* hop speed, on every page */
            int32_t tp = clamp((int32_t)set.seq_tempo + e[EN_SELECT], 0, 8);
            if (tp != set.seq_tempo) {
                set.seq_tempo = (uint8_t)tp;
                seq_changed();
            }
            overlay(OV_TEMPO);
        }
        if (e[EN_PRESET]) {                  /* the ready-made sounds, then My Croc */
            int32_t p = clamp((int32_t)set.preset + e[EN_PRESET], 0, NPRESET);
            if (p != set.preset) {
                set.preset = (uint8_t)p;
                snd = p < NPRESET ? PRESET[p] : set.mine;
                touched();
            }
            go_page(PG_HOME);
            ui.ov = OV_NONE;
        }
        for (k = 0; k < 4; k++) {            /* KNOB 1 to 4: this page's four settings */
            int32_t d = e[EN_K1 + k];
            int used = 1;
            if (!d)
                continue;
            if (ui.page == PG_HOME) {
                if (k == 0) {
                    step(&snd.teeth, d, 0, 2);
                } else if (k == 1) {         /* snap or yawn: open and close together */
                    step(&snd.atk, d, 0, 10);
                    step(&snd.rel, d, 0, 10);
                } else if (k == 2) {
                    step(&snd.wag, d, 0, 8);
                } else {
                    step(&snd.echo, d, 0, 6);
                }
            } else if (ui.page == PG_REC) {  /* how it is sung back: not saved */
                if (k == 0)
                    crocmem.rate = (uint8_t)clamp((int32_t)crocmem.rate + d, 0, 2);
                else if (k == 1)
                    crocmem.again = (uint8_t)clamp((int32_t)crocmem.again + d, 0, 1);
                else
                    used = 0;
            } else if (ui.page == PG_ARP) {  /* not part of the sound: saved, but it does not make My Croc */
                uint8_t *v = k == 0 ? &set.arp : k == 1 ? &set.seq_gate : &set.arp_keep;
                int32_t nv = clamp((int32_t)*v + d, 0, k == 0 ? 4 : k == 1 ? 4 : 1);
                if (k == 3) {
                    used = 0;
                } else if (nv != *v) {
                    *v = (uint8_t)nv;
                    seq_changed();
                }
            } else if (ui.page == PG_SEQ) {
                if (k == 0) {                /* which pad a key plants on */
                    ui.seq_cur = (uint8_t)clamp((int32_t)ui.seq_cur + d, 0, set.seq_pads - 1);
                } else if (k == 1) {         /* how many pads the croc hops */
                    set.seq_pads = (uint8_t)clamp((int32_t)set.seq_pads + d, 2, SEQ_N);
                    if (ui.seq_cur >= set.seq_pads)
                        ui.seq_cur = (uint8_t)(set.seq_pads - 1u);
                    seq_changed();
                } else if (k == 2) {         /* how long each note lasts */
                    set.seq_gate = (uint8_t)clamp((int32_t)set.seq_gate + d, 0, 4);
                    seq_changed();
                } else if (d < 0) {          /* turned left: this pad is emptied */
                    set.seq_note[ui.seq_cur] = -1;
                    seq_changed();
                }
            } else if (ui.page == PG_SCALE) {    /* the scale is the instrument's, not the sound's: no My Croc */
                uint8_t *v = k == 0 ? &set.scale : &set.scale_home;
                int32_t n = clamp((int32_t)*v + d, 0, k == 0 ? NSCALE : 11);
                if (k >= 2) {
                    used = 0;
                } else if (n != *v) {
                    *v = (uint8_t)n;
                    touched();
                }
            } else if (ui.page == PG_FX) {
                if (k == 0)
                    step(&snd.echo, d, 0, 6);
                else if (k == 1)
                    step(&snd.echo_far, d, 0, 4);
                else if (k == 2)
                    step(&snd.cave, d, 0, 6);
                else
                    used = 0;
            } else if (ui.page == PG_EDIT) {
                if (k == 0)
                    step(&snd.teeth, d, 0, 2);
                else if (k == 1)
                    step(&snd.rows, d, 1, 3);
                else if (k == 2)
                    step(&snd.crook, d, 0, 4);
                else
                    used = 0;
            } else if (ui.page == PG_ENV) {
                step(k == 0 ? &snd.atk : k == 1 ? &snd.dec : k == 2 ? &snd.sus : &snd.rel, d, 0, 10);
            } else if (ui.page == PG_LFO) {
                if (k == 0)
                    step(&snd.wag, d, 0, 8);
                else if (k == 1)
                    step(&snd.wag_size, d, 0, 4);
                else if (k == 2)
                    step(&snd.wag_to, d, 0, 2);
                else
                    used = 0;
            } else {
                used = 0;
            }
            if (used) {
                ui.focus = (int8_t)k;
                ui.ov = OV_NONE;
            }
        }
    }
    ui.keys = keys;
    ui.btn = btn;
    plat_glow(1);
    {                                        /* this page's button; PLAY while the croc hops; the key the pad is playing */
        int8_t sn = seq_sounding, hn = crochop_sounding;
        uint32_t page_btn = ui.page == PG_HOME ? B_HOME : ui.page == PG_EDIT ? B_EDIT : ui.page == PG_ENV ? B_ENV
                            : ui.page == PG_LFO ? B_LFO : ui.page == PG_GLO ? B_GLO : ui.page == PG_FX ? B_FX
                            : ui.page == PG_SEQ ? B_SEQ : ui.page == PG_SCALE ? B_SEL : ui.page == PG_ARP ? B_ARP
                            : ui.page == PG_REC ? B_REC : ui.soon;
        uint8_t ms = crocmem_state;
        ui.play_red = ms == CM_WAIT || ms == CM_LISTEN;      /* PLAY goes red while the croc listens */
        swamp_lights(1u << page_btn | (seq.playing || ms != CM_QUIET ? 1u << B_PLAY : 0u),
                     keys | (sn >= 53 && sn < 53 + NKEYS ? 1u << (sn - 53) : 0u) | (hn >= 53 && hn < 53 + NKEYS ? 1u << (hn - 53) : 0u) |
                         (crocmem_keys & ((1u << NKEYS) - 1u)));
    }
}

void app_input_only(void) { input(); }

/* the jaw through one note: open, relax, hold, close, a pause (0..1) */
static float env_jaw(float t)
{
    float a = (float)snd.atk / 10.0f, at = fm_maxf(0.06f, 0.005f + a * a * 0.8f), d = 0.05f + (float)snd.dec / 10.0f * 0.8f;
    float r = 0.05f + (float)snd.rel / 10.0f * 1.5f, s = fm_maxf(0.15f, (float)snd.sus / 10.0f), hold = 0.4f;
    float per = at + d + hold + r + 0.45f, ph = t - fm_floorf(t / per) * per;
    if (ph < at)
        return ph / at;
    ph -= at;
    if (ph < d)
        return 1.0f - (1.0f - s) * (ph / d);
    ph -= d;
    if (ph < hold)
        return s;
    ph -= hold;
    if (ph < r)
        return s * (1.0f - ph / r);
    return 0.0f;
}
/* the tail wag as the sound has it: -1..1, still when the wag is off */
static float wag_now(float t) { return snd.wag ? fm_sinf(t * 6.2831853f * (1.0f + (float)snd.wag * 0.9f)) : 0.0f; }

static void croc_look(croc_t *c)             /* the croc as the sound has it */
{
    c->jaw = 0.0f;
    c->tail = 0.0f;
    c->teeth = snd.teeth;
    c->rows = snd.rows;
    c->crook = snd.crook;
    c->sleep = 0;
    c->wave = 0;
    c->wave_ph = 0.0f;
    c->tail_len = 1.0f;
    c->puff = 0.0f;
}

static void num_text(char *b, int32_t v, const char *before, const char *after)   /* small whole numbers into a line */
{
    while (*before)
        *b++ = *before++;
    if (v >= 10)
        *b++ = (char)('0' + v / 10);
    *b++ = (char)('0' + v % 10);
    while (*after)
        *b++ = *after++;
    *b = 0;
}

static void picture_seq(float t)
{
    static const char *const KN[4] = {"PAD", "HOW MANY", "HOP", "EMPTY"};
    static const char *const HOPS[5] = {"TINY HOPS", "SHORT HOPS", "HOPS", "LONG HOPS", "SLIDING"};
    char buf[24];
    const char *what, *now;
    croc_t c;
    int32_t i, st = seq_step, hx = 0, hy = 0;
    (void)t;
    scene(60);
    t_center(120, 2, "LILY PAD STEPS", K_INK, 228, 1);
    if (ui.focus == F_K1) {
        what = "WHICH PAD YOU PLANT ON";
        num_text(buf, ui.seq_cur + 1, "PAD ", "");
        now = buf;
    } else if (ui.focus == F_K2) {
        what = "HOW MANY PADS THE CROC HOPS";
        num_text(buf, set.seq_pads, "", " PADS");
        now = buf;
    } else if (ui.focus == F_K3) {
        what = "HOW LONG EACH NOTE LASTS";
        now = HOPS[set.seq_gate];
    } else if (ui.focus == F_K4) {
        what = "TURN LEFT TO EMPTY THIS PAD";
        now = set.seq_note[ui.seq_cur] < 0 ? "EMPTY" : "HAS A NOTE";
    } else {
        what = "PRESS A KEY TO PLANT A NOTE";
        now = seq.playing ? "HOP HOP HOP!" : "PRESS PLAY TO HOP";
    }
    t_small(120, 27, what, K_INK);
    t_center(120, 41, now, K_CROCD, 220, 0);
    m_reset();
    for (i = 0; i < SEQ_N; i++) {            /* two rows of four pads; the croc goes along the top, then the bottom */
        float px = 36.0f + (float)(i % 4) * 56.0f, py = i < 4 ? 106.0f : 164.0f;
        int on = i < set.seq_pads, hot = on && i == st;
        if (on && i == ui.seq_cur) {         /* the pad a key plants on: a ring */
            s_ellipse(px, py, 28.0f, 18.0f, K_MOUTH);
            s_ellipse(px, py, 25.0f, 15.0f, K_WATER);
        }
        s_ellipse(px, py, 23.0f, 13.0f, !on ? RGB(98, 186, 196) : hot ? RGB(155, 232, 115) : RGB(79, 174, 69));
        s_tri(px + 4.0f, py, px + 25.0f, py - 6.5f, px + 25.0f, py + 6.5f, K_WATER);
        if (on && set.seq_note[i] >= 0) {
            const char *nm = NOTE_NAME[set.seq_note[i] % 12];
            cv_text((int32_t)px - 6 - text_w(&FONT_B, nm) / 2, (int32_t)py - 8, &FONT_B, nm, RGB(13, 51, 25));
        }
        if (hot) {
            hx = (int32_t)px;
            hy = (int32_t)py;
        }
    }
    croc_look(&c);
    if (st >= 0 && hx) {                     /* hopping: up and down once a step, mouth open on a note */
        uint32_t len = seq_len, pos = seq_pos;
        float f = len ? fm_clampf((float)pos / (float)len, 0.0f, 1.0f) : 0.0f;
        c.jaw = set.seq_note[st] >= 0 ? 0.7f : 0.1f;
        croc_draw((float)hx + 8.0f, (float)hy - 15.0f - fm_sinf(f * 3.14159f) * 9.0f, 0.3f, &c);
    } else {                                 /* waiting on the first pad */
        c.sleep = 1;
        croc_draw(44.0f, 91.0f, 0.3f, &c);
    }
    strip(KN, ui.focus);
}

/* safe swamp: the scale's name, the croc, and the keyboard with a dot on every key in the scale, as the keys' lights
 * show it (the home notes yellow) */
static void picture_scale(float t)
{
    static const char *const KN[4] = {"SCALE", "HOME", 0, 0};
    static const uint8_t BLACK[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
    char buf[24];
    const char *what, *now;
    croc_t c;
    int32_t k, w = 0, pass;
    scene(150);
    t_center(120, 4, SWAMP_NAME[set.scale], K_INK, 228, 2);
    if (ui.focus == F_K2) {
        what = "THE HOME NOTE: WHERE THE SCALE STARTS";
        text_join(buf, "HOME IS ", NOTE_NAME[set.scale_home], "");
        now = buf;
    } else {
        what = set.scale ? "THE LIT KEYS ARE IN THIS SCALE" : "A SCALE LIGHTS UP ITS KEYS";
        now = SWAMP_FEEL[set.scale];
    }
    t_small(120, 38, what, K_INK);
    t_center(120, 54, now, K_CROCD, 224, 0);
    croc_look(&c);
    c.jaw = ui.keys ? 0.6f : 0.15f;
    c.tail = fm_sinf(t * 3.0f) * 0.2f;
    croc_mid(144.0f, 0.78f, &c);
    label_box();
    for (pass = 0; pass < 2; pass++) {       /* the white keys, then the black ones over them: F3 .. G5 */
        w = 0;
        for (k = 0; k < NKEYS; k++) {
            int32_t n = 53 + k, bl = BLACK[n % 12], x = 24 + w * 12, held = (int32_t)(ui.keys >> k & 1u);
            int32_t home = set.scale && (n - (int32_t)set.scale_home) % 12 == 0, in = set.scale && swamp_has(n);
            if (!bl)
                w++;
            if (bl != pass)
                continue;
            if (!bl) {
                cv_rect(x, 175, 11, 34, held ? K_MOUTH : K_TOOTH);
                if (in)
                    s_ellipse((float)x + 5.5f, 202.0f, 3.6f, 3.6f, home ? K_SUN : K_CROCD);
                if (home)
                    s_ellipse((float)x + 5.5f, 202.0f, 1.6f, 1.6f, K_INK);
            } else {
                cv_rect(x - 4, 175, 8, 20, held ? K_MOUTH : K_OFF);
                if (in)
                    s_ellipse((float)x, 189.0f, 2.8f, 2.8f, home ? K_SUN : K_GLOW);
            }
        }
    }
    strip(KN, ui.focus);
}

/* croc remembers: the croc listening or singing, and the tune as dots (left to right in time, higher notes higher) */
static void picture_mem(float t)
{
    static const char *const KN[4] = {"SPEED", "AGAIN", 0, 0};
    static const char *const RATE[3] = {"SLOW", "THE SAME", "FAST"};
    char buf[24];
    const char *what, *now;
    croc_t c;
    uint32_t st = crocmem_state, n = crocmem_n, len = crocmem_len, pos = crocmem_pos, i, notes = 0;
    if (n > CROCMEM_MAX)
        n = CROCMEM_MAX;
    for (i = 0; i < n; i++)
        notes += cm_ev[i] >> 7;
    scene(150);
    t_center(120, 4, "CROC REMEMBERS", K_INK, 228, 1);
    if (ui.focus == F_K1) {
        what = "HOW FAST THE CROC SINGS IT";
        now = RATE[crocmem.rate];
    } else if (ui.focus == F_K2) {
        what = "SING IT AGAIN AND AGAIN";
        now = crocmem.again ? "YES" : "JUST ONCE";
    } else if (st == CM_WAIT) {
        what = "PLAY A TUNE ON THE KEYS";
        now = "I AM LISTENING";
    } else if (st == CM_LISTEN) {
        what = "PRESS REC WHEN YOU ARE DONE";
        num_text(buf, (int32_t)notes, "", notes == 1 ? " NOTE" : " NOTES");
        now = buf;
    } else if (st == CM_SING) {
        what = "PRESS REC OR PLAY TO STOP";
        now = "LA LA LA!";
    } else if (n) {
        what = "PLAY: HEAR IT AGAIN";
        now = "REC: A NEW TUNE";
    } else {
        what = "PRESS REC, THEN PLAY A TUNE";
        now = "NOTHING YET";
    }
    t_small(120, 30, what, K_INK);
    t_center(120, 46, now, st == CM_WAIT || st == CM_LISTEN ? RGB(214, 60, 70) : K_CROCD, 220, 0);
    croc_look(&c);
    if (st == CM_SING)                       /* singing: the mouth opens for each note */
        c.jaw = crocmem_keys ? 0.55f + 0.25f * fm_sinf(t * 18.0f) : 0.08f;
    else if (st == CM_WAIT || st == CM_LISTEN)
        c.jaw = 0.0f;                        /* listening: mouth shut, tail still */
    else
        c.jaw = 0.15f;
    c.tail = st == CM_SING ? fm_sinf(t * 6.0f) * 0.3f : 0.0f;
    croc_mid(140.0f, 0.82f, &c);
    m_reset();
    if (st == CM_WAIT || st == CM_LISTEN) {  /* a red light by its ear, blinking while it waits */
        if (st == CM_LISTEN || ((uint32_t)(t * 3.0f) & 1u))
            s_ellipse(206.0f, 96.0f, 7.0f, 7.0f, RGB(214, 60, 70));
    }
    label_box();
    {                                        /* the tune: a dot a note; the bar is how much room is left, or where the
                                              * singing is */
        uint32_t span = st == CM_LISTEN ? CROCMEM_BLOCKS : len ? len : 1u;
        if (st == CM_LISTEN)
            span = pos < 860u ? 860u : pos + pos / 4u;       /* the picture grows with the tune (5 s at first) */
        for (i = 0; i < n; i++) {
            if (!(cm_ev[i] & 0x80u))
                continue;
            s_ellipse(24.0f + 192.0f * (float)cm_t[i] / (float)span, 207.0f - (float)((cm_ev[i] & 0x7F) - 53) * 1.15f, 2.6f, 2.6f, K_GLOW);
        }
        if (st == CM_LISTEN || st == CM_SING)
            cv_rect(24 + (int32_t)(192u * (pos > span ? span : pos) / span), 174, 2, 36, st == CM_SING ? K_SUN : RGB(214, 60, 70));
    }
    strip(KN, ui.focus);
}

/* croc hops: the held keys as stepping stones, low to high like stairs, and the croc hopping along them */
static void picture_hops(float t)
{
    static const char *const KN[4] = {"WAY", "HOP", "KEEP", 0};
    static const char *const NOTE[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    static const char *const HOPS[5] = {"TINY HOPS", "SHORT HOPS", "HOPS", "LONG HOPS", "SLIDING"};
    static const char *const WAY[5] = {"NO HOPS", "UP", "DOWN", "UP AND DOWN", "ANY WAY"};
    const char *what, *now;
    croc_t c;
    int32_t i, n = crochop_n, at = crochop_at, hx = 0, hy = 0, m;
    uint8_t held[CROCHOP_MAX];
    (void)t;
    if (n > CROCHOP_MAX)
        n = CROCHOP_MAX;
    for (i = 0; i < n; i++)
        held[i] = crochop_held[i];
    scene(60);
    t_center(120, 2, "CROC HOPS", K_INK, 228, 1);
    if (ui.focus == F_K1) {
        what = "WHICH WAY THE CROC HOPS";
        now = WAY[set.arp];
    } else if (ui.focus == F_K2) {
        what = "HOW LONG EACH NOTE LASTS";
        now = HOPS[set.seq_gate];
    } else if (ui.focus == F_K3) {
        what = "KEEP HOPPING WHEN YOU LET GO";
        now = set.arp_keep ? "YES" : "NO";
    } else {
        what = "HOLD DOWN A FEW KEYS";
        now = !set.arp ? "TURN KNOB 1 TO START" : n ? "HOP HOP HOP!" : "THE CROC HOPS ON THEM";
    }
    t_small(120, 27, what, K_INK);
    t_center(120, 41, now, K_CROCD, 220, 0);
    m_reset();
    m = n ? n : 4;                           /* no keys held: four pale stones, waiting */
    for (i = 0; i < m; i++) {
        float sp = m > 1 ? fm_minf(44.0f, 188.0f / (float)(m - 1)) : 0.0f;
        float px = 120.0f + ((float)i - (float)(m - 1) * 0.5f) * sp, py = 186.0f - (m > 1 ? (float)i * 78.0f / (float)(m - 1) : 39.0f);
        int hot = n && i == at;
        s_ellipse(px, py + 3.0f, 17.0f, 8.0f, RGB(74, 160, 178));
        s_ellipse(px, py, 16.0f, 8.0f, !n ? RGB(140, 196, 200) : hot ? RGB(240, 224, 150) : RGB(168, 160, 140));
        if (n) {
            const char *nm = NOTE[held[i] % 12];
            cv_text((int32_t)px - text_w(&FONT_T, nm) / 2, (int32_t)py - 5, &FONT_T, nm, K_INK);
        }
        if (hot) {
            hx = (int32_t)px;
            hy = (int32_t)py;
        }
    }
    croc_look(&c);
    if (hx) {                                /* hopping: up and down once a step */
        uint32_t len = seq_len, pos = seq_pos;
        float f = len ? fm_clampf((float)pos / (float)len, 0.0f, 1.0f) : 0.0f;
        c.jaw = 0.7f;
        croc_draw((float)hx + 8.0f, (float)hy - 13.0f - fm_sinf(f * 3.14159f) * 9.0f, 0.3f, &c);
    } else {                                 /* waiting beside the first stone */
        c.sleep = set.arp ? 0 : 1;
        croc_draw(34.0f, 205.0f, 0.3f, &c);
    }
    strip(KN, ui.focus);
}

static void picture_name(float t)
{
    static const char *const LINE[3] = {"KNOB 1: PICK A LETTER", "KNOB 2 OR A KEY: NEXT SPOT", "PRESS HOME WHEN DONE"};
    croc_t c;
    int i;
    scene(118);
    t_center(120, 6, "NAME YOUR CROC", K_INK, 228, 1);
    croc_look(&c);
    c.teeth = 0;
    c.rows = 1;
    c.jaw = 0.3f + 0.1f * fm_sinf(t * 3.0f);
    c.tail = fm_sinf(t * 5.0f) * 0.3f;
    croc_mid(96.0f, 0.62f, &c);
    m_reset();
    for (i = 0; i < NAME_LEN; i++) {
        float x0 = 13.5f + (float)i * 27.0f, lift = i == ui.ncur ? fm_fabsf(fm_sinf(t * 4.0f)) * 3.0f : 0.0f;
        char ch[2];
        ch[0] = ui.letter[i] ? (char)('A' + ui.letter[i] - 1) : ' ';
        ch[1] = 0;
        s_rrect(x0 - 2.0f, 132.0f - lift, 28.0f, 34.0f, 7.0f, i == ui.ncur ? K_MOUTH : K_INK);
        s_rrect(x0, 134.0f - lift, 24.0f, 30.0f, 5.0f, K_CREAM);
        cv_text((int32_t)x0 + 12 - text_w(&FONT_M, ch) / 2, 137 - (int32_t)lift, &FONT_M, ch, K_INK);
    }
    s_rrect(12.0f, 174.0f, 216.0f, 58.0f, 11.0f, K_INK);
    for (i = 0; i < 3; i++)
        t_small(120, 179 + i * 17, LINE[i], i == 2 ? K_SUN : K_CREAM);
}

/* the whole picture; called once a band, with the canvas clipped to that band's rows */
static void picture(float t)
{
    static const float SIZE_S[3] = {0.55f, 0.8f, 1.05f};
    char buf[32];
    croc_t c;
    m_reset();
    croc_look(&c);
    if (ui.mode == M_BOOT) {
        scene(150);
        t_center(120, 16, "BEEN AWHILE,", K_INK, 220, 1);
        t_center(120, 46, "CROCO-DIAL!", K_CROCD, 224, 2);
        c.teeth = 0;
        c.rows = 1;
        c.jaw = 0.35f + 0.15f * fm_sinf(t * 3.0f);
        c.tail = fm_sinf(t * 6.0f) * 0.35f;
        croc_mid(142.0f, 0.85f, &c);
        m_reset();
        if (set.name[0]) {
            s_rrect(30.0f, 190.0f, 180.0f, 28.0f, 14.0f, K_INK);
            text_join(buf, "I'M ", set.name, "!");
            t_center(120, 196, buf, K_SUN, 164, 0);
        }
        cv_text(6, 226, &FONT_T, CROC_BUILD, RGB(46, 118, 130));
        return;
    }
    if (ui.mode == M_BYE) {
        cv_rect(0, 0, 240, 168, K_NIGHT);
        cv_rect(0, 168, 240, 72, K_NIGHTW);
        s_ellipse(216.0f, 24.0f, 13.0f, 13.0f, K_MOON);
        s_ellipse(210.0f, 20.0f, 11.5f, 11.5f, K_NIGHT);
        t_center(112, 18, "SEE YOU LATER,", K_PALE, 176, 1);
        t_center(120, 48, "OSCILLATOR!", K_SUN, 224, 2);
        c.teeth = 0;
        c.rows = 1;
        c.jaw = 0.25f;
        c.tail = fm_sinf(t * 3.0f) * 0.2f;
        c.wave = 1;
        c.wave_ph = t * 7.0f;
        croc_mid(150.0f, 0.85f, &c);
        if (set.name[0]) {
            text_join(buf, "LOVE, ", set.name, "");
            t_center(120, 204, buf, K_PALE, 200, 0);
        }
        return;
    }
    if (ui.mode == M_OFF) {
        cv_rect(0, 0, 240, 240, K_OFF);
        t_center(176, 58, "ZZZ", K_OFFT, 100, 1);
        t_center(120, 198, "PRESS ANY KEY", K_OFFT, 200, 0);
        return;
    }
    if (ui.mode == M_NAME) {
        picture_name(t);
        return;
    }
    if (ui.ov == OV_SIZE) {
        static const char *const NAME[3] = {"SMALL CROC", "CROC", "BIG CROC"};
        static const char *const NOTES[3] = {"HIGH NOTES", "MIDDLE NOTES", "LOW NOTES"};
        float s = SIZE_S[snd.size + 1];
        scene(150);
        t_center(120, 8, NAME[snd.size + 1], K_INK, 224, 2);
        c.jaw = 0.2f;
        c.tail = fm_sinf(t * 4.0f) * 0.2f;
        croc_mid(150.0f - 8.0f * s, s, &c);
        label_box();
        t_center(120, 184, NOTES[snd.size + 1], K_CREAM, 200, 0);
        return;
    }
    if (ui.ov == OV_TEMPO) {
        float bps = (60.0f + 15.0f * (float)set.seq_tempo) / 60.0f;
        scene(150);
        t_center(120, 8, "HOP SPEED", K_INK, 224, 2);
        c.jaw = 0.2f;
        croc_mid(140.0f - fm_fabsf(fm_sinf(t * 3.14159f * bps)) * 10.0f, 0.85f, &c);
        label_box();
        t_center(120, 184, set.seq_tempo <= 1 ? "SLOW" : set.seq_tempo <= 3 ? "WALKING" : set.seq_tempo <= 5 ? "QUICK" : "ZOOM!",
                 K_CREAM, 200, 0);
        return;
    }
    if (ui.ov == OV_JAWS) {
        int i;
        float jw = (float)snd.jaws / 12.0f;
        scene(150);
        t_center(120, 8, "JAWS", K_INK, 224, 2);
        c.jaw = jw;
        c.tail = fm_sinf(t * 3.0f) * 0.15f;
        croc_draw(106.0f, 138.0f, 0.8f, &c);
        m_reset();
        for (i = 0; i < snd.jaws; i++) {     /* the brightness flying out of the mouth */
            float p = t * 0.5f + (float)i / (float)snd.jaws, r;
            p -= fm_floorf(p);
            r = 1.5f + (1.0f - p) * 2.5f;
            s_ellipse(162.0f + p * 56.0f, 134.0f - p * 26.0f * (0.4f + jw) + fm_sinf((float)i * 2.4f) * 16.0f * p, r, r, K_SUN);
        }
        label_box();
        t_center(120, 184, snd.jaws < 4 ? "CLOSED: MUFFLED" : snd.jaws < 9 ? "HALF OPEN" : "WIDE OPEN: BRIGHT", K_CREAM, 200, 0);
        return;
    }
    if (ui.page == PG_HOME) {
        static const char *const KN[4] = {"TEETH", "SNAP", "WAG", "ECHO"};
        float s = SIZE_S[snd.size + 1] * 0.93f, w = wag_now(t);
        scene(150);
        if (set.name[0])
            text_join(buf, "HI, ", set.name, "!");
        else
            text_join(buf, "HELLO!", "", "");
        t_center(120, 8, buf, K_INK, 224, 2);
        c.jaw = (0.12f + 0.88f * (float)snd.jaws / 12.0f) * (0.25f + 0.75f * env_jaw(t));
        if (snd.wag && snd.wag_to == 2)
            c.jaw = fm_clampf(c.jaw + 0.15f * w * (0.3f + (float)snd.wag_size / 5.0f), 0.0f, 1.0f);
        if (snd.wag && snd.wag_to == 1)
            c.puff = w * (0.3f + (float)snd.wag_size * 0.17f);
        if (snd.wag)
            c.tail_len = 1.0f + (float)snd.wag_size * 0.1f;
        c.tail = w * 0.5f;
        croc_draw(120.0f + 19.0f * s, 150.0f - 8.0f * s - (snd.wag && snd.wag_to == 0 ? w * (1.0f + (float)snd.wag_size) : 0.0f), s, &c);
        label_box();
        t_center(120, 184, set.preset < NPRESET ? PRESET_NAME[set.preset] : "MY CROC", set.preset < NPRESET ? K_SUN : K_BLUSH,
                 200, 0);
        strip(KN, ui.focus);
        return;
    }
    if (ui.page == PG_EDIT) {
        static const char *const KN[4] = {"TEETH", "ROWS", "CROOKED", 0};
        static const char *const TEETH[3] = {"ROUND TEETH", "POINTY TEETH", "BLOCK TEETH"};
        static const char *const ROWS[3] = {"ONE ROW OF TEETH", "TWO ROWS OF TEETH", "THREE ROWS OF TEETH"};
        static const char *const XN[3] = {"X1", "X2", "X3"};
        scene(166);
        t_center(120, 8, ui.focus == F_K2 ? ROWS[snd.rows - 1] : ui.focus == F_K3 ? (snd.crook ? "CROOKED TEETH" : "STRAIGHT TEETH")
                             : TEETH[snd.teeth], K_INK, 228, 1);
        c.jaw = 0.62f;
        croc_draw(44.0f, 110.0f, 2.1f, &c);
        m_reset();
        s_rrect(196.0f, 148.0f, 34.0f, 18.0f, 7.0f, K_SUN);
        cv_text(213 - text_w(&FONT_XS, XN[snd.rows - 1]) / 2, 150, &FONT_XS, XN[snd.rows - 1], K_INK);
        label_box();
        if (snd.rows >= 3)
            wave_plot(snd.teeth, snd.crook * 3 + 2, K_GLOW2);
        if (snd.rows >= 2)
            wave_plot(snd.teeth, 64 - snd.crook * 3 - 2, K_GLOW2);
        wave_plot(snd.teeth, 0, K_GLOW);
        strip(KN, ui.focus);
        return;
    }
    if (ui.page == PG_ENV) {
        static const char *const KN[4] = {"OPEN", "RELAX", "HOLD", "CLOSE"};
        static const char *const WHAT[4] = {"HOW FAST THE SOUND STARTS", "HOW FAST IT DROPS AFTER THE START",
                                            "HOW LOUD IT STAYS AS YOU HOLD", "HOW LONG IT FADES AFTER YOU LET GO"};
        int32_t avg = (snd.atk + snd.rel) / 2, px[5], py[5], i;
        const char *now;
        scene(150);
        t_center(120, 4, ui.focus >= 0 ? KN[ui.focus] : avg <= 2 ? "SNAP!" : avg <= 5 ? "CHOMP" : "YAAAWN", K_INK, 224, 2);
        if (ui.focus == F_K1)
            now = snd.atk <= 1 ? "RIGHT AWAY" : snd.atk <= 5 ? "A LITTLE SLOW" : "VERY SLOW";
        else if (ui.focus == F_K2)
            now = snd.dec <= 2 ? "QUICK DROP" : snd.dec <= 6 ? "GENTLE DROP" : "SLOW DROP";
        else if (ui.focus == F_K3)
            now = snd.sus == 0 ? "SILENT" : snd.sus <= 3 ? "QUIET" : snd.sus <= 7 ? "MEDIUM" : "LOUD";
        else if (ui.focus == F_K4)
            now = snd.rel <= 1 ? "STOPS RIGHT AWAY" : snd.rel <= 5 ? "SHORT FADE" : "LONG FADE";
        else
            now = "TURN KNOBS 1 TO 4";
        t_small(120, 38, ui.focus >= 0 ? WHAT[ui.focus] : "A SOUND HAS FOUR PARTS", K_INK);
        t_center(120, 54, now, K_CROCD, 220, 0);
        c.jaw = env_jaw(t) * 0.9f;
        croc_mid(144.0f, 0.78f, &c);
        label_box();
        px[0] = 26;                          /* the shape of one note: up, down to the hold, along, down */
        py[0] = 207;
        px[1] = px[0] + 10 + snd.atk * 4;
        py[1] = 178;
        px[2] = px[1] + 10 + snd.dec * 4;
        py[2] = 207 - (29 * (snd.sus < 1 ? 1 : snd.sus)) / 10;
        px[3] = px[2] + 38;
        py[3] = py[2];
        px[4] = px[3] + 10 + snd.rel * 4;
        py[4] = 207;
        m_reset();
        p_begin();
        for (i = 0; i < 5; i++)
            p_add((float)px[i], (float)py[i]);
        p_fill(K_GLOW2);
        for (i = 0; i < 4; i++)
            thick_line(px[i], py[i], px[i + 1], py[i + 1], i == ui.focus ? 5 : 2, i == ui.focus ? K_SUN : K_GLOW);
        strip(KN, ui.focus);
        return;
    }
    if (ui.page == PG_LFO) {
        static const char *const KN[4] = {"SPEED", "SIZE", "TICKLE", 0};
        static const char *const TICKLE[3] = {"TICKLE THE TAIL", "TICKLE THE BELLY", "TICKLE THE SNOOT"};
        static const char *const DOES[3] = {"THE NOTE WOBBLES UP AND DOWN", "THE SOUND GOES LOUD AND SOFT", "THE SOUND GOES WAH WAH"};
        static const char *const BIG[5] = {"TINY", "SMALL", "MEDIUM", "BIG", "HUGE"};
        /* where the feather's tip touches, on a croc drawn by croc_mid(144, 0.78): the tail, the belly, the snoot */
        static const float TIPX[3] = {62.0f, 112.0f, 184.0f}, TIPY[3] = {138.0f, 152.0f, 131.0f}, TIPR[3] = {-0.35f, 0.45f, 0.35f};
        float w = wag_now(t), big = (float)snd.wag_size, amp = 3.0f + big * 2.6f, rate = snd.wag ? 1.0f + (float)snd.wag * 0.9f : 0.0f;
        const char *what, *now;
        int32_t i, py = 192;
        scene(150);
        if (ui.focus == F_K2) {
            what = "HOW BIG THE WAG IS";
            now = BIG[snd.wag_size];
        } else if (ui.focus == F_K3) {
            what = DOES[snd.wag_to];
            now = snd.wag ? "HEE HEE HEE!" : "TURN SPEED UP TO TICKLE";
        } else {
            what = "HOW FAST THE TAIL WAGS";
            now = snd.wag == 0 ? "STILL" : snd.wag <= 2 ? "SLOW" : snd.wag <= 5 ? "QUICK" : "SUPER FAST";
        }
        t_center(120, 4, ui.focus == F_K3 ? TICKLE[snd.wag_to] : snd.wag == 0 ? "TAIL: STILL" : snd.wag < 6 ? "TAIL WAG" : "WIGGLE WIGGLE",
                 K_INK, 228, 2);
        t_small(120, 38, what, K_INK);
        t_center(120, 54, now, K_CROCD, 220, 0);
        c.tail = w * (0.2f + 0.12f * big);
        c.tail_len = 1.0f + big * 0.17f;     /* a bigger wag, a longer tail */
        c.jaw = 0.15f;
        if (snd.wag) {
            c.sleep = 2;                     /* it tickles: laughing eyes */
            if (snd.wag_to == 1)
                c.puff = w * (0.35f + big * 0.16f);
            if (snd.wag_to == 2)
                c.jaw = 0.4f + 0.35f * w * (0.4f + big / 6.0f);
        }
        croc_mid(144.0f - (snd.wag && snd.wag_to == 0 ? w * (1.5f + big) : 0.0f), 0.78f, &c);
        m_reset();
        feather(TIPX[snd.wag_to] + w * (3.0f + big * 1.5f), TIPY[snd.wag_to], TIPR[snd.wag_to] + w * 0.25f);
        label_box();
        for (i = 0; i <= 192; i++) {         /* the wag as a line: faster is tighter, bigger is taller */
            int32_t y = 192 - (snd.wag ? (int32_t)(fm_sinf((float)i / 192.0f * 6.2831853f * (float)snd.wag * 0.75f - t * rate * 2.0f) * amp) : 0);
            if (i)
                thick_line(24 + i - 1, py, 24 + i, y, 2, K_GLOW);
            py = y;
        }
        strip(KN, ui.focus);
        return;
    }
    if (ui.page == PG_SEQ) {
        picture_seq(t);
        return;
    }
    if (ui.page == PG_SCALE) {
        picture_scale(t);
        return;
    }
    if (ui.page == PG_ARP) {
        picture_hops(t);
        return;
    }
    if (ui.page == PG_REC) {
        picture_mem(t);
        return;
    }
    if (ui.page == PG_FX) {
        static const char *const KN[4] = {"ECHO", "FAR", "CAVE", 0};
        static const char *const FAR[5] = {"VERY NEAR", "NEAR", "MIDDLE", "FAR", "VERY FAR"};
        /* the echo crocs, further off each time: where, and how small */
        static const float EX[3] = {62.0f, 176.0f, 80.0f}, EY[3] = {130.0f, 117.0f, 106.0f}, ES[3] = {0.34f, 0.24f, 0.17f};
        int32_t i, n = snd.echo > 4 ? 3 : (snd.echo + 1) / 2, dark = snd.cave >= 3;
        uint16_t sky = RGB(200 - snd.cave * 22, 241 - snd.cave * 30, 230 - snd.cave * 18), ink = dark ? K_CREAM : K_INK;
        const char *what, *now;
        float far = 1.0f - (float)snd.echo_far * 0.09f;
        cv_rect(0, 0, 240, 92, sky);         /* the sky darkens as the cave closes in */
        cv_rect(0, 92, 240, 148, K_WATER);
        cv_rect(0, 92, 240, 3, K_FOAM);
        m_reset();
        for (i = 0; i < 2 && snd.cave; i++) {    /* the cave's walls, in from each side */
            float wdt = 6.0f + (float)snd.cave * 4.0f, x0 = i ? 240.0f : 0.0f, sg = i ? -1.0f : 1.0f;
            uint16_t rock = RGB(58, 46, 84);
            cv_rect(i ? 240 - (int32_t)wdt : 0, 0, (int32_t)wdt, 95, rock);
            s_tri(x0 + sg * wdt, 0.0f, x0 + sg * (wdt + 14.0f), 0.0f, x0 + sg * wdt, 34.0f, rock);
            s_tri(x0 + sg * wdt, 40.0f, x0 + sg * (wdt + 9.0f), 66.0f, x0 + sg * wdt, 95.0f, rock);
        }
        if (ui.focus == F_K2) {
            what = "HOW FAR AWAY THE ECHO IS";
            now = FAR[snd.echo_far];
        } else if (ui.focus == F_K3) {
            what = "HOW BIG THE CAVE IS";
            now = snd.cave == 0 ? "NO CAVE" : snd.cave < 3 ? "SMALL CAVE" : snd.cave < 5 ? "BIG CAVE" : "GIANT CAVE";
        } else {
            what = "HOW MANY TIMES THE SWAMP ANSWERS";
            now = snd.echo == 0 ? "NO ECHO" : n == 1 ? "ONE ECHO" : n == 2 ? "TWO ECHOES" : "LOTS OF ECHOES";
        }
        t_center(120, 4, ui.focus == F_K3 ? "CAVE" : "SWAMP ECHO", ink, 196, 2);
        t_small(120, 38, what, ink);
        t_center(120, 54, now, dark ? K_SUN : K_CROCD, 190, 0);
        c.jaw = 0.5f;
        for (i = n - 1; i >= 0; i--)         /* the answers: the same croc, smaller and further away */
            croc_draw(EX[i], EY[i] + (1.0f - far) * 30.0f * (float)(2 - i) * 0.4f, ES[i] * far, &c);
        m_reset();
        for (i = 0; i < 2 && snd.echo; i++) {    /* ripples under the croc */
            float q = t * 0.35f + (float)i * 0.5f;
            q -= fm_floorf(q);
            s_ellipse(140.0f, 163.0f, 30.0f + q * 60.0f, 4.0f + q * 6.0f, K_FOAM);
            s_ellipse(140.0f, 163.0f, 28.0f + q * 60.0f, 3.0f + q * 5.5f, K_WATER);
        }
        c.tail = fm_sinf(t * 3.0f) * 0.2f;
        croc_draw(150.0f, 150.0f, 0.6f, &c);
        strip(KN, ui.focus);
        return;
    }
    if (ui.page == PG_GLO) {
        scene(150);
        t_center(120, 8, "GROWN-UPS", K_INK, 228, 2);
        c.jaw = 0.1f;
        croc_mid(140.0f, 0.85f, &c);
        label_box();
        t_small(120, 176, "HOLD GLO: NAME THE CROC", K_CREAM);
        t_small(120, 193, "HOLD HOME: BYE BYE", K_CREAM);
        return;
    }
    scene(150);                              /* a lesson still to come */
    t_center(120, 8, SOON_NAME[ui.soon], K_INK, 228, 2);
    c.sleep = 1;
    croc_mid(140.0f, 0.85f, &c);
    t_center(196, 78, "ZZZ", K_CROCD, 80, 0);
    label_box();
    t_center(120, 184, "COMING SOON", K_CREAM, 200, 0);
}

static void draw(void)
{
    float t = (float)(plat_ms() % 600000u) / 1000.0f;
    int band;
    for (band = 0; band < 240 / BAND_H; band++) {
        cv_begin(240, BAND_H, K_SKY);
        cv_oy = -band * BAND_H;              /* the picture's rows from this band's top: clipped to the canvas */
        picture(t);
        cv_blit(0, (uint32_t)(band * BAND_H));
    }
}

void app_ui_init(void)
{
    lcd_fill(0, 0, 240, 240, K_SKY);
    go_page(PG_HOME);
    ui.last_touch = plat_ms();
    set_mode(M_BOOT);
}

void app_frame(void)
{
    uint32_t now = plat_ms();
    input();
    ann_tick(now);
    phones_in = plat_headphones() == 1;      /* the speaker gets no deep bass; headphones do */
    if (ui.mode == M_BOOT && now - ui.t0 > 2600u) {
        if (set.name[0])
            set_mode(M_PLAY);
        else
            naming_start();                  /* the first power-on: she names the croc */
    }
    if (ui.mode == M_BYE && now - ui.t0 > 5000u)
        set_mode(M_OFF);
    if (ui.mode == M_PLAY && now - ui.last_touch > 180000u && !voices_sounding() && !seq.playing)   /* three quiet minutes */
        set_mode(M_BYE);
    if (ui.ov && now >= ui.ov_until)
        ui.ov = OV_NONE;
    /* kept, a moment after a change: in a quiet moment if there is one, and anyway after a quarter of a minute (the
     * lily pads or a kept handful of hops can play for as long as she likes) */
    if (dirty_at && now - dirty_at > 3000u && (!voices_sounding() || now - dirty_at > 15000u)) {
        if (plat_store_save(OBJ_SETTINGS, &set, sizeof set) == 0)
            dirty_at = 0;
        else
            dirty_at = now | 1u;
    }
    if (now - ui.last_draw >= 66u) {         /* the croc moves: about 15 pictures a second */
        ui.last_draw = now;
        draw();
    }
}

#ifdef PLAT_HOST
int app_expect(const char *what, int *got)
{
    static const struct {
        const char *name;
        int8_t *v;
    } S[] = {{"teeth", &snd.teeth}, {"rows", &snd.rows}, {"crook", &snd.crook}, {"size", &snd.size},
             {"jaws", &snd.jaws}, {"atk", &snd.atk}, {"dec", &snd.dec}, {"sus", &snd.sus},
             {"rel", &snd.rel}, {"wag", &snd.wag}, {"wagsize", &snd.wag_size}, {"wagto", &snd.wag_to},
             {"echo", &snd.echo}, {"far", &snd.echo_far}, {"cave", &snd.cave}};
    unsigned i;
    for (i = 0; i < sizeof S / sizeof S[0]; i++)
        if (!strcmp(what, S[i].name)) {
            *got = *S[i].v;
            return 1;
        }
    if (!strcmp(what, "mode"))
        *got = ui.mode;
    else if (!strcmp(what, "page"))
        *got = ui.page;
    else if (!strcmp(what, "voices"))
        *got = voices_sounding();
    else if (!strcmp(what, "preset"))
        *got = set.preset;
    else if (!strcmp(what, "playing"))
        *got = seq.playing;
    else if (!strcmp(what, "step"))
        *got = seq_step;
    else if (!strcmp(what, "pads"))
        *got = set.seq_pads;
    else if (!strcmp(what, "cursor"))
        *got = ui.seq_cur;
    else if (!strcmp(what, "hop"))
        *got = set.seq_gate;
    else if (!strcmp(what, "tempo"))
        *got = set.seq_tempo;
    else if (!strcmp(what, "scale"))
        *got = set.scale;
    else if (!strcmp(what, "scalehome"))
        *got = set.scale_home;
    else if (!strcmp(what, "litscale"))          /* the keys glowing for the scale, as bits (key 0 is bit 0) */
        *got = (int)ui.lit_scale;
    else if (!strcmp(what, "lithome"))
        *got = (int)ui.lit_home;
    else if (!strncmp(what, "keynote", 7))       /* keynoteK: the note key K (two digits) plays now */
        *got = (int)key_to_note((uint32_t)((what[7] - '0') * 10 + (what[8] - '0')));
    else if (!strcmp(what, "arp"))
        *got = set.arp;
    else if (!strcmp(what, "arpkeep"))
        *got = set.arp_keep;
    else if (!strcmp(what, "arpn"))
        *got = crochop_n;
    else if (!strcmp(what, "arpat"))
        *got = crochop_at;
    else if (!strcmp(what, "memstate"))          /* 0 quiet, 1 waiting for a key, 2 listening, 3 singing */
        *got = crocmem_state;
    else if (!strcmp(what, "memn"))
        *got = crocmem_n;
    else if (!strcmp(what, "memlen"))
        *got = crocmem_len;
    else if (!strcmp(what, "memrate"))
        *got = crocmem.rate;
    else if (!strcmp(what, "memagain"))
        *got = crocmem.again;
    else if (!strncmp(what, "pad", 3) && what[3] >= '0' && what[3] < '0' + SEQ_N && !what[4])   /* padN: pad N's note */
        *got = set.seq_note[what[3] - '0'];
    else if (!strcmp(what, "named"))
        *got = set.name[0] != 0;
    else if (!strncmp(what, "name", 4) && what[4] >= '0' && what[4] < '0' + NAME_LEN)   /* nameN: letter N's code */
        *got = set.name[what[4] - '0'];
    else
        return 0;
    return 1;
}
#endif
