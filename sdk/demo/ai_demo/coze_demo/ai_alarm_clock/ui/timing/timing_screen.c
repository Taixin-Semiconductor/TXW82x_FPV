#include "ui_manager.h"

#include "screen_manager.h"
#include "timing_screen.h"

#include <stdint.h>

#include "../ui_theme.h"
#include "osal/string.h"

#define UI_TIMING_TAB_COUNT    3
#define UI_TIMING_PRESET_COUNT 9

typedef enum
{
    UI_TIMING_MODE_COUNTDOWN = 0,
    UI_TIMING_MODE_STOPWATCH,
    UI_TIMING_MODE_POMODORO
} ui_timing_mode_t;

typedef struct
{
    const char *title;
    const char *accent;
} ui_timing_tab_config_t;

typedef struct
{
    const char *main_text;
    const char *unit_text;
} ui_timing_preset_config_t;

typedef struct ui_timing_state ui_timing_state_t;

typedef struct
{
    ui_timing_state_t *state;
    uint8_t index;
} ui_timing_index_event_t;

struct ui_timing_state
{
    screen_t *screen;
    lv_obj_t *detail_page;
    lv_obj_t *tab_buttons[UI_TIMING_TAB_COUNT];
    lv_obj_t *preset_buttons[UI_TIMING_PRESET_COUNT];
    ui_timing_index_event_t tab_events[UI_TIMING_TAB_COUNT];
    ui_timing_index_event_t preset_events[UI_TIMING_PRESET_COUNT];
    ui_timing_mode_t selected_mode;
    uint8_t selected_preset_index;
};

static void ui_timing_create_detail(screen_t *screen, void *params);
static void ui_timing_destroy_detail(screen_t *screen);
static void ui_timing_get_lifecycle(screen_lifecycle_t *lifecycle);
static void ui_timing_push_new_screen(void);
static void ui_timing_on_entry_click(lv_event_t *event);
static void ui_timing_handle_back_click(lv_event_t *event);

static void ui_timing_refresh_tab_styles(ui_timing_state_t *state)
{
    uint8_t i;

    if(state == NULL) {
        return;
    }

    for(i = 0; i < UI_TIMING_TAB_COUNT; i++) {
        bool is_selected = (i == (uint8_t)state->selected_mode);
        lv_obj_t *btn = state->tab_buttons[i];

        if(btn == NULL) {
            continue;
        }

        lv_obj_set_style_bg_color(btn, is_selected ? lv_color_hex(UI_COLOR_PRIMARY) : lv_color_hex(0xf4f6fb), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(btn, is_selected ? lv_color_hex(UI_COLOR_PRIMARY_DARK) : lv_color_hex(UI_COLOR_TEXT_DARK), 0);
    }
}

static void ui_timing_refresh_preset_styles(ui_timing_state_t *state)
{
    uint8_t i;

    if(state == NULL) {
        return;
    }

    for(i = 0; i < UI_TIMING_PRESET_COUNT; i++) {
        bool is_selected = (i == state->selected_preset_index);
        lv_obj_t *btn = state->preset_buttons[i];

        if(btn == NULL) {
            continue;
        }

        lv_obj_set_style_bg_color(btn, is_selected ? lv_color_hex(0xfff3bf) : lv_color_hex(UI_COLOR_BG_CARD), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(btn, is_selected ? lv_color_hex(UI_COLOR_PRIMARY) : lv_color_hex(UI_COLOR_BORDER), 0);
        lv_obj_set_style_border_width(btn, is_selected ? 2 : 1, 0);
        lv_obj_set_style_text_color(btn, lv_color_hex(UI_COLOR_TEXT_DARK), 0);
    }
}

static void ui_timing_handle_back_click(lv_event_t *event)
{
    ui_timing_state_t *state = (ui_timing_state_t *)lv_event_get_user_data(event);

    if(state == NULL || state->screen == NULL) {
        return;
    }

    screen_finish(state->screen);
}

static void ui_timing_handle_tab_click(lv_event_t *event)
{
    ui_timing_index_event_t *index_event = (ui_timing_index_event_t *)lv_event_get_user_data(event);

    if(index_event == NULL || index_event->state == NULL || index_event->index >= UI_TIMING_TAB_COUNT) {
        return;
    }

    index_event->state->selected_mode = (ui_timing_mode_t)index_event->index;
    ui_timing_refresh_tab_styles(index_event->state);
}

static void ui_timing_handle_preset_click(lv_event_t *event)
{
    ui_timing_index_event_t *index_event = (ui_timing_index_event_t *)lv_event_get_user_data(event);

    if(index_event == NULL || index_event->state == NULL || index_event->index >= UI_TIMING_PRESET_COUNT) {
        return;
    }

    index_event->state->selected_preset_index = index_event->index;
    ui_timing_refresh_preset_styles(index_event->state);
}

static void ui_timing_create_tabs(lv_obj_t *header_row, ui_timing_state_t *state)
{
    const ui_timing_tab_config_t tab_configs[UI_TIMING_TAB_COUNT] = {
        {"Countdown", "CD"},
        {"Stopwatch", "SW"},
        {"Pomodoro", "PM"}
    };
    const lv_coord_t tab_x_positions[UI_TIMING_TAB_COUNT] = {36, 122, 208};
    uint8_t i;

    if(state == NULL) {
        return;
    }

    for(i = 0; i < UI_TIMING_TAB_COUNT; i++) {
        lv_obj_t *tab_btn;
        lv_obj_t *accent_label;
        lv_obj_t *title_label;

        tab_btn = lv_btn_create(header_row);
        if(tab_btn == NULL) {
            continue;
        }

        lv_obj_set_size(tab_btn, 80, 44);
        lv_obj_set_pos(tab_btn, tab_x_positions[i], 0);
        lv_obj_set_style_radius(tab_btn, 10, 0);
        lv_obj_set_style_border_width(tab_btn, 0, 0);
        lv_obj_set_style_shadow_width(tab_btn, 0, 0);
        lv_obj_set_style_pad_all(tab_btn, 0, 0);
        lv_obj_clear_flag(tab_btn, LV_OBJ_FLAG_SCROLLABLE);
        state->tab_events[i].state = state;
        state->tab_events[i].index = i;
        lv_obj_add_event_cb(tab_btn, ui_timing_handle_tab_click, LV_EVENT_CLICKED, &state->tab_events[i]);

        accent_label = lv_label_create(tab_btn);
        if(accent_label != NULL) {
            lv_label_set_text(accent_label, tab_configs[i].accent);
            lv_obj_set_style_text_font(accent_label, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(accent_label, lv_color_hex(0x56637a), 0);
            lv_obj_align(accent_label, LV_ALIGN_TOP_LEFT, 8, 8);
        }

        title_label = lv_label_create(tab_btn);
        if(title_label != NULL) {
            lv_label_set_text(title_label, tab_configs[i].title);
            lv_obj_set_style_text_font(title_label, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(title_label, lv_color_hex(UI_COLOR_TEXT_DARK), 0);
            lv_obj_align(title_label, LV_ALIGN_BOTTOM_LEFT, 8, -8);
        }

        state->tab_buttons[i] = tab_btn;
    }
}

static void ui_timing_create_presets(lv_obj_t *content_panel, ui_timing_state_t *state)
{
    const ui_timing_preset_config_t preset_configs[UI_TIMING_PRESET_COUNT] = {
        {"1", "min"},
        {"3", "min"},
        {"5", "min"},
        {"10", "min"},
        {"15", "min"},
        {"20", "min"},
        {"25", "min"},
        {"30", "min"},
        {"More", ""}
    };
    const lv_coord_t x_positions[3] = {10, 108, 206};
    const lv_coord_t y_positions[3] = {10, 64, 118};
    uint8_t i;

    if(state == NULL) {
        return;
    }

    for(i = 0; i < UI_TIMING_PRESET_COUNT; i++) {
        uint8_t row = i / 3;
        uint8_t col = i % 3;
        lv_obj_t *preset_btn;
        lv_obj_t *main_label;
        lv_obj_t *unit_label;

        preset_btn = lv_btn_create(content_panel);
        if(preset_btn == NULL) {
            continue;
        }

        lv_obj_set_size(preset_btn, 86, 44);
        lv_obj_set_pos(preset_btn, x_positions[col], y_positions[row]);
        lv_obj_set_style_radius(preset_btn, 15, 0);
        lv_obj_set_style_shadow_width(preset_btn, 0, 0);
        lv_obj_set_style_pad_all(preset_btn, 0, 0);
        lv_obj_clear_flag(preset_btn, LV_OBJ_FLAG_SCROLLABLE);
        state->preset_events[i].state = state;
        state->preset_events[i].index = i;
        lv_obj_add_event_cb(preset_btn, ui_timing_handle_preset_click, LV_EVENT_CLICKED, &state->preset_events[i]);

        main_label = lv_label_create(preset_btn);
        if(main_label == NULL) {
            continue;
        }

        lv_label_set_text(main_label, preset_configs[i].main_text);
        lv_obj_set_style_text_font(main_label,
                                   i == (UI_TIMING_PRESET_COUNT - 1) ? UI_FONT_BODY : UI_FONT_BODY, 0);
        lv_obj_set_style_text_color(main_label, lv_color_hex(UI_COLOR_TEXT_DARK), 0);

        if(preset_configs[i].unit_text[0] == '\0') {
            lv_obj_center(main_label);
        } else {
            unit_label = lv_label_create(preset_btn);
            if(unit_label != NULL) {
                lv_label_set_text(unit_label, preset_configs[i].unit_text);
                lv_obj_set_style_text_font(unit_label, UI_FONT_BODY, 0);
                lv_obj_set_style_text_color(unit_label, lv_color_hex(0x5d6880), 0);
                lv_obj_align(main_label, LV_ALIGN_CENTER, -10, -1);
                lv_obj_align_to(unit_label, main_label, LV_ALIGN_OUT_RIGHT_BOTTOM, 2, -1);
            }
        }

        state->preset_buttons[i] = preset_btn;
    }
}

static void ui_timing_create_detail(screen_t *screen, void *params)
{
    ui_timing_state_t *state;
    lv_obj_t *page;
    lv_obj_t *header_row;
    lv_obj_t *content_panel;

    (void)params;

    if(screen == NULL) {
        return;
    }

    state = (ui_timing_state_t *)os_malloc(sizeof(ui_timing_state_t));
    if(state == NULL) {
        return;
    }
    lv_memset(state, 0, sizeof(ui_timing_state_t));
    state->screen = screen;
    state->selected_mode = UI_TIMING_MODE_COUNTDOWN;
    state->selected_preset_index = 1;
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
    lv_obj_set_style_bg_color(page, lv_color_hex(UI_COLOR_BLUE_DEEP), 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    state->detail_page = page;

    header_row = lv_obj_create(page);
    if(header_row != NULL) {
        lv_obj_remove_style_all(header_row);
        lv_obj_set_size(header_row, LV_PCT(96), 44);
        lv_obj_align(header_row, LV_ALIGN_TOP_MID, 0, 10);

        lv_obj_t *back_btn = lv_btn_create(header_row);
        if(back_btn != NULL) {
            lv_obj_remove_style_all(back_btn);
            lv_obj_set_size(back_btn, 30, 30);
            lv_obj_set_style_radius(back_btn, 15, 0);
            lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(back_btn, 0, 0);
            lv_obj_set_style_shadow_width(back_btn, 0, 0);
            lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 2, 0);
            lv_obj_add_event_cb(back_btn, ui_timing_handle_back_click, LV_EVENT_CLICKED, state);

            lv_obj_t *back_label = lv_label_create(back_btn);
            if(back_label != NULL) {
                lv_label_set_text(back_label, LV_SYMBOL_LEFT);
                lv_obj_set_style_text_font(back_label, UI_FONT_BODY, 0);
                lv_obj_set_style_text_color(back_label, lv_color_hex(UI_COLOR_BG_CARD), 0);
                lv_obj_center(back_label);
            }
        }

        ui_timing_create_tabs(header_row, state);
    }

    content_panel = lv_obj_create(page);
    if(content_panel != NULL) {
        lv_obj_set_size(content_panel, LV_PCT(96), 175);
        lv_obj_align(content_panel, LV_ALIGN_TOP_MID, 0, 58);
        lv_obj_set_style_bg_color(content_panel, lv_color_hex(UI_COLOR_BG_LIGHT), 0);
        lv_obj_set_style_bg_opa(content_panel, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(content_panel, 15, 0);
        lv_obj_set_style_border_width(content_panel, 0, 0);
        lv_obj_set_style_shadow_width(content_panel, 0, 0);
        lv_obj_set_style_pad_all(content_panel, 0, 0);
        lv_obj_clear_flag(content_panel, LV_OBJ_FLAG_SCROLLABLE);

        ui_timing_create_presets(content_panel, state);
    }

    ui_timing_refresh_tab_styles(state);
    ui_timing_refresh_preset_styles(state);
}

static void ui_timing_destroy_detail(screen_t *screen)
{
    ui_timing_state_t *state;

    if(screen == NULL) {
        return;
    }

    state = (ui_timing_state_t *)screen_get_user_data(screen);
    if(state == NULL) {
        return;
    }

    lv_memset(state->tab_buttons, 0, sizeof(state->tab_buttons));
    lv_memset(state->preset_buttons, 0, sizeof(state->preset_buttons));
    lv_memset(state->tab_events, 0, sizeof(state->tab_events));
    lv_memset(state->preset_events, 0, sizeof(state->preset_events));
    state->detail_page = NULL;
    state->screen = NULL;
    screen_set_user_data(screen, NULL);
    os_free(state);
}

static void ui_timing_screen_start(screen_t *screen)
{
    (void)screen;
}

static void ui_timing_screen_resume(screen_t *screen)
{
    (void)screen;
}

static void ui_timing_screen_pause(screen_t *screen)
{
    (void)screen;
}

static void ui_timing_screen_stop(screen_t *screen)
{
    (void)screen;
}

static void ui_timing_screen_restart(screen_t *screen)
{
    (void)screen;
}

static void ui_timing_screen_finish(screen_t *screen)
{
    (void)screen;
}

static void ui_timing_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if(lifecycle == NULL) {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
        .on_create = ui_timing_create_detail,
        .on_start = ui_timing_screen_start,
        .on_resume = ui_timing_screen_resume,
        .on_pause = ui_timing_screen_pause,
        .on_stop = ui_timing_screen_stop,
        .on_destroy = ui_timing_destroy_detail,
        .on_restart = ui_timing_screen_restart,
        .on_finish = ui_timing_screen_finish,
    };
}

static void ui_timing_push_new_screen(void)
{
    screen_lifecycle_t lifecycle;
    screen_t *screen;

    ui_timing_get_lifecycle(&lifecycle);
    screen = screen_register(NULL, &lifecycle);
    if(screen != NULL) {
        screen_push(screen, NULL);
    }
}

static void ui_timing_on_entry_click(lv_event_t *event)
{
    (void)event;

    ui_timing_push_new_screen();
}

void ui_timing_screen_create(lv_obj_t *screen, const char *path)
{
    if(screen == NULL) {
        return;
    }

    ui_image_show(screen, path);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, ui_timing_on_entry_click, LV_EVENT_CLICKED, NULL);
}
