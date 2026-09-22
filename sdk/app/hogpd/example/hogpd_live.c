#include "hogpd_live.h"
#include "hogpd.h"
#include "basic_include.h"
#include <stdio.h>
#include <string.h>
static int32 pd_live_clamp_s32(int32 v,int32 lo,int32 hi){if(v<lo)return lo;if(v>hi)return hi;return v;}

static void pd_live_draw_rect_y(uint8 *y_plane, int32 width, int32 height,
                                const hogpd_box_t *box, uint8 value, int32 thickness)
{
    int32 x0 = pd_live_clamp_s32(box->x, 0, width - 1);
    int32 y0 = pd_live_clamp_s32(box->y, 0, height - 1);
    int32 x1 = pd_live_clamp_s32(box->x + box->width - 1, 0, width - 1);
    int32 y1 = pd_live_clamp_s32(box->y + box->height - 1, 0, height - 1);

    if (x1 <= x0 || y1 <= y0) {
        return;
    }

    for (int32 t = 0; t < thickness; t++) {
        int32 yt = y0 + t;
        int32 yb = y1 - t;
        if (yt >= 0 && yt < height) {
            memset(y_plane + yt * width + x0, value, (size_t)(x1 - x0 + 1));
        }
        if (yb >= 0 && yb < height) {
            memset(y_plane + yb * width + x0, value, (size_t)(x1 - x0 + 1));
        }
        for (int32 y = y0; y <= y1; y++) {
            int32 xl = x0 + t;
            int32 xr = x1 - t;
            if (xl >= 0 && xl < width) y_plane[y * width + xl] = value;
            if (xr >= 0 && xr < width) y_plane[y * width + xr] = value;
        }
    }
}

static void pd_live_draw_cross_y(uint8 *y_plane, int32 width, int32 height,
                                 const hogpd_box_t *box, uint8 value);

static void pd_live_set_uv420(uint8 *yuv420p, int32 width, int32 height,
                              int32 x, int32 y, uint8 u_value, uint8 v_value)
{
    int32 uv_w;
    int32 uv_h;
    int32 uv_x;
    int32 uv_y;
    uint8 *u_plane;
    uint8 *v_plane;

    if (!yuv420p || width < 2 || height < 2 || x < 0 || y < 0 ||
        x >= width || y >= height) {
        return;
    }

    uv_w = width >> 1;
    uv_h = height >> 1;
    uv_x = x >> 1;
    uv_y = y >> 1;
    if (uv_x < 0 || uv_y < 0 || uv_x >= uv_w || uv_y >= uv_h) {
        return;
    }

    u_plane = yuv420p + width * height;
    v_plane = u_plane + uv_w * uv_h;
    u_plane[uv_y * uv_w + uv_x] = u_value;
    v_plane[uv_y * uv_w + uv_x] = v_value;
}

static void pd_live_uv_cache_clean(uint8 *yuv420p,
                                   int32 width, int32 height)
{
    int32 y_bytes;
    int32 uv_bytes;
    if (!yuv420p || width < 2 || height < 2) return;
    y_bytes = width * height;
    uv_bytes = (width >> 1) * (height >> 1) * 2;
    sys_dcache_clean_range((uint32 *)(yuv420p + y_bytes), uv_bytes);
}

static void pd_live_draw_rect_yuv420p(uint8 *yuv420p, int32 width, int32 height,
                                      const hogpd_box_t *box,
                                      uint8 y_value, uint8 u_value, uint8 v_value,
                                      int32 thickness)
{
    int32 x0;
    int32 y0;
    int32 x1;
    int32 y1;

    if (!yuv420p || !box || thickness <= 0) {
        return;
    }

    x0 = pd_live_clamp_s32(box->x, 0, width - 1);
    y0 = pd_live_clamp_s32(box->y, 0, height - 1);
    x1 = pd_live_clamp_s32(box->x + box->width - 1, 0, width - 1);
    y1 = pd_live_clamp_s32(box->y + box->height - 1, 0, height - 1);
    if (x1 <= x0 || y1 <= y0) {
        return;
    }

    pd_live_draw_rect_y(yuv420p, width, height, box, y_value, thickness);
    for (int32 t = 0; t < thickness; t++) {
        int32 yt = y0 + t;
        int32 yb = y1 - t;
        for (int32 x = x0; x <= x1; x++) {
            pd_live_set_uv420(yuv420p, width, height, x, yt, u_value, v_value);
            pd_live_set_uv420(yuv420p, width, height, x, yb, u_value, v_value);
        }
        for (int32 y = y0; y <= y1; y++) {
            pd_live_set_uv420(yuv420p, width, height, x0 + t, y, u_value, v_value);
            pd_live_set_uv420(yuv420p, width, height, x1 - t, y, u_value, v_value);
        }
    }
}

static void pd_live_draw_cross_yuv420p(uint8 *yuv420p, int32 width, int32 height,
                                       const hogpd_box_t *box,
                                       uint8 y_value, uint8 u_value, uint8 v_value)
{
    int32 cx;
    int32 cy;
    int32 span;

    if (!yuv420p || !box) {
        return;
    }

    pd_live_draw_cross_y(yuv420p, width, height, box, y_value);
    cx = pd_live_clamp_s32(box->x + box->width / 2, 0, width - 1);
    cy = pd_live_clamp_s32(box->y + box->height / 2, 0, height - 1);
    span = box->width / 8;
    if (span < 8) span = 8;
    if (span > 28) span = 28;

    for (int32 x = cx - span; x <= cx + span; x++) {
        pd_live_set_uv420(yuv420p, width, height, x, cy, u_value, v_value);
    }
    for (int32 y = cy - span; y <= cy + span; y++) {
        pd_live_set_uv420(yuv420p, width, height, cx, y, u_value, v_value);
    }
}
static void pd_live_draw_cross_y(uint8 *y_plane, int32 width, int32 height,
                                 const hogpd_box_t *box, uint8 value)
{
    int32 cx = pd_live_clamp_s32(box->x + box->width / 2, 0, width - 1);
    int32 cy = pd_live_clamp_s32(box->y + box->height / 2, 0, height - 1);
    int32 span = box->width / 8;
    if (span < 8) span = 8;
    if (span > 28) span = 28;

    for (int32 x = cx - span; x <= cx + span; x++) {
        if (x >= 0 && x < width) {
            y_plane[cy * width + x] = value;
        }
    }
    for (int32 y = cy - span; y <= cy + span; y++) {
        if (y >= 0 && y < height) {
            y_plane[y * width + cx] = value;
        }
    }
}

static const uint8 s_pd_live_font5x7[][7] = {
    {0x1e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e}, /* D */
    {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f}, /* E */
    {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10}, /* F */
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}, /* N */
    {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}, /* O */
    {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, /* T */
    {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e},
    {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11},
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
    {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11},
    {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10},
    {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11},
    {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04},
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0a},
    {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e},
    {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e},
    {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f},
    {0x1e, 0x01, 0x01, 0x0e, 0x01, 0x01, 0x1e},
    {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02},
    {0x1f, 0x10, 0x10, 0x1e, 0x01, 0x01, 0x1e},
    {0x0e, 0x10, 0x10, 0x1e, 0x11, 0x11, 0x0e},
    {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e},
    {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x01, 0x0e},
    {0x0f, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0f}, /* C */
    {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00}, /* - */
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f}, /* L */
    {0x0f, 0x10, 0x10, 0x17, 0x11, 0x11, 0x0f}, /* G */
    {0x11, 0x0a, 0x04, 0x04, 0x04, 0x0a, 0x11}, /* X */
    {0x11, 0x0a, 0x04, 0x04, 0x04, 0x04, 0x04}, /* Y */
};

static int32 pd_live_font_index(char ch)
{
    switch (ch) {
    case 'D': return 0;
    case 'E': return 1;
    case 'F': return 2;
    case 'N': return 3;
    case 'O': return 4;
    case 'T': return 5;
    case 'B': return 6;
    case 'H': return 7;
    case 'K': return 8;
    case 'M': return 9;
    case 'P': return 10;
    case 'R': return 11;
    case 'S': return 12;
    case 'V': return 13;
    case 'W': return 14;
    case '0': return 15;
    case '1': return 16;
    case '2': return 17;
    case '3': return 18;
    case '4': return 19;
    case '5': return 20;
    case '6': return 21;
    case '7': return 22;
    case '8': return 23;
    case '9': return 24;
    case 'C': return 25;
    case '-': return 26;
    case 'L': return 27;
    case 'G': return 28;
    case 'X': return 29;
    case 'Y': return 30;
    default: return -1;
    }
}

static void pd_live_draw_char_y(uint8 *y_plane, int32 width, int32 height,
                                int32 x, int32 y, char ch, uint8 value, int32 scale)
{
    int32 idx = pd_live_font_index(ch);
    if (idx < 0 || scale <= 0) {
        return;
    }

    for (int32 row = 0; row < 7; row++) {
        uint8 bits = s_pd_live_font5x7[idx][row];
        for (int32 col = 0; col < 5; col++) {
            if (!(bits & (1 << (4 - col)))) {
                continue;
            }
            int32 px0 = x + col * scale;
            int32 py0 = y + row * scale;
            for (int32 yy = 0; yy < scale; yy++) {
                int32 py = py0 + yy;
                if (py < 0 || py >= height) {
                    continue;
                }
                for (int32 xx = 0; xx < scale; xx++) {
                    int32 px = px0 + xx;
                    if (px >= 0 && px < width) {
                        y_plane[py * width + px] = value;
                    }
                }
            }
        }
    }
}

static void pd_live_draw_text_y(uint8 *y_plane, int32 width, int32 height,
                                int32 x, int32 y, const char *text,
                                uint8 fg, uint8 bg, int32 scale)
{
    int32 len = 0;
    while (text[len]) {
        len++;
    }

    int32 pad = scale * 2;
    int32 char_w = 5 * scale;
    int32 char_h = 7 * scale;
    int32 gap = scale;
    int32 box_w = len * char_w + (len > 0 ? (len - 1) * gap : 0) + pad * 2;
    int32 box_h = char_h + pad * 2;
    int32 x0 = pd_live_clamp_s32(x, 0, width - 1);
    int32 y0 = pd_live_clamp_s32(y, 0, height - 1);
    int32 x1 = pd_live_clamp_s32(x0 + box_w - 1, 0, width - 1);
    int32 y1 = pd_live_clamp_s32(y0 + box_h - 1, 0, height - 1);

    for (int32 yy = y0; yy <= y1; yy++) {
        memset(y_plane + yy * width + x0, bg, (size_t)(x1 - x0 + 1));
    }

    int32 cx = x0 + pad;
    int32 cy = y0 + pad;
    for (int32 i = 0; i < len; i++) {
        pd_live_draw_char_y(y_plane, width, height, cx, cy, text[i], fg, scale);
        cx += char_w + gap;
    }
}

static void pd_live_draw_status_y(uint8 *y_plane, int32 width, int32 height,
                                  int32 detected, int32 hog_enabled)
{
    int32 scale = width / 120;
    const char *text = "NO";
    uint8 fg = 220;
    uint8 bg = 40;

    if (scale < 3) scale = 3;
    if (scale > 7) scale = 7;

    if (!hog_enabled) {
        text = "OFF";
        fg = 80;
        bg = 220;
    } else if (detected) {
        text = "DET";
        fg = 255;
        bg = 20;
    }

    pd_live_draw_text_y(y_plane, width, height, scale * 3, scale * 3,
                        text, fg, bg, scale);
}





static void pd_live_source_color(hogpd_detection_source_t source,
                                 uint8 *y, uint8 *u, uint8 *v)
{
    switch (source) {
    case HOGPD_DETECTION_SOURCE_FLOW_BLOCK:
        *y = 149; *u = 43; *v = 21;
        break;
    case HOGPD_DETECTION_SOURCE_FLOW_BLOCK_MOTION:
        *y = 170; *u = 32; *v = 32;
        break;
    case HOGPD_DETECTION_SOURCE_FLOW_BOUNDARY:
        *y = 178; *u = 171; *v = 0;
        break;
    case HOGPD_DETECTION_SOURCE_FLOW_HOLD:
        *y = 220; *u = 128; *v = 128;
        break;
    case HOGPD_DETECTION_SOURCE_FLOW_HOG:
        *y = 145; *u = 54; *v = 193;
        break;
    case HOGPD_DETECTION_SOURCE_KALMAN_HOG:
        *y = 105; *u = 212; *v = 235;
        break;
    case HOGPD_DETECTION_SOURCE_FULL_STALE:
        *y = 130; *u = 100; *v = 220;
        break;
    case HOGPD_DETECTION_SOURCE_FULL_HOG:
    default:
        *y = 76; *u = 84; *v = 255;
        break;
    }
}

/* 事件节流间隔(ms)，0=每帧都发；LOST下降沿不受节流。 */
static uint32 s_pd_live_event_interval_ms = 50;
/* 事件观察器开关(打印坐标, 联调用, 置0关闭)。 */
static int32 s_pd_live_sysevt_test_log = 1;

// 设置人形事件节流间隔(ms)，0=每帧都发；LOST下降沿不受节流。
void hogpd_set_event_interval_ms(uint32 interval_ms)
{
    s_pd_live_event_interval_ms = interval_ms;
}

#if 1 /* 测试观察器(由s_pd_live_sysevt_test_log运行时控制) */
// sysevt测试订阅者：解包坐标并打印相对画面中心(800×480)的偏差。
static sysevt_hdl_res pd_live_sysevt_test_hdl(uint32 event_id,
                                               uint32 data, uint32 priv)
{
    int32 cx, cy;
    (void)priv;
    switch (event_id) {
    case HOGPD_SYSEVT_HUMAN_DETECTED:
    case HOGPD_SYSEVT_HUMAN_PREDICT:
        cx = HOGPD_SYSEVT_UNPACK_X(data);
        cy = HOGPD_SYSEVT_UNPACK_Y(data);
        if (s_pd_live_sysevt_test_log) os_printf("[PD_EVT] %s cx=%d cy=%d dx=%d dy=%d\r\n",
                  event_id == HOGPD_SYSEVT_HUMAN_DETECTED ? "DET" : "PRE",
                  cx, cy, cx - 400, cy - 240);
        break;
    case HOGPD_SYSEVT_HUMAN_LOST:
        if (s_pd_live_sysevt_test_log) os_printf("[PD_EVT] LOST\r\n");
        break;
    default:
        break;
    }
    return SYSEVT_CONTINUE;
}

// 首次发送前注册测试订阅(幂等)。
static void pd_live_sysevt_test_register(void)
{
    static int32 s_registered;
    if (!s_registered) {
        s_registered = 1;
        hogpd_set_log(hgprintf);  /* 库日志出口(os_printf为宏, 传底层hgprintf) */
        sys_event_take(HOGPD_SYSEVT_HUMAN_DETECTED,
                       pd_live_sysevt_test_hdl, 0);
        sys_event_take(HOGPD_SYSEVT_HUMAN_PREDICT,
                       pd_live_sysevt_test_hdl, 0);
        sys_event_take(HOGPD_SYSEVT_HUMAN_LOST,
                       pd_live_sysevt_test_hdl, 0);
        if (s_pd_live_sysevt_test_log) os_printf("[PD_EVT] test subscriber ready\r\n");
    }
}
#endif

// 向方案侧发送人形目标事件(坐标为scale3输出帧坐标, 主画面800×480)。
static void pd_live_send_target_event(const hogpd_frame_result_t *r)
{
    static int32 s_prev_visible;
    static uint32 s_last_ms;
    static int32 s_fail_logged;
    uint32 now = (uint32)(os_useconds() / 1000);
    int32 cx, cy;

    if (s_pd_live_sysevt_test_log) {
        pd_live_sysevt_test_register();
    }
    if (r->detection_visible || r->prediction_visible) {
        const hogpd_box_t *box = r->detection_visible
                                     ? &r->detection_box
                                     : &r->prediction_box;
        cx = box->x + box->width / 2;
        cy = box->y + box->height / 2;
        if (cx < 0) cx = 0;
        if (cy < 0) cy = 0;

        if (s_pd_live_event_interval_ms == 0 ||
            (uint32)(now - s_last_ms) >= s_pd_live_event_interval_ms) {
            int32 evt = r->detection_visible
                          ? HOGPD_SYSEVT_HUMAN_DETECTED
                          : HOGPD_SYSEVT_HUMAN_PREDICT;
            if (sys_event_new(evt, HOGPD_SYSEVT_PACK_XY(cx, cy)) < 0) {
                if (!s_fail_logged) {
                    s_fail_logged = 1;
                    os_printf("[PD_LIVE] sysevt new fail\r\n");
                }
            } else {
                s_last_ms = now;
            }
        }
        s_prev_visible = 1;
    } else if (s_prev_visible) {
        if (sys_event_new(HOGPD_SYSEVT_HUMAN_LOST, 0) < 0) {
            if (!s_fail_logged) {
                s_fail_logged = 1;
                os_printf("[PD_LIVE] sysevt new fail\r\n");
            }
        }
        s_prev_visible = 0;
    }
}

void hogpd_live_process_yuv420p(uint8 *yuv420p,int32 width,int32 height)
{
    hogpd_frame_result_t r;

    if (!yuv420p) {
        return;
    }

    memset(&r, 0, sizeof(r));
    hogpd_tracker_process_yuv420p(yuv420p, width, height, &r);
    pd_live_send_target_event(&r);

    if (r.detection_visible) {
        uint8 y;
        uint8 u;
        uint8 v;

        pd_live_source_color(r.detection_source, &y, &u, &v);
        if (r.prediction_visible) {
            pd_live_draw_rect_yuv420p(yuv420p, width, height,
                                      &r.prediction_box, 29, 255, 107, 1);
            pd_live_draw_cross_yuv420p(yuv420p, width, height,
                                       &r.prediction_box, 29, 255, 107);
        }

        pd_live_draw_rect_yuv420p(yuv420p, width, height,
                                  &r.detection_box, y, u, v, 3);
        pd_live_draw_cross_yuv420p(yuv420p, width, height,
                                   &r.detection_box, y, u, v);
        pd_live_draw_status_y(yuv420p, width, height, 1, r.hog_enabled);
    } else if (r.prediction_visible) {
        pd_live_draw_rect_yuv420p(yuv420p, width, height,
                                  &r.prediction_box, 29, 255, 107, 1);
        pd_live_draw_cross_yuv420p(yuv420p, width, height,
                                   &r.prediction_box, 29, 255, 107);
        pd_live_draw_status_y(yuv420p, width, height, 0, r.hog_enabled);
    } else {
        pd_live_draw_status_y(yuv420p, width, height, 0, r.hog_enabled);
    }

    pd_live_uv_cache_clean(yuv420p, width, height);
}

int32 hogpd_live_init(const hogpd_config_t *config)
{
    if (hogpd_tracker_init(config) != RET_OK) {
        return RET_ERR;
    }

    return RET_OK;
}

void hogpd_live_deinit(void)
{
    hogpd_tracker_deinit();
}
