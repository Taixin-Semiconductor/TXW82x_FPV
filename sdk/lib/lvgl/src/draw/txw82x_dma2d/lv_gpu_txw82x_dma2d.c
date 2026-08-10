/**
 * @file lv_gpu_txw82x_dma2d.c
 * 
 */

#include "lv_gpu_txw82x_dma2d.h"
#include "../../core/lv_refr.h"

#if LV_USE_GPU_TXW82X_DMA2D

#include "dev.h"
#include "devid.h"
#include "chip/txw82x/misc.h"
#include "hal/dma2d.h"
#include "osal/string.h"    // debug

/* ============================ 配置 ============================ */

#if LV_COLOR_DEPTH == 16
    #define LV_DMA2D_COLOR_FORMAT DMA2D_COLOR_TYPE_RGB565
#elif LV_COLOR_DEPTH == 32
    #define LV_DMA2D_COLOR_FORMAT DMA2D_COLOR_TYPE_ARGB8888
#else
    /*Can't use GPU with other formats*/
    #error "TXW82x DMA2D: only LV_COLOR_DEPTH 16 / 32 supported"
#endif

/* ======================== static 原型 ========================= */

static bool blend_fill(lv_color_t *dest_buf, lv_coord_t dest_stride,
                       lv_coord_t dest_height, const lv_area_t *fill_area,
                       lv_color_t color);
static bool blend_map(lv_color_t *dest_buf, const lv_area_t *dest_area,
                      lv_coord_t dest_stride, const lv_color_t *src_buf,
                      lv_coord_t src_stride, lv_opa_t opa);
static bool blend_mix(lv_color_t *dest_buf, const lv_area_t *dest_area,
                      lv_coord_t dest_stride, const lv_color_t *src_buf,
                      lv_coord_t src_stride, lv_opa_t opa);
static bool blend_mix_fill(lv_color_t *dest_buf, const lv_area_t *dest_area,
                           lv_coord_t dest_stride, lv_color_t color, lv_opa_t opa);

                            
/* ======================== static 变量 ========================= */

static struct dma2d_device *dma2d_dev = NULL;

/* ======================== inline 工具 ========================== */

static inline bool dma2d_is_psram_addr(const void *buf)
{
    uint32_t addr = (uint32_t)buf;

    return ((addr >= PSRAM_BASE && addr < PSRAM_END_ADDR) ||
            (addr >= PSRAM_BASE_I && addr < PSRAM_END_ADDR_I));
}

static inline int32_t dma2d_buffer_span(int32_t width, int32_t height, int32_t stride)
{
    if (width <= 0 || height <= 0 || stride <= 0) {
        return 0;
    }

    uint64_t row_offset = ((uint64_t)height - 1ULL) * (uint64_t)stride;
    uint64_t pixel_span = row_offset + (uint64_t)width;
    uint64_t byte_span = pixel_span * (uint64_t)sizeof(lv_color_t);

    if (byte_span == 0 || byte_span > 0x7fffffffULL) {
        return 0;
    }

    return (int32_t)byte_span;
}

static inline void dma2d_cache_clean_src(const lv_color_t *src_buf,
                                         int32_t width, int32_t height, int32_t stride)
{
    if (!dma2d_is_psram_addr(src_buf)) {
        return;
    }

    int32_t span = dma2d_buffer_span(width, height, stride);
    if (span > 0) {
        sys_dcache_clean_range_unaligned((uint32_t *)src_buf, span);
    }
}

static inline void dma2d_cache_prepare_dest(lv_color_t *dest_buf,
                                            int32_t width, int32_t height, int32_t stride)
{
    if (!dma2d_is_psram_addr(dest_buf)) {
        return;
    }

    int32_t span = dma2d_buffer_span(width, height, stride);
    if (span > 0) {
        sys_dcache_clean_invalid_range_unaligned((uint32_t *)dest_buf, span);
    }
}

static inline void dma2d_cache_invalidate_dest(lv_color_t *dest_buf,
                                               int32_t width, int32_t height, int32_t stride)
{
    if (!dma2d_is_psram_addr(dest_buf)) {
        return;
    }

    int32_t span = dma2d_buffer_span(width, height, stride);
    if (span > 0) {
        sys_dcache_invalid_range_unaligned((uint32_t *)dest_buf, span);
    }
}

// 轮询硬件状态寄存器直到 DMA2D 传输完成 (DONE/ERROR) 或超时
static inline int32_t dma2d_wait_complete(struct dma2d_device *dev, uint32_t timeout)
{
    int32_t  status;
    uint32_t to = 0;

    do {
        status = dma2d_check_status(dev);
        if (++to > timeout) {
            dma2d_abort_trans(dev);     // 超时后终止传输
            return DMA2D_STATUS_TIMEOUT;
        }
    } while (status == DMA2D_STATUS_WORKING);

    return status;
}

// debug
#if TXW82X_DMA2D_DEBUG
    #define DMA2D_LOG(fmt, ...)  os_printf("[DMA2D] " fmt "\r\n", ##__VA_ARGS__)
    // 调用次数
    static uint32_t stat_fill_calls  = 0;
    static uint32_t stat_copy_calls  = 0;
    static uint32_t stat_mix_calls   = 0;
    static uint32_t stat_mix_fill_calls = 0;
    static uint32_t stat_fallback    = 0;
    // 处理像素数
    static uint64_t stat_fill_px     = 0;
    static uint64_t stat_copy_px     = 0;
    static uint64_t stat_mix_px      = 0;
    static uint64_t stat_mix_fill_px = 0;
    static uint64_t stat_sw_px       = 0;
#else
    #define DMA2D_LOG(fmt, ...)
#endif


/* ======================== 全局函数 ============================ */

// 初始化：获取 DMA2D 设备
void lv_draw_txw82x_dma2d_init(void)
{
    dma2d_dev = (struct dma2d_device *)dev_get(HG_DMA2D_DEVID);

    if (dma2d_dev == NULL) {
        DMA2D_LOG("ERROR: Failed to get DMA2D device!");
    } else {
        DMA2D_LOG("init OK: DMA2D device at 0x%08X", (uint32_t)dma2d_dev);
    }
}

// 上下文初始化：用 DMA2D 版本的 blend 替换软件 blend
void lv_draw_txw82x_dma2d_ctx_init(lv_disp_drv_t *drv, lv_draw_ctx_t *draw_ctx)
{
    LV_UNUSED(drv);
    lv_draw_sw_init_ctx(drv, draw_ctx);

    lv_draw_txw82x_dma2d_ctx_t *dma2d_draw_ctx = (lv_draw_sw_ctx_t *)draw_ctx;

    dma2d_draw_ctx->blend = lv_draw_txw82x_dma2d_blend;
    dma2d_draw_ctx->base_draw.wait_for_finish = lv_gpu_txw82x_dma2d_wait_cb;

    DMA2D_LOG("ctx_init OK");
}

void lv_draw_txw82x_dma2d_ctx_deinit(lv_disp_drv_t *drv, lv_draw_ctx_t *draw_ctx)
{
    LV_UNUSED(drv);
    LV_UNUSED(draw_ctx);
}


// Blend 入口
void lv_draw_txw82x_dma2d_blend(lv_draw_ctx_t *draw_ctx, const lv_draw_sw_blend_dsc_t *dsc)
{
    lv_area_t blend_area;
    if (!_lv_area_intersect(&blend_area, dsc->blend_area, draw_ctx->clip_area)) {
        return;
    }

    bool done = false;

    // DMA2D 使用条件: 设备可用，无 mask，普通混合模式，混合区域大于阈值 100，render_with_alpha 为 0
    if (dma2d_dev != NULL && dsc->mask_buf == NULL && dsc->blend_mode == LV_BLEND_MODE_NORMAL
        && lv_area_get_size(&blend_area) > 100 && draw_ctx->render_with_alpha == 0)
    {
        lv_coord_t dest_stride = lv_area_get_width(draw_ctx->buf_area); // 目标缓冲区的行宽度
        lv_coord_t dest_height = lv_area_get_height(draw_ctx->buf_area);// 总行数
        lv_color_t *dest_buf = draw_ctx->buf;
        dest_buf += dest_stride * (blend_area.y1 - draw_ctx->buf_area->y1) + (blend_area.x1 - draw_ctx->buf_area->x1);
        const lv_color_t *src_buf = dsc->src_buf;

        if (src_buf) {
            lv_coord_t src_stride = lv_area_get_width(dsc->blend_area);
            src_buf += src_stride * (blend_area.y1 - dsc->blend_area->y1) + (blend_area.x1 - dsc->blend_area->x1);

            if (dsc->opa >= LV_OPA_MAX ) {
                /* 不透明像素块 → blkcpy 拷贝 ── */
                lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
                done = blend_map(dest_buf, &blend_area, dest_stride, src_buf, src_stride, dsc->opa);
            }
            else {
                /* opa < 255 半透明像素块 → mixture 混合 ── */
                lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
                done = blend_mix(dest_buf, &blend_area, dest_stride, src_buf, src_stride, dsc->opa);
            }
        }
        else if (dsc->opa >= LV_OPA_MAX) {
            /* 纯色填充 → memset */ 
            lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
            done = blend_fill(dest_buf, dest_stride, dest_height, &blend_area, dsc->color);
        }
        else if (dsc->opa < LV_OPA_MAX && dsc->opa > LV_OPA_MIN) {
            /* 纯色半透明 → mixture */
            lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
            done = blend_mix_fill(dest_buf, &blend_area, dest_stride, dsc->color, dsc->opa);
        }
    }
    if (!done) {
#if TXW82X_DMA2D_DEBUG
        stat_fallback++;
        stat_sw_px += lv_area_get_size(&blend_area);
#endif
        lv_draw_sw_blend_basic(draw_ctx, dsc);
    }
}

void lv_gpu_txw82x_dma2d_wait_cb(lv_draw_ctx_t *draw_ctx)
{
    /* DMA2D 等待移到各 blend 函数内（dma2d_wait_complete） */
    lv_draw_sw_wait_for_finish(draw_ctx);
}

/* ======================== static 函数 ========================= */

// DMA2D memset 硬件填充
static bool blend_fill(lv_color_t *dest_buf, lv_coord_t dest_stride,
                       lv_coord_t dest_height, const lv_area_t *fill_area,
                       lv_color_t color)
{
    int32_t area_w = lv_area_get_width(fill_area);
    int32_t area_h = lv_area_get_height(fill_area);
    
    struct dma2d_memset_param param;
    memset(&param, 0, sizeof(param));
    param.color_mode         = LV_DMA2D_COLOR_FORMAT;
    param.color_set          = color.full;
    param.dst_addr           = (uint32_t)dest_buf;
    param.dst_pixel_width    = dest_stride;
    param.dst_pixel_height   = dest_height;
    param.pixel_start_width  = 0;
    param.pixel_start_height = 0;
    param.set_pixel_width    = area_w;
    param.set_pixel_height   = area_h;

    dma2d_cache_prepare_dest(dest_buf, area_w, area_h, dest_stride);

    int32_t ret = dma2d_memset(dma2d_dev, &param);
    if (ret != 0) {
        DMA2D_LOG("fill ERR: dma2d_memset returned %d", (int)ret);
        return false;
    }
    int32_t status = dma2d_wait_complete(dma2d_dev, 100000u);
    dma2d_cache_invalidate_dest(dest_buf, area_w, area_h, dest_stride);

    if (status != DMA2D_STATUS_DONE) {
        DMA2D_LOG("fill ERR: status=%d", (int)status);
        return true;
    }

#if TXW82X_DMA2D_DEBUG
    stat_fill_calls++;
    stat_fill_px += (uint64_t)area_w * area_h;
    if (stat_fill_calls % 50 == 1) {
        DMA2D_LOG("fill #%u: %dx%d dst=%08X stride=%d dst_h=%d status=%d", stat_fill_calls, area_w, area_h,
                  (uint32_t)dest_buf, dest_stride, dest_height, (int)status);
    }
#endif

    return true;
}

// DMA2D blkcpy 硬件图层拷贝
static bool blend_map(lv_color_t *dest_buf, const lv_area_t *dest_area,
                      lv_coord_t dest_stride, const lv_color_t *src_buf,
                      lv_coord_t src_stride, lv_opa_t opa)
{
    int32_t dest_w = lv_area_get_width(dest_area);
    int32_t dest_h = lv_area_get_height(dest_area);
    LV_UNUSED(opa);

    struct dma2d_blkcpy_param param;
    memset(&param, 0, sizeof(param));
    param.color_mode             = LV_DMA2D_COLOR_FORMAT;
    param.src_addr               = (uint32_t)src_buf;
    param.dst_addr               = (uint32_t)dest_buf;
    param.src_pixel_width        = src_stride;      // 源 stride
    param.dst_pixel_width        = dest_stride;     // 目标缓冲区 stride
    param.blk_pixel_width        = dest_w;
    param.blk_pixel_height       = dest_h;
    param.src_pixel_start_width  = 0;
    param.src_pixel_start_height = 0;
    param.dst_pixel_start_width  = 0;
    param.dst_pixel_start_height = 0;

    dma2d_cache_clean_src(src_buf, dest_w, dest_h, src_stride);
    dma2d_cache_prepare_dest(dest_buf, dest_w, dest_h, dest_stride);

    int32_t ret = dma2d_blkcpy(dma2d_dev, &param);
    if (ret != 0) {
        DMA2D_LOG("copy ERR: dma2d_blkcpy returned %d", (int)ret);
        return false;
    }
    int32_t status = dma2d_wait_complete(dma2d_dev, 100000u);
    dma2d_cache_invalidate_dest(dest_buf, dest_w, dest_h, dest_stride);

    if (status != DMA2D_STATUS_DONE) {
        DMA2D_LOG("copy ERR: status=%d", (int)status);
        return true;
    }

#if TXW82X_DMA2D_DEBUG
    stat_copy_calls++;
    stat_copy_px += (uint64_t)dest_w * dest_h;
    if (stat_copy_calls % 200 == 1) {
        DMA2D_LOG("copy #%u: %dx%d src=%08X(s=%d) -> dst=%08X(s=%d)", stat_copy_calls, dest_w, dest_h,
                  (uint32_t)src_buf, src_stride, (uint32_t)dest_buf, dest_stride);
    }
#endif

    return true;
}


// DMA2D mixture 半透明图层混合
static bool blend_mix(lv_color_t *dest_buf, const lv_area_t *dest_area,
                      lv_coord_t dest_stride, const lv_color_t *src_buf,
                      lv_coord_t src_stride, lv_opa_t opa)
{
    int32_t dest_w = lv_area_get_width(dest_area);
    int32_t dest_h = lv_area_get_height(dest_area);

    struct dma2d_mixture_param param;
    memset(&param, 0, sizeof(param));

    param.photo0_addr = (uint32_t)src_buf;   // 前景
    param.photo1_addr = (uint32_t)dest_buf;  // 背景
    param.output_addr = (uint32_t)dest_buf;  // 输出

    param.photo0_color_val = 0;
    param.photo1_color_val = 0;

    param.mixture_pixel_width  = dest_w;
    param.mixture_pixel_height = dest_h;

    param.photo0_color_mode = LV_DMA2D_COLOR_FORMAT;
    param.photo1_color_mode = LV_DMA2D_COLOR_FORMAT;
    param.output_color_mode = LV_DMA2D_COLOR_FORMAT;

    param.photo0_pixel_width = src_stride;
    param.photo1_pixel_width = dest_stride;
    param.output_pixel_width = dest_stride;

    param.photo0_pixel_start_width  = 0;
    param.photo0_pixel_start_height = 0;
    param.photo1_pixel_start_width  = 0;
    param.photo1_pixel_start_height = 0;
    param.output_pixel_start_width  = 0;
    param.output_pixel_start_height = 0;

    param.src_param[0].alpha_val     = opa;
    param.src_param[0].alpha_mode    = DMA2D_ALPHA_MODE_FIXED_VAL;
    param.src_param[0].alpha_reverse = 0;
    param.src_param[0].r_b_swap      = 0;

    param.src_param[1].alpha_val     = 0;
    param.src_param[1].alpha_mode    = DMA2D_ALPHA_MODE_SAVE;
    param.src_param[1].alpha_reverse = 0;
    param.src_param[1].r_b_swap      = 0;

    param.dst_param.r_b_swap      = 0;
    param.dst_param.alpha_reverse = 0;
    param.dst_param.byte_swap     = 0;

    dma2d_cache_clean_src(src_buf, dest_w, dest_h, src_stride);
    dma2d_cache_prepare_dest(dest_buf, dest_w, dest_h, dest_stride);

    int32_t ret = dma2d_mixture(dma2d_dev, &param);
    if (ret != 0) {
        DMA2D_LOG("mix ERR: dma2d_mixture returned %d", (int)ret);
        return false;
    }
    int32_t status = dma2d_wait_complete(dma2d_dev, 100000u);
    dma2d_cache_invalidate_dest(dest_buf, dest_w, dest_h, dest_stride);

    if (status != DMA2D_STATUS_DONE) {
        DMA2D_LOG("mix ERR: status=%d", (int)status);
        return true;
    }

#if TXW82X_DMA2D_DEBUG
    stat_mix_calls++;
    stat_mix_px += (uint64_t)dest_w * dest_h;
    if (stat_mix_calls % 200 == 1) {
        DMA2D_LOG("mix #%u: %dx%d opa=%d src=%08X(s=%d) dst=%08X(s=%d)", stat_mix_calls, dest_w, dest_h, (int)opa,
                  (uint32_t)src_buf, src_stride, (uint32_t)dest_buf, dest_stride);
    }
#endif

    return true;
}


// DMA2D mixture 纯色半透明填充
static bool blend_mix_fill(lv_color_t *dest_buf, const lv_area_t *dest_area,
                           lv_coord_t dest_stride, lv_color_t color, lv_opa_t opa)
{
    int32_t dest_w = lv_area_get_width(dest_area);
    int32_t dest_h = lv_area_get_height(dest_area);

    /* 位运算扩展 RGB565 → 8-bit:
     *   R8 = (R5 << 3) | (R5 >> 2)    // 5-bit → 8-bit
     *   G8 = (G6 << 2) | (G6 >> 4)    // 6-bit → 8-bit
     *   B8 = (B5 << 3) | (B5 >> 2)    // 5-bit → 8-bit
     */
    uint8_t r5 = (color.full >> 11) & 0x1F; // & 11111
    uint8_t g6 = (color.full >> 5)  & 0x3F; // & 111111
    uint8_t b5 =  color.full        & 0x1F;
    uint8_t r8 = (r5 << 3) | (r5 >> 2);     
    uint8_t g8 = (g6 << 2) | (g6 >> 4);
    uint8_t b8 = (b5 << 3) | (b5 >> 2);

    /*
     * photo0_color_val:
     *   bit[31:24] = opa   ← HAL 从此提取 alpha → FGPFCCON.ALPHA
     *   bit[23:16] = R8    ← FGCOLOR.RED
     *   bit[15:8]  = G8    ← FGCOLOR.GREEN
     *   bit[7:0]   = B8    ← FGCOLOR.BLUE
     */
    uint32_t fgcolor_val = ((uint32_t)opa << 24)
                         | ((uint32_t)r8  << 16)
                         | ((uint32_t)g8  << 8)
                         |  (uint32_t)b8;

    struct dma2d_mixture_param param;
    memset(&param, 0, sizeof(param));

    param.photo0_color_val        = fgcolor_val;
    param.photo0_addr             = (uint32_t)dest_buf;
    param.photo1_addr             = (uint32_t)dest_buf;
    param.output_addr             = (uint32_t)dest_buf;
    param.mixture_pixel_width     = dest_w;
    param.mixture_pixel_height    = dest_h;
    param.photo0_color_mode       = LV_DMA2D_COLOR_FORMAT;
    param.photo1_color_mode       = LV_DMA2D_COLOR_FORMAT;
    param.output_color_mode       = LV_DMA2D_COLOR_FORMAT;
    param.photo0_pixel_width      = dest_stride;
    param.photo1_pixel_width      = dest_stride;
    param.output_pixel_width      = dest_stride;

    param.src_param[0].alpha_val   = 0;
    param.src_param[0].alpha_mode  = 0;
    param.src_param[0].r_b_swap    = 0;

    param.src_param[1].alpha_mode  = DMA2D_ALPHA_MODE_SAVE;
    param.src_param[1].r_b_swap    = 0;

    param.dst_param.r_b_swap      = 0;
    param.dst_param.alpha_reverse = 0;
    param.dst_param.byte_swap     = 0;

    dma2d_cache_prepare_dest(dest_buf, dest_w, dest_h, dest_stride);

    int32_t ret = dma2d_mixture(dma2d_dev, &param);
    if (ret != 0) {
        DMA2D_LOG("mix_fill ERR: %d", (int)ret);
        return false;
    }
    int32_t status = dma2d_wait_complete(dma2d_dev, 100000u);
    dma2d_cache_invalidate_dest(dest_buf, dest_w, dest_h, dest_stride);

    if (status != DMA2D_STATUS_DONE) {
        DMA2D_LOG("mix_fill ERR: status=%d", (int)status);
        return true;
    }

#if TXW82X_DMA2D_DEBUG
    stat_mix_fill_calls++;
    stat_mix_fill_px += (uint64_t)dest_w * dest_h;
    if (stat_mix_fill_calls % 200 == 1) {
        DMA2D_LOG("mix_fill #%u: %dx%d color=%08X opa=%d -> dst=%08X(s=%d)", 
                  stat_mix_fill_calls, dest_w, dest_h,
                  (uint32_t)color.full, (int)opa, (uint32_t)dest_buf, dest_stride);
    }
#endif

    return true;
}

/* ======================== debug =============================== */

#if TXW82X_DMA2D_DEBUG
void lv_gpu_txw82x_dma2d_print_stats(void)
{
    DMA2D_LOG("=== DMA2D Stats ===");
    DMA2D_LOG("  fill  (memset) : %u calls, %llu px", stat_fill_calls, stat_fill_px);
    DMA2D_LOG("  copy  (blkcpy) : %u calls, %llu px", stat_copy_calls, stat_copy_px);
    DMA2D_LOG("  mix   (mixture): %u calls, %llu px", stat_mix_calls, stat_mix_px);
    DMA2D_LOG("  mix_fill (mixture fill): %u calls, %llu px", stat_mix_fill_calls, stat_mix_fill_px);
    DMA2D_LOG("  SW fallback    : %u calls, %llu px", stat_fallback, stat_sw_px);
    {
        uint64_t hw_px = stat_fill_px + stat_copy_px + stat_mix_px + stat_mix_fill_px;
        uint64_t total = hw_px + stat_sw_px;
        if (total > 0) {DMA2D_LOG("  HW pixels      : %llu (%.1f%%)", hw_px, 100.0 * hw_px / total);}
    }
    DMA2D_LOG("  device         : 0x%08X %s", (uint32_t)dma2d_dev, dma2d_dev ? "OK" : "NULL!");
}
#endif

#endif  /* LV_USE_GPU_TXW82X_DMA2D */
