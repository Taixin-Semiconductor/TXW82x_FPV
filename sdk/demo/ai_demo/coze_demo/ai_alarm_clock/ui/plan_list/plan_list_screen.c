#include "ui_manager.h"

#include "screen_manager.h"
#include "plan_list_screen.h"

#include <stdint.h>

#include "../ui_manager.h"
#include "../ui_theme.h"
#include "osal/string.h"

#define PLAN_LIST_DATE_ITEM_COUNT 20

typedef struct ui_plan_list_state ui_plan_list_state_t;

typedef struct
{
    int year;
    int month;
    int day;
    int weekday;
} ui_plan_list_date_item_t;

typedef struct
{
    ui_plan_list_state_t *state;
    uint8_t index;
} ui_plan_list_date_event_t;

struct ui_plan_list_state
{
    screen_t *screen;
    lv_obj_t *detail_page;
    lv_obj_t *date_items[PLAN_LIST_DATE_ITEM_COUNT];
    ui_plan_list_date_item_t dates[PLAN_LIST_DATE_ITEM_COUNT];
    ui_plan_list_date_event_t date_events[PLAN_LIST_DATE_ITEM_COUNT];
    uint8_t selected_index;
};

static void ui_plan_list_create_detail(screen_t *screen, void *params);
static void ui_plan_list_destroy_detail(screen_t *screen);

static void ui_plan_list_on_back_click(lv_event_t *event);

static void ui_plan_list_push_new_screen(void);
static void ui_plan_list_on_entry_click(lv_event_t *event);
static void ui_plan_list_get_lifecycle(screen_lifecycle_t *lifecycle);

/* ── Date calculation helpers ── */

static bool ui_plan_list_is_leap_year(int year)
{
    return ((year % 4) == 0 && (year % 100) != 0) || ((year % 400) == 0);
}

static int ui_plan_list_get_days_in_month(int year, int month)
{
    const int month_days[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    if(month < 1 || month > 12) {
        return 31;
    }

    if(month == 2 && ui_plan_list_is_leap_year(year)) {
        return 29;
    }

    return month_days[month - 1];
}

static void ui_plan_list_add_days(ui_plan_list_date_item_t *item, int delta_days)
{
    int remaining = delta_days;

    while(remaining > 0) {
        int days_in_month = ui_plan_list_get_days_in_month(item->year, item->month);

        item->day++;
        item->weekday++;

        if(item->day > days_in_month) {
            item->day = 1;
            item->month++;

            if(item->month > 12) {
                item->month = 1;
                item->year++;
            }
        }

        if(item->weekday > 7) {
            item->weekday = 1;
        }

        remaining--;
    }
}

static void ui_plan_list_build_dates(ui_plan_list_state_t *state)
{
    uint8_t i;

    if(state == NULL) {
        return;
    }

    for(i = 0; i < PLAN_LIST_DATE_ITEM_COUNT; i++) {
        state->dates[i].year = 2025;
        state->dates[i].month = 6;
        state->dates[i].day = 15;
        state->dates[i].weekday = 7;
        ui_plan_list_add_days(&state->dates[i], i);
    }
    state->selected_index = 0;
}

/* ── Style refresh ── */

static void ui_plan_list_refresh_date_item_style(ui_plan_list_state_t *state, uint8_t index)
{
    lv_obj_t *item_obj;
    bool is_selected;

    if(state == NULL) {
        return;
    }

    if(index >= PLAN_LIST_DATE_ITEM_COUNT) {
        return;
    }

    item_obj = state->date_items[index];
    if(item_obj == NULL) {
        return;
    }

    is_selected = (index == state->selected_index);
    lv_obj_set_style_bg_color(item_obj, is_selected ? lv_color_hex(UI_COLOR_PRIMARY) : lv_color_hex(UI_COLOR_BG_CARD), 0);
    lv_obj_set_style_text_color(item_obj, is_selected ? lv_color_hex(UI_COLOR_PRIMARY_DARK) : lv_color_hex(0x2c3444), 0);
}

static void ui_plan_list_refresh_date_selection(ui_plan_list_state_t *state)
{
    uint8_t i;

    if(state == NULL) {
        return;
    }

    for(i = 0; i < PLAN_LIST_DATE_ITEM_COUNT; i++) {
        ui_plan_list_refresh_date_item_style(state, i);
    }
}

/* ── Callbacks ── */

static void ui_plan_list_on_back_click(lv_event_t *event)
{
    ui_plan_list_state_t *state = (ui_plan_list_state_t *)lv_event_get_user_data(event);

    if(state == NULL || state->screen == NULL) {
        return;
    }

    screen_finish(state->screen);
}

static void ui_plan_list_on_date_item_click(lv_event_t *event)
{
    ui_plan_list_date_event_t *date_event = (ui_plan_list_date_event_t *)lv_event_get_user_data(event);
    ui_plan_list_state_t *state;

    if(date_event == NULL || date_event->state == NULL) {
        return;
    }

    if(date_event->index >= PLAN_LIST_DATE_ITEM_COUNT) {
        return;
    }

    state = date_event->state;
    state->selected_index = date_event->index;
    ui_plan_list_refresh_date_selection(state);
}

/* ── Date strip item creation ── */

static lv_obj_t *ui_plan_list_create_date_item(lv_obj_t *parent, ui_plan_list_state_t *state, uint8_t index)
{
    const char * const weekday_names[] = {
        "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
    };
    lv_obj_t *item_obj;
    lv_obj_t *weekday_label;
    lv_obj_t *day_label;
    char day_text[8];
    const ui_plan_list_date_item_t *date_item;

    if(state == NULL || index >= PLAN_LIST_DATE_ITEM_COUNT) {
        return NULL;
    }

    date_item = &state->dates[index];

    item_obj = lv_btn_create(parent);
    if(item_obj == NULL) {
        return NULL;
    }

    lv_obj_set_size(item_obj, 40, 45);
    lv_obj_set_style_radius(item_obj, UI_CORNER_RADIUS, 0);
    lv_obj_set_style_border_width(item_obj, 0, 0);
    lv_obj_set_style_shadow_width(item_obj, 0, 0);
    lv_obj_set_style_pad_all(item_obj, 0, 0);
    state->date_events[index].state = state;
    state->date_events[index].index = index;
    lv_obj_add_event_cb(item_obj, ui_plan_list_on_date_item_click, LV_EVENT_CLICKED, &state->date_events[index]);

    weekday_label = lv_label_create(item_obj);
    if(weekday_label != NULL) {
        lv_label_set_text(weekday_label, weekday_names[date_item->weekday - 1]);
        lv_obj_set_style_text_font(weekday_label, UI_FONT_BODY, 0);
        lv_obj_set_style_text_opa(weekday_label, LV_OPA_70, 0);
        lv_obj_align(weekday_label, LV_ALIGN_TOP_MID, 0, 7);
    }

    day_label = lv_label_create(item_obj);
    if(day_label != NULL) {
        lv_snprintf(day_text, sizeof(day_text), "%02d", date_item->day);
        lv_label_set_text(day_label, day_text);
        lv_obj_set_style_text_font(day_label, UI_FONT_BODY, 0);
        lv_obj_align(day_label, LV_ALIGN_CENTER, 0, 4);
    }

    return item_obj;
}

/* ── Detail page creation ── */

static void ui_plan_list_create_detail(screen_t *screen, void *params)
{
    lv_obj_t *parent;
    lv_obj_t *page;
    lv_obj_t *header_panel;
    lv_obj_t *date_strip_panel;
    lv_obj_t *date_strip;
    lv_obj_t *plan_panel;
    lv_obj_t *empty_title;
    lv_obj_t *empty_desc;
    lv_obj_t *exit_button;
    lv_obj_t *exit_label;
    ui_plan_list_state_t *state;
    uint8_t i;

    (void)params;

    if(screen == NULL) {
        return;
    }

    state = (ui_plan_list_state_t *)os_malloc(sizeof(ui_plan_list_state_t));
    if(state == NULL) {
        return;
    }
    lv_memset(state, 0, sizeof(ui_plan_list_state_t));

    state->screen = screen;
    screen_set_user_data(screen, state);

    parent = screen->root;
    if(parent == NULL) {
        return;
    }

    lv_obj_remove_style_all(parent);
    lv_obj_set_size(parent, LV_PCT(100), LV_PCT(100));
    lv_obj_center(parent);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    parent->user_data = state;

    page = lv_obj_create(parent);
    if(page == NULL) {
        return;
    }

    lv_obj_remove_style_all(page);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_center(page);
    lv_obj_set_style_bg_color(page, lv_color_hex(UI_COLOR_BLUE), 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    state->detail_page = page;

    ui_plan_list_build_dates(state);

    /* Header with date strip */
    header_panel = lv_obj_create(page);
    if(header_panel != NULL) {
        lv_obj_remove_style_all(header_panel);
        lv_obj_set_size(header_panel, LV_PCT(100), 52);
        lv_obj_align(header_panel, LV_ALIGN_TOP_MID, 0, 8);
        lv_obj_set_style_bg_opa(header_panel, LV_OPA_TRANSP, 0);

        date_strip_panel = lv_obj_create(header_panel);
        if(date_strip_panel != NULL) {
            lv_obj_remove_style_all(date_strip_panel);
            lv_obj_set_size(date_strip_panel, LV_PCT(100), 52);
            lv_obj_align(date_strip_panel, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_style_bg_opa(date_strip_panel, LV_OPA_TRANSP, 0);

            date_strip = lv_obj_create(date_strip_panel);
            if(date_strip != NULL) {
                lv_obj_remove_style_all(date_strip);
                lv_obj_set_size(date_strip, LV_PCT(100), 52);
                lv_obj_align(date_strip, LV_ALIGN_CENTER, 0, 0);
                lv_obj_set_flex_flow(date_strip, LV_FLEX_FLOW_ROW);
                lv_obj_set_scroll_dir(date_strip, LV_DIR_HOR);
                lv_obj_set_scrollbar_mode(date_strip, LV_SCROLLBAR_MODE_OFF);
                lv_obj_set_style_pad_left(date_strip, 12, 0);
                lv_obj_set_style_pad_right(date_strip, 12, 0);
                lv_obj_set_style_pad_row(date_strip, 0, 0);
                lv_obj_set_style_pad_column(date_strip, 10, 0);
                lv_obj_set_style_bg_opa(date_strip, LV_OPA_TRANSP, 0);

                for(i = 0; i < PLAN_LIST_DATE_ITEM_COUNT; i++) {
                    state->date_items[i] = ui_plan_list_create_date_item(date_strip, state, i);
                }
            }
        }
    }

    /* Plan content panel */
    plan_panel = lv_obj_create(page);
    if(plan_panel != NULL) {
        lv_obj_set_size(plan_panel, 304, 170);
        lv_obj_align(plan_panel, LV_ALIGN_TOP_MID, 0, 60);
        lv_obj_set_style_radius(plan_panel, 24, 0);
        lv_obj_set_style_bg_color(plan_panel, lv_color_hex(UI_COLOR_BG_LIGHT), 0);
        lv_obj_set_style_bg_opa(plan_panel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(plan_panel, 0, 0);
        lv_obj_set_style_shadow_width(plan_panel, 0, 0);
        lv_obj_clear_flag(plan_panel, LV_OBJ_FLAG_SCROLLABLE);

        empty_title = lv_label_create(plan_panel);
        if(empty_title != NULL) {
            lv_label_set_text(empty_title, "No plans yet");
            lv_obj_set_style_text_font(empty_title, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(empty_title, lv_color_hex(0x273248), 0);
            lv_obj_align(empty_title, LV_ALIGN_CENTER, 0, -12);
        }

        empty_desc = lv_label_create(plan_panel);
        if(empty_desc != NULL) {
            lv_label_set_text(empty_desc, "Select a date above and start creating items.");
            lv_obj_set_width(empty_desc, 220);
            lv_label_set_long_mode(empty_desc, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_font(empty_desc, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(empty_desc, lv_color_hex(0x6a7485), 0);
            lv_obj_set_style_text_align(empty_desc, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_align(empty_desc, LV_ALIGN_CENTER, 0, 18);
        }
    }

    /* Exit button (bottom right) */
    exit_button = lv_obj_create(page);
    if(exit_button != NULL) {
        lv_obj_remove_style_all(exit_button);
        lv_obj_set_size(exit_button, 34, 34);
        lv_obj_set_style_radius(exit_button, 27, 0);
        lv_obj_set_style_bg_color(exit_button, lv_color_hex(UI_COLOR_PRIMARY), 0);
        lv_obj_set_style_bg_opa(exit_button, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(exit_button, 0, 0);
        lv_obj_set_style_shadow_width(exit_button, 0, 0);
        lv_obj_add_flag(exit_button, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_align(exit_button, LV_ALIGN_BOTTOM_RIGHT, -14, -14);
        lv_obj_add_event_cb(exit_button, ui_plan_list_on_back_click, LV_EVENT_CLICKED, state);

        exit_label = lv_label_create(exit_button);
        if(exit_label != NULL) {
            lv_label_set_text(exit_label, "exit");
            lv_obj_set_style_text_font(exit_label, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(exit_label, lv_color_hex(UI_COLOR_PRIMARY_DARK), 0);
            lv_obj_center(exit_label);
        }
    }

    ui_plan_list_refresh_date_selection(state);
}

static void ui_plan_list_destroy_detail(screen_t *screen)
{
    ui_plan_list_state_t *state;

    if(screen == NULL) {
        return;
    }

    state = (ui_plan_list_state_t *)screen_get_user_data(screen);
    if(state == NULL) {
        return;
    }

    lv_memset(state->date_items, 0, sizeof(state->date_items));
    lv_memset(state->date_events, 0, sizeof(state->date_events));
    state->detail_page = NULL;
    if(screen->root != NULL && screen->root->user_data == state) {
        screen->root->user_data = NULL;
    }
    state->screen = NULL;
    screen_set_user_data(screen, NULL);
    os_free(state);
}

static void ui_plan_list_screen_start(screen_t *screen)
{
    (void)screen;
}

static void ui_plan_list_screen_resume(screen_t *screen)
{
    (void)screen;
}

static void ui_plan_list_screen_pause(screen_t *screen)
{
    (void)screen;
}

static void ui_plan_list_screen_stop(screen_t *screen)
{
    (void)screen;
}

static void ui_plan_list_screen_restart(screen_t *screen)
{
    (void)screen;
}

static void ui_plan_list_screen_finish(screen_t *screen)
{
    (void)screen;
}

static void ui_plan_list_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if(lifecycle == NULL) {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
        .on_create = ui_plan_list_create_detail,
        .on_start = ui_plan_list_screen_start,
        .on_resume = ui_plan_list_screen_resume,
        .on_pause = ui_plan_list_screen_pause,
        .on_stop = ui_plan_list_screen_stop,
        .on_destroy = ui_plan_list_destroy_detail,
        .on_restart = ui_plan_list_screen_restart,
        .on_finish = ui_plan_list_screen_finish,
    };
}

static void ui_plan_list_push_new_screen(void)
{
    screen_lifecycle_t lifecycle;
    screen_t *screen;

    ui_plan_list_get_lifecycle(&lifecycle);
    screen = screen_register(NULL, &lifecycle);

    if(screen != NULL) {
        screen_push(screen, NULL);
    }
}

static void ui_plan_list_on_entry_click(lv_event_t *event)
{
    (void)event;

    ui_plan_list_push_new_screen();
}

/* ── Public API ── */

void ui_plan_list_screen_create(lv_obj_t *screen,const char *path)
{
    if(screen == NULL) {
        return;
    }

    ui_image_show(screen,path);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, ui_plan_list_on_entry_click, LV_EVENT_CLICKED, NULL);
}
