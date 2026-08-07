/*
 * Copyright 2024, Hiroyuki OYAMA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef FAT_H
#define FAT_H
/** \file fat.h
 *  \defgroup filesystem_fat filesystem_fat
 *  \ingroup filesystem
 *  \brief FAT file system
 */
#ifdef __cplusplus
extern "C" {
#endif

#include "filesystem.h"

//#define	S_IRWXU 	(S_IRUSR | S_IWUSR | S_IXUSR)
//#define		S_IRUSR	0000400	/* read permission, owner */
//#define		S_IWUSR	0000200	/* write permission, owner */
//#define		S_IXUSR 0000100/* execute/search permission, owner */
//#define	S_IRWXG		(S_IRGRP | S_IWGRP | S_IXGRP)
//#define		S_IRGRP	0000040	/* read permission, group */
//#define		S_IWGRP	0000020	/* write permission, grougroup */
//#define		S_IXGRP 0000010/* execute/search permission, group */
//#define	S_IRWXO		(S_IROTH | S_IWOTH | S_IXOTH)
//#define		S_IROTH	0000004	/* read permission, other */
//#define		S_IWOTH	0000002	/* write permission, other */
//#define		S_IXOTH 0000001/* execute/search permission, other */

#ifndef S_IFDIR
#define		_IFDIR	0040000	/* directory */
#define	S_IFDIR		0100000
#endif

#ifndef S_IFREG
#define		_IFREG	0040000	/* directory */
#define	S_IFREG		_IFREG
#endif



#define FFS_DBG  0


/*! \brief Create FAT file system object
 * \ingroup filesystem_fat
 *
 * \return File system object. Returns NULL in case of failure.
 * \retval NULL failed to create file system object.
 */
filesystem_t *filesystem_fat_create();

/*! \brief Release FAT file system object
 * \ingroup filesystem_fat
 *
 * \param fs FAT file system object
 */
void filesystem_fat_free(filesystem_t *fs);

#ifdef __cplusplus
}
#endif
#endif
