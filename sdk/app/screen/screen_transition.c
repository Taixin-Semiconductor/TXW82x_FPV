#include "screen_transition.h"
#include "screen_memory.h"
#include <string.h>

typedef struct {
    lv_anim_t            enter_anim;
    lv_anim_t            exit_anim;
    screen_t            *enter_screen;
    screen_t            *exit_screen;
    lv_anim_ready_cb_t   user_callback;
    uint8_t              entered_done;
    uint8_t              exited_done;
} trans_ctx_t;

static void anim_set_x(void *obj, int32_t v)
{
    lv_obj_set_x((lv_obj_t *)obj, (lv_coord_t)v);
}

static void anim_set_opa(void *obj, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)v, 0);
}

static void on_trans_enter_ready(lv_anim_t *a)
{
    trans_ctx_t *ctx = (trans_ctx_t *)a->user_data;
    ctx->entered_done = 1;
    if (ctx->entered_done && ctx->exited_done) {
        lv_obj_add_flag(ctx->exit_screen->root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(ctx->exit_screen->root, 0, 0);
        lv_obj_set_pos(ctx->enter_screen->root, 0, 0);
        lv_obj_set_style_opa(ctx->enter_screen->root, LV_OPA_COVER, 0);
        lv_obj_set_style_opa(ctx->exit_screen->root, LV_OPA_COVER, 0);
        if (ctx->user_callback) ctx->user_callback(a);
        SCREEN_FREE(ctx);
    }
}

static void on_trans_exit_ready(lv_anim_t *a)
{
    trans_ctx_t *ctx = (trans_ctx_t *)a->user_data;
    ctx->exited_done = 1;
    if (ctx->entered_done && ctx->exited_done) {
        lv_obj_add_flag(ctx->exit_screen->root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(ctx->exit_screen->root, 0, 0);
        lv_obj_set_pos(ctx->enter_screen->root, 0, 0);
        lv_obj_set_style_opa(ctx->enter_screen->root, LV_OPA_COVER, 0);
        lv_obj_set_style_opa(ctx->exit_screen->root, LV_OPA_COVER, 0);
        if (ctx->user_callback) ctx->user_callback(a);
        SCREEN_FREE(ctx);
    }
}

void screen_transition_start(screen_t *from, screen_t *to,
                             screen_transition_t *trans,
                             lv_anim_ready_cb_t on_complete)
{
    if (!from || !to || !trans) {
        if (on_complete) on_complete(NULL);
        return;
    }

    lv_disp_t *disp = lv_disp_get_default();
    lv_coord_t h_res = lv_disp_get_hor_res(disp);
    lv_coord_t v_res = lv_disp_get_ver_res(disp);

    lv_coord_t enter_start_x = 0, enter_start_y = 0;
    lv_coord_t exit_end_x = 0, exit_end_y = 0;

    lv_obj_set_pos(to->root, 0, 0);
    lv_obj_set_pos(from->root, 0, 0);

    uint8_t is_fade = 0;

    switch (trans->enter) {
        case SCREEN_TRANS_SLIDE_LEFT:
            enter_start_x = h_res;
            enter_start_y = 0;
            exit_end_x = -h_res;
            exit_end_y = 0;
            break;
        case SCREEN_TRANS_SLIDE_RIGHT:
            enter_start_x = -h_res;
            enter_start_y = 0;
            exit_end_x = h_res;
            exit_end_y = 0;
            break;
        case SCREEN_TRANS_SLIDE_UP:
            enter_start_x = 0;
            enter_start_y = v_res;
            exit_end_x = 0;
            exit_end_y = -v_res;
            break;
        case SCREEN_TRANS_SLIDE_DOWN:
            enter_start_x = 0;
            enter_start_y = -v_res;
            exit_end_x = 0;
            exit_end_y = v_res;
            break;
        case SCREEN_TRANS_FADE:
            is_fade = 1;
            break;
        default:
            break;
    }

    lv_obj_clear_flag(to->root, LV_OBJ_FLAG_HIDDEN);

    trans_ctx_t *ctx = SCREEN_MALLOC(sizeof(trans_ctx_t));
    if (!ctx) {
        if (on_complete) on_complete(NULL);
        return;
    }
    memset(ctx, 0, sizeof(trans_ctx_t));
    ctx->enter_screen = to;
    ctx->exit_screen = from;
    ctx->user_callback = on_complete;

    if (is_fade) {
        lv_anim_set_exec_cb(&ctx->enter_anim, anim_set_opa);
        lv_anim_set_var(&ctx->enter_anim, to->root);
        lv_anim_set_values(&ctx->enter_anim, LV_OPA_TRANSP, LV_OPA_COVER);
        lv_anim_set_time(&ctx->enter_anim, trans->duration);
        lv_anim_set_path_cb(&ctx->enter_anim, lv_anim_path_ease_out);
        lv_anim_set_ready_cb(&ctx->enter_anim, on_trans_enter_ready);
        lv_anim_set_user_data(&ctx->enter_anim, ctx);
        lv_anim_start(&ctx->enter_anim);

        lv_anim_set_exec_cb(&ctx->exit_anim, anim_set_opa);
        lv_anim_set_var(&ctx->exit_anim, from->root);
        lv_anim_set_values(&ctx->exit_anim, LV_OPA_COVER, LV_OPA_TRANSP);
        lv_anim_set_time(&ctx->exit_anim, trans->duration);
        lv_anim_set_path_cb(&ctx->exit_anim, lv_anim_path_ease_out);
        lv_anim_set_ready_cb(&ctx->exit_anim, on_trans_exit_ready);
        lv_anim_set_user_data(&ctx->exit_anim, ctx);
        lv_anim_start(&ctx->exit_anim);
    } else {
        lv_obj_set_pos(to->root, enter_start_x, enter_start_y);

        lv_anim_set_exec_cb(&ctx->enter_anim, anim_set_x);
        lv_anim_set_var(&ctx->enter_anim, to->root);
        lv_anim_set_values(&ctx->enter_anim, enter_start_x, 0);
        lv_anim_set_time(&ctx->enter_anim, trans->duration);
        lv_anim_set_path_cb(&ctx->enter_anim, lv_anim_path_ease_out);
        lv_anim_set_ready_cb(&ctx->enter_anim, on_trans_enter_ready);
        lv_anim_set_user_data(&ctx->enter_anim, ctx);
        lv_anim_start(&ctx->enter_anim);

        lv_anim_set_exec_cb(&ctx->exit_anim, anim_set_x);
        lv_anim_set_var(&ctx->exit_anim, from->root);
        lv_anim_set_values(&ctx->exit_anim, 0, exit_end_x);
        lv_anim_set_time(&ctx->exit_anim, trans->duration);
        lv_anim_set_path_cb(&ctx->exit_anim, lv_anim_path_ease_out);
        lv_anim_set_ready_cb(&ctx->exit_anim, on_trans_exit_ready);
        lv_anim_set_user_data(&ctx->exit_anim, ctx);
        lv_anim_start(&ctx->exit_anim);
    }
}
