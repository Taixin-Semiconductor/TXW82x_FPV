/**
 * @file lv_fs_vfs.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../../lvgl.h"

#if LV_USE_FS_VFS
#include <stdio.h>

/*********************
 *      DEFINES
 *********************/

#if LV_FS_VFS_LETTER == '\0'
    #error "LV_FS_VFS_LETTER must be an upper case ASCII letter"
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void fs_init(void);

static void * fs_open(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode);
static lv_fs_res_t fs_close(lv_fs_drv_t * drv, void * file_p);
static lv_fs_res_t fs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br);
static lv_fs_res_t fs_write(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw);
static lv_fs_res_t fs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence);
static lv_fs_res_t fs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_fs_vfs_init(void)
{
    /*----------------------------------------------------
     * Initialize your storage device and File System
     * -------------------------------------------------*/
    fs_init();

    /*---------------------------------------------------
     * Register the file system interface in LVGL
     *--------------------------------------------------*/

    /*Add a simple drive to open images*/
    static lv_fs_drv_t fs_drv; /*A driver descriptor*/
    lv_fs_drv_init(&fs_drv);

    /*Set up fields...*/
    fs_drv.letter = LV_FS_VFS_LETTER;
    fs_drv.cache_size = LV_FS_VFS_CACHE_SIZE;

    fs_drv.open_cb = fs_open;
    fs_drv.close_cb = fs_close;
    fs_drv.read_cb = fs_read;
    fs_drv.write_cb = fs_write;
    fs_drv.seek_cb = fs_seek;
    fs_drv.tell_cb = fs_tell;

    fs_drv.dir_close_cb = NULL;
    fs_drv.dir_open_cb = NULL;
    fs_drv.dir_read_cb = NULL;

    lv_fs_drv_register(&fs_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your Storage device and File system.*/
static void fs_init(void)
{
    /*Initialize your storage device and file system.
     *Better to do it in your code to keep this library untouched for easy updating*/
}

/**
 * Open a file
 * @param drv pointer to a driver where this function belongs
 * @param path path to the file beginning with the driver letter (e.g. S:/folder/file.txt)
 * @param mode read: FS_MODE_RD, write: FS_MODE_WR, both: FS_MODE_RD | FS_MODE_WR
 * @return pointer to FILE struct or NULL in case of fail
 */
static void * fs_open(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode)
{
    LV_UNUSED(drv);
    const char * flags = "";
    if(mode == LV_FS_MODE_WR) flags = "w";
    else if(mode == LV_FS_MODE_RD) flags = "r";
    else if(mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) flags = "w+";
    FILE * f = fopen(path, flags);
    return f;
}

/**
 * Close an opened file
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a FILE variable. (opened with fs_open)
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_close(lv_fs_drv_t * drv, void * file_p)
{
    LV_UNUSED(drv);
    fclose((FILE *)file_p);
    return LV_FS_RES_OK;
}

/**
 * Read data from an opened file
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a FILE variable.
 * @param buf pointer to a memory block where to store the read data
 * @param btr number of Bytes To Read
 * @param br the real number of read bytes (Byte Read)
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br)
{
    LV_UNUSED(drv);
    *br = fread(buf, 1, btr, (FILE *)file_p);
    if(ferror((FILE *)file_p)) return LV_FS_RES_UNKNOWN;
    return LV_FS_RES_OK;
}

/**
 * Write into a file
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a FILE variable
 * @param buf pointer to a buffer with the bytes to write
 * @param btw Bytes To Write
 * @param bw the number of real written bytes (Bytes Written). NULL if unused.
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_write(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw)
{
    LV_UNUSED(drv);
    *bw = fwrite(buf, 1, btw, (FILE *)file_p);
    if(ferror((FILE *)file_p)) return LV_FS_RES_UNKNOWN;
    return LV_FS_RES_OK;
}

/**
 * Set the read write pointer. Also expand the file size if necessary.
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a FILE variable. (opened with fs_open)
 * @param pos the new position of read write pointer
 * @param whence LV_FS_SEEK_SET, LV_FS_SEEK_CUR or LV_FS_SEEK_END
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence)
{
    LV_UNUSED(drv);
    int origin = SEEK_SET;

    switch(whence) {
        case LV_FS_SEEK_SET:
            origin = SEEK_SET;
            break;
        case LV_FS_SEEK_CUR:
            origin = SEEK_CUR;
            break;
        case LV_FS_SEEK_END:
            origin = SEEK_END;
            break;
        default:
            break;
    }

    fseek((FILE *)file_p, pos, origin);
    return LV_FS_RES_OK;
}

/**
 * Give the position of the read write pointer
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a FILE variable.
 * @param pos_p pointer to to store the result
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p)
{
    LV_UNUSED(drv);
    *pos_p = ftell((FILE *)file_p);
    return LV_FS_RES_OK;
}

#else /*LV_USE_FS_VFS == 0*/

#if defined(LV_FS_VFS_LETTER) && LV_FS_VFS_LETTER != '\0'
    #warning "LV_USE_FS_VFS is not enabled but LV_FS_VFS_LETTER is set"
#endif

#endif /*LV_USE_FS_VFS*/
