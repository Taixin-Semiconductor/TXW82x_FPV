#include "basic_include.h"
#include "lib/posix/stdio.h"
#include "lib/multimedia/msi.h"
#include "fatfs/osal_file.h"
#include "fatfs/ff.h"

#define MSI_URLFILE (0)
#if MSI_URLFILE
#include "lib/net/urlfile/urlfile.h"
#endif

enum MSI_FILE_CMD {
    MSI_FILE_CMD_OPEN,
    MSI_FILE_CMD_CLOSE,
};

enum MSI_FILE_STATE {
    MSI_FILE_STATE_IDLE,
    MSI_FILE_STATE_OPEN,
    MSI_FILE_STATE_READ,
};

#define MSI_FILENAME_MAX (512)
#define MSI_FILE_CMD_MAX (8)
#define MSI_FILE_NUM_TRY (5)

#define MSIFILE_DBG(fmt, ...)   os_printf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define MSIFILE_ERR(fmt, ...)   os_printf(KERN_ERR"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define URLFILE(file)          (os_strstr(file, "://") != NULL)

struct msi_file {
    struct os_task task;
    struct msi    *msi;
    const char    *name;
    uint8  running: 1, eof: 1, filelist: 1, rev: 5;
    uint8  state;
    uint16 file_idx;
    void  *fp;
    char   file_name[MSI_FILENAME_MAX];
    char  *file_ext;     //文件扩展名
    atomic8_t fb_cnt;    //未被释放的framebuff个数
    uint8 fb_max;        //允许使用的framebuff最大个数
    uint8 speed;         //读取速度，用于倍速播放控制
    RBUFFER_DEF(cmd, uint8, MSI_FILE_CMD_MAX);
};

extern void *fopen(const char *filename, const char *mode);
extern size_t fread(void *ptr, size_t size, size_t nmemb, void *stream);
extern int fclose(void *stream);

static void *msi_filestream_fopen(struct msi_file *msifile)
{
#if MSI_URLFILE	
    if(URLFILE(msifile->file_name))
        return uf_open(msifile->file_name, 0);
#endif
	return fopen(msifile->file_name, "r");
}
static int msi_filestream_fclose(struct msi_file *msifile)
{
    if(msifile->fp){
#if MSI_URLFILE
        if(URLFILE(msifile->file_name))
            return uf_close(msifile->fp);
#endif
        return fclose(msifile->fp);
    }
    return 0;
}
static size_t msi_filestream_fread(void *ptr, size_t size, size_t nmemb, struct msi_file *msifile)
{
#if MSI_URLFILE
    if(URLFILE(msifile->file_name))
        return uf_read(ptr, size, nmemb, msifile->fp);
#endif
    return fread(ptr, size, nmemb, msifile->fp);
}
static int msi_filestream_fseek(struct msi_file *msifile, off_t offset, int whence)
{
#if MSI_URLFILE
    if(URLFILE(msifile->file_name))
        return uf_seek(msifile->fp, offset, whence);
#endif
    return fseek(msifile->fp, offset, whence);//
}
static int msi_filestream_feof(struct msi_file *msifile)
{
#if MSI_URLFILE
    if(URLFILE(msifile->file_name))
        return uf_eof(msifile->fp);
#endif
    return feof(msifile->fp);
}

static int32 msi_filestream_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    struct msi_file *msifile = (struct msi_file *)msi->priv;

    switch (cmd_id) {
        case MSI_CMD_POST_DESTROY:
            while (msifile->running) {
                os_sleep_ms(10);
            }
            os_free(msifile);
            break;
        case MSI_CMD_FREE_FB:
            break;
        case MSI_CMD_SET_SPEED: //设置播放倍速，控制数据读取行为
            break;
        default:
            break;
    }
    return ret;
}

static void msi_filestream_check_filename(struct msi_file *msifile)
{
    msifile->filelist = 0;
    msifile->file_idx = 0;

    //检查是否存在文件列表，支持自动切换文件，格式要求：xxxx_001.avi
    if (msifile->file_ext) {
        char *ptr = msifile->file_ext - 4;
        if (ptr[0] == '_' && isdigit(ptr[1]) && isdigit(ptr[2]) && isdigit(ptr[3])) {
            msifile->filelist = 1;
            msifile->file_idx = os_atoi(ptr + 1);
            MSIFILE_DBG("start index %d\r\n", msifile->file_idx);
        }
    }
}

static int32 msi_filestream_open_file(struct msi_file *msifile)
{
    char *ptr;
    int32 i = 0;

    msi_filestream_fclose(msifile);
    if (msifile->eof) {
        if (!msifile->filelist) {
            MSIFILE_ERR("read over!\r\n");
            msifile->state = MSI_FILE_STATE_IDLE;
            SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_READ_EOF, (uint32)msifile);
            return RET_ERR;
        }

        ptr = msifile->file_ext - 4;
        for (i = 1; i <= MSI_FILE_NUM_TRY; i++) {
            os_sprintf(ptr + 1, "%03d", msifile->file_idx + i);
            msifile->fp = msi_filestream_fopen(msifile->file_name);
            if (msifile->fp) {
                MSIFILE_ERR("open %s success!\r\n", msifile->file_name);
                msifile->file_idx += i;
                msifile->state = MSI_FILE_STATE_READ;
                msifile->eof = 0;
                return RET_OK;
            } else {
                MSIFILE_ERR("open %s fail!\r\n", msifile->file_name);
            }
        }

        os_sprintf(ptr + 1, "%03d", msifile->file_idx);
        MSIFILE_ERR("read over! last file:%s\r\n", msifile->file_name);
        SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_READ_EOF, (uint32)msifile);
    } else {
        msifile->file_ext = os_strrchr(msifile->file_name, '.'); //文件扩展名
        msifile->fp = msi_filestream_fopen(msifile->file_name);
        if (msifile->fp) {
            msi_filestream_check_filename(msifile);
        } else {
            SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_OPEN_FAIL, (uint32)msifile);
            MSIFILE_ERR("open %s fail!\r\n", msifile->file_name);
        }
    }

    //根据扩展名，加载对应的插件

    //////////////////////////////////////////////////////////////////////////////////
    msifile->state = msifile->fp ? MSI_FILE_STATE_READ : MSI_FILE_STATE_IDLE;
    return msifile->fp ? RET_OK : RET_ERR;
}

static int32 msi_filestream_read_file(struct msi_file *msifile, struct framebuff **fb)
{
    if (atomic_read(&msifile->fb_cnt) >= msifile->fb_max + 1) {
        return -ENOMEM;
    }

    //插件参与控制读取数据的行为: 比如 按帧读取数据，倍速跳帧

    return RET_OK;
}

static void msi_filestream_proc_cmd(struct msi_file *msifile)
{
    uint8 cmd;
    while (!RB_EMPTY(&msifile->cmd)) {
        RB_GET(&msifile->cmd, cmd);
        switch (cmd) {
            case MSI_FILE_CMD_OPEN:
                msifile->eof   = 0;
                msifile->state = MSI_FILE_STATE_OPEN;
                break;
            case MSI_FILE_CMD_CLOSE:
                msifile->state = MSI_FILE_STATE_IDLE;
                break;
        }
    }
}

static void msi_filestream_task(void *arg)
{
    int32 ret = 0;
    struct framebuff *fb = NULL;
    struct msi_file *msifile = (struct msi_file *)arg;

    MSIFILE_DBG("msi_file running ...\r\n");
    msifile->running = 1;
    while (msifile->msi->enable) {
        msi_filestream_proc_cmd(msifile);
        switch (msifile->state) {
            case MSI_FILE_STATE_IDLE:
                os_sleep_ms(10);
                break;
            case MSI_FILE_STATE_OPEN:
                msi_filestream_open_file(msifile);
                break;
            case MSI_FILE_STATE_READ:
                fb  = NULL;
                ret = msi_filestream_read_file(msifile, &fb);
                if (fb) {
                    atomic_inc(&msifile->fb_cnt);
                    msi_output_fb(msifile->msi, fb);
                }

                if (msifile->eof) { //当前文件读取结束
                    msifile->state = MSI_FILE_STATE_OPEN;
                } else if (ret < 0 && msifile->msi->enable) {
                    os_sleep_ms(10);
                }
                break;
        }
    }

    if (msifile->fp) {
        msi_filestream_fclose(msifile);
        msifile->fp = NULL;
    }

    MSIFILE_DBG("msi_file stopped!\r\n");
    msifile->running = 0;
}

void *msi_filestream_init(const char *name, uint8 fb_max)
{
    struct msi_file *msifile = os_zalloc(sizeof(struct msi_file));
    if (msifile == NULL) {
        MSIFILE_ERR("no memory!\r\n");
        return NULL;
    }

    RB_INIT(&msifile->cmd, MSI_FILE_CMD_MAX);
    msifile->fb_max = fb_max;
    msifile->name   = name;
    msifile->msi    = msi_new(name, 0, NULL);
    if (msifile->msi == NULL) {
        os_free(msifile);
        MSIFILE_ERR("no memory!\r\n");
        return NULL;
    }

    ASSERT(msifile->msi->priv == NULL);
    msifile->msi->priv   = msifile;
    msifile->msi->action = msi_filestream_action;
    msifile->msi->enable = 1;
    OS_TASK_INIT(name, &msifile->task, msi_filestream_task, msifile, OS_TASK_PRIORITY_NORMAL, NULL,1024);
    MSIFILE_DBG("msi_file %s init!\r\n", name);
    return msifile;
}

int32 msi_filestream_destory(void *hdl)
{
    if (hdl) {
        struct msi_file *msifile = (struct msi_file *)hdl;
        msi_destroy(msifile->msi); //msi被释放时会通过MSI_CMD_DESTORY通知msifile释放资源
        return RET_OK;
    } else {
        return -EINVAL;
    }
}

int32 msi_filestream_close(void *hdl)
{
    if (hdl) {
        struct msi_file *msifile = (struct msi_file *)hdl;
        RB_SET(&msifile->cmd, MSI_FILE_CMD_CLOSE);
        return RET_OK;
    } else {
        return -EINVAL;
    }
}

int32 msi_filestream_open(void *hdl, char *file)
{
    if (hdl) {
        struct msi_file *msifile = (struct msi_file *)hdl;
        if (os_strlen(file) >= MSI_FILENAME_MAX) {
            return -ENAMETOOLONG;
        }
        os_strcpy(msifile->file_name, file);
        RB_SET(&msifile->cmd, MSI_FILE_CMD_OPEN);
        return RET_OK;
    } else {
        return -EINVAL;
    }
}

