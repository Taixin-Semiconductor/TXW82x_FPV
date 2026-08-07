/*------------------------------------------------------------------------*/
/* Sample code of OS dependent controls for FatFs                         */
/* (C)ChaN, 2012                                                          */
/*------------------------------------------------------------------------*/

#include <stdlib.h>		/* ANSI process controls */
#include <stdio.h>

#include "ff.h"
#include "csi_kernel.h"
#include "osal/string.h"
//#include "osal/osal_sema.h"
//#include "osal/osal_alloc.h"



#if FF_FS_REENTRANT
/*------------------------------------------------------------------------*/
/* Create a Synchronization Object                                        */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount() function to create a new
   synchronization object, such as semaphore and mutex. When a 0 is returned,
   the f_mount() function fails with FR_INT_ERR.
*/

int ff_cre_syncobj (	/* 1:Function succeeded, 0:Could not create due to any error */
	BYTE vol,			/* Corresponding logical drive being processed */
	FF_SYNC_t *sobj		/* Pointer to return the created sync object */
)
{
	
	int ret = 1;
	static os_mutex_t *sem[FF_VOLUMES];	/* FreeRTOS */

	//只会申请一次,不考虑会失败
	if(!sem[vol])
	{
		sem[vol] = os_zalloc(sizeof(os_mutex_t));
		int mutex_ret = os_mutex_init(sem[vol]);
		if(mutex_ret)
		{
			ret = 0;
			os_free(sem[vol]);
			sem[vol] = NULL;
		}
		else
		{
			*sobj = sem[vol];
		}
	}
	return ret;
}



/*------------------------------------------------------------------------*/
/* Delete a Synchronization Object                                        */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount() function to delete a synchronization
/  object that created with ff_cre_syncobj function. When a 0 is returned,
/  the f_mount() function fails with FR_INT_ERR.
*/

int ff_del_syncobj (	/* 1:Function succeeded, 0:Could not delete due to any error */
	FF_SYNC_t sobj		/* Sync object tied to the logical drive to be deleted */
)
{
	int ret = 1;
	//不移除信号量
	return ret;

}



/*------------------------------------------------------------------------*/
/* Request Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on entering file functions to lock the volume.
/  When a 0 is returned, the file function fails with FR_TIMEOUT.
*/

int ff_req_grant (	/* 1:Got a grant to access the volume, 0:Could not get a grant */
	FF_SYNC_t sobj	/* Sync object to wait */
)
{
	int ret;
	ret = os_mutex_lock(sobj,FF_FS_TIMEOUT);
	if(!ret)
	{
		ret = 1;
	}
	else
	{
		ret = 0;
	}
	return ret;
}



/*------------------------------------------------------------------------*/
/* Release Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on leaving file functions to unlock the volume.
*/

void ff_rel_grant (
	FF_SYNC_t sobj	/* Sync object to be signaled */
)
{
	os_mutex_unlock(sobj);
}

#endif




#if FF_USE_LFN == 3	/* LFN with a working buffer on the heap */
/*------------------------------------------------------------------------*/
/* Allocate a memory block                                                */
/*------------------------------------------------------------------------*/
/* If a NULL is returned, the file function fails with FR_NOT_ENOUGH_CORE.
*/

void* ff_memalloc (	/* Returns pointer to the allocated memory block */
	UINT msize		/* Number of bytes to allocate */
)
{
	//_os_printf("msize:%d\r\n",msize);
	return os_malloc_psram(msize);	/* Allocate a memory block with POSIX API */
}


/*------------------------------------------------------------------------*/
/* Free a memory block                                                    */
/*------------------------------------------------------------------------*/


void ff_memfree (
	void* mblock	/* Pointer to the memory block to free */
)
{
	os_free_psram(mblock);
}

#endif
