/* SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
 * Copyright 2026 goose */
/* Croc-O-Synth's drawing: filled shapes through a small transform, on the platform's canvas (base/device/gfx.c),
 * and the placeholder croc built from them. The screen is drawn in two bands of 240 x 120, each the whole picture
 * clipped to its rows. Included by app.c, after synth.c. */

#define BAND_H 120

/* the picture's colours */
#define K_SKY RGB(200, 241, 230)
#define K_WATER RGB(108, 199, 214)
#define K_FOAM RGB(190, 230, 238)
#define K_INK RGB(18, 53, 45)
#define K_CROC RGB(92, 192, 74)
#define K_CROCD RGB(61, 154, 58)
#define K_BELLY RGB(217, 242, 140)
#define K_MOUTH RGB(255, 127, 159)
#define K_BLUSH RGB(240, 170, 150)
#define K_GLOW RGB(184, 243, 107)
#define K_GLOW2 RGB(96, 140, 70)
#define K_SUN RGB(255, 217, 74)
#define K_CREAM RGB(234, 255, 208)
#define K_TOOTH RGB(255, 255, 255)
#define K_TOOTH2 RGB(207, 220, 192)
#define K_NIGHT RGB(39, 53, 107)
#define K_NIGHTW RGB(63, 103, 163)
#define K_MOON RGB(255, 243, 176)
#define K_PALE RGB(234, 242, 255)
#define K_OFF RGB(11, 21, 18)
#define K_OFFT RGB(90, 125, 108)

/* ---------------------------------------------------------- transform --- */
typedef struct {
    float a, b, c, d, tx, ty;            /* X = a x + c y + tx, Y = b x + d y + ty */
} mat_t;
static mat_t g_m, g_stack[6];
static int g_sp;

static void m_reset(void)
{
    g_m.a = g_m.d = 1.0f;
    g_m.b = g_m.c = g_m.tx = g_m.ty = 0.0f;
    g_sp = 0;
}
static void m_push(void)
{
    if (g_sp < 6)
        g_stack[g_sp++] = g_m;
}
static void m_pop(void)
{
    if (g_sp > 0)
        g_m = g_stack[--g_sp];
}
static void m_translate(float x, float y)
{
    g_m.tx += g_m.a * x + g_m.c * y;
    g_m.ty += g_m.b * x + g_m.d * y;
}
static void m_scale(float sx, float sy)
{
    g_m.a *= sx;
    g_m.b *= sx;
    g_m.c *= sy;
    g_m.d *= sy;
}
static void m_rotate(float r)            /* clockwise on the screen (y runs down) */
{
    float cs = fm_cosf(r), sn = fm_sinf(r);
    mat_t m = g_m;
    g_m.a = m.a * cs + m.c * sn;
    g_m.b = m.b * cs + m.d * sn;
    g_m.c = m.c * cs - m.a * sn;
    g_m.d = m.d * cs - m.b * sn;
}

/* ------------------------------------------------------------ polygons --- */
#define PMAX 28
static float p_x[PMAX], p_y[PMAX];
static int p_n;

static void p_begin(void) { p_n = 0; }
static void p_add(float x, float y)
{
    if (p_n < PMAX) {
        p_x[p_n] = g_m.a * x + g_m.c * y + g_m.tx;
        p_y[p_n] = g_m.b * x + g_m.d * y + g_m.ty;
        p_n++;
    }
}
/* a convex polygon, filled: each screen row between its leftmost and rightmost crossing */
static void p_fill(uint16_t col)
{
    float ymin = 1e9f, ymax = -1e9f;
    int32_t y, y0, y1, top = -cv_oy, bot = top + (int32_t)cv_h;
    int i;
    if (p_n < 3)
        return;
    for (i = 0; i < p_n; i++) {
        ymin = fm_minf(ymin, p_y[i]);
        ymax = fm_maxf(ymax, p_y[i]);
    }
    y0 = (int32_t)fm_floorf(ymin + 0.5f);
    y1 = (int32_t)fm_floorf(ymax + 0.5f);
    if (y0 < top)
        y0 = top;
    if (y1 > bot)
        y1 = bot;
    for (y = y0; y < y1; y++) {
        float yc = (float)y + 0.5f, xl = 1e9f, xr = -1e9f;
        for (i = 0; i < p_n; i++) {
            int j = i + 1 < p_n ? i + 1 : 0;
            float ya = p_y[i], yb = p_y[j];
            if ((ya <= yc && yb > yc) || (yb <= yc && ya > yc)) {
                float x = p_x[i] + (yc - ya) * (p_x[j] - p_x[i]) / (yb - ya);
                xl = fm_minf(xl, x);
                xr = fm_maxf(xr, x);
            }
        }
        if (xr > xl) {
            int32_t xa = (int32_t)fm_floorf(xl + 0.5f), xb = (int32_t)fm_floorf(xr + 0.5f);
            cv_rect(xa, y, xb - xa, 1, col);
        }
    }
}

static const float UC[16] = {1.0f, 0.9239f, 0.7071f, 0.3827f, 0.0f, -0.3827f, -0.7071f, -0.9239f,
                             -1.0f, -0.9239f, -0.7071f, -0.3827f, 0.0f, 0.3827f, 0.7071f, 0.9239f};
static const float US[16] = {0.0f, 0.3827f, 0.7071f, 0.9239f, 1.0f, 0.9239f, 0.7071f, 0.3827f,
                             0.0f, -0.3827f, -0.7071f, -0.9239f, -1.0f, -0.9239f, -0.7071f, -0.3827f};

static void s_ellipse(float cx, float cy, float rx, float ry, uint16_t col)
{
    int i;
    p_begin();
    for (i = 0; i < 16; i++)
        p_add(cx + rx * UC[i], cy + ry * US[i]);
    p_fill(col);
}
static void s_rrect(float x, float y, float w, float h, float r, uint16_t col)   /* rounded corners */
{
    static const float QC[4] = {1.0f, 0.866f, 0.5f, 0.0f}, QS[4] = {0.0f, 0.5f, 0.866f, 1.0f};
    int i;
    p_begin();
    for (i = 0; i < 4; i++)
        p_add(x + w - r + r * QC[i], y + h - r + r * QS[i]);
    for (i = 0; i < 4; i++)
        p_add(x + r - r * QS[i], y + h - r + r * QC[i]);
    for (i = 0; i < 4; i++)
        p_add(x + r - r * QC[i], y + r - r * QS[i]);
    for (i = 0; i < 4; i++)
        p_add(x + w - r + r * QS[i], y + r - r * QC[i]);
    p_fill(col);
}
static void s_tri(float x0, float y0, float x1, float y1, float x2, float y2, uint16_t col)
{
    p_begin();
    p_add(x0, y0);
    p_add(x1, y1);
    p_add(x2, y2);
    p_fill(col);
}

/* ---------------------------------------------------------------- text --- */
/* centred on cx with its top at y, in the largest of the given faces that fits maxw */
static void t_center(int32_t cx, int32_t y, const char *s, uint16_t col, int32_t maxw, int big)
{
    const felucca_font_t *f = big >= 2 ? &FONT_L : big == 1 ? &FONT_M : &FONT_B;
    if (f == &FONT_L && text_w(f, s) > maxw)
        f = &FONT_M;
    if (f == &FONT_M && text_w(f, s) > maxw)
        f = &FONT_B;
    cv_text(cx - text_w(f, s) / 2, y, f, s, col);
}
static void t_small(int32_t cx, int32_t y, const char *s, uint16_t col) { cv_text(cx - text_w(&FONT_XS, s) / 2, y, &FONT_XS, s, col); }

/* ---------------------------------------------------------------- croc --- */
typedef struct {
    float jaw;                           /* 0 shut .. 1 wide */
    float tail;                          /* radians */
    int teeth, rows, crook;
    int sleep;
    int wave;                            /* an arm waves */
    float wave_ph;
    float tail_len;                      /* 1 the usual tail; longer for a bigger wag */
    float puff;                          /* the belly swells: -1..1 */
} croc_t;

static void tooth(float x, int style, uint16_t col)   /* hangs from y = 1 */
{
    if (style == 0) {
        int i;
        p_begin();
        for (i = 0; i <= 8; i++)
            p_add(x + 3.6f * UC[i], 1.0f + 3.6f * US[i]);
        p_fill(col);
    } else if (style == 1) {
        s_tri(x - 3.6f, 1.0f, x + 3.6f, 1.0f, x, 9.5f, col);
    } else {
        p_begin();
        p_add(x - 3.2f, 1.0f);
        p_add(x + 3.2f, 1.0f);
        p_add(x + 3.2f, 7.4f);
        p_add(x - 3.2f, 7.4f);
        p_fill(col);
    }
}

/* the placeholder croc: faces right, its origin near the shoulder, about 200 units long at scale 1 */
static void croc_draw(float cx, float cy, float s, const croc_t *o)
{
    int i;
    float jaw = o->jaw;
    m_push();
    m_translate(cx, cy);
    m_scale(s, s);
    /* tail */
    m_push();
    m_translate(-62.0f, 2.0f);
    m_rotate(o->tail);
    m_scale(o->tail_len, 1.0f);
    p_begin();
    for (i = 0; i <= 4; i++) {           /* the upper edge out to the tip, then the lower edge back */
        float t = (float)i / 4.0f, u = 1.0f - t;
        p_add(u * u * 8.0f + 2.0f * u * t * -30.0f + t * t * -58.0f, u * u * -16.0f + 2.0f * u * t * -14.0f + t * t * -4.0f);
    }
    for (i = 1; i <= 4; i++) {
        float t = (float)i / 4.0f, u = 1.0f - t;
        p_add(u * u * -58.0f + 2.0f * u * t * -30.0f + t * t * 8.0f, u * u * -4.0f + 2.0f * u * t * 10.0f + t * t * 16.0f);
    }
    p_fill(K_CROC);
    m_pop();
    /* legs, back spikes, body, belly */
    s_rrect(-58.0f, 12.0f, 15.0f, 16.0f, 6.0f, K_CROCD);
    s_rrect(-16.0f, 12.0f, 15.0f, 16.0f, 6.0f, K_CROCD);
    for (i = 0; i < 5; i++) {
        float x = -60.0f + (float)i * 13.0f, q = (x + 36.0f) / 40.0f;
        float yt = -22.0f * fm_sqrtf(fm_maxf(0.0f, 1.0f - q * q));
        s_tri(x, yt + 4.0f, x + 6.0f, yt - 8.0f, x + 12.0f, yt + 4.0f, K_CROCD);
    }
    s_ellipse(-30.0f, 0.0f, 40.0f, 22.0f, K_CROC);
    s_ellipse(-30.0f, 11.0f + o->puff * 2.5f, 29.0f * (1.0f + o->puff * 0.12f), 8.0f * (1.0f + o->puff * 0.55f), K_BELLY);
    if (o->wave) {
        m_push();
        m_translate(-6.0f, -6.0f);
        m_rotate(-0.5f + fm_sinf(o->wave_ph) * 0.45f);
        s_rrect(-5.0f, -40.0f, 10.0f, 42.0f, 5.0f, K_CROCD);
        s_ellipse(0.0f, -40.0f, 7.0f, 7.0f, K_CROC);
        m_pop();
    }
    /* head, the open mouth, the jaws */
    s_ellipse(18.0f, -6.0f, 22.0f, 20.0f, K_CROC);
    if (jaw > 0.02f) {
        float a1 = jaw * 0.5f, a2 = jaw * 0.25f;
        s_tri(22.0f, -2.0f, 26.0f + 54.0f * fm_cosf(a1), -2.0f - 54.0f * fm_sinf(a1), 26.0f + 52.0f * fm_cosf(a2),
              4.0f + 52.0f * fm_sinf(a2), K_MOUTH);
    }
    m_push();                            /* lower jaw: the second row of teeth points up from it */
    m_translate(26.0f, -2.0f);
    m_rotate(jaw * 0.25f);
    s_rrect(-8.0f, 0.0f, 58.0f, 11.0f, 5.5f, K_CROC);
    if (o->rows >= 2) {
        m_push();
        m_scale(1.0f, -1.0f);
        for (i = 0; i < 4; i++)
            tooth(12.0f + (float)i * 9.0f + (float)o->crook * 1.5f, o->teeth, K_TOOTH);
        m_pop();
    }
    m_pop();
    m_push();                            /* upper jaw: the first row, and a third row behind it */
    m_translate(26.0f, -2.0f);
    m_rotate(-jaw * 0.5f);
    s_rrect(-8.0f, -13.0f, 64.0f, 14.0f, 7.0f, K_CROC);
    s_ellipse(50.0f, -13.0f, 6.0f, 4.5f, K_CROC);
    s_ellipse(52.0f, -14.0f, 1.6f, 1.6f, K_INK);
    if (o->rows >= 3) {
        m_push();
        m_translate(0.0f, 1.5f);
        m_scale(1.0f, 1.25f);
        for (i = 0; i < 4; i++)
            tooth(14.5f + (float)i * 9.0f - (float)o->crook * 0.9f, o->teeth, K_TOOTH2);
        m_pop();
    }
    for (i = 0; i < 5; i++)
        tooth(10.0f + (float)i * 9.0f, o->teeth, K_TOOTH);
    m_pop();
    /* eye, cheek */
    s_ellipse(14.0f, -24.0f, 11.0f, 11.0f, K_CROC);
    if (o->sleep == 2) {                 /* laughing: the eye squeezed into a little arch */
        s_ellipse(15.0f, -24.0f, 6.5f, 5.0f, K_INK);
        s_ellipse(15.0f, -21.8f, 6.5f, 4.6f, K_CROC);
    } else if (o->sleep) {
        s_rrect(9.0f, -24.0f, 12.0f, 2.4f, 1.2f, K_INK);
    } else {
        s_ellipse(15.0f, -25.0f, 7.5f, 7.5f, K_TOOTH);
        s_ellipse(17.0f, -25.0f, 3.7f, 3.7f, K_INK);
        s_ellipse(18.4f, -26.6f, 1.3f, 1.3f, K_TOOTH);
    }
    s_ellipse(25.0f, -9.0f, 5.0f, 3.0f, K_BLUSH);
    m_pop();
}
static void croc_mid(float y, float s, const croc_t *o) { croc_draw(120.0f + 19.0f * s, y, s, o); }

/* a five-pointed star, r to its points: a pentagon and five triangles (each a convex piece) */
static void s_star(float cx, float cy, float r, uint16_t col)
{
    static const float SC[10] = {0.0f, 0.5878f, 0.9511f, 0.9511f, 0.5878f, 0.0f, -0.5878f, -0.9511f, -0.9511f, -0.5878f};
    static const float SS[10] = {-1.0f, -0.809f, -0.309f, 0.309f, 0.809f, 1.0f, 0.809f, 0.309f, -0.309f, -0.809f};
    float q = r * 0.42f;
    int i;
    p_begin();
    for (i = 1; i < 10; i += 2)
        p_add(cx + q * SC[i], cy + q * SS[i]);
    p_fill(col);
    for (i = 0; i < 10; i += 2)
        s_tri(cx + r * SC[i], cy + r * SS[i], cx + q * SC[(i + 1) % 10], cy + q * SS[(i + 1) % 10], cx + q * SC[(i + 9) % 10],
              cy + q * SS[(i + 9) % 10], col);
}

/* a feather with its tip at (x, y), the rest of it off at angle r (0: straight up from the tip) */
static void feather(float x, float y, float r)
{
    m_push();
    m_translate(x, y);
    m_rotate(r);
    s_ellipse(0.0f, -13.0f, 5.0f, 12.0f, K_MOUTH);
    s_ellipse(1.5f, -13.0f, 2.2f, 9.0f, K_BLUSH);
    s_rrect(-0.9f, -30.0f, 1.8f, 28.0f, 0.9f, K_TOOTH);
    m_pop();
}

/* ------------------------------------------------------- screen pieces --- */
static void scene(int32_t water_y)
{
    cv_rect(0, 0, 240, water_y, K_SKY);
    cv_rect(0, water_y, 240, 240 - water_y, K_WATER);
    cv_rect(0, water_y, 240, 3, K_FOAM);
}
static void label_box(void)
{
    m_reset();
    s_rrect(12.0f, 170.0f, 216.0f, 44.0f, 11.0f, K_INK);
}
/* the wave across the panel: kind 0 sine, 1 saw, 2 square; dx slides it sideways */
static void wave_plot(int kind, int32_t dx, uint16_t col)
{
    int32_t x, py = 0;
    for (x = 0; x <= 192; x++) {
        float p = (float)(x + dx) / 64.0f, v;
        int32_t y;
        p -= fm_floorf(p);
        v = kind == 0 ? fm_sin_turns(p + 0.14f * fm_sin_turns(p)) : kind == 1 ? 1.0f - 2.0f * p : p < 0.5f ? 1.0f : -1.0f;
        y = 192 - (int32_t)(v * 12.0f);
        if (x) {
            cv_line(24 + x - 1, py, 24 + x, y, col);
            cv_line(24 + x - 1, py + 1, 24 + x, y + 1, col);
            cv_line(24 + x, py, 24 + x, y, col);
        }
        py = y;
    }
}
/* a line w pixels thick */
static void thick_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, uint16_t col)
{
    int32_t i;
    for (i = 0; i < w; i++) {
        cv_line(x0, y0 + i - w / 2, x1, y1 + i - w / 2, col);
        cv_line(x0 + 1, y0 + i - w / 2, x1 + 1, y1 + i - w / 2, col);
    }
}

/* what KNOB 1 to 4 do on this screen, along the bottom; `lit` is the one turned last (-1 none) */
static void strip(const char *const *names, int lit)
{
    int i;
    m_reset();
    for (i = 0; i < 4; i++) {
        float x = 8.0f + (float)i * 56.5f;
        if (!names[i])
            continue;
        s_rrect(x, 219.0f, 54.0f, 17.0f, 6.0f, i == lit ? K_INK : RGB(60, 120, 122));
        cv_text((int32_t)x + 27 - text_w(&FONT_T, names[i]) / 2, 222, &FONT_T, names[i], i == lit ? K_SUN : K_CREAM);
    }
}
