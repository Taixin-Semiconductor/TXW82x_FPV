#include "basic_include.h"
#include "keyWork.h"
#include "keyScan.h"
#include "osal/msgqueue.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "stream_define.h"
#include "osal/work.h"
#include "user_work/user_work.h"
#include "audio_msi/audio_adc.h"
#include "fs/fatfs/osal_file.h"
#include "file_process.h"
struct msi *mp4_encode_msi2_init(const char *mp4_msi_name, uint8_t srcID, uint8_t filter_type, uint8_t rec_time, uint32_t audio_encode, struct file_process *file_process, uint8_t mode);
struct msi *avi_encode_msi_init(const char *avi_msi_name, uint16_t filter_type, uint8_t rec_time);
struct adkey_work
{
    struct os_work     wk;
    struct os_msgqueue key_demo_msg;
    struct msi        *mp4_msi;
    struct msi        *h264_msi;
    struct msi        *avi_msi;
};

static void *adkey_demo_mp4_file(struct file_process *file_process, char *filename, char *file_path, uint32_t file_size)
{
    os_sprintf(filename, "0:key_mp4/%016d.mp4", (uint32_t) os_jiffies());
    void *fp = osal_fopen_auto((const char *) filename, "w+", 0);
    return fp;
}
static uint32_t push_key_demo(struct key_callback_list_s *callback_list, uint32_t keyvalue, uint32_t extern_value)
{
    struct adkey_work *adkey_work = (struct adkey_work *) callback_list->priv;
    os_msgq_put(&adkey_work->key_demo_msg, keyvalue, 0);
    os_run_work(&adkey_work->wk);
    return 0;
}

int32_t adkey_demo_work(struct os_work *work)
{
    struct adkey_work *adkey_work = (struct adkey_work *) work;
    uint32_t           val;
    int32              err;
    val = os_msgq_get2(&adkey_work->key_demo_msg, 0, &err);
    if (!err)
    {
        if ((val & 0xff) == KEY_EVENT_SUP)
        {
            switch (val >> 8)
            {
                case AD_UP:
                    if (!adkey_work->mp4_msi)
                    {

                        struct file_process adkey_demo_file_process = {
                                .loop        = NULL,
                                .rec_path    = NULL,
                                .ext_name    = NULL,
                                .create_file = adkey_demo_mp4_file,
                                .loop_free   = NULL,
                                .lock_file   = NULL,
                        };

                        adkey_work->h264_msi = msi_find(AUTO_H264, 1);
                        adkey_work->mp4_msi  = mp4_encode_msi2_init("ad_mp4", FRAMEBUFF_SOURCE_CAMERA0, FSTYPE_H264_VPP_DATA0, 1, 0, &adkey_demo_file_process, 0);
                        if (adkey_work->h264_msi && adkey_work->mp4_msi)
                        {
                            msi_add_output(adkey_work->h264_msi, NULL, adkey_work->mp4_msi->name);
                        }
                        else
                        {
                            if (adkey_work->h264_msi)
                            {
                                msi_put(adkey_work->h264_msi);
                                adkey_work->h264_msi = NULL;
                            }
                            if (adkey_work->mp4_msi)
                            {
                                msi_destroy(adkey_work->mp4_msi);
                                adkey_work->mp4_msi = NULL;
                            }
                        }
                    }
                    else
                    {
                        if (adkey_work->h264_msi)
                        {
                            msi_del_output(adkey_work->h264_msi, NULL, adkey_work->mp4_msi->name);
                            msi_put(adkey_work->h264_msi);
                            adkey_work->h264_msi = NULL;
                        }
                        if (adkey_work->mp4_msi)
                        {
                            msi_destroy(adkey_work->mp4_msi);
                            adkey_work->mp4_msi = NULL;
                        }
                    }

                    break;

                case AD_DOWN:
                {
                    if (!adkey_work->avi_msi)
                    {
                        adkey_work->avi_msi = avi_encode_msi_init("ad_avi", (uint16_t) ~0, 60 * 1);
                        if (adkey_work->avi_msi)
                        {
                            msi_add_output(NULL, R_GEN420_JPG_RECODE, adkey_work->avi_msi->name);
                            auadc_msi_add_output(AUSYS_AUAD, adkey_work->avi_msi->name);
                        }
                    }
                    else
                    {
                        msi_del_output(NULL, R_GEN420_JPG_RECODE, adkey_work->avi_msi->name);
                        auadc_msi_del_output(AUSYS_AUAD, adkey_work->avi_msi->name);
                        msi_destroy(adkey_work->avi_msi);
                        adkey_work->avi_msi = NULL;
                    }
                }
                break;
                default:
                    break;
            }
        }
    }
    return 0;
}
void adkey_work_init()
{
    struct adkey_work *adkey_work = (struct adkey_work *) os_zalloc(sizeof(struct adkey_work));
    os_msgq_init(&adkey_work->key_demo_msg, 5);
    OS_WORK_INIT(&adkey_work->wk, adkey_demo_work, 0);
    add_keycallback(push_key_demo, adkey_work);
}