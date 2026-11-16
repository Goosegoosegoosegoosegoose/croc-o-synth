/* SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
 * Copyright 2026 goose */
/* Croc-O-Synth's voice: up to three oscillators a note (the rows of teeth), an envelope, one low-pass filter (the
 * jaws) over the mix. The main loop sets `snd` and the voices' notes; the audio interrupt (app_render) plays them.
 * Included by app.c. */

#define NV 4                             /* notes at once */
#define NROW 3                           /* oscillators a note */
#define SRATE 44100.0f

typedef struct {
    int8_t teeth;                        /* 0 round (sine), 1 pointy (saw), 2 block (square) */
    int8_t rows;                         /* 1..3 oscillators */
    int8_t crook;                        /* 0..4: how far apart the rows are tuned */
    int8_t size;                         /* -1 small, 0 croc, 1 big: a bigger croc sings an octave lower */
    int8_t jaws;                         /* 0..12: the filter, closed to wide open */
    int8_t atk, dec, sus, rel;           /* 0..10 each: open, relax, hold, close */
    int8_t wag;                          /* 0 still, 1..8: the tail wag's speed (the LFO) */
    int8_t wag_size;                     /* 0..4: how far it wags */
    int8_t wag_to;                       /* what the wag tickles: 0 the tail (pitch), 1 the belly (volume), 2 the snoot (filter) */
    int8_t echo;                         /* 0..6: how many times the swamp answers */
    int8_t echo_far;                     /* 0..4: how far away the echo is (its delay) */
    int8_t cave;                         /* 0..6: how big the cave is (reverb) */
    int8_t rsv[6];                       /* room for more, so the saved record keeps its size */
} sound_t;
static sound_t snd;

typedef struct {
    volatile int32_t note;               /* MIDI note */
    volatile uint8_t gate;
    volatile uint32_t trig;              /* counts the note-ons: the interrupt restarts the envelope on a new one */
    uint32_t seen, age;
    float ph[NROW], env;
    uint8_t stage;                       /* 0 idle, 1 open, 2 relax and hold, 3 close */
} voice_t;
static voice_t vv[NV];
static uint32_t note_clock;
static float flt_ic1, flt_ic2, gain_now;
static float mixbuf[APP_BLOCK];

/* the swamp echo: one delay line, up to 0.6 s, kept as 16-bit in the platform's big pool (not in the small RAM) */
#define DL_N 27000
static int16_t dl_buf[DL_N] __attribute__((section(".pool")));
static uint32_t dl_w;
static float dl_now = 8000.0f, echo_wet, echo_fb, cave_g;
static const float ECHO_SEC[5] = {0.18f, 0.26f, 0.34f, 0.45f, 0.6f};
/* the cave: four combs and two all-passes (the classic small reverb), also in the pool */
#define RV_C0 1116
#define RV_C1 1188
#define RV_C2 1277
#define RV_C3 1356
#define RV_A0 556
#define RV_A1 441
static float rv_c0[RV_C0] __attribute__((section(".pool"))), rv_c1[RV_C1] __attribute__((section(".pool"))),
             rv_c2[RV_C2] __attribute__((section(".pool"))), rv_c3[RV_C3] __attribute__((section(".pool"))),
             rv_a0[RV_A0] __attribute__((section(".pool"))), rv_a1[RV_A1] __attribute__((section(".pool")));
static uint32_t rv_i[6];
static float rv_lp[4];

static void fx_clear(void)                   /* the pool is not cleared at power-on */
{
    uint32_t i;
    for (i = 0; i < DL_N; i++)
        dl_buf[i] = 0;
    for (i = 0; i < RV_C0; i++)
        rv_c0[i] = 0.0f;
    for (i = 0; i < RV_C1; i++)
        rv_c1[i] = 0.0f;
    for (i = 0; i < RV_C2; i++)
        rv_c2[i] = 0.0f;
    for (i = 0; i < RV_C3; i++)
        rv_c3[i] = 0.0f;
    for (i = 0; i < RV_A0; i++)
        rv_a0[i] = 0.0f;
    for (i = 0; i < RV_A1; i++)
        rv_a1[i] = 0.0f;
}

static inline float rv_comb(float *buf, uint32_t len, uint32_t *idx, float *lp, float in)
{
    float y = buf[*idx];
    *lp = y * 0.75f + *lp * 0.25f;           /* the cave's walls soak up the brightest part */
    buf[*idx] = in + *lp * 0.82f;
    if (++*idx >= len)
        *idx = 0;
    return y;
}
static inline float rv_allpass(float *buf, uint32_t len, uint32_t *idx, float in)
{
    float y = buf[*idx];
    buf[*idx] = in + y * 0.5f;
    if (++*idx >= len)
        *idx = 0;
    return y - in;
}
static inline float cave_tick(float in)
{
    float o = rv_comb(rv_c0, RV_C0, &rv_i[0], &rv_lp[0], in) + rv_comb(rv_c1, RV_C1, &rv_i[1], &rv_lp[1], in) +
              rv_comb(rv_c2, RV_C2, &rv_i[2], &rv_lp[2], in) + rv_comb(rv_c3, RV_C3, &rv_i[3], &rv_lp[3], in);
    o = rv_allpass(rv_a0, RV_A0, &rv_i[4], o * 0.25f);
    return rv_allpass(rv_a1, RV_A1, &rv_i[5], o);
}

static const float CROOK_CENTS[5] = {0.0f, 6.0f, 12.0f, 20.0f, 30.0f};
static const float WAVE_LEVEL[3] = {0.62f, 0.42f, 0.35f};
static const float ROW_LEVEL[3] = {1.0f, 0.72f, 0.58f};

static void note_on(int32_t note)
{
    int i, pick = 0;
    uint32_t best = 0xFFFFFFFFu;
    for (i = 0; i < NV; i++) {           /* the same note again, else a silent voice, else the oldest */
        uint32_t score = vv[i].gate ? 0x80000000u + vv[i].age : vv[i].age;
        if (vv[i].gate && vv[i].note == note) {
            pick = i;
            break;
        }
        if (score < best) {
            best = score;
            pick = i;
        }
    }
    vv[pick].note = note;
    vv[pick].age = ++note_clock;
    vv[pick].gate = 1;
    vv[pick].trig++;
}

static void note_off(int32_t note)
{
    int i;
    for (i = 0; i < NV; i++)
        if (vv[i].gate && vv[i].note == note)
            vv[i].gate = 0;
}

static void notes_off(void)
{
    int i;
    for (i = 0; i < NV; i++)
        vv[i].gate = 0;
}

static int voices_sounding(void)
{
    int i, n = 0;
    for (i = 0; i < NV; i++)
        n += vv[i].gate ? 1 : 0;
    return n;
}

static void midi_in(void);               /* platglue.c: notes that came in over USB or TRS play too */

/* The keys' notes reach the voices through this queue, so every note-on and note-off happens in the audio interrupt
 * (the keys, MIDI in and the lily pads would otherwise pick voices from two places at once). One writer: the main
 * loop. One reader: the interrupt. A byte each: the note, with the top bit set for a note-on. */
static volatile uint8_t evq[32];
static volatile uint32_t ev_w, ev_r;
static void key_event(int32_t note, int on)
{
    evq[ev_w & 31u] = (uint8_t)((uint32_t)note | (on ? 0x80u : 0u));
    ev_w++;
}

/* ---------------------------------------------------- croc hops (the arp) --- */
/* With a way chosen, held keys do not sound together: they go on a list, lowest first, and the clock below plays
 * them one at a time. `keep` leaves the list in place when the keys are let go, until a new key starts a new one. */
#define CROCHOP_MAX 8
static struct {
    volatile uint8_t way;                /* 0 off, 1 up, 2 down, 3 up and down, 4 any way (random) */
    volatile uint8_t keep;               /* 1: keep hopping when the keys are let go */
    volatile uint8_t flush;              /* the main loop bumps this to drop the list (a change of mode) */
} crochop;
static volatile uint8_t crochop_held[CROCHOP_MAX], crochop_n;   /* the list, for the screen too */
static volatile int8_t crochop_at = -1;  /* which of them the croc is on (-1 none) */
static volatile int8_t crochop_sounding = -1;   /* its note (-1 none), for the keys' lights */
static uint8_t ch_way, ch_flush, ch_down, ch_fresh;
static int8_t ch_note = -1, ch_dir = 1;
static int32_t ch_gate_left;
static uint32_t ch_rng = 0x2545F491u;

static void crochop_sync(void)               /* the main loop changed the way, or asked for the list to go */
{
    uint8_t way = crochop.way, fl = crochop.flush;
    if (way == ch_way && fl == ch_flush)
        return;
    if (fl != ch_flush || !way != !ch_way) { /* on, off, or a flush: nothing held over from before */
        notes_off();
        crochop_n = 0;
        ch_down = 0;
        ch_note = -1;
        crochop_at = -1;
        crochop_sounding = -1;
    }
    ch_way = way;
    ch_flush = fl;
}
static void crochop_add(uint8_t note)
{
    uint8_t n = crochop_n, i, j;
    if (crochop.keep && ch_down == 0)        /* the first key of a new handful replaces the kept one */
        n = 0;
    ch_down++;
    if (n == 0)
        ch_fresh = 1;
    for (i = 0; i < n && crochop_held[i] < note; i++)
        ;
    if (i < n && crochop_held[i] == note) {
        crochop_n = n;
        return;
    }
    if (n >= CROCHOP_MAX)
        return;
    for (j = n; j > i; j--)
        crochop_held[j] = crochop_held[j - 1];
    crochop_held[i] = note;
    crochop_n = (uint8_t)(n + 1u);
}
static void crochop_del(uint8_t note)
{
    uint8_t n = crochop_n, i;
    if (ch_down)
        ch_down--;
    if (crochop.keep)
        return;
    for (i = 0; i < n; i++)
        if (crochop_held[i] == note) {
            for (; i + 1u < n; i++)
                crochop_held[i] = crochop_held[i + 1];
            crochop_n = (uint8_t)(n - 1u);
            return;
        }
}
/* the next one to hop to, by the way chosen */
static uint8_t crochop_next(uint8_t n)
{
    int32_t at = crochop_at;
    if (ch_fresh || at < 0 || at >= (int32_t)n) {
        ch_fresh = 0;
        ch_dir = 1;
        return ch_way == 2 ? (uint8_t)(n - 1u) : 0;
    }
    if (n < 2)
        return 0;
    if (ch_way == 1)
        return (uint8_t)((at + 1) % (int32_t)n);
    if (ch_way == 2)
        return (uint8_t)(at ? at - 1 : (int32_t)n - 1);
    if (ch_way == 3) {
        if (at + ch_dir < 0 || at + ch_dir >= (int32_t)n)
            ch_dir = (int8_t)-ch_dir;
        return (uint8_t)(at + ch_dir);
    }
    ch_rng = ch_rng * 1664525u + 1013904223u;
    {
        uint8_t r = (uint8_t)((ch_rng >> 16) % n);
        return r == (uint8_t)at ? (uint8_t)((r + 1u) % n) : r;
    }
}

/* ------------------------------------------------ croc remembers (REC) --- */
/* The croc listens to the keys and sings the tune back. What is kept is each key going down and coming up, and when,
 * counted in audio blocks (5.8 ms) from the first key. It is sung back through the voices as they are now, so the
 * sound can be changed while it sings. Kept until the power goes off (not in the flash). */
#define CROCMEM_MAX 240                  /* key-downs and key-ups: 120 notes */
#define CROCMEM_BLOCKS 5168u             /* 30 s */
enum { CM_QUIET, CM_WAIT, CM_LISTEN, CM_SING };   /* waiting: REC pressed, no key yet */
enum { CM_DO_LISTEN = 1, CM_DO_SING_NOW, CM_DO_QUIET, CM_DO_SING };
static struct {
    volatile uint8_t cmd, cmd_n;         /* the main loop: what to do, and a count that changes with each ask */
    volatile uint8_t rate;               /* 0 slow (half speed), 1 the same, 2 fast (double) */
    volatile uint8_t again;              /* 1: sing it again and again */
} crocmem;
static volatile uint8_t crocmem_state;
static volatile uint16_t crocmem_n, crocmem_len, crocmem_pos;   /* key moves kept; the tune's length and where the
                                                                  * singing is, in blocks */
static volatile uint32_t crocmem_keys;   /* the keys it is singing now, as bits (53 is bit 0): for the lights */
static uint16_t cm_t[CROCMEM_MAX];       /* when, in blocks */
static uint8_t cm_ev[CROCMEM_MAX];       /* the note, top bit set for a key going down */
static uint8_t cm_seen;
static uint32_t cm_clock, cm_fp, cm_i, cm_down;

static void crocmem_hush(void)               /* let go of whatever it is singing */
{
    uint32_t m = crocmem_keys, k;
    for (k = 0; k < 32u; k++)
        if (m >> k & 1u)
            note_off(53 + (int32_t)k);
    crocmem_keys = 0;
}
static void crocmem_sing(void)
{
    crocmem_hush();
    cm_fp = 0;
    cm_i = 0;
    crocmem_pos = 0;
    crocmem_state = crocmem_n ? CM_SING : CM_QUIET;
}
static void crocmem_done(void)               /* the listening ends: close the notes still held, fix the length */
{
    uint32_t k, n = crocmem_n;
    for (k = 0; k < 32u && n < CROCMEM_MAX; k++)
        if (cm_down >> k & 1u) {
            cm_t[n] = (uint16_t)cm_clock;
            cm_ev[n] = (uint8_t)(53u + k);
            n++;
        }
    cm_down = 0;
    crocmem_n = (uint16_t)n;
    crocmem_len = (uint16_t)(cm_clock ? cm_clock : 1u);
}
static void crocmem_hear(uint8_t b)          /* a key moved while it listens */
{
    uint32_t n = crocmem_n, bit = (uint32_t)(b & 0x7Fu) - 53u;
    if (crocmem_state == CM_WAIT) {
        if (!(b & 0x80u))
            return;
        crocmem_state = CM_LISTEN;       /* the first key: the tune starts here */
        cm_clock = 0;
    }
    if (crocmem_state != CM_LISTEN || bit >= 32u)
        return;
    if (b & 0x80u ? n + 33u > CROCMEM_MAX : n >= CROCMEM_MAX)
        return;                          /* full (a key going down leaves room for every key to come up) */
    if (b & 0x80u)
        cm_down |= 1u << bit;
    else if (!(cm_down >> bit & 1u))
        return;                          /* a key that went down before the listening began */
    else
        cm_down &= ~(1u << bit);
    cm_t[n] = (uint16_t)cm_clock;
    cm_ev[n] = b;
    crocmem_n = (uint16_t)(n + 1u);
}
static void crocmem_tick(void)               /* audio interrupt, once a block, after the keys */
{
    if (crocmem.cmd_n != cm_seen) {
        uint8_t c = crocmem.cmd;
        cm_seen = crocmem.cmd_n;
        if (c == CM_DO_LISTEN) {
            crocmem_hush();
            crocmem_n = 0;
            crocmem_len = 0;
            cm_down = 0;
            crocmem_state = CM_WAIT;
        } else if (c == CM_DO_SING_NOW || c == CM_DO_QUIET) {
            if (crocmem_state == CM_LISTEN)
                crocmem_done();
            crocmem_hush();
            crocmem_state = CM_QUIET;
            if (c == CM_DO_SING_NOW)
                crocmem_sing();
        } else if (c == CM_DO_SING && crocmem_state == CM_QUIET) {
            crocmem_sing();
        }
    }
    if (crocmem_state == CM_LISTEN) {
        if (++cm_clock >= CROCMEM_BLOCKS) {  /* half a minute: that is the tune */
            crocmem_done();
            crocmem_sing();
        }
        crocmem_pos = (uint16_t)cm_clock;
    } else if (crocmem_state == CM_SING) {
        uint32_t pos = cm_fp >> 8, n = crocmem_n;
        while (cm_i < n && cm_t[cm_i] <= pos) {
            uint8_t b = cm_ev[cm_i++];
            uint32_t bit = (uint32_t)(b & 0x7Fu) - 53u;
            if (b & 0x80u) {
                note_on(b & 0x7F);
                crocmem_keys |= 1u << bit;
            } else {
                note_off(b & 0x7F);
                crocmem_keys &= ~(1u << bit);
            }
        }
        cm_fp += crocmem.rate == 0 ? 128u : crocmem.rate == 2 ? 512u : 256u;
        crocmem_pos = (uint16_t)pos;
        if ((cm_fp >> 8) >= crocmem_len && cm_i >= n) {
            crocmem_hush();
            if (crocmem.again)
                crocmem_sing();
            else
                crocmem_state = CM_QUIET;
        }
    }
}

static void key_events(void)
{
    crochop_sync();
    while (ev_r != ev_w) {
        uint8_t b = evq[ev_r & 31u];
        ev_r++;
        crocmem_hear(b);
        if (ch_way) {
            if (b & 0x80u)
                crochop_add(b & 0x7F);
            else
                crochop_del(b & 0x7F);
        } else if (b & 0x80u) {
            note_on(b & 0x7F);
        } else {
            note_off(b & 0x7F);
        }
    }
}

/* --------------------------------------------------- the lily pads' clock --- */
/* Eight steps, two to a beat. The main loop sets the pads and the speed; the interrupt counts frames and plays each
 * pad's note as its step comes round (to the nearest audio block, 5.8 ms). Croc hops (the arpeggiator) is meant to
 * run on this same clock. */
#define SEQ_N 8
typedef struct {
    volatile int8_t note[SEQ_N];         /* each pad's MIDI note (a key's: 53..79), -1 an empty pad */
    volatile uint8_t pads;               /* 2..8: how many pads the croc hops before going round again */
    volatile uint8_t gate;               /* 0..4: how long each note lasts, tiny hops to sliding */
    volatile uint8_t tempo;              /* 0..8: hop speed, 60 to 180 beats a minute */
    volatile uint8_t playing;
} seq_t;
static seq_t seq;
static volatile int8_t seq_step = -1;    /* the pad the croc is on (-1: not hopping), for the screen */
static volatile int8_t seq_sounding = -1;   /* the note its pad is playing now (-1 none), for the keys' lights */
static volatile uint32_t seq_pos, seq_len;  /* frames into this step, and a step's length */
static uint32_t sq_count;
static int32_t sq_gate_left;
static int8_t sq_note = -1;
static uint8_t sq_next, sq_was;
static const float SEQ_GATE[5] = {0.25f, 0.45f, 0.65f, 0.85f, 0.98f};

static uint8_t ck_run;

/* One clock for the lily pads and for croc hops: it runs while either has something to play, and both step on the
 * same beat. It starts with a step at once, so the first note is not late. */
static void seq_tick(uint32_t n)             /* audio interrupt, once a block */
{
    uint32_t len = (uint32_t)(SRATE * 30.0f / (60.0f + 15.0f * (float)seq.tempo));
    float gate = SEQ_GATE[seq.gate > 4 ? 4 : seq.gate];
    int run_seq = seq.playing, run_hop = ch_way && crochop_n;
    if (sq_note >= 0 && (sq_gate_left -= (int32_t)n) <= 0) {
        note_off(sq_note);
        sq_note = -1;
        seq_sounding = -1;
    }
    if (ch_note >= 0 && (ch_gate_left -= (int32_t)n) <= 0) {
        note_off(ch_note);
        ch_note = -1;
        crochop_sounding = -1;
    }
    if (!run_seq && sq_was) {                /* PLAY again: the pads stop */
        if (sq_note >= 0)
            note_off(sq_note);
        sq_note = -1;
        sq_was = 0;
        seq_step = -1;
        seq_sounding = -1;
    }
    if (!run_hop)
        crochop_at = -1;
    if (!run_seq && !run_hop) {
        ck_run = 0;
        return;
    }
    if (!ck_run) {
        ck_run = 1;
        sq_count = len;
    } else {
        sq_count += n;
    }
    if (run_seq && !sq_was) {
        sq_was = 1;
        sq_next = 0;
    }
    if (sq_count >= len) {
        sq_count -= len;
        if (sq_count >= len)
            sq_count = 0;
        if (run_seq) {
            uint8_t st = (uint8_t)(sq_next % (seq.pads ? seq.pads : 1u));
            int8_t nt = seq.note[st];
            if (sq_note >= 0)
                note_off(sq_note);
            sq_note = -1;
            if (nt >= 0) {
                note_on(nt);
                sq_note = nt;
                sq_gate_left = (int32_t)((float)len * gate);
            }
            seq_sounding = sq_note;
            seq_step = (int8_t)st;
            sq_next = (uint8_t)(st + 1u);
        }
        if (run_hop) {
            uint8_t nn = crochop_n, at = crochop_next(nn);
            if (ch_note >= 0)
                note_off(ch_note);
            ch_note = (int8_t)crochop_held[at];
            note_on(ch_note);
            ch_gate_left = (int32_t)((float)len * gate);
            crochop_at = (int8_t)at;
            crochop_sounding = ch_note;
        }
    }
    seq_pos = sq_count;
    seq_len = len;
}

static inline float polyblep(float t, float dt)
{
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

static inline float row_cents(int j, int n, float d) { return n == 1 ? 0.0f : n == 2 ? (j ? d * 0.5f : -d * 0.5f) : (float)(j - 1) * d; }

/* The round tooth: a sine bent by itself (by `bend` turns), so it has harmonics. A pure sine's low notes are below
 * what the FM-1's small speaker can play; the lower the note, the more it is bent, so its harmonics carry it. */
static inline float wave_round(float ph, float bend) { return fm_sin_turns(ph + bend * fm_sin_turns(ph)); }

static volatile uint8_t phones_in;       /* headphones plugged in (the main loop reads the jack) */
static float hp_ic1, hp_ic2;

/* what the mix is made of, kept from block to block: a change of rows or teeth fades over one block (5.8 ms)
 * instead of jumping, which crackled on the FM-1 */
static float row_g[NROW], wave_w[3], norm_now, fc_now, lim_pk;
static float lfo_ph, lfo_prev, pm_prev = 1.0f, am_prev = 1.0f;
static const float WAG_VOL[5] = {0.2f, 0.35f, 0.5f, 0.7f, 0.9f};            /* how far the volume dips */
static const float WAG_PITCH[5] = {8.0f, 18.0f, 35.0f, 60.0f, 100.0f};      /* cents each way */
static const float WAG_JAWS[5] = {200.0f, 500.0f, 900.0f, 1500.0f, 2400.0f};
static int8_t rows_prev = 1;
static uint8_t mix_init;

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    sound_t s = snd;
    uint32_t i;
    int v, j, kk;
    float gain = (float)master_q12 / 4096.0f * 8300000.0f * 0.6f;      /* MASTER, with room kept for the small speaker */
    float a = (float)s.atk / 10.0f;
    float a_inc = 1.0f / ((0.005f + a * a * 0.8f) * SRATE);
    float kd = 3.0f / ((0.05f + (float)s.dec / 10.0f * 0.8f) * SRATE);
    float kr = 3.0f / ((0.05f + (float)s.rel / 10.0f * 1.5f) * SRATE);
    float sus = (float)s.sus / 10.0f;
    float fc = 200.0f * fm_powf(40.0f, (float)s.jaws / 12.0f);        /* 200 Hz closed .. 8 kHz wide open */
    float g, k = 0.5f, a1, a2, a3, inv_n, norm0, norm1;
    float hg = fm_tanf(3.14159265f * (phones_in ? 30.0f : 170.0f) / SRATE), hk = 1.414f;   /* below this the speaker only rattles */
    float h1 = 1.0f / (1.0f + hg * (hg + hk)), h2 = hg * h1, h3 = hg * h2;
    float rg0[NROW], rg1[NROW], ww0[3], ww1[3];
    int act[NROW], wact[3];
    float lfo0 = lfo_prev, lfo1, pm0 = pm_prev, pm1, am0 = am_prev, am1, jw_cents;
    float wet_t = (float)s.echo / 6.0f * 0.5f, fb_t = s.echo ? 0.25f + (float)s.echo * 0.05f : 0.0f;
    float dl_t = ECHO_SEC[s.echo_far] * SRATE, cave_t = (float)s.cave / 6.0f * 0.55f;
    midi_in();
    key_events();
    crocmem_tick();
    if (n > APP_BLOCK)
        n = APP_BLOCK;
    if (!n)
        return;
    seq_tick(n);
    inv_n = 1.0f / (float)n;
    if (!mix_init) {
        mix_init = 1;
        for (j = 0; j < NROW; j++)
            row_g[j] = j < s.rows ? 1.0f : 0.0f;
        for (kk = 0; kk < 3; kk++)
            wave_w[kk] = kk == s.teeth ? 1.0f : 0.0f;
        norm_now = ROW_LEVEL[s.rows - 1];
        fc_now = fc;
        rows_prev = s.rows;
    }
    for (j = 0; j < NROW; j++) {
        rg0[j] = row_g[j];
        rg1[j] = j < s.rows ? 1.0f : 0.0f;
        act[j] = rg0[j] > 0.0f || rg1[j] > 0.0f;
    }
    for (kk = 0; kk < 3; kk++) {
        ww0[kk] = wave_w[kk];
        ww1[kk] = kk == s.teeth ? 1.0f : 0.0f;
        wact[kk] = ww0[kk] > 0.0f || ww1[kk] > 0.0f;
    }
    norm0 = norm_now;
    norm1 = ROW_LEVEL[s.rows - 1];
    fc_now += (fc - fc_now) * 0.4f;          /* the jaws glide: no steps in the filter */
    /* the tail wag: one slow sine, on the voice's pitch or on the jaws */
    lfo_ph += (s.wag ? 1.0f + (float)s.wag * 0.9f : 0.0f) * (float)n / SRATE;
    lfo_ph -= fm_floorf(lfo_ph);
    lfo1 = s.wag ? fm_sin_turns(lfo_ph) : 0.0f;
    pm1 = s.wag && s.wag_to == 0 ? fm_exp2f(lfo1 * WAG_PITCH[s.wag_size] / 1200.0f) : 1.0f;
    am1 = s.wag && s.wag_to == 1 ? 1.0f - WAG_VOL[s.wag_size] * (0.5f + 0.5f * lfo1) : 1.0f;
    jw_cents = s.wag && s.wag_to == 2 ? WAG_JAWS[s.wag_size] : 0.0f;
    g = a1 = a2 = a3 = 0.0f;
    for (i = 0; i < n; i++)
        mixbuf[i] = 0.0f;
    for (v = 0; v < NV; v++) {
        voice_t *vc = &vv[v];
        float inc[NROW], base, bend, env = vc->env;
        uint32_t trig = vc->trig;
        uint8_t stage = vc->stage, gate = vc->gate;
        if (trig != vc->seen) {
            vc->seen = trig;
            stage = 1;
            if (env < 0.02f)                 /* a note from silence starts its rows together, the same every time */
                for (j = 0; j < NROW; j++)
                    vc->ph[j] = 0.0f;
        }
        if (!gate && stage && stage != 3)
            stage = 3;
        if (!stage)
            continue;
        /* the keys are F3..G5; the croc sings one octave above that, so every size stays in the speaker's range:
         * small F5..G7, croc F4..G6, big F3..G5 */
        base = 440.0f * fm_exp2f((float)(vc->note - 69) / 12.0f + 1.0f - (float)s.size) / SRATE;
        bend = fm_clampf(0.14f * fm_powf(350.0f / (base * SRATE), 0.75f), 0.14f, 0.6f);   /* lower notes, more harmonics */
        for (j = 0; j < NROW; j++) {
            float cents;
            if (!act[j])
                continue;
            cents = row_cents(j, j < s.rows ? s.rows : rows_prev, CROOK_CENTS[s.crook]);
            inc[j] = fm_minf(0.45f, base * fm_exp2f(cents / 1200.0f));
            if (rg0[j] == 0.0f && j > 0)
                vc->ph[j] = vc->ph[0];       /* a new row comes in beside the first */
        }
        for (i = 0; i < n; i++) {
            float x = 0.0f, f = (float)i * inv_n;
            float w0 = ww0[0] + (ww1[0] - ww0[0]) * f, w1 = ww0[1] + (ww1[1] - ww0[1]) * f, w2 = ww0[2] + (ww1[2] - ww0[2]) * f;
            if (stage == 1) {
                env += a_inc;
                if (env >= 1.0f) {
                    env = 1.0f;
                    stage = 2;
                }
            } else if (stage == 2) {
                env += (sus - env) * kd;
            } else {
                env -= env * kr;
            }
            for (j = 0; j < NROW; j++) {
                float ph, w = 0.0f;
                if (!act[j])
                    continue;
                ph = vc->ph[j] + inc[j] * (pm0 + (pm1 - pm0) * f);
                if (ph >= 1.0f)
                    ph -= 1.0f;
                vc->ph[j] = ph;
                if (wact[0])
                    w += w0 * WAVE_LEVEL[0] * wave_round(ph, bend);
                if (wact[1])
                    w += w1 * WAVE_LEVEL[1] * (2.0f * ph - 1.0f - polyblep(ph, inc[j]));
                if (wact[2]) {
                    float p2 = ph + 0.5f;
                    if (p2 >= 1.0f)
                        p2 -= 1.0f;
                    w += w2 * WAVE_LEVEL[2] * ((ph < 0.5f ? 1.0f : -1.0f) + polyblep(ph, inc[j]) - polyblep(p2, inc[j]));
                }
                x += w * (rg0[j] + (rg1[j] - rg0[j]) * f);
            }
            mixbuf[i] += x * env * (norm0 + (norm1 - norm0) * f);
        }
        if (stage == 3 && env < 0.0005f) {
            env = 0.0f;
            stage = 0;
        }
        vc->env = env;
        vc->stage = stage;
    }
    for (j = 0; j < NROW; j++)
        row_g[j] = rg1[j];
    for (kk = 0; kk < 3; kk++)
        wave_w[kk] = ww1[kk];
    norm_now = norm1;
    rows_prev = s.rows;
    lfo_prev = lfo1;
    pm_prev = pm1;
    am_prev = am1;
    for (i = 0; i < n; i++) {                /* the jaws: a two-pole low-pass; then a limiter, so chords stay clean */
        float v0 = mixbuf[i] * (am0 + (am1 - am0) * (float)i * inv_n), v1, v2, v3, out, av;
        if ((i & 31u) == 0u) {               /* the filter's setting, every 32 frames: a wobbling jaw has no steps */
            float lf = lfo0 + (lfo1 - lfo0) * (float)i * inv_n;
            float fw = fm_clampf(fc_now * fm_exp2f(lf * jw_cents / 1200.0f), 60.0f, 12000.0f);
            g = fm_tanf(3.14159265f * fw / SRATE);
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }
        v3 = v0 - flt_ic2;
        v1 = a1 * flt_ic1 + a2 * v3;
        v2 = flt_ic2 + a2 * flt_ic1 + a3 * v3;
        flt_ic1 = 2.0f * v1 - flt_ic1;
        flt_ic2 = 2.0f * v2 - flt_ic2;
        {                                    /* the swamp echo: the delay's length glides, like a tape, so no clicks */
            float rd, fr, e, in;
            uint32_t i0, i1;
            dl_now += (dl_t - dl_now) * 0.0004f;
            echo_wet += (wet_t - echo_wet) * 0.002f;
            echo_fb += (fb_t - echo_fb) * 0.002f;
            cave_g += (cave_t - cave_g) * 0.002f;
            rd = (float)dl_w - dl_now;
            if (rd < 0.0f)
                rd += (float)DL_N;
            i0 = (uint32_t)rd;
            fr = rd - (float)i0;
            if (i0 >= DL_N)
                i0 = DL_N - 1u;
            i1 = i0 + 1u >= DL_N ? 0u : i0 + 1u;
            e = ((float)dl_buf[i0] + ((float)dl_buf[i1] - (float)dl_buf[i0]) * fr) * (1.0f / 16384.0f);
            in = fm_clampf(v2 + e * echo_fb, -1.99f, 1.99f);
            dl_buf[dl_w] = (int16_t)(in * 16384.0f);
            if (++dl_w >= DL_N)
                dl_w = 0;
            v2 += e * echo_wet;
            if (cave_g > 0.0005f || cave_t > 0.0f)   /* the cave, after the echo */
                v2 += cave_tick(v2 * 0.3f) * cave_g;
        }
        {                                    /* a high-pass under the speaker's range (lower with headphones) */
            float u3 = v2 - hp_ic2, u1 = h1 * hp_ic1 + h2 * u3, u2 = hp_ic2 + h2 * hp_ic1 + h3 * u3;
            hp_ic1 = 2.0f * u1 - hp_ic1;
            hp_ic2 = 2.0f * u2 - hp_ic2;
            v2 = v2 - hk * u1 - u2;
        }
        av = fm_fabsf(v2);
        lim_pk = av > lim_pk ? av : lim_pk * 0.99993f;   /* lets go over a third of a second: steady through a low note */
        gain_now += (gain - gain_now) * 0.004f;
        out = v2 * (lim_pk > 1.0f ? 1.0f / lim_pk : 1.0f) * gain_now;
        lr[2 * i] = (int32_t)out;
        lr[2 * i + 1] = (int32_t)out;
    }
    flt_ic1 = fm_flush(flt_ic1);
    flt_ic2 = fm_flush(flt_ic2);
    lim_pk = fm_flush(lim_pk);
    hp_ic1 = fm_flush(hp_ic1);
    hp_ic2 = fm_flush(hp_ic2);
    for (j = 0; j < 4; j++)
        rv_lp[j] = fm_flush(rv_lp[j]);
}

void app_panic(void)
{
    int i;
    for (i = 0; i < NV; i++) {
        vv[i].gate = 0;
        vv[i].stage = 0;
        vv[i].env = 0.0f;
    }
}
