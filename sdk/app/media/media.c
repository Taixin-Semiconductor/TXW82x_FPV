#include <stdio.h>
#include "diskio.h"
#include "integer.h"
#include "ff.h"
#include "osal_file.h"
#include "osal/string.h"

#define MIN(a,b) ( (a)>(b)?b:a )
#define MAX(a,b) ( (a)>(b)?a:b )
#define JPG_FILE_NAME "JPG"
#define JPG_DIR "0:/DCIM"
#define VIDEO_DIR "0:/VIDEO"
#define VIDEO_FILE_NAME "AVI"



uint32_t file_mode (const char *mode);
/************************************************************
由于用的是全局变量,所以尽量单线程调用,否则可能有异常
*************************************************************/
FIL fp_jpg;
char jpg_name[50];
char jpg_indx[20];
FIL fp_jpg;
//视频录像文件名
char video_name[50];


static void create_avi_dir(char *dir)
{
    //_os_printf("make avi dir\n");
    f_mkdir(dir);
}


static uint32_t a2i(char *str)
{
    uint32_t ret = 0;
    uint32_t indx =0;
    while(str[indx] != '\0'){
        if(str[indx] >= '0' && str[indx] <='9'){
            ret = ret*10+str[indx]-'0';
        }
        indx ++;
    }
    return ret;
}

#if 0
static void i2a(char *str,uint32_t data)
{
    uint32_t temp = data;
    uint32_t tempa[64] ;
    uint32_t i = 0;
    tempa[i++] = '\0';
    tempa[i++] = data%10 + '0';
    while(temp /= 10)
    {
        tempa[i++] = temp%10 +'0';
    }
    while(*str++ = tempa[--i]);
}
#endif


static void create_jpg_dir(char *dir)
{
   // _os_printf("make jpg dir\n");
    f_mkdir(dir);
}

//设置拍照文件的名字
static void make_jpg_name(char *dest,char* indx,char *dir_name)
{
    char *avi_dir = dir_name;
    char *publicname = JPG_FILE_NAME;
    char *end = ".jpg";
	#if 0
    uint32_t ofs = 0;
    ofs += str_cpy_noend(dest+ofs,avi_dir);
    ofs += str_cpy_noend(dest+ofs,publicname);
    ofs += str_cpy_noend(dest+ofs,indx);
    str_cpy(dest+ofs,end);
	#endif
	sprintf(dest,"%s%s%s%s",avi_dir,publicname,indx,end);
}

int osal_fopen_no_malloc(F_FILE *fp,const char *filename,const char *mode){
	 FRESULT res;
	 uint32_t mod = file_mode (mode);
	
	 //fp = osal_malloc (sizeof (FIL));
	// if (fp == NULL) {
	   // todo errno
	 //  _os_printf("osal_fopen malloc fail!\r\n");
	//	 return fp;
	// }
	// _os_printf("fp:%x\r\n",fp);
	 res = f_open (fp, filename, mod);
	// _os_printf("res:%x\r\n",res);
	 if (FR_OK != res) {
	   // todo errno
	   _os_printf("osal_fopen fail:%d\r\n",res);
	   return 0;
	 }
	
	 return 1;

}



//创建拍照的文件夹并且创建jpg文件
bool create_jpg_file(char *dir_name)
{
    DIR  avi_dir;
    FRESULT ret;
    FILINFO finfo;
	uint32_t indx= 0;
	int fp;

    ret = f_opendir(&avi_dir,dir_name);
    if(ret != FR_OK){
        create_avi_dir(dir_name);
        f_opendir(&avi_dir,dir_name);
    }
    while(1){
        ret = f_readdir(&avi_dir,&finfo);
        if(ret != FR_OK || finfo.fname[0] == 0) break;
        indx = MAX(indx,a2i(finfo.fname));
    }
    f_closedir(&avi_dir);    
    indx++;

	sprintf(jpg_name,"%s/jpeg%d.%s",dir_name,indx,JPG_FILE_NAME);

	
    _os_printf("create jpg file name %s \r\n",jpg_name);
	fp = osal_fopen_no_malloc(&fp_jpg,jpg_name,"wb+");
	if(fp)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
	
}







//获取标号最小的文件
char *get_min_video_file(char *dir_name)
{
	FRESULT ret;
	FILINFO finfo;
	int flag = 0;
	uint32_t indx= 0;
	uint32_t min = ~0;
	DIR  avi_dir;
	ret = f_opendir(&avi_dir,dir_name);
	if(ret != FR_OK)
	{
		return NULL;
	}
    while(1){
        ret = f_readdir(&avi_dir,&finfo);
        if(ret != FR_OK || finfo.fname[0] == 0) break;
        indx = MIN(min,a2i(finfo.fname));
		if(indx < min)
		{
			min = indx;
			sprintf(video_name,"%s/%s",dir_name,finfo.fname);
			flag = 1;
		}
    }
	if(flag)
	{
		_os_printf("%s:%d name:%s\n",__FUNCTION__,__LINE__,video_name);
		return video_name;
	}
	else
	{
		return NULL;
	}	
}



void *create_video_file(char *dir_name)
{
    DIR  avi_dir;
    FRESULT ret;
    FILINFO finfo;
	uint32_t indx= 0;
	void *fp;

    ret = f_opendir(&avi_dir,dir_name);
    if(ret != FR_OK){
        ret = f_mkdir(dir_name);
		if(ret!=FR_OK)
		{
			return NULL;
		}
        ret = f_opendir(&avi_dir,dir_name);
    }
	if(ret!=FR_OK)
	{
		return NULL;
	}

    while(1){
        ret = f_readdir(&avi_dir,&finfo);
        if(ret != FR_OK || finfo.fname[0] == 0) break;
        indx = MAX(indx,a2i(finfo.fname));
    }
    f_closedir(&avi_dir);    
    indx++;


	sprintf(video_name,"%s/avi%d.%s",dir_name,indx,VIDEO_FILE_NAME);
    _os_printf("create video file name %s \r\n",video_name);
	fp = (void*)osal_fopen(video_name,"wb+");
	return fp;
	
}


void fatfs_create_jpg(){
	create_jpg_file(JPG_DIR);
}

void fatfs_write_data(uint8_t *buf,uint32_t len){
	unsigned int bw;
	int res;
	//_os_printf("fatfs_write data:%d\r\n",len);
	res = f_write(&fp_jpg,buf,len,&bw);
	if (FR_OK != res){
		_os_printf("f_write fail:%d\r\n",res);
	}
}

void fatfs_close_jpg(){
	int res;
	res = f_close(&fp_jpg);
	if (FR_OK != res){
		_os_printf("f_close fail:%d\r\n",res);
	}	
}


