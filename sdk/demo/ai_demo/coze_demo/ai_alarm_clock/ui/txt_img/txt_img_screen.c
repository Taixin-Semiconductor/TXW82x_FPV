#include "ui_manager.h"

#include "screen_manager.h"
#include "txt_img_screen.h"

#include "ui_theme.h"
#include "osal/string.h"

typedef struct
{
    screen_t *screen;
    lv_obj_t *detail_page;
} ui_txt_img_state_t;

static void ui_txt_img_create_detail(screen_t *screen, void *params);
static void ui_txt_img_destroy_detail(screen_t *screen);
static void ui_txt_img_get_lifecycle(screen_lifecycle_t *lifecycle);
static void ui_txt_img_push_new_screen(void);
static void ui_txt_img_on_entry_click(lv_event_t *event);
static void ui_txt_img_handle_back_click(lv_event_t *event);

static void ui_txt_img_handle_back_click(lv_event_t *event)
{
    ui_txt_img_state_t *state = (ui_txt_img_state_t *)lv_event_get_user_data(event);

    if(state == NULL || state->screen == NULL) {
        return;
    }

    screen_finish(state->screen);
}

static void ui_txt_img_create_detail(screen_t *screen, void *params)
{
    ui_txt_img_state_t *state;
    lv_obj_t *page;
    lv_obj_t *header_panel;
    lv_obj_t *back_btn;
    lv_obj_t *back_label;
    lv_obj_t *title_label;

    (void)params;

    if(screen == NULL) {
        return;
    }

    state = (ui_txt_img_state_t *)os_malloc(sizeof(ui_txt_img_state_t));
    if(state == NULL) {
        return;
    }
    lv_memset(state, 0, sizeof(ui_txt_img_state_t));
    state->screen = screen;
    screen_set_user_data(screen, state);

    lv_obj_remove_style_all(screen->root);
    lv_obj_set_size(screen->root, LV_PCT(100), LV_PCT(100));
    lv_obj_center(screen->root);
    lv_obj_clear_flag(screen->root, LV_OBJ_FLAG_SCROLLABLE);

    page = lv_obj_create(screen->root);
    if(page == NULL) {
        return;
    }

    lv_obj_remove_style_all(page);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_center(page);
    lv_obj_set_style_bg_color(page, lv_color_hex(UI_COLOR_BG_DETAIL), 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    state->detail_page = page;

    header_panel = lv_obj_create(page);
    if(header_panel != NULL) {
        lv_obj_remove_style_all(header_panel);
        lv_obj_set_size(header_panel, LV_PCT(96), UI_HEADER_HEIGHT);
        lv_obj_set_style_radius(header_panel, UI_HEADER_RADIUS, 0);
        lv_obj_align(header_panel, LV_ALIGN_TOP_MID, 0, 4);
        lv_obj_set_style_bg_color(header_panel, lv_color_hex(UI_COLOR_PRIMARY), 0);
        lv_obj_set_style_bg_opa(header_panel, LV_OPA_COVER, 0);
        lv_obj_clear_flag(header_panel, LV_OBJ_FLAG_SCROLLABLE);

        back_btn = lv_obj_create(header_panel);
        if(back_btn != NULL) {
            lv_obj_remove_style_all(back_btn);
            lv_obj_set_size(back_btn, 30, 30);
            lv_obj_set_style_radius(back_btn, 15, 0);
            lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(back_btn, 0, 0);
            lv_obj_set_style_shadow_width(back_btn, 0, 0);
            lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 6, 0);
            lv_obj_add_event_cb(back_btn, ui_txt_img_handle_back_click, LV_EVENT_CLICKED, state);

            back_label = lv_label_create(back_btn);
            if(back_label != NULL) {
                lv_label_set_text(back_label, LV_SYMBOL_LEFT);
                lv_obj_set_style_text_font(back_label, UI_FONT_BODY, 0);
                lv_obj_set_style_text_color(back_label, lv_color_hex(UI_COLOR_PRIMARY_DARK), 0);
                lv_obj_center(back_label);
            }
        }

        title_label = lv_label_create(header_panel);
        if(title_label != NULL) {
            lv_label_set_text(title_label, "AI Q&A");
            lv_obj_set_style_text_font(title_label, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(title_label, lv_color_hex(UI_COLOR_PRIMARY_DARK), 0);
            lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);
        }
    }
}

static void ui_txt_img_destroy_detail(screen_t *screen)
{
    ui_txt_img_state_t *state;

    if(screen == NULL) {
        return;
    }

    state = (ui_txt_img_state_t *)screen_get_user_data(screen);
    if(state == NULL) {
        return;
    }

    state->detail_page = NULL;
    state->screen = NULL;
    screen_set_user_data(screen, NULL);
    os_free(state);
}

static void ui_txt_img_screen_start(screen_t *screen)
{
    (void)screen;
}

static void ui_txt_img_screen_resume(screen_t *screen)
{
    (void)screen;
}

static void ui_txt_img_screen_pause(screen_t *screen)
{
    (void)screen;
}

static void ui_txt_img_screen_stop(screen_t *screen)
{
    (void)screen;
}

static void ui_txt_img_screen_restart(screen_t *screen)
{
    (void)screen;
}

static void ui_txt_img_screen_finish(screen_t *screen)
{
    (void)screen;
}

static void ui_txt_img_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if(lifecycle == NULL) {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
        .on_create = ui_txt_img_create_detail,
        .on_start = ui_txt_img_screen_start,
        .on_resume = ui_txt_img_screen_resume,
        .on_pause = ui_txt_img_screen_pause,
        .on_stop = ui_txt_img_screen_stop,
        .on_destroy = ui_txt_img_destroy_detail,
        .on_restart = ui_txt_img_screen_restart,
        .on_finish = ui_txt_img_screen_finish,
    };
}

static void ui_txt_img_push_new_screen(void)
{
    screen_lifecycle_t lifecycle;
    screen_t *screen;

    ui_txt_img_get_lifecycle(&lifecycle);
    screen = screen_register(NULL, &lifecycle);
    if(screen != NULL) {
        screen_push(screen, NULL);
    }
}

static void ui_txt_img_on_entry_click(lv_event_t *event)
{
    (void)event;

    ui_txt_img_push_new_screen();
}

void ui_txt_img_screen_create(lv_obj_t *screen, const char *path)
{
    if(screen == NULL) {
        return;
    }

    ui_image_show(screen, path);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, ui_txt_img_on_entry_click, LV_EVENT_CLICKED, NULL);
}
