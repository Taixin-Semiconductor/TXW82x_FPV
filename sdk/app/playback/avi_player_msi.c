#include "avi_player_msi.h"

/*****************************************************************************
 * AVI1.0
*****************************************************************************/

// data申请空间函数
#define STREAM_MALLOC               av_psram_malloc
#define STREAM_FREE                 av_psram_free
#define STREAM_ZALLOC               av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC          av_malloc
#define STREAM_LIBC_FREE            av_free
#define STREAM_LIBC_ZALLOC          av_zalloc

#define AVI_PLAYER_MAX_TX_NUM       8

#define AUDIO_TMP_BUF_SIZE          (1024)

struct player_msg_s
{
    void *task;
    struct msi *msi;
    struct fbpool tx_pool;
    uint32_t magic;
    struct avi_msg_s *avi_msg;
    struct fast_index_list *g_fast_index_list;

    uint32_t audio_frame_number;            //读取索引的长度
    uint32_t audio_frame_number_send;       //已经播放的长度
    uint32_t frame_number;  //当前读取的帧
    uint32_t not_read_size; //剩余未读取的空间长度
    uint32_t cache_addr;        //读取下一次的偏移地址

    struct framebuff *data_tmp;
    struct framebuff *audio_data_tmp;

    int32_t locate_frame_number;      //跳转视频的偏移
    uint8_t running: 1, thread_stop : 1, speed_flag:1, rev: 5;
};

static void avi_player_thread(void *d)
{
    struct player_msg_s *msg  = (struct player_msg_s *)d;
    struct msi *msi = msg->msi;
    struct avi_msg_s *avi_msg = msg->avi_msg;
    uint32_t magic = 0;
    uint8_t *audio_tmp_buf = (uint8_t *)STREAM_LIBC_MALLOC(AUDIO_TMP_BUF_SIZE);
    uint32_t now_audio_offset = 0;
    uint32_t video_time = 0;
    uint32_t audio_time = 0;
    if (!avi_msg->sample)
    {
        audio_time = ~0;        //没有音频
    }
    while (!msg->thread_stop)
    {
        if (!msi->enable)
        {
            //这里是否可以考虑先去建立部分快速索引?
            goto avi_player_thread_end;
        }
        //正常这里去建立快速索引
        gen_fast_index_list(avi_msg);

       //判断一下是否要快进或者快退?
        //所以先设置locate_frame_number,再配置speed_flag
        if (msg->speed_flag)
        {
            msg->speed_flag = 0;
            locate_avi_index(avi_msg,msg->locate_frame_number);
            msg->frame_number = msg->locate_frame_number;
            magic = msg->magic;
            video_time = msg->frame_number * 1000 / avi_msg->fps;
            
            //os_printf("msg->frame_number:%d\n",msg->frame_number);

            if (avi_msg->sample)
            {
                //重置音频,音频是跟着视频走的,表示音频和video同步
                msg->audio_frame_number_send = video_time * avi_msg->sample * 2 / 1000;;
                audio_time = msg->audio_frame_number_send * 500 / avi_msg->sample;
                now_audio_offset = 0;

                //实际音频索引的位置应该在前面,意思是快速索引在前面,实际播放时间可能在后面,后面需要慢慢去轮询读取到对应的音频
                msg->audio_frame_number = avi_msg->audio_frame_num;
            }

        }

        //如果视频比音频播放前,就需要去读取音频了
        if (video_time > audio_time)
        {
            //os_printf("goto avi_player_thread_audio_start:%d\n",__LINE__);
            goto avi_player_thread_audio_start;
        }


        if (!msg->data_tmp)
        {
            msg->data_tmp = fbpool_get(&msg->tx_pool, 0, msi);
            if (!msg->data_tmp)
            {
                //os_printf("goto avi_player_thread_audio_start:%d\n",__LINE__);
                // _os_printf("P2");
                goto avi_player_thread_audio_start;
            }
        }

 
        
        //理论要判断当前播放的视频帧是否已经最大,最大就直接退出
        uint32_t offset;
        uint32_t size = 0;
        avi_read_next_index(avi_msg, &offset, &size, &msg->frame_number);
        if (!size)
        {
            // os_printf("goto avi_player_thread_audio_start:%d\n",__LINE__);
            goto avi_player_thread_audio_start;
        }
        uint8_t *fb_video_data = (uint8_t *)STREAM_MALLOC(size);
        if (fb_video_data)
        {
            osal_fseek(avi_msg->fp, offset);
            osal_fread(fb_video_data, 1, size, avi_msg->fp);
            sys_dcache_clean_range((uint32_t*)fb_video_data, size);
            msg->data_tmp->data = (uint8_t *)fb_video_data;
            msg->data_tmp->mtype = F_JPG;
            msg->data_tmp->stype = FSTYPE_YUV_P0;
            //转换成ms
            //os_printf("msg->avi_msg->fps:%d\t%d\n",msg->avi_msg->fps,msg->frame_number*1000/msg->avi_msg->fps);
            msg->data_tmp->time = msg->frame_number * 1000 / msg->avi_msg->fps;
            msg->data_tmp->len = size;

            video_time = msg->frame_number * 1000 / msg->avi_msg->fps;

            msg->data_tmp->datatag = magic;
            msi_output_fb(msi, msg->data_tmp);
            msg->data_tmp = NULL;
            _os_printf("(V:%d)",size);
            
        }

avi_player_thread_audio_start:

        if (!avi_msg->sample)
        {
            goto avi_player_thread_end;
        }
        if (video_time < audio_time)
        {
            //os_printf("audio_time:%d\tvideo_time:%d\n",audio_time,video_time);
            goto avi_player_thread_end;
        }


        if (!msg->audio_data_tmp)
        {
            msg->audio_data_tmp = fbpool_get(&msg->tx_pool, 0, msi);
            if (!msg->audio_data_tmp)
            {
                goto avi_player_thread_end;
            }
        }
        //先去获取一个音频节点
        if (msg->audio_data_tmp)
        {

            //os_printf("audio_time:%d\tvideo_time:%d\n",audio_time,video_time);
            //为stream申请音频的空间,然后读取音频发送
            uint8_t *fb_audio_data = (uint8_t *)STREAM_LIBC_MALLOC(AUDIO_TMP_BUF_SIZE);
            //申请到空间,就将音频先读取到缓冲区,没有空间,则下次重新尝试
            if (fb_audio_data)
            {
                uint32_t max_read_len;
                uint32_t read_len = 0;
                max_read_len = AUDIO_TMP_BUF_SIZE - now_audio_offset;
                while (max_read_len)
                {
                    
                    //读取够音频数据长度,然后通过流发送,然后再将剩下的拷贝到缓冲区,由于
                    avi_read_next_audio_index(avi_msg, &offset, &size, &msg->audio_frame_number);
                    //音频已经读取完毕了
                    if (!size)
                    {
                        break;
                    }
                    osal_fseek(avi_msg->fp, offset);
                    read_len = (max_read_len > size) ? size : max_read_len;
                    osal_fread(audio_tmp_buf + now_audio_offset, 1, read_len, avi_msg->fp);
                    now_audio_offset += read_len;
                    max_read_len -= read_len;
                }

                if (!now_audio_offset) {
                    avi_msg->sample = 0;
                    audio_time = ~0;
                    msi_delete_fb(NULL, msg->audio_data_tmp);
                    msg->audio_data_tmp = NULL;
                    goto avi_player_thread_end;
                }

                //流发送,先拷贝数据
                hw_memcpy(fb_audio_data, audio_tmp_buf, now_audio_offset);
                msg->audio_data_tmp->data = (uint8_t *)fb_audio_data;
                msg->audio_data_tmp->mtype = F_AUDIO;
                msg->audio_data_tmp->stype = FSTYPE_AUDIO_PCM;
                //转换成ms
                //os_printf("msg->avi_msg->fps:%d\t%d\n",msg->avi_msg->fps,msg->frame_number*1000/msg->avi_msg->fps);
                //设置音频的时间
                msg->audio_data_tmp->time = msg->audio_frame_number_send * 500 / avi_msg->sample;
                msg->audio_data_tmp->len = now_audio_offset;
                msg->audio_data_tmp->datatag = magic;
                _os_printf("(A:%d\t%d)",now_audio_offset,avi_msg->sample);

                msi_output_fb(msi, msg->audio_data_tmp);

                msg->audio_data_tmp = NULL;
                msg->audio_frame_number_send += now_audio_offset;

                audio_time = msg->audio_frame_number_send * 500 / avi_msg->sample;

                //将剩余的数据拷贝
                now_audio_offset = size - read_len;
                //os_printf("remain:%d\n",now_audio_offset);
                //如果有剩余,紧接着读取数据就可以了
                if (now_audio_offset)
                {
                    osal_fread(audio_tmp_buf, 1, now_audio_offset, avi_msg->fp);
                }
            }
        }

    avi_player_thread_end:
        os_sleep_ms(1);
    }
    msg->running = 0;

    if (audio_tmp_buf)
    {
        STREAM_LIBC_FREE(audio_tmp_buf);
    }
}


static int32_t avi_player_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct player_msg_s *msg = (struct player_msg_s *)msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_PRE_DESTROY:
        {
            if(msg) {
                msg->thread_stop = 1;

                while(msg->running)
                {
                    os_sleep_ms(1);
                }

                if(msg->data_tmp)
                {
                    msi_delete_fb(NULL, msg->data_tmp);
                    msg->data_tmp = NULL;
                }

                if(msg->audio_data_tmp)
                {
                    msi_delete_fb(NULL, msg->audio_data_tmp);
                    msg->audio_data_tmp = NULL;
                }
            }
        }
            break;

        case MSI_CMD_POST_DESTROY:
        {
            if(msg) {

                if(msg->avi_msg)
                {
                    avi_deinit(msg->avi_msg);
                    msg->avi_msg = NULL;
                }

                fbpool_destroy(&msg->tx_pool);

                STREAM_LIBC_FREE(msg);

                msi->priv = NULL;
            }
        }
            break;

        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            //os_printf("player free fb:0x%x\n",fb);
            if (fb->data)
            {
                if (fb->mtype == F_AUDIO) {
                    STREAM_LIBC_FREE(fb->data);
                } else if (fb->mtype == F_JPG) {
                    STREAM_FREE(fb->data);
                }
                fb->data = NULL;
            }
            fbpool_put(&msg->tx_pool, fb);
            // 不需要内核去释放fb
            ret = RET_OK + 1;
        }
            break;

        case MSI_CMD_PLAYER:
        {
            uint32_t opcode = param1;
            uint32_t arg    = param2;
            switch(opcode)
            {
                case MSI_FORWARD_INDEX:
                {
                    //快进3s的时间
                    uint32_t play_time = arg + 3000;
                    msg->locate_frame_number = play_time * msg->avi_msg->fps / 1000;
                    //要判断是否已经到最后
                    if (msg->locate_frame_number > msg->avi_msg->avi.dwTotalFrame )
                    {
                        msg->locate_frame_number = msg->avi_msg->avi.dwTotalFrame;
                    }
                    //os_printf("msg->locate_frame_number:%d\targ:%d\n",msg->locate_frame_number,arg);
                    ret = msg->locate_frame_number * 1000 / msg->avi_msg->fps;
                }
                    break;

                case MSI_REWIND_INDEX:
                {
                    uint32_t play_time = 0;
                    if (arg > 3000)
                    {
                        play_time = arg - 3000;
                    }
                    msg->locate_frame_number = play_time * msg->avi_msg->fps / 1000;
                    //os_printf("msg->locate_frame_number:%d\targ:%d\n",msg->locate_frame_number,arg);
                    ret = msg->locate_frame_number * 1000 / msg->avi_msg->fps;
                }
                    break;

                case MSI_PLAYER_MAGIC:
                {
                    msg->magic = arg;
                    msg->speed_flag = 1;
                }
                    break;

                default:
                    break;
            }
        }

        default:
            break;
    }

    return ret;
}


struct msi *avi_player_init(const char *stream_name, const char *filename)
{
    struct avi_msg_s *avi_msg = NULL;
    struct msi *msi = NULL;
    avi_msg = avi_read_init(filename);
    os_printf("avi_msg:%X\n",avi_msg);
    //创建workqueue去不停读取视频数据,然后发送出去

    msi = msi_new(stream_name, 0, NULL);
    if (!msi)
    {
        avi_deinit(avi_msg);
        return NULL;
    }
    struct player_msg_s *msg = (struct player_msg_s *)msi->priv;

    if (!msg)
    {
        msg = (struct player_msg_s*)STREAM_LIBC_ZALLOC(sizeof(struct player_msg_s));
        msi->priv = (void*)msg;
        msg->msi = msi;
        msg->avi_msg = (struct avi_msg_s*)avi_msg;
        msg->magic = 0;
        msg->not_read_size = msg->avi_msg->idx1_size;
        msg->cache_addr = msg->avi_msg->idx1_addr;
        msg->running = 1;

        fbpool_init(&msg->tx_pool, AVI_PLAYER_MAX_TX_NUM);
        msi->action = (msi_action)avi_player_action;
        msi->enable = 1;

        msg->task = os_task_create(msi->name, avi_player_thread, (void*)msg, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);

    }
    else
    {
        msi_destroy(msi);
        msi = NULL;
        avi_deinit(avi_msg);
    }


    return msi;
}