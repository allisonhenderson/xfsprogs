// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 Oracle, Inc.
 * All Rights Reserved.
 */
#ifndef	__XFS_PARENT_H__
#define	__XFS_PARENT_H__

/*
 * Dynamically allocd structure used to wrap the needed data to pass around
 * the defer ops machinery
 */
struct xfs_parent_defer {
	struct xfs_parent_name_rec	rec;
	struct xfs_da_args		args;
};

/*
 * Parent pointer attribute prototypes
 */
void xfs_init_parent_name_rec(struct xfs_parent_name_rec *rec,
			      struct xfs_inode *ip,
			      uint32_t p_diroffset);
void xfs_init_parent_name_irec(struct xfs_parent_name_irec *irec,
			       struct xfs_parent_name_rec *rec);
int xfs_parent_init(xfs_mount_t *mp, struct xfs_parent_defer **parentp);
int xfs_parent_defer_add(struct xfs_trans *tp, struct xfs_parent_defer *parent,
			 struct xfs_inode *dp, struct xfs_name *parent_name,
			 xfs_dir2_dataptr_t diroffset, struct xfs_inode *child);
int xfs_parent_defer_remove(struct xfs_trans *tp, struct xfs_inode *dp,
			    struct xfs_parent_defer *parent,
			    xfs_dir2_dataptr_t diroffset,
			    struct xfs_inode *child);
void xfs_parent_cancel(xfs_mount_t *mp, struct xfs_parent_defer *parent);

#endif	/* __XFS_PARENT_H__ */
