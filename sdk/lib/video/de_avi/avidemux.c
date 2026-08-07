#include <stdio.h>
#include <string.h>
#include "osal_file.h"
#include "avidemux.h"
#include "osal/string.h"

#define FourCC(aa, bb, cc, dd) ( ((aa)&0xFF) | (((bb)<<8)&0xFF00) | (((cc)<<16)&0xFF0000) | (((dd)<<24)&0xFF000000) )
#define FourCCCC(i) (i)&0xFF,(i>>8)&0xFF,(i>>16)&0xFF,(i>>24)&0xFF

#ifndef BIT
#define BIT(b) (1<<(b))
#endif





// 在parse階段，哪些LIST是需要深入分析的
static int need_parse_list(uint32_t fourcc)
{
  return
    fourcc == FourCC('A','V','I',' ') ||
    fourcc == FourCC('h','d','r','l') ||
    fourcc == FourCC('s','t','r','l') ||
    0;
}

#define MAX_LIST_DEPTH 5



//avi数据头解析，不是全局变量
void *avidemux_parse(void *fp,void *aviinfo_point)
{
  AVISTREAMHEADER strh;
  struct _aviindex_chunk indx;
  struct avistrinfo *str = NULL;
  struct aviinfo *aviinfo = (struct aviinfo *)aviinfo_point;
  uint32_t list_end[MAX_LIST_DEPTH]; // RIFF/LIST棧，記錄其結束地址
  uint32_t lvl;
  uint32_t fcc_size[3];              // 緩存讀入的FourCC及其size
  uint32_t pos;
  uint32_t err;
  memset(aviinfo_point,0,sizeof(struct aviinfo));
  lvl = 0;
  list_end[lvl] = osal_fsize (fp);
  _os_printf ("%s : file size %08X\n", __func__, list_end[lvl]);
uint32_t msk = 0;
  pos = 0;
  while (pos < list_end[lvl]) {

    if (osal_fread ((char*)fcc_size, 1, 8, fp) < 8) {
      _os_printf ("%s : read fourcc & size error\n", __func__);
      break;
    }
//    _os_printf ("%s : %c%c%c%c %08X", __func__, FourCCCC(fcc_size[0]), fcc_size[1]);
    pos += 8;
    // 檢查長度
    if (fcc_size[1] + pos > list_end[lvl]) {
      _os_printf ("%s : fourcc length error\n", __func__);
      break;
    }

    err = 0;
    switch (fcc_size[0]) {
    case FourCC('R','I','F','F'):
    case FourCC('L','I','S','T'):
      // 檢查LIST的類型
      if (osal_fread((char*)&fcc_size[2], 1, 4, fp) < 4) {
        _os_printf ("%s : read LIST name error\n", __func__);
        err = 1;
        break;
      }
      if (need_parse_list(fcc_size[2]) && lvl < MAX_LIST_DEPTH-1) {
        // 需要分析這個LIST，並且棧空間還夠用
        list_end[++lvl] = pos + fcc_size[1];
        pos += 4;
//        _os_printf ("%s : pos  %08X %08X %08X", __func__, pos, list_end[lvl], fcc_size[1]);
      } else {
        pos += fcc_size[1];
        osal_fseek (fp, pos);
      }
      break;
    case FourCC('s','t','r','h'):
      if (osal_fread(((void*)&strh)+2*sizeof(DWORD), 1, sizeof(strh)-2*sizeof(DWORD), fp) < sizeof(strh)-2*sizeof(DWORD)) {
        _os_printf ("%s : read strh type error\n", __func__);
        err = 1;
        break;
      }
      pos += fcc_size[1];
//      _os_printf ("found strh %s %dx%d %d frames", &strh.fccType, strh.rcFrame.right, strh.rcFrame.bottom, strh.dwLength);

      if (strh.fccType == FourCC('v','i','d','s')) {
        str = &aviinfo->str[0];
        str->type = 0;
        aviinfo->strmsk |= BIT(0);
        msk |= BIT(0);
      } else {
        str = &aviinfo->str[1];
        str->type = 1;
        aviinfo->strmsk |= BIT(1);
        msk |= BIT(1);
      }
      str->dwScale       = strh.dwScale * 1000;
      str->dwRate        = strh.dwRate;
      str->dwTotalFrames = strh.dwLength;
      str->cur = 0;
      str->avi = aviinfo;
      break;
    case FourCC('i','n','d','x'):
      if (NULL == str) {
        _os_printf ("%s : indx but no strh\n", __func__);
        err = 1;
        break;
      }
      if (osal_fread(((void*)&indx)+2*sizeof(DWORD), 1, sizeof(indx)-2*sizeof(DWORD), fp) < sizeof(indx)-2*sizeof(DWORD)) {
        _os_printf ("%s : read indx type error\n", __func__);
        err = 1;
        break;
      }
      //找到超级索引的数量,然后保存超级索引表,并且计算序号(用于快速搜索)
      str->dwIndexNum  = indx.nEntriesInUse;
      //一次性申请足够空间,读取超级索引表
      str->superindex_cache = (struct _avisuperindex_entry_cache*)malloc(str->dwIndexNum*sizeof(struct _avisuperindex_entry_cache));

      if(str->superindex_cache)
      {
        if(osal_fread((char*)str->superindex_cache, 1, str->dwIndexNum*sizeof(struct _avisuperindex_entry_cache), fp) < str->dwIndexNum*sizeof(struct _avisuperindex_entry_cache))
        {
          _os_printf ("%s : read indx type error\n", __func__);
          err = 1;
          break;
        }
        //扫描超级索引,记录索引序号起始
        uint32_t offset = 0;
        uint32_t max_size = 0;
        uint32_t count = 0;
        for(int i=0;i<str->dwIndexNum;i++)
        {
          str->superindex_cache[i].offset = offset;
          count = ((str->superindex_cache[i].dwSize - sizeof(struct _aviindex_chunk))/sizeof(struct _avistdindex_entry));
          str->superindex_cache[i].dwSize = count;
          
          offset+= count;
          if(count > max_size)
          {
            max_size = count;
          }

          //_os_printf("offset:%d\t%X\n",str->superindex_cache[i].offset,str->superindex_cache[i].dwOffsetl);
        }
        //如果是音频,则帧数修改一下
        if(str->type == 1)
        {
          str->dwTotalFrames = offset;
        }
        //读取完毕后,去扫描索引最大值,申请最大空间,一次读取一个超级索引

        str->stindex_cache = (struct _avistdindex_entry*)malloc(max_size*sizeof(struct _avistdindex_entry));
        if(!str->stindex_cache)
        {
          err = 1;
          break;
        }
      }

      str->dwIndexBase = pos + sizeof(indx)-2*sizeof(DWORD);
      //str->dwDuration  = str->indx[0].dwDuration;
      str->cached_indx_base = 0;
      str->cached_ixnn_base = 0;
      str->super_idx        = ~0;
      str->cur = 0;
      
//      _os_printf ("found indx @%08X", str->dwIndexBase);
      pos += fcc_size[1];
      // 雖然緩存了512B的超級索引，但該長度不是最終的長度
      osal_fseek (fp, pos);
      break;
    default:
      pos += fcc_size[1];
      osal_fseek (fp, pos);
    } // switch (fcc_size[0])
    if (err) break;

    if (pos == list_end[lvl]) {
      // pop
      --lvl;
    }
    if (pos > list_end[lvl]) {
      _os_printf ("%s : list overread\n", __func__);
      break;
    }
  }  // while (pos < list_end[lvl])
  _os_printf ("str[0] : %d @%08X:%08X\n", (int)aviinfo->str[0].dwTotalFrames, (int)aviinfo->str[0].dwIndexBase, (int)aviinfo->str[0].dwIndexNum);
  _os_printf ("str[1] : %d @%08X:%08X\n", (int)aviinfo->str[1].dwTotalFrames, (int)aviinfo->str[1].dwIndexBase, (int)aviinfo->str[1].dwIndexNum);
  aviinfo->fp = fp;
  if(err)
  {

    for(int i=0;i<2;i++)
    {
      if(aviinfo->str[i].superindex_cache)
      {
        free(aviinfo->str[i].superindex_cache);
      }

      if(aviinfo->str[i].stindex_cache)
      {
        free(aviinfo->str[i].stindex_cache);
      }
    }

  	return NULL;
  }
  return aviinfo;
}




uint32_t get_avidemux_parse_stream(char *path,struct avi_msg *msg)
{
  _os_printf("my path:%s\n",path);
  void *fp = osal_fopen(path,"rb");
  if(!fp)
  {
    return 0;
  }
  memset(msg,0,sizeof(struct avi_msg));
  AVISTREAMHEADER strh;
  //DWORD fccType;
  uint32_t list_end[MAX_LIST_DEPTH]; // RIFF/LIST棧，記錄其結束地址
  uint32_t lvl;
  uint32_t fcc_size[3];              // 緩存讀入的FourCC及其size
  uint32_t pos;
  uint32_t err;
  uint32_t msk = 0;
  lvl = 0;
  list_end[lvl] = osal_fsize (fp);


  pos = 0;
  while (pos < list_end[lvl]) 
  {
    if (osal_fread ((char*)fcc_size, 1, 8, fp) < 8) {
      _os_printf ("%s : read fourcc & size error\n", __func__);
      break;
    }
    pos += 8;
    // 檢查長度
    if (fcc_size[1] + pos > list_end[lvl]) {
      _os_printf ("%s : fourcc length error\n", __func__);
      break;
    }

    err = 0;
    switch (fcc_size[0]) 
    {
      case FourCC('R','I','F','F'):
      case FourCC('L','I','S','T'):
        // 檢查LIST的類型
        if (osal_fread((char*)&fcc_size[2], 1, 4, fp) < 4) {
          _os_printf ("%s : read LIST name error\n", __func__);
          err = 1;
          break;
        }
        if (need_parse_list(fcc_size[2]) && lvl < MAX_LIST_DEPTH-1) {
          // 需要分析這個LIST，並且棧空間還夠用
          list_end[++lvl] = pos + fcc_size[1];
          pos += 4;
        } else {
          pos += fcc_size[1];
          osal_fseek (fp, pos);
        }
        break;
      case FourCC('s','t','r','h'):
        if (osal_fread((void*)&strh.fccType, 1, sizeof(strh)-2*sizeof(DWORD), fp) < sizeof(strh)-2*sizeof(DWORD)) {
          _os_printf ("%s : read strh type error\n", __func__);
          err = 1;
          break;
        }
        pos += fcc_size[1];
        if (strh.fccType == FourCC('v','i','d','s')) {
          msk |= BIT(0);
          msg->video_sample_rate = strh.dwRate;
        } else {
          msk |= BIT(1);
          msg->audio_sample_rate = strh.dwRate/strh.dwSampleSize;
        }
        osal_fseek (fp, pos);
        break;
      default:
        pos += fcc_size[1];
        osal_fseek (fp, pos);
        break;
    }
      if (pos == list_end[lvl]) 
      {
        --lvl;
      }
      if (pos > list_end[lvl]) {
        _os_printf ("%s : list overread\n", __func__);
        break;
      }
  }

  _os_printf("end pos:%X\t%X\t%X\n",pos,list_end[lvl],msk);
    if(fp)
    {
      osal_fclose(fp);
    }

  return msk;
}




//主要是偏移到cur的对应位置,并且返回对应数据的位置以及size,返回一个错误
uint32_t read_avi_offset(struct avistrinfo *str,uint32_t *base,uint32_t *size)
{
  uint32_t err = 0;
  *base = 0;
  *size = 0;
  uint8_t st_index_read = 0;
  if(str->cur >= str->dwTotalFrames)
  {
    err = 1;
    goto read_avi_offset_end;
  }
  //如果超级索引序号比最大的索引大,可能是第一次读取
  if(str->super_idx >= str->dwIndexNum)
  {
    err = 2;
    //那么计算cur当前的位置
    for(int i=0;i<str->super_idx;i++)
    {
      if(str->superindex_cache[i].offset+str->superindex_cache[i].dwSize > str->cur && str->superindex_cache[i].offset <= str->cur)
      {
        str->super_idx = i;
        st_index_read = 1;
        err = 0;
        break;
      }
    }
  }
  else
  {
      err = 3;
      //如果在范围,则不需要处理
      if(str->superindex_cache[str->super_idx].offset+str->superindex_cache[str->super_idx].dwSize > str->cur && str->superindex_cache[str->super_idx].offset <= str->cur)
      {
        err = 0;
      }
      else
      {
        //现在超级索引得到的偏移比cur大,那么就要从0开始搜索
        if(str->superindex_cache[str->super_idx].offset > str->cur)
        {
          for(int i=0;i<str->dwIndexNum;i++)
          {
            if(str->superindex_cache[i].offset+str->superindex_cache[i].dwSize > str->cur && str->superindex_cache[i].offset <= str->cur)
            {
              str->super_idx = i;
              st_index_read = 1;
              err = 0;
              break;
            }
          }
        }
        //从当前位置开始搜索
        else
        {
          for(int i=str->super_idx;i<str->dwIndexNum;i++)
          {
            if(str->superindex_cache[i].offset+str->superindex_cache[i].dwSize > str->cur && str->superindex_cache[i].offset <= str->cur)
            {
              str->super_idx = i;
              st_index_read = 1;
              err = 0;
              break;
            }
          }
        }
      }
  }
  if(err)
  {
    _os_printf("err:%d\n",err);
    goto read_avi_offset_end;
  }
  //已经找到对应的索引号,那就判断是否需要读取标准索引区
  if(!err && st_index_read)
  {
    //读取标准索引区
    osal_fseek (str->avi->fp, str->superindex_cache[str->super_idx].dwOffsetl);
    osal_fread((char*)&str->chunk,1,sizeof(struct _aviindex_chunk),str->avi->fp);
    osal_fread ((char*)str->stindex_cache, 1, str->superindex_cache[str->super_idx].dwSize*sizeof(struct _avistdindex_entry), str->avi->fp);
  }

  //读取完标准索引,就返回base、size
  uint32_t offset = str->cur - str->superindex_cache[str->super_idx].offset;
  *base = str->stindex_cache[offset].dwOffset+str->chunk.dwBaseOffsetl;
  *size = str->stindex_cache[offset].dwSize;
  //偏移位置
  osal_fseek (str->avi->fp, *base);


  str->cur ++;
read_avi_offset_end:
  
  return err;
}



//从当前位置去读取数据内容,buf是4 byte对齐
uint32_t read_avi_data(struct avistrinfo *str,char *buf,uint32_t size)
{
  uint32_t read_len = 0;
  read_len = osal_fread(buf,1,size,str->avi->fp);
  return read_len;
}

void free_aviinfo_point(struct aviinfo *info)
{
    for(int i=0;i<2;i++)
    {
      if(info->str[i].superindex_cache)
      {
        free(info->str[i].superindex_cache);
      }

      if(info->str[i].stindex_cache)
      {
        free(info->str[i].stindex_cache);
      }
    }
}

void set_avi_cur(struct avistrinfo *info,int cur)
{
  info->cur = cur;
}

uint32_t get_avi_cur(struct avistrinfo *info)
{
  return info->cur;
}

#if 0
uint32_t test_space[100*1024];
void play_avi_file(void *aviinfo_point)
{
  uint32_t base,size;
  uint32_t base_tmp = 0;
  char *buf = (char*)test_space;
  struct aviinfo *info = (struct aviinfo *)aviinfo_point;
  _os_printf("%s:%d\n",__FUNCTION__,__LINE__);
  int count = 0;
  int count_tmp = 0;
  while(1)
  {
    read_avi_offset(&info->str[0],&base,&size);
    _os_printf("base:%X\tsize:%d\tcount:%d\n",base,size,count++);
    if(base == 0)
    {
      break;
    }

    if(base_tmp != base)
    {
      base_tmp = base;
    }
    else
    {
      count_tmp++;
    }

  }
  _os_printf("count_tmp:%d\n",count_tmp);
}
#endif