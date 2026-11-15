/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal
 * Adapted for Croc-O-Synth from fm1-base's template/app.h: the name, the splash colour and the store's objects
 * changed, the touch and Bluetooth options removed (see PROVENANCE.md). */
/* The app's half of the contract with the platform (base/device/unity.c, base/host/plat_host.c). */
#pragma once
#include <stdint.h>
#include "plat.h"

#define APP_NAME "CROC-O-SYNTH"
#define APP_SPLASH_RGB RGB(92, 192, 74)
#define APP_BLOCK 256u                   /* frames per audio block: 5.8 ms at 44.1 kHz */

/* the store: the app's objects and the panel calibration (the platform's), and their flash offsets */
enum { OBJ_SETTINGS, OBJ_PANEL, OBJ_NOBJ };
#define APP_STORE_MAP {0xFC000u, 0xDC000u}

/* the contract: the platform calls these */
void app_init(void);                     /* boot: the engine, then the saved settings (or defaults) */
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);   /* audio interrupt: n stereo frames, 24-bit */
void app_panic(void);                    /* silence now (before an update rewrites the flash) */
void app_ui_init(void);                  /* the first screen, after the splash */
void app_frame(void);                    /* the UI: input and a redraw, about 60 times a second */
void app_input_only(void);               /* between frames: input only */
void app_say(const char *a, const char *b);   /* a short message to a screen reader */

/* the host simulator's checks (base/host/plat_host.c "expect"): mode, page, teeth, rows, crook, size, jaws, voices */
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
