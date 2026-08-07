/***************************************************
    该demo主要是录制AVI使用
***************************************************/
#include "basic_include.h"
#include "stream_frame.h"
#include "avi_record_msi.h"
#include "osal_file.h"

int32 demo_atcmd_save_avi(const char *cmd, char *argv[], uint32 argc)
{
	#if OPENDML_EN && SDH_EN && FS_EN
    uint32_t fps = 0;
    uint32_t frq = 0;
    uint32_t record_num = 0;
    if(argc < 3)
    {
        os_printf("%s argc too small:%d,should more 2 arg\n",__FUNCTION__,argc);
        return 0;
    }

    if(os_atoi(argv[0]) == 0)
    {
        avi_record_msi_deinit();
    }
    else if(os_atoi(argv[0]) == 1)
    {
            fps = os_atoi(argv[1]);
            frq = os_atoi(argv[2]);
            if(argc > 3)
            {
                record_num = os_atoi(argv[3]);  
            }
    
            if(record_num == 0)
            {
                record_num = 1;     
            }
            os_printf("fps:%d\tfrq:%d\n",fps,frq);
            avi_record_msi_init(1280, 720, fps, frq, 30, NULL, NULL); 
    }
	#endif

    return 0;
}


