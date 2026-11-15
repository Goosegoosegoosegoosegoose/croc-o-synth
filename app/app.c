/* SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
 * Copyright 2026 goose */
/* Croc-O-Synth: a crocodile teaches synth basics, one lesson a button. This first firmware has the start-up
 * greeting, the home croc that wears every setting, the Teeth lesson (EDIT), the jaws (ALGORITHM), big and baby croc
 * (OCT), the goodbye (HOME held), and a placeholder for the lessons to come.
 * Part of the platform's unity build (base/device/unity.c, base/host/plat_host.c), after the platform: gfx.c and
 * announce.c are in scope. Written against the contract in app.h; see PROVENANCE.md for what came from where. */
#include "fastmath.h"
#include "synth.c"
#include "platglue.c"
#include "draw.c"

/* --------------------------------------------------------------- settings --- */
#define SET_MAGIC 0x43524F31u            /* "CRO1": the saved sound; another magic loads the defaults */
typedef struct {
    uint32_t magic;
    sound_t s;
    uint8_t rsv[3];
} settings_t;
static settings_t set;
static uint32_t dirty_at;                /* the sound changed at this time and is not saved yet (0: saved) */

static void sound_defaults(void)
{
    snd.teeth = 0;
    snd.rows = 1;
    snd.crook = 2;
    snd.size = 0;
    snd.jaws = 9;
    snd.atk = 0;
    snd.dec = 4;
    snd.sus = 7;
    snd.rel = 2;
}
static int sound_ok(const sound_t *s)
{
    return s->teeth >= 0 && s->teeth <= 2 && s->rows >= 1 && s->rows <= 3 && s->crook >= 0 && s->crook <= 4 &&
           s->size >= -2 && s->size <= 2 && s->jaws >= 0 && s->jaws <= 12 && s->atk >= 0 && s->atk <= 10 &&
           s->dec >= 0 && s->dec <= 10 && s->sus >= 0 && s->sus <= 10 && s->rel >= 0 && s->rel <= 10;
}

void app_init(void)
{
    if (plat_store_load(OBJ_SETTINGS, &set, sizeof set) == (int)sizeof set && set.magic == SET_MAGIC && sound_ok(&set.s))
        snd = set.s;
    else
        sound_defaults();
}

/* --------------------------------------------------------------------- UI --- */
enum { M_BOOT, M_PLAY, M_BYE, M_OFF };
enum { PG_HOME, PG_EDIT, PG_SOON };
enum { OV_NONE, OV_SIZE, OV_JAWS };
enum { F_NONE = -1, F_TEETH, F_ROWS, F_CROOK };

static struct {
    uint8_t mode, page, ov, soon;        /* soon: the button whose lesson is still to come */
    int8_t focus;                        /* the knob turned last on this page (F_*) */
    uint32_t t0;                         /* when this mode began */
    uint32_t ov_until, home_t, last_draw, last_touch;
    uint32_t keys, btn;
} ui;

static const char *const SOON_NAME[NB] = {"SWAMP ECHO", "SAFE SWAMP", "SNAP OR YAWN", "TAIL WAG", "", "GROWN-UPS", "",
                                          "SAVE", "CROC HOPS", "LILY PAD STEPS", "PLAY", "CROC REMEMBERS", "", ""};

void app_say(const char *a, const char *b)
{
    if (ann_on())
        ann_say(a ? a : "", b && b[0] ? b : 0, ANN_NOW);
}

static void changed(void)
{
    dirty_at = plat_ms() | 1u;
}
static void set_mode(int m)
{
    ui.mode = (uint8_t)m;
    ui.t0 = plat_ms();
    ui.ov = OV_NONE;
    if (m != M_PLAY)
        notes_off();
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

static void input(void)
{
    uint32_t keys = plat_keys(), btn = plat_buttons(), k, now = plat_ms();
    int32_t e[NE];
    int any = keys != ui.keys || btn != ui.btn;
    for (k = 0; k < NE; k++) {
        e[k] = plat_enc((int)k);
        any |= e[k] != 0;
    }
    if (any)
        ui.last_touch = now;
    if (ui.mode != M_PLAY) {
        /* the greeting ends at any touch; the goodbye and the sleeping croc wake at a new press */
        if (ui.mode == M_BOOT ? any && now - ui.t0 > 400u : (keys & ~ui.keys) || (btn & ~ui.btn))
            set_mode(ui.mode == M_BOOT ? M_PLAY : M_BOOT);
        ui.keys = keys;
        ui.btn = btn;
        plat_leds(0, 0);
        return;
    }
    for (k = 0; k < NKEYS; k++) {            /* key 0 is F3 (MIDI 53) */
        uint32_t bit = 1u << k;
        int32_t note = 53 + (int32_t)k;
        if ((keys & bit) && !(ui.keys & bit)) {
            note_on(note);
            key_midi(note, 1);
        } else if (!(keys & bit) && (ui.keys & bit)) {
            note_off(note);
            key_midi(note, 0);
        }
    }
    for (k = 0; k < NB; k++) {
        uint32_t bit = 1u << k;
        if (!(btn & bit) || (ui.btn & bit))
            continue;
        ui.ov = OV_NONE;
        if (k == B_OCTDN || k == B_OCTUP) {  /* OCT- : a bigger croc, lower notes */
            step(&snd.size, k == B_OCTDN ? 1 : -1, -2, 2);
            overlay(OV_SIZE);
        } else if (k == B_HOME) {
            ui.page = PG_HOME;
            ui.focus = F_NONE;
            ui.home_t = now | 1u;
        } else if (k == B_EDIT) {
            ui.page = PG_EDIT;
            ui.focus = F_NONE;
        } else if (k != B_PLAY) {
            ui.page = PG_SOON;
            ui.soon = (uint8_t)k;
        }
    }
    if (ui.home_t) {                         /* HOME held: goodbye */
        if (!(btn >> B_HOME & 1u)) {
            ui.home_t = 0;
        } else if (now - ui.home_t >= 900u) {
            ui.home_t = 0;
            set_mode(M_BYE);
        }
    }
    if (e[EN_ALGO]) {                        /* the jaws, on every page */
        step(&snd.jaws, e[EN_ALGO], 0, 12);
        overlay(OV_JAWS);
    }
    if (ui.page == PG_EDIT || ui.page == PG_HOME) {
        if (e[EN_K1]) {
            step(&snd.teeth, e[EN_K1], 0, 2);
            ui.focus = F_TEETH;
            ui.ov = OV_NONE;
        }
    }
    if (ui.page == PG_EDIT) {
        if (e[EN_K2]) {
            step(&snd.rows, e[EN_K2], 1, 3);
            ui.focus = F_ROWS;
            ui.ov = OV_NONE;
        }
        if (e[EN_K3]) {
            step(&snd.crook, e[EN_K3], 0, 4);
            ui.focus = F_CROOK;
            ui.ov = OV_NONE;
        }
    }
    ui.keys = keys;
    ui.btn = btn;
    plat_glow(1);
    plat_leds(ui.page == PG_HOME ? 1u << B_HOME : ui.page == PG_EDIT ? 1u << B_EDIT : 1u << ui.soon, keys);
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
}

/* the whole picture; called once a band, with the canvas clipped to that band's rows */
static void picture(float t)
{
    static const float SIZE_S[5] = {0.45f, 0.6f, 0.75f, 0.9f, 1.05f};
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
        return;
    }
    if (ui.mode == M_OFF) {
        cv_rect(0, 0, 240, 240, K_OFF);
        t_center(176, 58, "ZZZ", K_OFFT, 100, 1);
        t_center(120, 198, "PRESS ANY KEY", K_OFFT, 200, 0);
        return;
    }
    if (ui.ov == OV_SIZE) {
        static const char *const NAME[5] = {"BABY CROC", "LITTLE CROC", "CROC", "BIG CROC", "GIANT CROC"};
        static const char *const NOTES[5] = {"VERY HIGH NOTES", "HIGH NOTES", "MIDDLE NOTES", "LOW NOTES", "VERY LOW NOTES"};
        float s = SIZE_S[snd.size + 2];
        scene(150);
        t_center(120, 8, NAME[snd.size + 2], K_INK, 224, 2);
        c.jaw = 0.2f;
        c.tail = fm_sinf(t * 4.0f) * 0.2f;
        croc_mid(150.0f - 8.0f * s, s, &c);
        panel();
        t_center(120, 184, NOTES[snd.size + 2], K_CREAM, 200, 0);
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
        panel();
        t_center(120, 184, snd.jaws < 4 ? "CLOSED: MUFFLED" : snd.jaws < 9 ? "HALF OPEN" : "WIDE OPEN: BRIGHT", K_CREAM, 200, 0);
        return;
    }
    if (ui.page == PG_HOME) {
        static const char *const KN[4] = {"TEETH", 0, 0, 0};
        float s = SIZE_S[snd.size + 2] * 0.93f;
        scene(150);
        t_center(120, 8, "HELLO!", K_INK, 224, 2);
        c.jaw = (0.12f + 0.88f * (float)snd.jaws / 12.0f) * (0.25f + 0.75f * env_jaw(t));
        croc_draw(120.0f + 19.0f * s, 150.0f - 8.0f * s, s, &c);
        panel();
        t_center(120, 184, "MY CROC", K_BLUSH, 200, 0);
        strip(KN, ui.focus == F_TEETH ? 0 : -1);
        return;
    }
    if (ui.page == PG_EDIT) {
        static const char *const KN[4] = {"TEETH", "ROWS", "CROOKED", 0};
        static const char *const TEETH[3] = {"ROUND TEETH", "POINTY TEETH", "BLOCK TEETH"};
        static const char *const ROWS[3] = {"ONE ROW OF TEETH", "TWO ROWS OF TEETH", "THREE ROWS OF TEETH"};
        static const char *const XN[3] = {"X1", "X2", "X3"};
        scene(166);
        t_center(120, 8, ui.focus == F_ROWS ? ROWS[snd.rows - 1] : ui.focus == F_CROOK ? (snd.crook ? "CROOKED TEETH" : "STRAIGHT TEETH")
                             : TEETH[snd.teeth], K_INK, 228, 1);
        c.jaw = 0.62f;
        croc_draw(44.0f, 110.0f, 2.1f, &c);
        m_reset();
        s_rrect(196.0f, 148.0f, 34.0f, 18.0f, 7.0f, K_SUN);
        cv_text(213 - text_w(&FONT_XS, XN[snd.rows - 1]) / 2, 150, &FONT_XS, XN[snd.rows - 1], K_INK);
        panel();
        if (snd.rows >= 3)
            wave_plot(snd.teeth, snd.crook * 3 + 2, K_GLOW2);
        if (snd.rows >= 2)
            wave_plot(snd.teeth, 64 - snd.crook * 3 - 2, K_GLOW2);
        wave_plot(snd.teeth, 0, K_GLOW);
        strip(KN, ui.focus);
        return;
    }
    scene(150);                              /* a lesson still to come */
    t_center(120, 8, SOON_NAME[ui.soon], K_INK, 228, 2);
    c.sleep = 1;
    croc_mid(140.0f, 0.85f, &c);
    t_center(196, 78, "ZZZ", K_CROCD, 80, 0);
    panel();
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
    ui.focus = F_NONE;
    ui.page = PG_HOME;
    ui.last_touch = plat_ms();
    set_mode(M_BOOT);
}

void app_frame(void)
{
    uint32_t now = plat_ms();
    input();
    ann_tick(now);
    if (ui.mode == M_BOOT && now - ui.t0 > 2600u)
        set_mode(M_PLAY);
    if (ui.mode == M_BYE && now - ui.t0 > 5000u)
        set_mode(M_OFF);
    if (ui.mode == M_PLAY && now - ui.last_touch > 180000u && !voices_sounding())   /* three quiet minutes */
        set_mode(M_BYE);
    if (ui.ov && now >= ui.ov_until)
        ui.ov = OV_NONE;
    if (dirty_at && now - dirty_at > 3000u && !voices_sounding()) {   /* her sound is kept, a moment after a change */
        set.magic = SET_MAGIC;
        set.s = snd;
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
    if (!strcmp(what, "mode"))
        *got = ui.mode;
    else if (!strcmp(what, "page"))
        *got = ui.page;
    else if (!strcmp(what, "teeth"))
        *got = snd.teeth;
    else if (!strcmp(what, "rows"))
        *got = snd.rows;
    else if (!strcmp(what, "crook"))
        *got = snd.crook;
    else if (!strcmp(what, "size"))
        *got = snd.size;
    else if (!strcmp(what, "jaws"))
        *got = snd.jaws;
    else if (!strcmp(what, "voices"))
        *got = voices_sounding();
    else
        return 0;
    return 1;
}
#endif
