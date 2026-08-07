#include "basic_include.h"
//#include "keyWork.h"
//#include "keyScan.h"
#include "osal_file.h"
#include "mp3_getInfo.h"
#include "mp3_decode.h"
#include "audio_code_ctrl.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

#define MP3_SAVE_INFO   0

#if MP3_SAVE_INFO
#define MAX_FORWARD_TIME     10 
struct key_callback_list_s *key_seek_list = NULL;
#endif

const uint32_t  bitrate_table[5][15] = {
    /* MPEG-1 */
    { 0,  32000,  64000,  96000, 128000, 160000, 192000, 224000,  /* Layer I   */
    256000, 288000, 320000, 352000, 384000, 416000, 448000 },
    { 0,  32000,  48000,  56000,  64000,  80000,  96000, 112000,  /* Layer II  */
    128000, 160000, 192000, 224000, 256000, 320000, 384000 },
    { 0,  32000,  40000,  48000,  56000,  64000,  80000,  96000,  /* Layer III */
    112000, 128000, 160000, 192000, 224000, 256000, 320000 },
    /* MPEG-2 ，MPEG-2.5 */
    { 0,  32000,  48000,  56000,  64000,  80000,  96000, 112000,  /* Layer I   */
    128000, 144000, 160000, 176000, 192000, 224000, 256000 },
    { 0,   8000,  16000,  24000,  32000,  40000,  48000,  56000,  /* Layer    */
    64000,  80000,  96000, 112000, 128000, 144000, 160000 } /* II & III  */
};
const uint32_t samplingrate_table[4][3] = {  //value s are in Hz
    {11025 , 12000 , 8000},   //MPEG Version 2.5 
    {0,0,0},                  //reserved 
    {22050, 24000, 16000},    //MPEG Version 2 (ISO/IEC 13818-3) 
    {44100, 48000, 32000}     //MPEG Version 1 (ISO/IEC 11172-3) 
};
const uint8_t layerIII_inbytes[4][4] = {
	{17,17,17,9},                /*MPEG Version 2.5*/
	{0, 0, 0, 0},                /*reserved*/
	{17,17,17,9},                /*MPEG Version 2*/
	{32,32,32,17}                /*MPEG Version 1*/
};
const uint16_t samples_table[4][4] = {
	{0,576,1152,384},           /*MPEG Version 2.5 {reserve,layerIII,layerII,layerI}*/
	{0,0,0,0},					/*reserved {reserve,layerIII,layerII,layerI}*/
	{0,576,1152,384},			/*MPEG Version 2 {reserve,layerIII,layerII,layerI}*/
	{0,1152,1152,384}			/*MPEG Version 1 {reserve,layerIII,layerII,layerI}*/
};

uint32_t get_curmp3_size(uint32_t s)
{
    if(!cur_mp3_info) 
		return 0;     
	cur_mp3_info->total_size =s;
	return cur_mp3_info->total_size;
}

void curmp3_info_init(uint8_t *mp3_filename)
{
#if MP3_SAVE_INFO
	char dirname_len = strlen("0:mp3/");
	char extension_len = strlen("mp3");
	char filename_temp[50] = {0};
#endif
    if(!cur_mp3_info)
        cur_mp3_info = (CUR_MP3_INFO*)MP3_DECODE_ZALLOC(sizeof(CUR_MP3_INFO)); 
    else
        os_memset(cur_mp3_info, 0, sizeof(cur_mp3_info)); 
#if MP3_SAVE_INFO
	os_memcpy(filename_temp, mp3_filename+dirname_len, strlen(mp3_filename)-dirname_len-extension_len);
	filename_temp[strlen(mp3_filename)-dirname_len-extension_len] = '\0';
	os_sprintf(cur_mp3_info->mp3_st_filename, "0:MP3_SEEK/%s%s", filename_temp, "st");
	os_printf("mp3_st_filename:%s\n",cur_mp3_info->mp3_st_filename);
#endif
}       

void get_curmp3_totaltime_ergodic(void *fp, uint32_t first_frame_offset)
{
	
	uint8_t bitrate_index = 0;
	uint8_t padding = 0;
	uint8_t frame_head[4];
	int32_t read_len = 0;
	uint32_t bitrate = 0;
	uint32_t frame_len = 0;
	uint32_t offset = 0;
	uint32_t current_seek = 0;
	uint32_t read_frame_cnt = 0;
	uint32_t read_second_cnt = 0;

    if(!cur_mp3_info)
		return;
        
	current_seek = osal_ftell(fp);
	osal_fseek(fp, first_frame_offset);
	osal_fread(frame_head, 1, 4, fp);
	offset = first_frame_offset;

	bitrate_index = (frame_head[2]>>4)&0xf;
	bitrate = bitrate_table[cur_mp3_info->bitrate_table_index][bitrate_index];
	padding = ((frame_head[2]>>1)&0x1) * cur_mp3_info->padding_mul;
	frame_len = cur_mp3_info->samples/8*bitrate/cur_mp3_info->samplingrate + padding;

	offset = first_frame_offset+4;
	// printf("samples:%d samplingrate:%d bitrate:%d\n",cur_mp3_info->samples,cur_mp3_info->samplingrate,bitrate);
	INIT_LIST_HEAD((struct list_head *)&cur_mp3_info->seek_table_head);
	MP3_SEEK_TABLE *sub_seek_table = MP3_DECODE_ZALLOC(sizeof(MP3_SEEK_TABLE));
	list_add_tail((struct list_head *)&sub_seek_table->sub_table_list,(struct list_head *)&cur_mp3_info->seek_table_head);
	sub_seek_table->sub_table[0] = first_frame_offset;
	while(1) {
		osal_fseek(fp, offset+frame_len-4);
		offset += frame_len;
		read_len = osal_fread(frame_head, 1, 4, fp);
		if(((cur_mp3_info->total_size-offset)==124) && (strncmp((const char*)frame_head, "TAG", 3)==0)) {   //ID3V1
			break;
		}
		if(((frame_head[0]<<8) | (frame_head[1]&0xE0)) != 0xFFE0) {
			break;
		}
		if((offset+frame_len-4) >= cur_mp3_info->total_size )  { //end
			break;
		}
		cur_mp3_info->total_frame++;
		bitrate_index = (frame_head[2]>>4)&0xf;
		bitrate = bitrate_table[cur_mp3_info->bitrate_table_index][bitrate_index];			
		padding = ((frame_head[2]>>1)&0x1) * cur_mp3_info->padding_mul;
		frame_len = cur_mp3_info->samples/8*bitrate/cur_mp3_info->samplingrate + padding;
		read_frame_cnt++;
		if(read_frame_cnt >= (cur_mp3_info->one_second_frame)) {
			read_second_cnt++;
			if(read_second_cnt >= 100) {
				sub_seek_table = (MP3_SEEK_TABLE*)MP3_DECODE_ZALLOC(sizeof(MP3_SEEK_TABLE));
				list_add_tail((struct list_head *)&sub_seek_table->sub_table_list,(struct list_head *)&cur_mp3_info->seek_table_head);
				read_second_cnt -= 100;
			}
			sub_seek_table->sub_table[read_second_cnt] = offset-4;
			read_frame_cnt -= (cur_mp3_info->one_second_frame);
		}	
	}
	cur_mp3_info->totaltime = cur_mp3_info->total_frame * cur_mp3_info->samples / cur_mp3_info->samplingrate;
	cur_mp3_info->once_move_time = (cur_mp3_info->totaltime%100)?(cur_mp3_info->totaltime/100+1):(cur_mp3_info->totaltime/100);
	MP3_INFO("total_frame:%d total_time:%d once_move_time:%d\n",cur_mp3_info->total_frame, cur_mp3_info->totaltime,cur_mp3_info->once_move_time);
	cur_mp3_info->get_totaltime = 1;
	osal_fseek(fp, current_seek);
}

uint32_t get_mp3_seek(void)
{
    if(!cur_mp3_info)
		return 0;
        
	return cur_mp3_info->offset_seek;
}

uint32_t get_curmp3_playtime(void)
{
    if(!cur_mp3_info)
		return 0;
        
    cur_mp3_info->playtime = cur_mp3_info->play_frame * cur_mp3_info->samples / cur_mp3_info->samplingrate;
	// printf("record:%d %d\n",cur_mp3_info->playtime,cur_mp3_info->play_frame);
	return cur_mp3_info->playtime;
}
uint32_t get_curmp3_playframe(void)
{
    if(!cur_mp3_info)
		return 0;
        
	cur_mp3_info->play_frame++;
	return cur_mp3_info->play_frame;
}
void update_curmp3_playtime(uint32_t update_time)
{
    if(!cur_mp3_info)
		return;

	cur_mp3_info->playtime = update_time;
}
void update_curmp3_playframe(uint32_t update_frame)
{
    if(!cur_mp3_info)
		return;

	cur_mp3_info->play_frame = update_frame;
}

void set_mp3_seek(int16_t move_time)
{
	uint8_t table_pos = 0;
	uint32_t offset_seek = 0;
	uint32_t target_time = 0;
	uint32_t target_frame = 0;
	uint32_t table_index = 0;
	struct list_head *seek_table_list = NULL;
	MP3_SEEK_TABLE *seek_table = NULL;
	
	target_time = ((cur_mp3_info->playtime + move_time)<0)?0:(cur_mp3_info->playtime + move_time);
	target_time = (target_time>cur_mp3_info->totaltime)?cur_mp3_info->totaltime:target_time;
	target_frame = cur_mp3_info->one_second_frame*target_time;
	table_index = target_time/100;
	table_pos = target_time%100;
	seek_table_list = cur_mp3_info->seek_table_head.next;
	while(table_index>0) {
		seek_table_list = seek_table_list->next;
		table_index--;
	}
	seek_table = list_entry((struct list_head *)seek_table_list,MP3_SEEK_TABLE,sub_table_list);
	offset_seek =  seek_table->sub_table[table_pos];
	osal_fseek(cur_mp3_info->mp3_fp,offset_seek);
	update_curmp3_playtime(target_time);
	update_curmp3_playframe(target_frame);
    cur_mp3_info->offset_seek = offset_seek;
    MP3_INFO("**file pos:%x time:%d frame:%d**\n",offset_seek,cur_mp3_info->playtime,cur_mp3_info->play_frame);
}
//
//uint32_t key_seek_callback(struct key_callback_list_s *callback_list,uint32_t keyvalue,uint32_t extern_value)
//{
//	uint32 key_val = 0;
//	
//    if(!cur_mp3_info)
//		return 0;
//        
//	if(((keyvalue>>8) != AD_A) && ((keyvalue>>8) != AD_B))
//		return 0;  
//    if(get_mp3_decode_status() == MP3_STOP) {
//		_os_printf("mp3 stop\n");
//        return 0;
//	}
//    key_val = (keyvalue & 0xff); 
//    if((keyvalue>>8) == AD_A) {
//        if(key_val == KEY_EVENT_DOWN) {
//            os_sema_down(&seek_sema,5000);
//			set_mp3_seek(-cur_mp3_info->once_move_time);
//        }
//        else if((key_val == KEY_EVENT_LDOWN) || (key_val == KEY_EVENT_REPEAT)) {
//			set_mp3_seek(-cur_mp3_info->once_move_time);            
//        }
//        else if((key_val == KEY_EVENT_SUP) || (key_val == KEY_EVENT_LUP)) {
//			cur_mp3_info->update_fops = 1;
//            os_sema_up(&seek_sema);
//        }        
//    }
//    else if((keyvalue>>8) == AD_B) {
//        if(key_val == KEY_EVENT_DOWN) {			
//            os_sema_down(&seek_sema,5000);
//			set_mp3_seek(cur_mp3_info->once_move_time);           
//        }
//		else if((key_val == KEY_EVENT_LDOWN) || (key_val == KEY_EVENT_REPEAT)) {
//			set_mp3_seek(cur_mp3_info->once_move_time);			
//		}
//        else if((key_val == KEY_EVENT_SUP) || (key_val == KEY_EVENT_LUP)) {
//			cur_mp3_info->update_fops = 1;
//            os_sema_up(&seek_sema);
//        }        
//    }
//    return 1;
//}

int32_t at_set_mp3_seek(const char *cmd, char *argv[], uint32_t argc)
{
	int32_t seek_time = 0;
    uint32_t forward_time = 0;
	
    if(!cur_mp3_info)
        return 0;
    if(get_mp3_decode_status() == AUDIO_STOP) {
		MP3_INFO("mp3 stop\n");
        return 0;
	}	
	if(argc < 1) {
        MP3_INFO("%s %d,enter the seek\n",__FUNCTION__,argc);
        return 0;
    }
	if(argv[0]) {
		seek_time = os_atoi(argv[0]);
		if(seek_time == 0)
			return 1;
		// forward_time = (seek_time<MAX_FORWARD_TIME)?seek_time:MAX_FORWARD_TIME;
		forward_time = seek_time;
		os_sema_down(&seek_sema,5000);
		set_mp3_seek(forward_time);    
		cur_mp3_info->update_fops = 1;
		os_sema_up(&seek_sema);  		
	}
	return 0;
}

int seektable_check(CUR_MP3_INFO *mp3_info)
{
	int32_t ret = 0;
	uint32_t read_buf[4] = {0};
	uint32_t seek_table_len = 0;
	uint32_t *cache_buf = NULL;
	
	cache_buf = (uint32_t*)MP3_DECODE_ZALLOC(sizeof(uint32_t)*100);
	if(!cache_buf)
		goto seektable_check_end;
	osal_fread(read_buf, 4, 4, mp3_info->mp3_st_fp);
	if((read_buf[0]==mp3_info->total_size) && (read_buf[1]==mp3_info->first_frame_offset)
	                                  && (read_buf[2]==mp3_info->normal_frame_offset)) {
		mp3_info->total_frame = read_buf[3];
		seek_table_len = osal_fsize(mp3_info->mp3_st_fp) - sizeof(read_buf);
		INIT_LIST_HEAD((struct list_head *)&mp3_info->seek_table_head);
		while(seek_table_len) {
			MP3_SEEK_TABLE *sub_seek_table = MP3_DECODE_ZALLOC(sizeof(MP3_SEEK_TABLE));
			list_add_tail((struct list_head *)&sub_seek_table->sub_table_list,(struct list_head *)&mp3_info->seek_table_head);
			osal_fread((void*)cache_buf, 4, 100, mp3_info->mp3_st_fp);
			os_memcpy(sub_seek_table->sub_table, cache_buf, sizeof(uint32_t)*100);
			seek_table_len -= 100*4;
		}
		cur_mp3_info->totaltime = cur_mp3_info->total_frame * cur_mp3_info->samples / cur_mp3_info->samplingrate;
		cur_mp3_info->once_move_time = (cur_mp3_info->totaltime%100)?(cur_mp3_info->totaltime/100+1):(cur_mp3_info->totaltime/100);
		MP3_INFO("total_frame:%d total_time:%d once_move_time:%d\n",cur_mp3_info->total_frame, cur_mp3_info->totaltime,cur_mp3_info->once_move_time);
		cur_mp3_info->get_totaltime = 1;
		ret = 1;
	}
seektable_check_end:
	if(cache_buf)
		MP3_DECODE_FREE((void*)cache_buf);
	osal_fclose(mp3_info->mp3_st_fp);
	return ret;
}

void find_first_frame(void *fp)
{
	uint8_t mp3_head[512];
	uint8_t bitrate_index = 0;
	uint8_t channel_mode = 0x11;
	uint8_t padding = 0;
    uint8_t layer_index = 0;
    uint8_t mpeg_index = 0;
    uint8_t samplingrate_index = 0;
	int32_t bitrate = 0;
	uint32_t offset = 0;
	uint32_t first_frame_offset = 0;
	uint32_t seek = 0;
	uint32_t frame_len = 0;

    if(!cur_mp3_info) {
		return;
	}    
	osal_fread(mp3_head, 512, 1, fp);
	if(os_strncmp(mp3_head+offset, "ID3", 3) == 0) {
		uint32_t ID3V2_len = (mp3_head[6]&0x7F)*0x200000+ (mp3_head[7]&0x7F)*0x4000 + 
										(mp3_head[8]&0x7F)*0x80 +(mp3_head[9]&0x7F);
		offset += (ID3V2_len+10);
	}
    MP3_INFO("ID3 offset:%x\n",offset);
	osal_fseek(fp,offset);
	seek = offset;
	osal_fread(mp3_head,1,512,fp);
	while(((mp3_head[offset-seek]<<8) | (mp3_head[offset-seek+1]&0xE0)) != 0xFFE0) {
		offset++;
		if((offset+1) > (seek+512)) {
			osal_fseek(fp,offset);
			osal_fread(mp3_head, 1, 512, fp);
			seek = offset;
		}
	}
	osal_fseek(fp,offset);
	seek = offset;
	osal_fread(mp3_head,1,512,fp);
	cur_mp3_info->mp3_fp = fp;
	first_frame_offset = offset;
//	os_printf("\n***first frame offset:%x****\n",first_frame_offset);
	cur_mp3_info->first_frame_offset = first_frame_offset;
	layer_index = (mp3_head[first_frame_offset-seek+1]>>1)&0x3;
	mpeg_index = (mp3_head[first_frame_offset-seek+1]>>3)&0x3;
	samplingrate_index = (mp3_head[first_frame_offset-seek+2]>>2)&0x3;
	cur_mp3_info->samplingrate = samplingrate_table[mpeg_index][samplingrate_index];
	cur_mp3_info->samples = samples_table[mpeg_index][layer_index];
	channel_mode = (mp3_head[first_frame_offset-seek+3]>>6)&0x3;
	if(mpeg_index == 3) {                                 /*MPEG 1*/
		switch(layer_index) {
			case 0:break;
			case 1:cur_mp3_info->bitrate_table_index = 2;break;         /*layer III*/
			case 2:cur_mp3_info->bitrate_table_index = 1;break;		  /*layer II*/
			case 3:cur_mp3_info->bitrate_table_index = 0;break;         /*layer I*/
			default:break;
		}
	}
	else if((mpeg_index == 0) || (mpeg_index == 2)) {     /*MPEG 2、MPEG2.5*/
		switch(layer_index) {
			case 0:break;
			case 1:cur_mp3_info->bitrate_table_index = 4;break;
			case 2:cur_mp3_info->bitrate_table_index = 4;break;
			case 3:cur_mp3_info->bitrate_table_index = 3;break;
			default:break;
		}
	}
	bitrate_index = (mp3_head[first_frame_offset-seek+2]>>4)&0xf;
	bitrate = bitrate_table[cur_mp3_info->bitrate_table_index][bitrate_index];
	if(layer_index == 3)
		cur_mp3_info->padding_mul = 4;
	else if((layer_index == 1) || (layer_index == 2))
		cur_mp3_info->padding_mul = 1;
	padding = ((mp3_head[first_frame_offset-seek+2]>>1)&0x1) * cur_mp3_info->padding_mul;
	frame_len = cur_mp3_info->samples/8*bitrate/cur_mp3_info->samplingrate + padding;
	MP3_INFO("first frame samples:%d samplingrate:%d channel_mode:%d bitrate:%d frame_len:%d\n",
	 			cur_mp3_info->samples,cur_mp3_info->samplingrate,channel_mode,bitrate,frame_len);
	cur_mp3_info->normal_frame_offset = cur_mp3_info->first_frame_offset+frame_len;
	cur_mp3_info->one_second_frame = cur_mp3_info->samplingrate/cur_mp3_info->samples;

#if MP3_SAVE_INFO
	int32_t ret = 0;
	DIR st_dir;
	os_sema_init(&seek_sema, 1);
    if(!key_seek_list)
       key_seek_list = add_keycallback(key_seek_callback, NULL);
	MP3_INFO("one_second_frame:%d",cur_mp3_info->one_second_frame);
    ret = f_opendir(&st_dir,"0:/MP3_SEEK");
	if(ret == FR_OK) {
		cur_mp3_info->mp3_st_fp = osal_fopen(cur_mp3_info->mp3_st_filename, "r");
		if(cur_mp3_info->mp3_st_fp) {
			if(seektable_check(cur_mp3_info)) {
				return;
			}
			osal_fclose(cur_mp3_info->mp3_st_fp);	
		}
	}
    else{
        ret = f_mkdir("0:/MP3_SEEK");		
    }
	get_curmp3_totaltime_ergodic(fp, first_frame_offset);
	cur_mp3_info->mp3_st_fp = osal_fopen(cur_mp3_info->mp3_st_filename, "wb");
	if(cur_mp3_info->mp3_st_fp) {
		osal_fwrite(cur_mp3_info, 4, 4, cur_mp3_info->mp3_st_fp);
		struct list_head *seek_table_list = cur_mp3_info->seek_table_head.next;
		struct list_head *seek_table_head = &cur_mp3_info->seek_table_head;
		while(seek_table_list != seek_table_head) {
			MP3_SEEK_TABLE *seek_table = list_entry((struct list_head *)seek_table_list,MP3_SEEK_TABLE,sub_table_list);
			osal_fwrite(seek_table->sub_table, 100, 4, cur_mp3_info->mp3_st_fp);
			seek_table_list = seek_table_list->next;
		}
		osal_fclose(cur_mp3_info->mp3_st_fp);
	}
#endif
}

uint32_t get_curmp3_songtime(uint32_t brate)
{
    if(!cur_mp3_info) {
        return 0;
	}
	if(cur_mp3_info->get_totaltime == 1) {
		return cur_mp3_info->totaltime;
	}
	return 0;
}

void clear_curmp3_info(void)
{
    if(cur_mp3_info) {
        MP3_DECODE_FREE(cur_mp3_info);
        cur_mp3_info = NULL;
    }
#if MP3_SAVE_INFO
    if(key_seek_list) {
        remove_keycallback(key_seek_list);
        key_seek_list = NULL;
    }
	struct list_head *list_n = (struct list_head *)&cur_mp3_info->seek_table_head;
	struct list_head *head = (struct list_head *)&cur_mp3_info->seek_table_head;
	MP3_SEEK_TABLE *seek_table = NULL;
	while(list_n->next != head) {
		list_n = list_n->next;
		seek_table = list_entry((struct list_head *)list_n,MP3_SEEK_TABLE,sub_table_list);
		MP3_DECODE_FREE(seek_table);
	}
    os_sema_del(&seek_sema);
#endif
}