/***************************************************
    该demo主要是使用AT命令拍一张照片,前提要将jpeg打开
***************************************************/
#include "basic_include.h"
#include "stream_frame.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_heap.h"
#include "osal_file.h"
void at_save_photo_thread(void *d);
struct AT_PHOTO
{
    os_task_t task;
    uint32_t photo_num;
    uint8_t filename_prefix[4];
    uint8_t running;

};

static struct AT_PHOTO *photo_s = NULL;
int32 demo_atcmd_save_photo(const char *cmd, char *argv[], uint32 argc)
{
	#if OPENDML_EN &&  SDH_EN && FS_EN
    if(argc < 2)
    {
        os_printf("%s argc too small:%d,should more 2 arg\n",__FUNCTION__,argc);
        return 0;
    }
    if(os_atoi(argv[0]) == 0)
    {
        if(photo_s)
        {
            photo_s->running = 0;
        }
        else
        {
            os_printf("%s takephoto num err:%d\n",__FUNCTION__,os_atoi(argv[0]));
        }
        
        return 0;
    } 
	
    if(photo_s)
    {
        os_printf("%s already running\n",__FUNCTION__);
        return 0;
    }
    photo_s = av_malloc(sizeof(struct AT_PHOTO));
    if(photo_s)
    {
        memset(photo_s,0,sizeof(struct AT_PHOTO));
        //连续拍照多少张
        photo_s->photo_num = os_atoi(argv[0]);
        int prefix_len = strlen(argv[1]);
        if(prefix_len > 3)
        {
            os_memcpy(photo_s->filename_prefix,argv[1],3);
        }
        else
        {
            os_memcpy(photo_s->filename_prefix,argv[1],prefix_len);
        }

        //创建拍照的任务
        OS_TASK_INIT("at_photo", &photo_s->task, at_save_photo_thread, (uint32)photo_s, OS_TASK_PRIORITY_NORMAL, NULL, 1024);  
    }
	#endif
    return 0;
}


void at_save_photo_thread(void *d)
{
    struct framebuff *get_f = NULL;
    struct AT_PHOTO *p_s = (struct AT_PHOTO *)d;
    struct msi *s = NULL;
    uint32_t flen = 0;
    char filename[64] = {0};
    s = msi_new(R_AT_SAVE_PHOTO, 8, NULL);
    if(!s)
    {
        goto at_save_photo_thread_end;
    }
    s->enable = 1;
    p_s->running = 1;
    void *fp = NULL;
    uint32_t err_count = 0;
    uint32_t total_len = 0;
    uint8_t *frame_buf_addr = NULL;

    while(p_s->photo_num && p_s->running)
    {
        get_f = msi_get_fb(s, 0);
        if(get_f)
        {
            err_count = 0;
            os_sprintf(filename,"0:photo/%s_%04d.jpg",p_s->filename_prefix,(uint32_t)os_jiffies()%9999);
            os_printf("filename:%s\n",filename);
            fp = osal_fopen((const char *)filename,"wb+");
            if(!fp)
            {
                goto at_save_photo_thread_end;
            }
            flen = get_f->len;
            total_len = flen;
            frame_buf_addr = get_f->data;

            osal_fwrite((void*)frame_buf_addr,1,total_len,fp);
            
            msi_delete_fb(NULL, get_f);
            get_f = NULL;
            osal_fclose((F_FILE*)fp);
            p_s->photo_num--;
        }
        else
        {
            err_count++;
            os_sleep_ms(1);
            if(err_count++ > 1000)
            {
                goto at_save_photo_thread_end;
            }
        }
        
    }
    at_save_photo_thread_end:
    if(get_f)
    {
        msi_delete_fb(NULL, get_f);
        get_f = NULL;
    }
    
    if(s)
    {
        msi_destroy(s);
    }
    p_s->running = 0;
    av_free((void*)p_s);
    photo_s = NULL;
}
