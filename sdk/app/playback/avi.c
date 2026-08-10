#include "basic_include.h"
#include "osal_file.h"
#include "avi.h"
#include "utlist.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE av_free
#define STREAM_LIBC_ZALLOC av_zalloc

//目前没有做兼容性,所以只是识别strh和strl,也不考虑顺序,读取完这两个段就退出
void read_strh_strf(void *fp,struct avi_msg_s *avi_msg)
{
    struct chunk_s str;
    AVIStreamHeader stream;
    WAVEFORMATEX wav;
    uint32_t offset;
    uint8_t video_or_audio = 0;
    while(1)
    {
        osal_fread(&str,1,sizeof(str),fp);
        offset = osal_ftell(fp);
        if(strncasecmp((void*)&(str.dwFourCC)  ,"strh",4) == 0)
        {
            //分析流AVIStreamHeader
            osal_fread(&stream,1,sizeof(stream),fp);
            if(strncasecmp((void*)&(stream.fccType)  ,"vids",4) == 0)
            {
                if(avi_msg->offset == 0)
                {
                    avi_msg->v_id = 0x63643030;     // "00dc"
                }
                else
                {
                    avi_msg->v_id = 0x63643130;     // "01dc"
                }
                video_or_audio = 0;
                avi_msg->fps = (stream.dwRate + (stream.dwScale >> 2))/stream.dwScale;
                avi_msg->offset++;
            }
            else if(strncasecmp((void*)&(stream.fccType)  ,"auds",4) == 0)
            {
                if(avi_msg->offset == 0)
                {
                    avi_msg->a_id = 0x62773030;     // "00wb"
                }
                else
                {
                    avi_msg->a_id = 0x62773130;     // "01wb"
                }
                video_or_audio = 1;
                avi_msg->offset++;
                
            }
            //偏移
            osal_fseek(fp,offset + str.size);
        }
        else if(strncasecmp((void*)&(str.dwFourCC)  ,"strf",4) == 0)
        {
            //视频就不要读取了
            if(video_or_audio == 0)
            {
                
            }
            //音频就读取看看采样率
            else if(video_or_audio == 1)
            {
                osal_fread(&wav,1,sizeof(wav),fp);
                //仅仅支持pcm
                if(wav.wFormatTag == 1)
                {
                    avi_msg->sample = wav.nSamplesPerSec;
                }
                else
                {
                    avi_msg->sample = 0;
                }
                
            }
            //分析流
            osal_fseek(fp,offset + str.size);
            //默认读取到strf就退出,所以遇到strf在前面,就会有异常咯
            break;
        }
        else
        {
            break;
        }
    }

}

void *avi_read_init(const char *filename)
{
    struct avi_msg_s *avi_msg = NULL;
    int err = 1;
    char data[32];
    char chunk_name[5] = {0};
    uint32_t readlen;
    uint32_t chunk_size;
    uint32_t seek_offset = 0;
    uint32_t hdrl_offset = 0;
    uint32_t offset;
    void *fp = osal_fopen(filename,"rb");
    if(!fp)
    {
        goto avi_read_init_end;
    }
    readlen = osal_fread(data,1,12,fp);
    if(readlen < 12)
    {
        
        goto avi_read_init_end;
    }
    if( strncasecmp(data  ,"RIFF",4) !=0 ||strncasecmp(data+8,"AVI ",4) !=0 )
    {
        os_printf("avi file err\n");
        goto avi_read_init_end;
    }
    avi_msg = (struct avi_msg_s*)STREAM_LIBC_ZALLOC(sizeof(struct avi_msg_s));
    if(!avi_msg)
    {
        err = 2;
        goto avi_read_init_end;
    }

    avi_msg->fp = fp;

    while(1)
    {
        readlen = osal_fread(data,1,8,fp);
        if(readlen<8)
        {
            if(readlen)
            {
                os_printf("%s:%d err!!!!!!!!!!!!!\t%d\tosal_ftell:%d\n",__FUNCTION__,__LINE__,readlen,osal_ftell(fp));
                goto avi_read_init_end;
                
            }
            else
            {
                err = 0;
                goto avi_read_init_continue;
            }
            
        }
        os_memcpy(&chunk_size,data+4,4);
        
        chunk_size = (chunk_size+1)&(~1);
        os_printf("chunk_size:%X\t%d\ttell:%X\n",chunk_size,chunk_size,osal_ftell(fp));
        os_memcpy(chunk_name,data,4);
        if(strncasecmp(data  ,"LIST",4) == 0)
        {
            readlen = osal_fread(data,1,4,fp);
            chunk_size -= 4;
            os_memcpy(chunk_name,data,4);
            //os_printf("chunk_name[%d]:%s\n",__LINE__,chunk_name);
            if(strncasecmp(data  ,"hdrl",4) == 0)
            {
                seek_offset = chunk_size + osal_ftell(fp);
                hdrl_offset = osal_ftell(fp);
                //readlen = osal_fread(&avi_msg->avi,1,sizeof(AVIMainHeader),fp);
                //os_printf("width:%d\theight:%d\n",avi_msg->avi.dwWidth,avi_msg->avi.dwHeight);
                //os_printf("seek_offset:%X\n",seek_offset);
                osal_fseek(fp,seek_offset);

            }
            else if(strncasecmp(data  ,"INFO",4) == 0)
            {
                seek_offset = chunk_size + osal_ftell(fp);
                osal_fseek(fp,seek_offset);
            }
            else if(strncasecmp(data  ,"movi",4) == 0)
            {
                avi_msg->movi_addr = osal_ftell(fp) - 4;
                //os_printf("movi_addr:%X\n",avi_msg->movi_addr);
                seek_offset = chunk_size + osal_ftell(fp);
                osal_fseek(fp,seek_offset);
            }
            else
            {
                os_printf("%s:%d err!!!!!!!!!!!!!\n",__FUNCTION__,__LINE__);
                goto avi_read_init_end;
            }
        }
        else if(strncasecmp(data  ,"JUNK",4) == 0)
        {
                seek_offset = chunk_size + osal_ftell(fp);
                osal_fseek(fp,seek_offset);
        }
        else if(strncasecmp(data  ,"idx1",4) == 0)
        {
                avi_msg->idx1_addr = osal_ftell(fp);
                avi_msg->idx1_size = chunk_size;
                os_printf("idx1_addr:%X\tidx1_size:%d\n",avi_msg->idx1_addr,avi_msg->idx1_size);
                seek_offset = chunk_size + osal_ftell(fp);
                osal_fseek(fp,seek_offset);
        }
        else
        {
            os_printf("%s:%d err!!!!!!!!!!!!!\n",__FUNCTION__,__LINE__);
            goto avi_read_init_end;
        }
    }
    avi_read_init_continue:
    if(hdrl_offset)
    {
        os_printf("hdrl_offset:%d\n",hdrl_offset);
        osal_fseek(fp,hdrl_offset);
        readlen = osal_fread(&avi_msg->avi,1,sizeof(AVIMainHeader),fp);

        //开始解析是否有音频、视频.
        struct chunk_s List_msg;
        avi_read_init_List_again:
        readlen = osal_fread(&List_msg,1,sizeof(List_msg),fp);
        offset = osal_ftell(fp);
        //有流,去分析一下有哪些流
        if(strncasecmp((void*)&(List_msg.dwFourCC)  ,"LIST",4) == 0)
        {
            //尝试读取strl
            readlen = osal_fread(data,1,4,fp);
            os_memcpy(chunk_name,data,4);
            os_printf("chunk_name[%d]:%s\n",__LINE__,chunk_name);
            //如果有strl,代表有流
            if(strncasecmp(data  ,"strl",4) == 0)
            {
                //读取一下流,读取完毕后要seek到后面的块,然后重复读取是否有下一个流
                read_strh_strf(fp,avi_msg);

                //跳过List,重复读取
                osal_fseek(fp,offset+List_msg.size);
                goto avi_read_init_List_again;
            }
        }

    }


    avi_read_init_end:

    if(err)
    {
        if(avi_msg)
        {
            STREAM_LIBC_FREE(avi_msg);
            avi_msg = NULL;
        }
        if(fp)
        {
            osal_fclose(fp);
        }
    }
    else
    {
        read_msg_init(avi_msg);
        avi_msg->video_frame_num = 0;
        avi_msg->audio_frame_num = 0;
        avi_msg->video_file_addr = avi_msg->idx1_addr;
        avi_msg->video_not_read_size = avi_msg->idx1_size;
        avi_msg->video_cache_remain = 0;

        avi_msg->audio_file_addr = avi_msg->idx1_addr;
        avi_msg->audio_not_read_size = avi_msg->idx1_size;
        avi_msg->audio_cache_remain = 0;
    }

    os_printf("err:%d\n",err);
    return avi_msg;
}



struct fast_index_list *save_fast_index_list(struct fast_index_list *fast_index,uint32_t avi_index,uint32_t audio_index,uint32_t offset)
{
    struct fast_index_list *list = (struct fast_index_list*)STREAM_LIBC_MALLOC(sizeof(struct fast_index_list));
    if(!list)
    {
        return fast_index;
    }
    list->next = NULL;
    list->prev = NULL;
    list->avi_index = avi_index;
    list->audio_index = audio_index;
    list->offset = offset;
    DL_APPEND(fast_index, list);
    return fast_index;
}

static struct fast_index_list * parse_idx1_continue(struct fast_index_list *fast_index,struct avi_msg_s *avi_msg,char *buf,uint32_t size)
{
    if(size%16 != 0)
    {
        return fast_index;
    }
    struct idx1_s *idx1 = (struct idx1_s*)buf;
    struct avi_read_msg_s *avi_read_msg = avi_msg->avi_read_msg;
    uint32_t offset = avi_read_msg->addr_offset;

    for(int i=0;i<size/16;i++)
    {

        //idx1->dwFlags = 0;
        //音频
        if(idx1->dwChunkId == avi_msg->a_id && idx1->dwFlags)
        {
            avi_read_msg->audio_index += idx1->dwSize;
        }
        //视频
        else if(idx1->dwChunkId == avi_msg->v_id)
        {
            if(avi_read_msg->avi_index%512 == 0 && avi_read_msg->avi_index>0)
            {
                fast_index = save_fast_index_list(fast_index,avi_read_msg->avi_index,avi_read_msg->audio_index,offset);
            }        
            avi_read_msg->avi_index++;
        }
        idx1++;
        offset+=16; 
    }

    return fast_index;
}

//实现快进快退功能(跳转帧)
void avi_jmp_index(struct avi_msg_s *avi_msg,uint32_t index)
{
    //索引快速索引表,看看最接近的帧,如果是0,则从头开始,如果非0,就索引
}


//往下读取到下一帧到对应的视频帧
void avi_read_next_index(struct avi_msg_s *avi_msg,uint32_t *offset,uint32_t *size,uint32_t *frame_num)
{
    uint32_t read_size;
    avi_read_next_index_again:
    if(avi_msg->video_cache_remain > 0)
    {
        //不符合视频,就继续去读取,知道读取到视频帧
        if(!(avi_msg->video_idx1->dwChunkId == avi_msg->v_id && avi_msg->video_idx1->dwFlags))
        {
            avi_msg->video_idx1++;
            avi_msg->video_cache_remain -= sizeof(struct idx1_s);
            goto avi_read_next_index_again;
        }
        //继续读取索引
        *offset = avi_msg->video_idx1->dwOffset + avi_msg->movi_addr+8;
        *size = avi_msg->video_idx1->dwSize;
        
        //进行偏移
        avi_msg->video_frame_num++;
        avi_msg->video_idx1++;
        avi_msg->video_cache_remain -= sizeof(struct idx1_s);
        //还没有读取到对应的帧,就要继续读取
        if(*frame_num >= avi_msg->video_frame_num)
        {
            goto avi_read_next_index_again;
        }
        *frame_num = avi_msg->video_frame_num;
        return;
    }
    else
    {
        if(avi_msg->video_not_read_size >= 512)
        {
            read_size = 512;
        }
        else
        {
            read_size = avi_msg->video_not_read_size;
        }
        if(read_size)
        {
            osal_fseek(avi_msg->fp,avi_msg->video_file_addr);
            osal_fread(avi_msg->video_buf,1,read_size,avi_msg->fp);

            avi_msg->video_not_read_size -= read_size;
            avi_msg->video_file_addr += read_size;
            avi_msg->video_cache_remain = read_size;
            avi_msg->video_idx1 = (struct idx1_s *)avi_msg->video_buf;
            goto avi_read_next_index_again;
        }
        //读取结束
        else
        {
            *offset = 0;
            *size = 0;
            return;
        }
    }
}



void avi_read_next_audio_index(struct avi_msg_s *avi_msg,uint32_t *offset,uint32_t *size,uint32_t *frame_num)
{
    uint32_t read_size;
    avi_read_next_audio_index_again:
    if(avi_msg->audio_cache_remain > 0)
    {
        //不符合视频,就继续去读取,知道读取到视频帧
        //os_printf("avi_msg->audio_idx1->dwChunkId:%08X\n",avi_msg->audio_idx1->dwChunkId);
        if(!(avi_msg->audio_idx1->dwChunkId == avi_msg->a_id && avi_msg->audio_idx1->dwFlags))
        {
            avi_msg->audio_idx1++;
            avi_msg->audio_cache_remain -= sizeof(struct idx1_s);
            goto avi_read_next_audio_index_again;
        }
        //继续读取索引
        *offset = avi_msg->audio_idx1->dwOffset + avi_msg->movi_addr+8;
        *size = avi_msg->audio_idx1->dwSize;

        //os_printf("offset:%X\tsize:%X\n",*offset,*size);
        
        //进行偏移
        avi_msg->audio_frame_num += (*size);
        avi_msg->audio_idx1++;
        avi_msg->audio_cache_remain -= sizeof(struct idx1_s);
        //还没有读取到对应的帧,就要继续读取
        if(*frame_num >= avi_msg->audio_frame_num)
        {
            goto avi_read_next_audio_index_again;
        }
        *frame_num = avi_msg->audio_frame_num;
        return;
    }
    else
    {
        if(avi_msg->audio_not_read_size >= 512)
        {
            read_size = 512;
        }
        else
        {
            read_size = avi_msg->audio_not_read_size;
        }
        if(read_size)
        {
            osal_fseek(avi_msg->fp,avi_msg->audio_file_addr);
            osal_fread(avi_msg->audio_buf,1,read_size,avi_msg->fp);

            avi_msg->audio_not_read_size -= read_size;
            avi_msg->audio_file_addr += read_size;
            avi_msg->audio_cache_remain = read_size;
            avi_msg->audio_idx1 = (struct idx1_s *)avi_msg->audio_buf;
            goto avi_read_next_audio_index_again;
        }
        //读取结束
        else
        {
            *offset = 0;
            *size = 0;
            return;
        }
    }
}





void gen_fast_index_list(struct avi_msg_s *avi_msg)
{
    uint32_t read_size = 0;
    struct avi_read_msg_s *read_msg = avi_msg->avi_read_msg;
    
    //读取完毕后,需要释放空间,保留快速索引表
    if(read_msg && read_msg->remain_read_size)
    {
        osal_fseek(avi_msg->fp,read_msg->addr_offset);
        if(read_msg->remain_read_size >= 512)
        {
            read_size = 512;
        }
        else
        {
            read_size = read_msg->remain_read_size;
        }
        osal_fread((void*)read_msg->buf,1,read_size,avi_msg->fp);
        avi_msg->fast_index = parse_idx1_continue(avi_msg->fast_index,avi_msg,(char*)read_msg->buf,read_size);

        read_msg->addr_offset += read_size;
        read_msg->remain_read_size -= read_size;
    }
    else
    {
        if(read_msg)
        {
            read_msg_deinit(avi_msg);
        }
    }

}

//定位索引点,重新计算
void locate_avi_index(struct avi_msg_s *avi_msg,uint32_t index)
{
    
    if(index == 0)
    {
        avi_msg->video_frame_num = 0;
        avi_msg->video_cache_remain = 0;
        avi_msg->video_not_read_size = avi_msg->idx1_size;
        avi_msg->video_file_addr = avi_msg->idx1_addr;

        avi_msg->audio_frame_num = 0;
        avi_msg->audio_cache_remain = 0;
        avi_msg->audio_file_addr = avi_msg->idx1_addr;
        avi_msg->audio_not_read_size = avi_msg->idx1_size;
        
    }
    else
    {
        struct fast_index_list *fast_index = avi_msg->fast_index;
        struct fast_index_list *list,*tmp;
        struct fast_index_list *last_index = NULL;

        DL_FOREACH_SAFE(fast_index,list,tmp) 
        {
            if(list->avi_index > index)
            {
                break;
            }
            //完全匹配,保存last_index;
            else if(list->avi_index == index)
            {
                last_index = list;
                break;
            }
            last_index = list;
        }



        //如果存在,则就从这里开始索引,一直索引到对应的index
        if(last_index)
        {
            avi_msg->video_frame_num = last_index->avi_index;
            avi_msg->video_cache_remain = 0;
            //通过偏移,计算还有多少数据没有被读取
            avi_msg->video_not_read_size = (avi_msg->idx1_size-(last_index->offset-avi_msg->idx1_addr));
            avi_msg->video_file_addr = last_index->offset;
        }
        //从0开始索引
        else
        {
            avi_msg->video_frame_num = 0;
            avi_msg->video_cache_remain = 0;
            avi_msg->video_not_read_size = avi_msg->idx1_size;
            avi_msg->video_file_addr = avi_msg->idx1_addr;

            avi_msg->audio_frame_num = 0;
            avi_msg->audio_cache_remain = 0;
            avi_msg->audio_file_addr = avi_msg->idx1_addr;
            avi_msg->audio_not_read_size = avi_msg->idx1_size;
        }

        //os_printf("last_index:%X\tsample:%X\n",last_index,avi_msg->sample);
        //索引到了视频,那么就要开始索引音频了,不过需要先计算当前视频的时间,然后计算出音频位置,最后定位
        if(avi_msg->sample)
        {
            //index是要快进或者快退到的视频点
            uint32_t video_time = index*1000/avi_msg->fps;
            uint32_t audio_offset = video_time*avi_msg->sample*2/1000;
            //os_printf("audio want to offset time:%d\toffset:%d\n",video_time,audio_offset);
            last_index = NULL;
            //查找最近的快速索引位置
            DL_FOREACH_SAFE(fast_index,list,tmp) 
            {
                //os_printf("list->audio_index:%d\n",list->audio_index);
                if(list->audio_index > audio_offset)
                {
                    break;
                }
                //完全匹配,保存last_index;
                else if(list->audio_index == index)
                {
                    last_index = list;
                    break;
                }
                last_index = list;
            }


            //索引到了音频位置
            if(last_index)
            {
                //计算当前索引的音频位置
                //("index audio_offset:%d\n",list->audio_index);
                //这个代表当前缓冲区的索引位置
                avi_msg->audio_frame_num = list->audio_index;
                avi_msg->audio_cache_remain = 0;
                //通过偏移,计算还有多少数据没有被读取
                avi_msg->audio_not_read_size = (avi_msg->idx1_size-(last_index->offset-avi_msg->idx1_addr));
                avi_msg->audio_file_addr = last_index->offset;
            }
            else
            {
                avi_msg->audio_frame_num = 0;
                avi_msg->audio_cache_remain = 0;
                avi_msg->audio_not_read_size = avi_msg->idx1_size;
                avi_msg->audio_file_addr = avi_msg->idx1_addr;
                
            }


        }




    }
}


static void free_fast_index_list(struct fast_index_list *fast_index)
{
    struct fast_index_list *list,*tmp; 
    DL_FOREACH_SAFE(fast_index,list,tmp) 
    {
      //从已有链表释放
      DL_DELETE(fast_index,list);
      STREAM_LIBC_FREE(list);
    }
}


void avi_deinit(struct avi_msg_s *avi_msg)
{
    if(avi_msg)
    {
        os_printf("%s %d\n",__FUNCTION__,__LINE__);
        osal_fclose(avi_msg->fp);
        //如果没有释放,就会释放(快速索引在索引完后,会自动释放,但是如果在没有索引完毕又要关闭视频,就需要在这里释放)
        read_msg_deinit(avi_msg);
        if(avi_msg->fast_index)
        {
            free_fast_index_list(avi_msg->fast_index);
        }
        STREAM_LIBC_FREE(avi_msg);
    }
}

void parse_idx1(char *buf,uint32_t size)
{
    if(size%16 != 0)
    {
        return;
    }

    struct idx1_s *idx1 = (struct idx1_s*)buf;
    for(int i=0;i<size/16;i++)
    {
        idx1->dwFlags = 0;
        os_printf("id:%s\tflags:00000010\toffset:%08X\tsize:%08X\n",(char*)&idx1->dwChunkId,idx1->dwOffset,idx1->dwSize);
        idx1+=1;
    }
}







struct avi_read_msg_s *read_msg_init(struct avi_msg_s *avi_msg)
{
    struct avi_read_msg_s *read_msg = STREAM_LIBC_MALLOC(sizeof(struct avi_read_msg_s));
    if(read_msg)
    {
        read_msg->addr_offset = avi_msg->idx1_addr;
        read_msg->avi_index = 0;
        read_msg->audio_index = 0;
        read_msg->remain_read_size = avi_msg->idx1_size;
    }
    avi_msg->avi_read_msg = read_msg;
    return read_msg;
}

void read_msg_deinit(struct avi_msg_s *avi_msg)
{
    if(avi_msg->avi_read_msg)
    {
        STREAM_LIBC_FREE(avi_msg->avi_read_msg);
        avi_msg->avi_read_msg = NULL;
    }

}


