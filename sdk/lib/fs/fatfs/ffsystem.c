/*------------------------------------------------------------------------*/
/* A Sample Code of User Provided OS Dependent Functions for FatFs        */
/*------------------------------------------------------------------------*/

#include "ff.h"
#include "osal/string.h"

#if FF_USE_LFN == 3	/* Use dynamic memory allocation */

/*------------------------------------------------------------------------*/
/* Allocate/Free a Memory Block                                           */
/*------------------------------------------------------------------------*/

#include <stdlib.h>		/* with POSIX API */


void* ff_memalloc (	/* Returns pointer to the allocated memory block (null if not enough core) */
	UINT msize		/* Number of bytes to allocate */
)
{
	return os_malloc_psram((size_t)msize);	/* Allocate a new memory block */
}


void ff_memfree (
	void* mblock	/* Pointer to the memory block to free (no effect if null) */
)
{
	os_free_psram(mblock);	/* Free the memory block */
}

#endif




#if FF_FS_REENTRANT	/* Mutal exclusion */
/*------------------------------------------------------------------------*/
/* Definitions of Mutex                                                   */
/*------------------------------------------------------------------------*/

static os_mutex_t *Mutex[FF_VOLUMES + 1];

/*------------------------------------------------------------------------*/
/* Create a Mutex                                                         */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount function to create a new mutex
/  or semaphore for the volume. When a 0 is returned, the f_mount function
/  fails with FR_INT_ERR.
*/

int ff_mutex_create (	/* Returns 1:Function succeeded or 0:Could not create the mutex */
	int vol				/* Mutex ID: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES) */
)
{
	int ret = 1;

	if(!Mutex[vol])
	{
		Mutex[vol] = os_zalloc(sizeof(os_mutex_t));
		int mutex_ret = os_mutex_init(Mutex[vol]);
		if(mutex_ret)
		{
			ret = 0;
			os_free(Mutex[vol]);
			Mutex[vol] = NULL;
		}
	}
	return ret;
}


/*------------------------------------------------------------------------*/
/* Delete a Mutex                                                         */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount function to delete a mutex or
/  semaphore of the volume created with ff_mutex_create function.
*/

void ff_mutex_delete (	/* Returns 1:Function succeeded or 0:Could not delete due to an error */
	int vol				/* Mutex ID: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES) */
)
{
	//不移除信号量
	(void) vol;
}


/*------------------------------------------------------------------------*/
/* Request a Grant to Access the Volume                                   */
/*------------------------------------------------------------------------*/
/* This function is called on enter file functions to lock the volume.
/  When a 0 is returned, the file function fails with FR_TIMEOUT.
*/

int ff_mutex_take (	/* Returns 1:Succeeded or 0:Timeout */
	int vol			/* Mutex ID: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES) */
)
{
	int ret;
	ret = os_mutex_lock(Mutex[vol], FF_FS_TIMEOUT);
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
/* Release a Grant to Access the Volume                                   */
/*------------------------------------------------------------------------*/
/* This function is called on leave file functions to unlock the volume.
*/

void ff_mutex_give (
	int vol			/* Mutex ID: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES) */
)
{
	os_mutex_unlock(Mutex[vol]);
}

#endif	/* FF_FS_REENTRANT */

