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
    int8_t size;                         /* -2..2: octaves down (a bigger croc sings lower) */
    int8_t jaws;                         /* 0..12: the filter, closed to wide open */
    int8_t atk, dec, sus, rel;           /* 0..10 each: open, relax, hold, close */
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

static const float CROOK_CENTS[5] = {0.0f, 6.0f, 12.0f, 20.0f, 30.0f};
static const float WAVE_LEVEL[3] = {0.75f, 0.48f, 0.4f};
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

/* The round tooth: a sine bent by itself, so it has a second and third harmonic. A pure sine's low notes are below
 * what the FM-1's small speaker can play, and were nearly silent on it. */
static inline float wave_round(float ph) { return fm_sin_turns(ph + 0.14f * fm_sin_turns(ph)); }

/* what the mix is made of, kept from block to block: a change of rows or teeth fades over one block (5.8 ms)
 * instead of jumping, which crackled on the FM-1 */
static float row_g[NROW], wave_w[3], norm_now, fc_now, lim_pk;
static int8_t rows_prev = 1;
static uint8_t mix_init;

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    sound_t s = snd;
    uint32_t i;
    int v, j, kk;
    float gain = (float)master_q12 / 4096.0f * 8300000.0f * 0.85f;     /* MASTER, with a little kept back */
    float a = (float)s.atk / 10.0f;
    float a_inc = 1.0f / ((0.005f + a * a * 0.8f) * SRATE);
    float kd = 3.0f / ((0.05f + (float)s.dec / 10.0f * 0.8f) * SRATE);
    float kr = 3.0f / ((0.05f + (float)s.rel / 10.0f * 1.5f) * SRATE);
    float sus = (float)s.sus / 10.0f;
    float fc = 200.0f * fm_powf(40.0f, (float)s.jaws / 12.0f);        /* 200 Hz closed .. 8 kHz wide open */
    float g, k = 0.5f, a1, a2, a3, inv_n, norm0, norm1;
    float rg0[NROW], rg1[NROW], ww0[3], ww1[3];
    int act[NROW], wact[3];
    midi_in();
    if (n > APP_BLOCK)
        n = APP_BLOCK;
    if (!n)
        return;
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
    g = fm_tanf(3.14159265f * fc_now / SRATE);
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
    for (i = 0; i < n; i++)
        mixbuf[i] = 0.0f;
    for (v = 0; v < NV; v++) {
        voice_t *vc = &vv[v];
        float inc[NROW], base, kt, env = vc->env;
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
        base = 440.0f * fm_exp2f((float)(vc->note - 69) / 12.0f - (float)s.size) / SRATE;
        kt = fm_clampf(fm_sqrtf(400.0f / (base * SRATE)), 0.8f, 1.8f);     /* low notes a little louder */
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
                ph = vc->ph[j] + inc[j];
                if (ph >= 1.0f)
                    ph -= 1.0f;
                vc->ph[j] = ph;
                if (wact[0])
                    w += w0 * WAVE_LEVEL[0] * wave_round(ph);
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
            mixbuf[i] += x * env * kt * (norm0 + (norm1 - norm0) * f);
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
    for (i = 0; i < n; i++) {                /* the jaws: a two-pole low-pass; then a limiter, so chords stay clean */
        float v0 = mixbuf[i], v3 = v0 - flt_ic2;
        float v1 = a1 * flt_ic1 + a2 * v3, v2 = flt_ic2 + a2 * flt_ic1 + a3 * v3, out, av;
        flt_ic1 = 2.0f * v1 - flt_ic1;
        flt_ic2 = 2.0f * v2 - flt_ic2;
        av = fm_fabsf(v2);
        lim_pk = av > lim_pk ? av : lim_pk * 0.9995f;
        gain_now += (gain - gain_now) * 0.004f;
        out = v2 * (lim_pk > 1.0f ? 1.0f / lim_pk : 1.0f) * gain_now;
        lr[2 * i] = (int32_t)out;
        lr[2 * i + 1] = (int32_t)out;
    }
    flt_ic1 = fm_flush(flt_ic1);
    flt_ic2 = fm_flush(flt_ic2);
    lim_pk = fm_flush(lim_pk);
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
