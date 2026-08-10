#ifndef SCREEN_TRANSITION_H
#define SCREEN_TRANSITION_H

#include "lvgl.h"
#include "screen.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SCREEN_TRANS_NONE,
    SCREEN_TRANS_SLIDE_LEFT,
    SCREEN_TRANS_SLIDE_RIGHT,
    SCREEN_TRANS_SLIDE_UP,
    SCREEN_TRANS_SLIDE_DOWN,
    SCREEN_TRANS_FADE,
} screen_transition_type_t;

typedef struct {
    screen_transition_type_t enter;
    screen_transition_type_t exit;
    uint16_t                 duration;
} screen_transition_t;

void screen_transition_start(screen_t *from, screen_t *to,
                             screen_transition_t *trans,
                             lv_anim_ready_cb_t on_complete);

#ifdef __cplusplus
}
#endif

#endif
