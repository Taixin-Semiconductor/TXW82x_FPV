#include "ui_manager.h"
#include "screen_manager.h"
#include "osal/string.h"
#include "txt_img/txt_img_screen.h"
#include "calendar/calendar_screen.h"
#include "home/home_view.h"
#include "plan_list/plan_list_screen.h"
#include "settings/settings_screen.h"
#include "timing/timing_screen.h"
#include "ui_theme.h"
#include "ai_common.h"

typedef void (*ui_screen_create_fn_t)(lv_obj_t *screen, const char *path);

typedef struct
{
    const char           *image_path;
    ui_screen_create_fn_t create;
} ui_screen_config_t;

struct AI_UI_s
{
    screen_t *screen;
};
struct AI_UI_s *AI_UI;

static void ui_manager_create_root(screen_t *screen);
static void ui_manager_handle_tile_change(lv_event_t *event);
static void ui_manager_init_freetype_font(void);

extern void ui_ai_education_screen_create(lv_obj_t *parent, const char *path);
extern void ui_home_screen_create(lv_obj_t *parent, const char *path);
extern void settings_ui_create_entry(lv_obj_t *parent, const char *path);
/* tile页面配置信息 */
static const ui_screen_config_t tileview_configs[] = {
        {NULL, ui_home_screen_create},
        {AI_RES_PATH_PREFIX"jihuaqingdan.png", ui_plan_list_screen_create},
        {AI_RES_PATH_PREFIX"wenshengtu.png", ui_txt_img_screen_create},
        {AI_RES_PATH_PREFIX"aiduihua.png", ui_ai_education_screen_create},
        {AI_RES_PATH_PREFIX"miaobiao.png", ui_timing_screen_create},
        {AI_RES_PATH_PREFIX"rili.png", ui_calendar_screen_create},
        {AI_RES_PATH_PREFIX"shezhi.png", settings_ui_create_entry},
        {NULL, NULL},
};

/* 页面滑动切换 */
static void ui_manager_handle_tile_change(lv_event_t *event)
{
    (void) event;
}

static void ui_manager_root_create(screen_t *screen, void *params)
{
    (void) params;

    if (screen == NULL)
    {
        return;
    }

    screen_set_user_data(screen, params);
    struct AI_UI_s *ui_s = (struct AI_UI_s *) params;
    ui_manager_create_root(ui_s->screen);
}

static void ui_manager_root_start(screen_t *screen)
{
    (void) screen;
}

static void ui_manager_root_resume(screen_t *screen)
{
    (void) screen;
}

static void ui_manager_root_pause(screen_t *screen)
{
    (void) screen;
}

static void ui_manager_root_stop(screen_t *screen)
{
    (void) screen;
}

static void ui_manager_root_destroy(screen_t *screen)
{
}

static void ui_manager_root_restart(screen_t *screen)
{
    (void) screen;
}

static void ui_manager_root_finish(screen_t *screen)
{
    (void) screen;
}

static void ui_manager_root_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if (lifecycle == NULL)
    {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
            .on_create  = ui_manager_root_create,
            .on_start   = ui_manager_root_start,
            .on_resume  = ui_manager_root_resume,
            .on_pause   = ui_manager_root_pause,
            .on_stop    = ui_manager_root_stop,
            .on_destroy = ui_manager_root_destroy,
            .on_restart = ui_manager_root_restart,
            .on_finish  = ui_manager_root_finish,
    };
}

/* 创建根页面 */
static void ui_manager_create_root(screen_t *screen)
{
    lv_coord_t screen_width;
    lv_coord_t screen_height;
    lv_obj_t  *base_ui = screen->root;
    lv_obj_t  *tileview;

    screen_width  = lv_disp_get_hor_res(NULL);
    screen_height = lv_disp_get_ver_res(NULL);

    lv_obj_remove_style_all(base_ui);
    lv_obj_set_size(base_ui, screen_width, screen_height);
    lv_obj_set_style_bg_color(base_ui, lv_color_hex(UI_COLOR_BG_DARK), 0);
    lv_obj_set_style_bg_opa(base_ui, LV_OPA_COVER, 0);
    lv_obj_clear_flag(base_ui, LV_OBJ_FLAG_SCROLLABLE);

    tileview = lv_tileview_create(base_ui);
    lv_obj_set_size(tileview, screen_width, screen_height);
    lv_obj_center(tileview);
    lv_obj_set_style_bg_opa(tileview, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tileview, 0, 0);
    lv_obj_set_style_pad_all(tileview, 0, 0);

    lv_obj_set_style_anim_speed(tileview, 10, 0);

    lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_snap_x(tileview, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scroll_snap_y(tileview, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scroll_dir(tileview, LV_DIR_HOR);

    lv_obj_update_layout(base_ui);
    lv_obj_update_layout(tileview);

    const ui_screen_config_t *config    = tileview_configs;
    uint8_t                   screen_id = 0;
    while (config->create != NULL)
    {
        lv_obj_t *tile;
        tile = lv_tileview_add_tile(tileview, (uint8_t) screen_id, 0, LV_DIR_HOR);
        lv_obj_remove_style_all(tile);
        lv_obj_set_size(tile, screen_width, screen_height);
        lv_obj_set_pos(tile, screen_width * screen_id, 0);
        lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
        lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
        config->create(tile, (const char *) config->image_path);
        screen_id++;
        config++;
    }
    lv_obj_add_event_cb(tileview, ui_manager_handle_tile_change, LV_EVENT_VALUE_CHANGED, NULL);

    //ui_home_refresh_all();
}

/* UI初始化 */
void ui_manager_init(void)
{
    screen_lifecycle_t lifecycle;
    ui_theme_init();
    AI_UI = os_zalloc(sizeof(struct AI_UI_s));
    ui_manager_root_get_lifecycle(&lifecycle);
    AI_UI->screen = screen_register("ui_manager", &lifecycle);
    screen_push(AI_UI->screen, AI_UI);
}

/* tile 滑动开关 */
void ui_manager_set_tile_swipe_enabled(bool enabled)
{
}

/* 加载音乐页面 */
void ui_manager_load_music(void)
{
}

/* 加载菜单页面 */
void ui_manager_load_menu(void)
{
}
