#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "audio_adc.h"

#if HUWEN_WAKEUP_EN
#define HUWEN_APP_DEBUG_RECORDING   0
#define HUWEN_APP_DEBUG_RECORDING_TIME   (60 * 1000)

#define HUWEN_CHECK_CNT             10
#define HUWEN_TARGET_LEN            2048
#define HUWEN_INTERNAL_GAIN         6

extern void *huwen_mem_alloc(uint32 size);
extern void huwen_mem_free(void *ptr);

void *huwen_malloc(uint32 size) { return huwen_mem_alloc(size);}
void *huwen_zalloc(size_t size)
{
    void *ptr = huwen_malloc(size);
    if (ptr) {
        os_memset(ptr, 0, size);
    }
    return ptr;
}
void  huwen_free(void *ptr) { huwen_mem_free(ptr);}
#define huwen_app_printf(fmt, ...) os_printf(KERN_ERR "%s:%d:"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

typedef enum {
    HUWEN_SUCCESS                   = 0,
    HUWEN_ERROR                     = -1,
    HUWEN_AGAIN                     = -2,
    HUWEN_INVALID_PARAM             = -3,
    HUWEN_WAKEUP                    = -4,
} HuwenResult;

struct huwen_ref_ctx_t {
    char  *ref_buf;
    struct framebuff *ref_fb;      //当前正在处理的fb
    uint16 ref_fb_off;             //当前处理的fb的数据偏移
    uint32  ref_data_len;
};
static struct huwen_ref_ctx_t huwen_ref_ctx = {0};

/**
 * @brief 初始化唤醒模块
 * @param handle 句柄指针
 * @param mpo_level 0: 低功耗模式 1: 高功耗模式
 * @return int 0: 成功 -1: 失败
 */
extern int HuwenWakeupInit(void **handle, int mpo_level);
/**
 * @brief 写入麦克风数据和回踩数据，获取AEC输出数据
 * @param handle 句柄指针
 * @param mic 指向麦克风采集的音频数据 mic1 0~1023  mic2 1024~2047
 * @param ref 指向回踩音频数据 ref1 0~1023  ref2 1024~2047
 * @param aecout AEC输出数据指针
 * @param len 数据长度
 * @return int 0: 成功 -1: 失败
 */
extern int HuwenWakeupWrite(void *handle, char *mic, char *ref, short *aecout, int len);
/**
 * @brief 释放唤醒模块资源
 * @param handle 句柄指针
 * @return int 0: 成功 -1: 失败
 */
extern int HuwenWakeupUnit(void *handle);

extern uint32 __huwen_data_start;
extern uint32 __huwen_data_end;
extern uint32 _huwen_lma_start;

#if HUWEN_APP_DEBUG_RECORDING
void *huwen_mic_fp = NULL;
void *huwen_ref_fp = NULL;
void *huwen_aec_fp = NULL;
#endif

__ram int arr_mul_arr_dsp(unsigned char* in1, signed char* in2, int len)
{
	int64 tv1, tv2;
	int sum = 0;	
	__ASM volatile(
		"andi			%[tv1],%[len],0x3		\n\t"
		"bez            %[tv1],3f               \n\t"
		"1:                                  	\n\t"
		"ldbi.b 		%[tv1],(%[v1])      	\n\t"     
		"ldbi.bs 		%[tv2],(%[v2])      	\n\t"	
		"2:                                  	\n\t"	
		"mulall.s16.s   %[sum],%[tv1],%[tv2]    \n\t"		
		"bloop  		%[len],1b,2b            \n\t"
		"br 			6f						\n\t"
		
		"3:                                  	\n\t"
		"asri			%[len],%[len],2			\n\t"
		"4:                                  	\n\t"		
		"ldbi.w         %[tv1],(%[v1])          \n\t"		
		"ldbi.w         %[tv2],(%[v2])          \n\t"
		"pext.u8.e		%[tv1],%[tv1]			\n\t"
		"pext.s8.e		%[tv2],%[tv2]			\n\t"
		"mulaca.s16.s	%[sum],%[tv1],%[tv2]	\n\t"
		"5:                                  	\n\t"
		"mulaca.s16.s   %[sum],%R[tv1],%R[tv2]  \n\t"		
		"bloop  		%[len],4b,5b            \n\t"
		"6:                                  	\n\t"		
         :[sum]	"+r"(sum),
          [tv1] "=&r"(tv1),
          [tv2] "=&r"(tv2),		 
          [len] "+r"(len),
          [v1]  "+r"(in1),
          [v2]  "+r"(in2) 
     );
	 return sum;
}

static short aecout_buf[HUWEN_TARGET_LEN/2] = {0};
static HuwenResult huwen_wakeup_process(void *handle, struct msi *mic_msi, struct msi *ref_msi)
{
    int32 ret = HUWEN_SUCCESS;
    uint32 free_space = 0;
    uint32 copy_len = 0;
    char *mic_data = NULL;
    char *ref_data = NULL;
    struct framebuff *mic_fb = NULL;

    if (handle == NULL || mic_msi == NULL) {
        return HUWEN_INVALID_PARAM;
    }

    if (ref_msi) {
        if (huwen_ref_ctx.ref_data_len == HUWEN_TARGET_LEN) {
            ref_data = huwen_ref_ctx.ref_buf;
        } else if (huwen_ref_ctx.ref_data_len < HUWEN_TARGET_LEN) {
            free_space = HUWEN_TARGET_LEN - huwen_ref_ctx.ref_data_len;
            if (huwen_ref_ctx.ref_fb == NULL) {
                huwen_ref_ctx.ref_fb = msi_get_fb(ref_msi, 20);
                if (huwen_ref_ctx.ref_fb) {
                    copy_len = (huwen_ref_ctx.ref_fb->len < free_space) ? huwen_ref_ctx.ref_fb->len : free_space;
                }
            } else {
                copy_len = (huwen_ref_ctx.ref_fb->len - huwen_ref_ctx.ref_fb_off < free_space) ? huwen_ref_ctx.ref_fb->len - huwen_ref_ctx.ref_fb_off : free_space;
            }
            if (huwen_ref_ctx.ref_fb) {
                hw_memcpy(huwen_ref_ctx.ref_buf + huwen_ref_ctx.ref_data_len, huwen_ref_ctx.ref_fb->data, copy_len);
                huwen_ref_ctx.ref_data_len += copy_len;

                if (huwen_ref_ctx.ref_data_len == HUWEN_TARGET_LEN) {
                    ref_data = huwen_ref_ctx.ref_buf;
                    //huwen_app_printf("audio ref_fb data: %d:%d(%d)\n", huwen_ref_ctx.ref_data_len, huwen_ref_ctx.ref_fb->len, huwen_ref_ctx.ref_fb->time);
                }

                huwen_ref_ctx.ref_fb_off += copy_len;
                if (huwen_ref_ctx.ref_fb->len <= huwen_ref_ctx.ref_fb_off) {
                    msi_delete_fb(NULL, huwen_ref_ctx.ref_fb);
                    huwen_ref_ctx.ref_fb = NULL;
                    huwen_ref_ctx.ref_fb_off = 0;
                }
            } else {
                goto cleanup;
            }
        } else {
            huwen_app_printf("ref_data_len error!!! len: %d, target len: %d\n", huwen_ref_ctx.ref_data_len, HUWEN_TARGET_LEN);
            ret = HUWEN_ERROR;
            goto cleanup;
        }
    } else {
        ref_data = huwen_zalloc(HUWEN_TARGET_LEN);
        if (ref_data == NULL) {
            huwen_app_printf("huwen_zalloc ref_data error!!!\n");
            goto cleanup;
        }
    }
    if (ref_data == NULL) { goto cleanup; }

    mic_fb = msi_get_fb(mic_msi, 20);
    if (mic_fb) {
        if (mic_fb->len != HUWEN_TARGET_LEN) {
            huwen_app_printf("mic_fb len error!!! len: %d, target len: %d\n", mic_fb->len, HUWEN_TARGET_LEN);
            ret = HUWEN_ERROR;
            goto cleanup;
        }
        mic_data = (char *)mic_fb->data;
        //huwen_app_printf("audio mic_fb data: %d(%d)\n", mic_fb->len, mic_fb->time);
    } else {
        goto cleanup;
    }
    if (mic_data == NULL) { goto cleanup; }
#if HUWEN_APP_DEBUG_RECORDING
    osal_fwrite((char *)mic_data, HUWEN_TARGET_LEN, 1, huwen_mic_fp);
    osal_fwrite((char *)ref_data, HUWEN_TARGET_LEN, 1, huwen_ref_fp);
#endif
    ret = HuwenWakeupWrite(handle, mic_data, ref_data,
                           aecout_buf, 1024);
#if HUWEN_APP_DEBUG_RECORDING
    osal_fwrite((char *)aecout_buf, HUWEN_TARGET_LEN, 1, huwen_aec_fp);
#endif
    if (ret >= 0) {
        huwen_app_printf("Wakeup detect result: %d\n", ret);
        ret = HUWEN_WAKEUP;
    } else {
        ret = HUWEN_SUCCESS;
    }
    huwen_ref_ctx.ref_data_len = 0;

cleanup:
    if (!ref_msi && ref_data) {huwen_free(ref_data);}
    if (mic_fb) {msi_delete_fb(NULL, mic_fb);}
    return ret;
}

void huwen_wakeup_main(void)
{
    int32 ret = -1;
    void *handle = NULL;

    struct msi *huwen_mic_msi = NULL;
    struct msi *huwen_ref_msi = NULL;

    uint32 huwen_data_start  = (uint32)&__huwen_data_start;
    uint32 huwen_data_end    = (uint32)&__huwen_data_end;
    uint32 huwen_lma_start = (uint32)&_huwen_lma_start;

    if (huwen_data_end != huwen_data_start) {
        os_memcpy((void *)huwen_data_start, (void *)huwen_lma_start, huwen_data_end - huwen_data_start);
    }

    ret = HuwenWakeupInit(&handle, HUWEN_INTERNAL_GAIN);
    if (ret != 0) {
        huwen_app_printf("HuwenWakeupInit error!!!\n");
        goto exit;
    }

    huwen_mic_msi = msi_new("HUWEN_MSI", 32, NULL);
    if (huwen_mic_msi == NULL) {
        huwen_app_printf("msi_new huwen_mic_msi error!!!\n");
        goto exit;
    }
    auadc_msi_add_output(AUSYS_AUAD, huwen_mic_msi->name);
    huwen_mic_msi->fb_limits.counter = 32;
    huwen_mic_msi->enable = 1;

    huwen_ref_msi = msi_new("huwen_ref_msi", 32, NULL);
    if (huwen_ref_msi) {
        if (huwen_ref_ctx.ref_buf == NULL) {
            huwen_ref_ctx.ref_buf = huwen_zalloc(HUWEN_TARGET_LEN);
            if (huwen_ref_ctx.ref_buf == NULL) {
                huwen_app_printf("huwen_zalloc huwen_ref_ctx.ref_buf error!!!\n");
                goto exit;
            }
            huwen_ref_ctx.ref_data_len = 0;
        }

        msi_add_output(NULL, "dac_msg", NULL, huwen_ref_msi->name);
        huwen_ref_msi->fb_limits.counter = 32;
        huwen_ref_msi->enable = 1;
    }

    huwen_app_printf("huwen wakeup run gain %d\n", HUWEN_INTERNAL_GAIN);
#if HUWEN_APP_DEBUG_RECORDING
    huwen_mic_fp = (void *)osal_fopen("mic.pcm", "wb+");
    huwen_ref_fp = (void *)osal_fopen("ref.pcm", "wb+");
    huwen_aec_fp = (void *)osal_fopen("aec.pcm", "wb+");
    uint64 time = os_jiffies();
#endif
    while (huwen_mic_msi != NULL) {
        ret = huwen_wakeup_process(handle, huwen_mic_msi, huwen_ref_msi);
        if (ret == HUWEN_WAKEUP) {
            SYSEVT_NEW_ASR_EVT(SYSEVT_ASR_WAKEUP, 0);
        } else if (ret != HUWEN_SUCCESS) {
            huwen_app_printf("huwen_wakeup_process error!!!ret = %d\n", ret);
            break;
        }
#if HUWEN_APP_DEBUG_RECORDING
        if (os_jiffies_to_msecs(os_jiffies() - time) >= HUWEN_APP_DEBUG_RECORDING_TIME) {
            osal_fclose(huwen_mic_fp);
            osal_fclose(huwen_ref_fp);
            osal_fclose(huwen_aec_fp);
            huwen_app_printf("huwen debug!\n");
            break;
        }
#endif
    }

exit:
    huwen_app_printf("huwen wakeup exit\n");
    if (huwen_ref_ctx.ref_buf) { huwen_free(huwen_ref_ctx.ref_buf); }
    if (huwen_ref_ctx.ref_fb) { msi_delete_fb(NULL, huwen_ref_ctx.ref_fb); }
    if (huwen_mic_msi) { msi_destroy(huwen_mic_msi); }
    if (huwen_ref_msi) { msi_destroy(huwen_ref_msi); }
    if (handle) { HuwenWakeupUnit(handle); }
}

struct os_task huwen_wakeup_task;
void huwen_wakeup_init(void)
{
    OS_TASK_INIT("HUWEN_WAKEUP", &huwen_wakeup_task, huwen_wakeup_main, NULL, OS_TASK_PRIORITY_ABOVE_NORMAL, NULL, 2048);
}
#endif // HUWEN_WAKEUP_EN

