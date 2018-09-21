// SPDX-License-Identifier: LGPL-2.1
/*
 * Copyright (c) 2005 Silicon Graphics, Inc.
 * All Rights Reserved.
 */
#ifndef __PARENT_H__
#define	__PARENT_H__

typedef struct parent {
	__u64	p_ino;
	__u32	p_gen;
	__u16	p_reclen;
	char	p_name[1];
} parent_t;

typedef struct parent_cursor {
	__u32	opaque[4];      /* an opaque cookie */
} parent_cursor_t;

/* Print parent pointer option flags */
#define XFS_PPPTR_OFLAG_SHORT  (1<<0)	/* Print in short format */

struct path_list;

typedef int (*walk_pptr_fn)(struct xfs_pptr_info *pi, struct xfs_parent_ptr *pptr,
		void *arg, int flags);
typedef int (*walk_ppath_fn)(const char *mntpt, struct path_list *path,
		void *arg);

#define WALK_PPTRS_ABORT	1
int fd_walk_pptrs(int fd, uint64_t pino, char *pname, walk_pptr_fn fn,
		void *arg, int flags);
int handle_walk_pptrs(void *hanp, size_t hanlen, uint64_t pino, char *pname,
		walk_pptr_fn fn, void *arg, int flags);

#define WALK_PPATHS_ABORT	1
int fd_walk_ppaths(int fd, uint64_t pino, char *pname, walk_ppath_fn fn,
		void *arg, int flags);
int handle_walk_ppaths(void *hanp, size_t hanlen, uint64_t pino, char *pname,
		walk_ppath_fn fn, void *arg, int flags);

int fd_to_path(int fd, char *path, size_t pathlen);
int handle_to_path(void *hanp, size_t hlen, char *path, size_t pathlen);

#endif
