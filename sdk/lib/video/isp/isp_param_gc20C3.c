#include "sys_config.h"
#include "typesdef.h"
#include "osal/string.h"
#include "hal/isp_param.h"

#define ISP_DMA_ENABLE  1

const struct hgisp_param_info isp_master_param = 
{
    .enable_param = {
        .test_pattern_en    = 0,
        .dma_flush_en       = ISP_DMA_ENABLE,
        .blc_en             = 1,
        .awb_en             = 1,
        .ae_en              = 1,
        .hist_en            = 1,
        .ccm_en             = 1,
        .y_gamma_en         = 0,
        .rgb_gamma_en       = 1,
        .ce_en              = 1,
        .sharpen_en         = 1,
        .bnr_en             = 1,
        .ynr_en             = 1,
        .cnr_en             = 1,
        .csupp_en           = 1,
        .adj_hue_en         = 1,
        .lsc_en             = 0,
        .dpc_en             = 0,
        .af_en              = 0,
        .dehaze_en          = 0,
        .gic_en             = 0,
        .md_en              = 0,
        .luma_ca_en         = 0,
    },

    .awb_param = {
        .coarse_scale       = 128, 
        .coarse_thr         = 3 << 2,
        .fine_step          = 1 << 2, 
        .lock_hi_thr        = 4, 
        .lock_lo_thr        = 0, 
        .stable_thr         = 16, 
        .cbcr_thr           = 6 << 2, 
        .awb_auto_en        = 1, 
        .awb_meas_mode      = 0, 
        .cr_target          = 2048, 
        .cb_target          = 2048,
        .front_cr_val       = 30,
        .front_cb_val       = 30,
        .front_uv_sum       = 15, 
        .back_cr_val        = 30,
        .back_cb_val        = 30,
        .back_uv_sum        = 10, 
        .cons_cr_max        = 40,
        .cons_cb_max        = 40,
        .cons_uv_max        = 30,
        .cons_cr_min        = 10,
        .cons_cb_min        = 10,
        .cons_uv_min        =  2,
        .awb_wp_max         = 0xc0, 
        .awb_wp_min         = 0x08, 
        .awb_r_max          = 0xc0, 
        .awb_g_max          = 0xc0, 
        .awb_b_max          = 0xc0, 
        .awb_precision      = 1,
        .awb_fine_cons_en   = 1,
        .awb_coarse_cons_en = 0,
        .manual_gain        = {256, 256, 256, 256},
    },

    .cfg_ae = {
        .ae_manual_en              = 0,
        .luma_target               = 55, 
        .luma_weight_sum           = 61,
        .luma_weight               = { 2, 2, 2, 2, 2,
                                       2, 3, 3, 3, 2,
                                       2, 3, 5, 3, 2,
                                       2, 3, 3, 3, 2,
                                       2, 2, 2, 2, 2},
        .ae_lock_cnt               = 10,
        .ae_lock_tolerance         = 12,
        .reduce_fps_en             = 0,
        .lowlight_lsb_gain_en      = 0,
        .lowlight_lsb_gain_4hi_fps = 16,
        .lowlight_lsb_gain_4lo_fps = 16,
        .hist_hs_bin_thr           = 180,
        .hist_upper_hs_pixel_ratio = 0.94,
        .hist_upper_pixel_ratio    = 0.88,
        .hist_lower_pixel_ratio    = 0.00,

        .abl_luma_target_max       = 100,
        .dark_pos_thr_max          = 50,
        .abl_diff_ratio            = 0.03,
        .abl_dark_pos_diff_thr     = 9,     // 0.15 * 61
        .abl_bright_pos_diff_thr   = 6,     // 0.10 * 61
        .bright_pixel_high_ratio   = 0.10,
        .bright_pixel_sub_ratio    = 0.05,
        .dark_pixel_low_ratio      = 0.55,
        .dark_pixel_high_ratio     = 0.85,
        .abl_dark_pos_low_wthr     = 18,    // 0.30 * 61,
        .abl_dark_pos_add_wthr     = 12,    // 0.20 * 61,
        .bright_pos_adjust_ratio   = 0.80,
        .aoe_dark_pos_wthr         = 30,    // 0.50 * 61,
        .aoe_bright_pos_wthr       = 9,     // 0.15 * 61,
        .abl_bv_gain_sel           = 0,
        .abl_expo_line_low_ratio   = 0.80,
        .abl_expo_line_high_ratio  = 1.00,
        .abl_expo_gain_low_thr     = 256,
        .abl_expo_gain_high_thr    = 324,
        .abl_hist_thr[0]           = 50,
        .abl_hist_thr[1]           = 230,
        .aoe_expo_gain_thr[0]      = 384-50,
        .aoe_expo_gain_thr[1]      = 384,
        .abl_bv_thr[0]             = 6645,  // 80lx
        .abl_bv_thr[1]             = 13216, // 160lx
        .aoe_bv_thr[0]             = 3531,  // 40lx
        .aoe_bv_thr[1]             = 6645,  // 80lx

        .stg_mode                  = 0,
        .stg_ratio_slope           = 0.3*256,
        .stg_max_offset            = 20,
    },
    .config_wdr = {
        .wdr_en                    = 0,
        .temporal_smooth_alpha     = 0.1,
        .noise_floor               = 128,
        .noise_floor_out           = 128,
        .shadow_boost_target       = 512,
        .highlight_compress_target = 870,
    },
};


#define SENSOR_PARAM_SIZE (64*4*2+153*4*4)

void *isp_sensor_param_load(uint16 *buff)
{
    struct hgisp_sensor_init *init = NULL;
    uint8  sensor_index  = 0;
    uint8  data_offset   = 0;
    uint32 param_size    = buff[2] << 16 | buff[1];
    uint32 gamma_size    = 256;
    uint32 lsc_size      = 153*4*4;
    init = (struct hgisp_sensor_init *)os_malloc(sizeof(struct hgisp_sensor_init));
    if (init)
    {
        os_memset(init, 0, sizeof(struct hgisp_sensor_init));
        sensor_index = buff[3];

        if (param_size && sensor_index)
        {
            data_offset = 4;
            for (int i = 0; i < sensor_index; i++)
            {
                if (buff[data_offset] == ISPCFG_MAGIC)
                {
                    os_memcpy((void *)&init->sensor_info[i].sensor_param, (void *)&buff[data_offset], sizeof(struct hgisp_param_info));
                    data_offset += sizeof(struct hgisp_param_info);
                    init->sensor_info[i].sensor_param.y_gamma   = (uint32 *)&buff[data_offset];
                    data_offset += gamma_size;
                    init->sensor_info[i].sensor_param.rgb_gamma = (uint32 *)&buff[data_offset];
                    data_offset += gamma_size;
                    init->sensor_info[i].sensor_param.lsc_tbl   = (uint32 *)&buff[data_offset];
                    data_offset += lsc_size;
                } else {
                    os_printf("param_data flash : %04x target : %04x err!\r\n", buff[data_offset], ISPCFG_MAGIC);
                    goto _err;
                }
            }
            os_printf("sensor param use flash_param!\r\n");
            goto _end;
        } else {
            goto _err;
        }
    } else {
        os_printf("%s malloc sram size : %d err!\r\n", sizeof(struct hgisp_sensor_init));
    }
    return (void *)NULL;


_err:
    os_printf("sensor param use default param!\r\n");
    os_memcpy((void *)&init->sensor_info[0].sensor_param, (void *)&isp_master_param, sizeof(struct hgisp_param_info));
//    os_memcpy((void *)&init->sensor_info[1].sensor_param, (void *)&isp_slave0_param, sizeof(struct hgisp_param_info));
//    os_memcpy((void *)&init->sensor_info[2].sensor_param, (void *)&isp_slave1_param, sizeof(struct hgisp_param_info));
_end:
    return (void *)init;
}


