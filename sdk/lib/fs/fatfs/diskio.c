/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h" /* FatFs lower layer API */




/* Definitions of physical drive number for each drive */

#define FATFS_MAX_DRIVE_NUM 4
static struct fatfs_diskio* phys_drive[FATFS_MAX_DRIVE_NUM];
static void* phys_device[FATFS_MAX_DRIVE_NUM];




/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv /* Physical drive nmuber to identify the drive */
                    )
{
  if(pdrv >= FATFS_MAX_DRIVE_NUM || NULL == phys_drive[pdrv] || NULL == phys_drive[pdrv]->status)
    return STA_NOINIT;

  return (*phys_drive[pdrv]->status)(phys_device[pdrv]);
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE pdrv /* Physical drive nmuber to identify the drive */
                        )
{
  if(pdrv >= FATFS_MAX_DRIVE_NUM || NULL == phys_drive[pdrv] || NULL == phys_drive[pdrv]->init)
    return STA_NOINIT;

  return (*phys_drive[pdrv]->init)(phys_device[pdrv]);
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv,    /* Physical drive nmuber to identify the drive */
                  BYTE* buff,   /* Data buffer to store read data */
                  DWORD sector, /* Start sector in LBA */
                  UINT count    /* Number of sectors to read */
                  )
{
  if(pdrv >= FATFS_MAX_DRIVE_NUM || NULL == phys_drive[pdrv] || NULL == phys_drive[pdrv]->read)
    return RES_PARERR;

  return (*phys_drive[pdrv]->read)(phys_device[pdrv], buff, sector, count);

}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write(BYTE pdrv,        /* Physical drive nmuber to identify the drive */
                   const BYTE* buff, /* Data to be written */
                   DWORD sector,     /* Start sector in LBA */
                   UINT count        /* Number of sectors to write */
                   )
{
  if(pdrv >= FATFS_MAX_DRIVE_NUM || NULL == phys_drive[pdrv] || NULL == phys_drive[pdrv]->write)
    return RES_PARERR;
  	
  return (*phys_drive[pdrv]->write)(phys_device[pdrv], (BYTE*)buff, sector, count);
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, /* Physical drive nmuber (0..) */
                   BYTE cmd,  /* Control code */
                   void* buff /* Buffer to send/receive control data */
                   )
{
  if(pdrv >= FATFS_MAX_DRIVE_NUM || NULL == phys_drive[pdrv] || NULL == phys_drive[pdrv]->ioctl)
    return RES_PARERR;

  return (*phys_drive[pdrv]->ioctl)(phys_device[pdrv], cmd, buff);
}

void fatfs_register_drive(BYTE pdrv, struct fatfs_diskio* driver, void *device)
{
  if(pdrv < FATFS_MAX_DRIVE_NUM) {
    phys_drive[pdrv]  = driver;
    phys_device[pdrv] = device;
  }// else
    //TEST_INFO_SHOW("Drive Number %d too large", pdrv);
}
