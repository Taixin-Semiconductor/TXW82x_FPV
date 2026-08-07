/***************************************************
    该demo主要是录制AVI使用
***************************************************/
#include "basic_include.h"
#include "stream_frame.h"
#include "avi_record_msi.h"
#include "osal_file.h"
#include "audio_msi/audio_adc.h"

#if OPENDML_EN && SDH_EN && FS_EN
static struct msi *g_at_avi_msi = NULL;
#endif

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
        #if AUDIO_EN
        auadc_msi_del_output(AUSYS_AUAD, R_AT_AVI_JPEG);
        #endif
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
                g_at_avi_msi = avi_record_msi_init(R_AT_AVI_JPEG, FRAMEBUFF_SOURCE_CAMERA0, (uint8_t)~0, 1, frq ? 1 : 0, NULL, 0);
                if (g_at_avi_msi)
                {
                    msi_add_output(NULL, AUTO_JPG, R_AT_AVI_JPEG);
                    #if AUDIO_EN
                    if (frq)
                    {
                        auadc_msi_add_output(AUSYS_AUAD, R_AT_AVI_JPEG);
                    }
                    #endif
                    msi_do_cmd(g_at_avi_msi, MSI_CMD_MEDIA_CTRL, MSI_MEDIA_CTRL_SET_RECORD_SEC, 30);
                    msi_do_cmd(g_at_avi_msi, MSI_CMD_MEDIA_CTRL, MSI_MEDIA_CTRL_RECORD_START, 0);
                }
            }
    }
	#endif

    return 0;
}
