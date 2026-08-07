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
        .y_gamma_en         = 1,
        .rgb_gamma_en       = 1,
        .ce_en              = 1,
        .sharpen_en         = 1,
        .bnr_en             = 1,
        .ynr_en             = 1,
        .cnr_en             = 1,
        .csupp_en           = 1,
        .adj_hue_en         = 1,
        .lsc_en             = 1,
        .dpc_en             = 1,
        .af_en              = 0,
        .dehaze_en          = 0,
        .gic_en             = 0,
        .md_en              = 0,
        .luma_ca_en         = 0,
    },

    .awb_param = {
        .coarse_scale          = 255, 
        .coarse_thr            = 3 << 2,
        .fine_step             = 1 << 2, 
        .lock_hi_thr           = 4, 
        .lock_lo_thr           = 0, 
        .stable_thr            = 16, 
        .cbcr_thr              = 6 << 2, 
        .awb_auto_en           = 1, 
        .awb_meas_mode         = 2, 
        .cr_target             = 2048, 
        .cb_target             = 2048,
        .front_cr_val          = 30,
        .front_cb_val          = 30,
        .front_uv_sum          = 15, 
        .back_cr_val           = 30,
        .back_cb_val           = 30,
        .back_uv_sum           = 10, 
        .cons_cr_max           = 40,
        .cons_cb_max           = 40,
        .cons_uv_max           = 30,
        .cons_cr_min           = 10,
        .cons_cb_min           = 10,
        .cons_uv_min           =  2,
        .back_cr_max           = 30,
        .back_cb_max           = 30,
        .back_uv_max           = 10,
        .back_cr_min           = 5,
        .back_cb_min           = 5,
        .back_uv_min           = 5,
        .awb_wp_max            = 0xc0, 
        .awb_wp_min            = 0x08, 
        .awb_r_max             = 0xc0, 
        .awb_g_max             = 0xc0, 
        .awb_b_max             = 0xc0, 
        .awb_precision         = 1,
        .awb_fine_cons_en      = 1,
        .awb_coarse_cons_en    = 1,
        .awb_back_cons_en      = 1,
        .awb_back_wp_min_ratio = 0.2,
        .manual_gain           = {256, 256, 256, 256},
		.awb_crop_pixel_start_h = 0,
        .awb_crop_pixel_start_v = 360,
        .awb_crop_pixel_end_h   = 1919,
        .awb_crop_pixel_end_v   = 1079,
    },

    .cfg_ae = {
        .ae_manual_en              = 0,
        .luma_target               = 55, 
        .luma_weight_sum           = 380,
        .luma_weight               = {  5,  5,  5,  5,  5, //25
                                       10, 20, 30, 20, 10, //90
                                       20, 30, 50, 30, 20, //150
                                       10, 20, 30, 20, 10, //90
                                        5,  5,  5,  5,  5},//25
        .ae_crop_start_h           = 0,
        .ae_crop_start_v           = 270,
        .ae_crop_size_h            = 1920,
        .ae_crop_size_v            = 810,
        .hist_crop_start_h         = 1,
        .hist_crop_start_v         = 271,
        .hist_crop_end_h           = 1920,
        .hist_crop_end_v           = 1080,
        .ae_lock_cnt               = 10,
        .ae_lock_tolerance         = 12,
        .reduce_fps_en             = 0,
        .lowlight_lsb_gain_en      = 1,
        .lowlight_lsb_gain_4hi_fps = 64,    // lowlight_lsb_gain_en==0 ? 16 : 64
        .lowlight_lsb_gain_4lo_fps = 64,    // lowlight_lsb_gain_en==0 ? 16 : 64

        .hist_hs_bin_thr           = 180,
        .hist_upper_hs_pixel_ratio = 0.94,
        .hist_upper_pixel_ratio    = 0.88,
        .hist_lower_pixel_ratio    = 0.00,

        .abl_bv_gain_sel 		   = 1,
        .abl_expo_line_low_ratio   = 0,
        .abl_expo_gain_low_thr     = 0,
        .abl_expo_line_high_ratio  = 0,
        .abl_expo_gain_high_thr    = 0,
        .aoe_expo_gain_thr[0]      = 65535,
        .aoe_expo_gain_thr[1]      = 65535,
        .abl_bv_thr[0]			   = 1e20,
        .abl_bv_thr[1]			   = 1e20,
        .aoe_bv_thr[0]			   = 0,
        .aoe_bv_thr[0]			   = 0,
        .abl_hist_thr[0]		   = 50,			// hist
        .abl_hist_thr[1]		   = 236,
        .dark_pixel_low_ratio	   = 0.60,
        .dark_pixel_high_ratio	   = 0.90,
        .bright_pixel_high_ratio   = 0.15,
        .bright_pixel_sub_ratio    = 0.05,
        .dark_pos_thr_max		   = 50,		    // position
        .bright_pos_adjust_ratio   = 0.90,
        .abl_dark_pos_low_wthr	   = 0.40 * 61,
        .abl_dark_pos_add_wthr	   = 0.10 * 61,
        .aoe_dark_pos_wthr		   = 0.60 * 61,
        .aoe_bright_pos_wthr	   = 0.20 * 61,
        .abl_luma_target_max       = 100,		    // stable
        .abl_diff_ratio			   = 0.05,
        .abl_dark_pos_diff_thr     = 0.15 * 61,
        .abl_bright_pos_diff_thr   = 0.10 * 61,

        .stg_mode                  = 0,
        .stg_ratio_slope           = 0.3*256,
        .stg_max_offset            = 0,
    },
    
    .config_wdr = {
        .wdr_en                    = 1,
        .temporal_smooth_alpha     = 0.1,
        .noise_floor               = 80,
        .noise_floor_out           = 0,
        .shadow_boost_target       = 512,
        .highlight_compress_target = 870,
        .auto_noise_floor_out      = 1,
        .min_ns_percentile         = 0.01,
        .max_ns_percentile         = 0.07,
    },
};

const struct hgisp_param_info isp_slave0_param = 
{
    .enable_param = {
        .test_pattern_en    = 0,
        .dma_flush_en       = ISP_DMA_ENABLE,
        .blc_en             = 1,
        .awb_en             = 1,
        .ae_en              = 1,
        .hist_en            = 1,
        .ccm_en             = 1,
        .y_gamma_en         = 1,
        .rgb_gamma_en       = 1,
        .ce_en              = 1,
        .sharpen_en         = 1,
        .bnr_en             = 1,
        .ynr_en             = 1,
        .cnr_en             = 1,
        .csupp_en           = 1,
        .adj_hue_en         = 1,
        .lsc_en             = 1,
        .dpc_en             = 0,
        .af_en              = 0,
        .dehaze_en          = 0,
        .gic_en             = 0,
        .md_en              = 0,
        .luma_ca_en         = 0,
    },

    .awb_param = {
        .coarse_scale          = 128, 
        .coarse_thr            = 3 << 2,
        .fine_step             = 1 << 2, 
        .lock_hi_thr           = 4, 
        .lock_lo_thr           = 0, 
        .stable_thr            = 16, 
        .cbcr_thr              = 3 << 2, 
        .awb_auto_en           = 1, 
        .awb_meas_mode         = 0, 
        .cr_target             = 2048, 
        .cb_target             = 2048,
        .front_cr_val          = 30,
        .front_cb_val          = 30,
        .front_uv_sum          = 15, 
        .back_cr_val           = 30,
        .back_cb_val           = 30,
        .back_uv_sum           = 10, 
        .cons_cr_max           = 40,
        .cons_cb_max           = 40,
        .cons_uv_max           = 30,
        .cons_cr_min           = 10,
        .cons_cb_min           = 10,
        .cons_uv_min           =  2,
        .back_cr_max           = 30,
        .back_cb_max           = 30,
        .back_uv_max           = 10,
        .back_cr_min           = 5,
        .back_cb_min           = 5,
        .back_uv_min           = 5,
        .awb_wp_max            = 0xc0, 
        .awb_wp_min            = 0x08, 
        .awb_r_max             = 0xc0, 
        .awb_g_max             = 0xc0, 
        .awb_b_max             = 0xc0, 
        .awb_precision         = 1,
        .awb_fine_cons_en      = 1,
        .awb_coarse_cons_en    = 0,
        .awb_back_cons_en      = 1,
        .awb_back_wp_min_ratio = 0.2,
        .manual_gain           = {256, 256, 256, 256},
		.awb_crop_pixel_start_h = 0,
        .awb_crop_pixel_start_v = 0,
        .awb_crop_pixel_end_h   = 0,
        .awb_crop_pixel_end_v   = 0,
    },

    .cfg_ae = {
        .ae_manual_en             = 0,
        .luma_target              = 55, 
        .luma_weight_sum          = 61,
        .luma_weight              = { 2, 2, 2, 2, 2,
                                      2, 3, 3, 3, 2,
                                      2, 3, 5, 3, 2,
                                      2, 3, 3, 3, 2,
                                      2, 2, 2, 2, 2},
        .ae_lock_cnt              = 10,
        .ae_lock_tolerance        = 12,
        .reduce_fps_en            = 0,
        .lowlight_lsb_gain_en      = 0,
        .lowlight_lsb_gain_4hi_fps = 16,
        .lowlight_lsb_gain_4lo_fps = 16,
        .hist_hs_bin_thr           = 232,
        .abl_luma_target_max      = 100,
        .dark_pos_thr_max         = 50,
        .abl_diff_ratio            = 0.03,
        .abl_dark_pos_diff_thr     = 9,     // 0.15 * 61
        .abl_bright_pos_diff_thr   = 6,     // 0.10 * 61
        .bright_pixel_high_ratio   = 0.10,
        .bright_pixel_sub_ratio    = 0.05,
        .abl_dark_pos_low_wthr     = 18, // 0.30 * 61,
        .abl_dark_pos_add_wthr     = 12, // 0.20 * 61,
        .bright_pos_adjust_ratio   = 0.80,
        .dark_pixel_low_ratio      = 0.55,
        .dark_pixel_high_ratio     = 0.85,
        .aoe_dark_pos_wthr         = 30, // 0.50 * 61,
        .aoe_bright_pos_wthr       = 9,  // 0.15 * 61,
        .abl_bv_gain_sel           = 0,
        .abl_expo_line_low_ratio   = 0.80,
        .abl_expo_line_high_ratio  = 1.00,
        .abl_expo_gain_low_thr     = 256,
        .abl_expo_gain_high_thr    = 324,
        .abl_hist_thr[0]          = 50,
        .abl_hist_thr[1]          = 230,
        .aoe_expo_gain_thr[0]     = 384-50,
        .aoe_expo_gain_thr[1]     = 384,
        .abl_bv_thr[0]            = 5000,
        .abl_bv_thr[1]            = 8000,
        .aoe_bv_thr[0]            = 4000,
        .aoe_bv_thr[1]            = 6000,
        .hist_upper_pixel_ratio    = 0.88,
        .hist_upper_hs_pixel_ratio = 0.92,
        .hist_lower_pixel_ratio    = 0.00,
    },

    .config_wdr = {
        .wdr_en                    = 0,
        .temporal_smooth_alpha     = 0.1,
        .noise_floor               = 128,
        .noise_floor_out           = 128,
        .shadow_boost_target       = 512,
        .highlight_compress_target = 870,
        .auto_noise_floor_out      = 1,
        .min_ns_percentile         = 0.01,
        .max_ns_percentile         = 0.07,
    },
};

const struct hgisp_param_info isp_slave1_param = 
{
    .enable_param = {
        .test_pattern_en    = 0,
        .dma_flush_en       = ISP_DMA_ENABLE,
        .blc_en             = 1,
        .awb_en             = 1,
        .ae_en              = 1,
        .hist_en            = 1,
        .ccm_en             = 1,
        .y_gamma_en         = 1,
        .rgb_gamma_en       = 1,
        .ce_en              = 1,
        .sharpen_en         = 1,
        .bnr_en             = 1,
        .ynr_en             = 1,
        .cnr_en             = 1,
        .csupp_en           = 1,
        .adj_hue_en         = 1,
        .lsc_en             = 1,
        .dpc_en             = 0,
        .af_en              = 0,
        .dehaze_en          = 0,
        .gic_en             = 0,
        .md_en              = 0,
        .luma_ca_en         = 0,
    },

    .awb_param = {
        .coarse_scale          = 128, 
        .coarse_thr            = 3 << 2,
        .fine_step             = 1 << 2, 
        .lock_hi_thr           = 4, 
        .lock_lo_thr           = 0, 
        .stable_thr            = 16, 
        .cbcr_thr              = 3 << 2, 
        .awb_auto_en           = 1, 
        .awb_meas_mode         = 0, 
        .cr_target             = 2048, 
        .cb_target             = 2048,
        .front_cr_val          = 30,
        .front_cb_val          = 30,
        .front_uv_sum          = 15, 
        .back_cr_val           = 30,
        .back_cb_val           = 30,
        .back_uv_sum           = 10, 
        .cons_cr_max           = 40,
        .cons_cb_max           = 40,
        .cons_uv_max           = 30,
        .cons_cr_min           = 10,
        .cons_cb_min           = 10,
        .cons_uv_min           =  2,
        .back_cr_max           = 30,
        .back_cb_max           = 30,
        .back_uv_max           = 10,
        .back_cr_min           = 5,
        .back_cb_min           = 5,
        .back_uv_min           = 5,
        .awb_wp_max            = 0xc0, 
        .awb_wp_min            = 0x08, 
        .awb_r_max             = 0xc0, 
        .awb_g_max             = 0xc0, 
        .awb_b_max             = 0xc0, 
        .awb_precision         = 1,
        .awb_fine_cons_en      = 1,
        .awb_coarse_cons_en    = 0,
        .awb_back_cons_en      = 1,
        .awb_back_wp_min_ratio = 0.2,
        .manual_gain           = {256, 256, 256, 256},
		.awb_crop_pixel_start_h = 0,
        .awb_crop_pixel_start_v = 360,
        .awb_crop_pixel_end_h   = 1919,
        .awb_crop_pixel_end_v   = 1079,
    },

    .cfg_ae = {
        .ae_manual_en              = 0,
        .luma_target               = 55, 
        .luma_weight_sum           = 480,
        .luma_weight               = { 10, 10, 10, 10, 10,
                                       10, 10, 10, 10, 10,
                                       20, 30, 50, 30, 20,
                                       20, 30, 30, 30, 20,
                                       20, 20, 20, 20, 20},
        .ae_lock_cnt               = 10,
        .ae_lock_tolerance         = 12,
        .reduce_fps_en             = 0,
        .lowlight_lsb_gain_en      = 1,
        .lowlight_lsb_gain_4hi_fps = 64,    // lowlight_lsb_gain_en==0 ? 16 : 64
        .lowlight_lsb_gain_4lo_fps = 64,    // lowlight_lsb_gain_en==0 ? 16 : 64
        .hist_hs_bin_thr           = 180,
        .abl_luma_target_max       = 100,
        .dark_pos_thr_max          = 50,
        .abl_diff_ratio            = 0.03,
        .abl_dark_pos_diff_thr     = 9,
        .abl_bright_pos_diff_thr   = 6,
        .bright_pixel_high_ratio   = 0.10,
        .bright_pixel_sub_ratio    = 0.05,
        .dark_pixel_low_ratio      = 0.55,
        .dark_pixel_high_ratio     = 0.85,
        .abl_dark_pos_low_wthr     = 18, // 0.30 * 61,
        .abl_dark_pos_add_wthr     = 12, // 0.20 * 61,
        .bright_pos_adjust_ratio   = 0.80,
        .aoe_dark_pos_wthr         = 30, // 0.50 * 61,
        .aoe_bright_pos_wthr       = 9,  // 0.15 * 61,
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
        .hist_upper_pixel_ratio    = 0.88,
        .hist_upper_hs_pixel_ratio = 0.94,
        .hist_lower_pixel_ratio    = 0.00,
        .stg_mode                  = 0,         // 自动曝光策略 0：高光优先； 1：低光优先
        .stg_ratio_slope           = 0.3*256,   // fix(0,16,8), 值越大，roi对权重的影响越大。
        .stg_max_offset            = 20,        // fix(0,8,0)， roi对权重影响的最大程度
    },

    .config_wdr = {
        .wdr_en                    = 0,
        .temporal_smooth_alpha     = 0.1,
        .noise_floor               = 128,
        .noise_floor_out           = 128,
        .shadow_boost_target       = 512,
        .highlight_compress_target = 870,
        .auto_noise_floor_out      = 1,
        .min_ns_percentile         = 0.01,
        .max_ns_percentile         = 0.07,
    },
};

#define SENSOR_PARAM_SIZE (64*4*4+153*4*4)


void *isp_sensor_param_load(uint16 *buff)
{
    struct hgisp_sensor_init *init = NULL;
    uint8  sensor_index      = 0;
    uint32 data_offset       = 0;
    uint32 param_size        = buff[2] << 16 | buff[1];
    uint32 gamma_size        = 256;
    uint32 lsc_size          = 153*4*4;
    uint32 info_szie         = sizeof(struct hgisp_sensor_info);
    uint32 sensor_param_size = (gamma_size << 1) + lsc_size + info_szie;
    init = (struct hgisp_sensor_init *)os_malloc(sizeof(struct hgisp_sensor_init));
    if (init)
    {
        os_memset(init, 0, sizeof(struct hgisp_sensor_init));
        sensor_index = buff[3];
        if (sensor_index && (param_size >= sensor_index * sensor_param_size))
        {
            data_offset = 4;
            for (int i = 0; i < sensor_index; i++)
            {
                if (buff[data_offset+2] == ISPCFG_MAGIC)
                {
                    os_memcpy((void *)&init->sensor_info[i], (void *)&buff[data_offset], info_szie);
                    init->sensor_info[i].info_src = ISP_INFO_SRC_TYPE_CODE_PARAM;
                    data_offset += (info_szie >> 1);
                    init->sensor_info[i].sensor_param.y_gamma   = (uint32 *)&buff[data_offset];
                    data_offset += (gamma_size >> 1);
                    init->sensor_info[i].sensor_param.rgb_gamma = (uint32 *)&buff[data_offset];
                    data_offset += (gamma_size >> 1);
                    init->sensor_info[i].sensor_param.lsc_tbl   = (uint32 *)&buff[data_offset];
                    data_offset += (lsc_size >> 1);
                } else {
                    os_printf("param_data flash : %04x target : %04x err!\r\n", buff[data_offset], ISPCFG_MAGIC);
                    goto _err;
                }
            }
            os_printf("sensor param use flash_param!\r\n");
            goto _end;
        } else {
            os_printf("param_size : %d calc_size : %d\r\n", param_size, sensor_index * sensor_param_size);
            goto _err;
        }
    } else {
        os_printf("%s malloc sram size : %d err!\r\n", sizeof(struct hgisp_sensor_init));
    }
    return (void *)NULL;


_err:
    os_printf("sensor param use default param!\r\n");
    init->sensor_info[0].info_src = ISP_INFO_SRC_TYPE_CODE_DOC;
    init->sensor_info[1].info_src = ISP_INFO_SRC_TYPE_CODE_DOC;
    init->sensor_info[2].info_src = ISP_INFO_SRC_TYPE_CODE_DOC;
    os_memcpy((void *)&init->sensor_info[0].sensor_param, (void *)&isp_master_param, sizeof(struct hgisp_param_info));
    os_memcpy((void *)&init->sensor_info[1].sensor_param, (void *)&isp_slave0_param, sizeof(struct hgisp_param_info));
    os_memcpy((void *)&init->sensor_info[2].sensor_param, (void *)&isp_slave1_param, sizeof(struct hgisp_param_info));
_end:
    return (void *)init;
}


