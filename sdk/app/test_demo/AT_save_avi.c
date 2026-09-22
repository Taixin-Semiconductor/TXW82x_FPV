/***************************************************
    该demo主要是录制AVI使用
***************************************************/
#include "basic_include.h"
#include "stream_define.h"
#include "app/video_record/video_record.h"
#include "osal_file.h"
#include "audio_msi/audio_adc.h"
#include "dev.h"
#include "demo/app_common.h"

#if OPENDML_EN && SDH_EN && FS_EN
static struct msi *g_at_avi_msi = NULL;
static uint8_t g_at_avi_audio_en = 0;
#endif

static int32 get_dev_cb(const struct dev_obj *dev, void *arg)
{
    struct dev_obj **dev_param = (struct dev_obj **)arg;
    if(dev && dev_param)
    {
        *dev_param = (struct dev_obj *)dev;
    }

    return 1;
}

int32 demo_atcmd_save_avi(const char *cmd, char *argv[], uint32 argc)
{
	#if OPENDML_EN && SDH_EN && FS_EN
    uint32_t frq = 0;
    uint32_t record_num = 0;
    if(argc < 1)
    {
        os_printf("%s argc too small:%d\n",__FUNCTION__,argc);
        return 0;
    }

    if(os_atoi(argv[0]) == 0)
    {
        if(g_at_avi_audio_en)
        {
            auadc_msi_del_output(MAIN_MIC_ID, R_AT_AVI_JPEG);
            g_at_avi_audio_en = 0;
        }
        msi_del_output(NULL, AUTO_JPG, R_AT_AVI_JPEG);
        if (g_at_avi_msi)
        {
            msi_destroy(g_at_avi_msi);
            g_at_avi_msi = NULL;
        }
    }
    else if(os_atoi(argv[0]) == 1)
    {
        if(argc < 3)
        {
            os_printf("%s argc too small:%d,should more 2 arg\n",__FUNCTION__,argc);
            return 0;
        }
        frq = os_atoi(argv[2]);
        if(argc > 3)
        {
            record_num = os_atoi(argv[3]);  
        }

        if(record_num == 0)
        {
            record_num = 1;     
        }
        os_printf("frq:%d\n",frq);
        if (!g_at_avi_msi)
        {
            struct dev_obj *audio_dev = NULL;
            uint8_t audio_en = 0;
            if(frq)
            {
                dev_walk(DEV_TYPE_MIC, get_dev_cb, &audio_dev);
                audio_en = audio_dev ? 1 : 0;
            }
            struct video_record_cfg rec_cfg = {
                .name         = R_AT_AVI_JPEG,
                .srcID        = FRAMEBUFF_SOURCE_CAMERA0,
                .filter       = (uint8_t) ~0,
                .rec_time     = 1,
                .audio_en     = audio_en,
                .mode         = 0,
                .video_fps    = 25,
                .file_process = NULL,
            };
            g_at_avi_audio_en = 0;
            g_at_avi_msi = avi_record_msi_init(&rec_cfg);
            if (g_at_avi_msi)
            {
                msi_add_output(NULL, AUTO_JPG, R_AT_AVI_JPEG);
                if (audio_en)
                {
                    auadc_msi_add_output(MAIN_MIC_ID, R_AT_AVI_JPEG);
                    g_at_avi_audio_en = 1;
                }
                msi_do_cmd(g_at_avi_msi, MSI_CMD_MEDIA_CTRL, MSI_MEDIA_CTRL_SET_RECORD_SEC, 30);
                msi_do_cmd(g_at_avi_msi, MSI_CMD_MEDIA_CTRL, MSI_MEDIA_CTRL_RECORD_START, 0);
            }
        }
    }
	#endif

    return 0;
}
