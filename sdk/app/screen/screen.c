#include "screen.h"
#include "screen_memory.h"
#include <string.h>

screen_t *screen_create(const char *name, screen_lifecycle_t *lifecycle)
{
    screen_t *screen = SCREEN_MALLOC(sizeof(screen_t));
    if (!screen) return NULL;
    memset(screen, 0, sizeof(screen_t));

    screen->root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen->root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(screen->root, 0, 0);
    lv_obj_add_flag(screen->root, LV_OBJ_FLAG_HIDDEN);

    if (name) {
        size_t len = strlen(name) + 1;
        screen->name = (const char *)SCREEN_MALLOC(len);
        if (screen->name) lv_memcpy((void *)screen->name, name, len);
    }
    if (lifecycle) {
        lv_memcpy(&screen->lifecycle, lifecycle, sizeof(screen_lifecycle_t));
    }
    screen->state = SCREEN_STATE_NONE;
    screen->user_data = NULL;
    screen->only = lifecycle->only;

    return screen;
}

void screen_destroy(screen_t *screen)
{
    if (!screen) return;
    screen->state = SCREEN_STATE_DESTROYED;

    if (screen->root) {
        lv_obj_del(screen->root);
        screen->root = NULL;
    }
    if (screen->name) SCREEN_FREE((void *)screen->name);
    SCREEN_FREE(screen);
}

void screen_set_user_data(screen_t *screen, void *user_data)
{
    if (screen) screen->user_data = user_data;
}

void *screen_get_user_data(screen_t *screen)
{
    return screen ? screen->user_data : NULL;
}
