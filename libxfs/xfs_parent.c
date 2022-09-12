// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 Oracle, Inc.
 * All rights reserved.
 */
#include "libxfs_priv.h"
#include "xfs_shared.h"
#include "xfs_format.h"
#include "xfs_log_format.h"
#include "xfs_trans_resv.h"
#include "xfs_mount.h"
#include "xfs_inode.h"
#include "xfs_trans_resv.h"
#include "xfs_mount.h"
#include "xfs_trace.h"
#include "xfs.h"
#include "xfs_fs.h"
#include "xfs_da_format.h"
#include "xfs_bmap_btree.h"
#include "xfs_trans.h"
#include "xfs_da_btree.h"
#include "xfs_attr.h"
#include "xfs_da_btree.h"
#include "xfs_attr_sf.h"
#include "xfs_bmap.h"
#include "xfs_parent.h"
#include "xfs_da_format.h"
#include "xfs_format.h"
#include "xfs_trans_space.h"

/*
 * Parent pointer attribute handling.
 *
 * Because the attribute value is a filename component, it will never be longer
 * than 255 bytes. This means the attribute will always be a local format
 * attribute as it is xfs_attr_leaf_entsize_local_max() for v5 filesystems will
 * always be larger than this (max is 75% of block size).
 *
 * Creating a new parent attribute will always create a new attribute - there
 * should never, ever be an existing attribute in the tree for a new inode.
 * ENOSPC behavior is problematic - creating the inode without the parent
 * pointer is effectively a corruption, so we allow parent attribute creation
 * to dip into the reserve block pool to avoid unexpected ENOSPC errors from
 * occurring.
 */


/* Initializes a xfs_parent_name_rec to be stored as an attribute name */
void
xfs_init_parent_name_rec(
	struct xfs_parent_name_rec	*rec,
	struct xfs_inode		*ip,
	uint32_t			p_diroffset)
{
	xfs_ino_t			p_ino = ip->i_ino;
	uint32_t			p_gen = VFS_I(ip)->i_generation;

	rec->p_ino = cpu_to_be64(p_ino);
	rec->p_gen = cpu_to_be32(p_gen);
	rec->p_diroffset = cpu_to_be32(p_diroffset);
}

/* Initializes a xfs_parent_name_irec from an xfs_parent_name_rec */
void
xfs_init_parent_name_irec(
	struct xfs_parent_name_irec	*irec,
	struct xfs_parent_name_rec	*rec)
{
	irec->p_ino = be64_to_cpu(rec->p_ino);
	irec->p_gen = be32_to_cpu(rec->p_gen);
	irec->p_diroffset = be32_to_cpu(rec->p_diroffset);
}

int
xfs_parent_init(
	struct xfs_mount		*mp,
	struct xfs_parent_defer		**parentp)
{
	struct xfs_parent_defer		*parent;
	int				error;

	if (!xfs_has_parent(mp))
		return 0;

	error = xfs_attr_use_log_assist(mp);
	if (error)
		return error;

	parent = malloc(sizeof(*parent));
	if (!parent)
		return -ENOMEM;

	/* init parent da_args */
	parent->args.geo = mp->m_attr_geo;
	parent->args.whichfork = XFS_ATTR_FORK;
	parent->args.attr_filter = XFS_ATTR_PARENT;
	parent->args.op_flags = XFS_DA_OP_OKNOENT | XFS_DA_OP_LOGGED;
	parent->args.name = (const uint8_t *)&parent->rec;
	parent->args.namelen = sizeof(struct xfs_parent_name_rec);

	*parentp = parent;
	return 0;
}

int
xfs_parent_defer_add(
	struct xfs_trans	*tp,
	struct xfs_parent_defer	*parent,
	struct xfs_inode	*dp,
	struct xfs_name		*parent_name,
	xfs_dir2_dataptr_t	diroffset,
	struct xfs_inode	*child)
{
	struct xfs_da_args	*args = &parent->args;

	xfs_init_parent_name_rec(&parent->rec, dp, diroffset);
	args->hashval = xfs_da_hashname(args->name, args->namelen);

	args->trans = tp;
	args->dp = child;
	if (parent_name) {
		parent->args.value = (void *)parent_name->name;
		parent->args.valuelen = parent_name->len;
	}

	return xfs_attr_defer_add(args);
}

int
xfs_parent_defer_remove(
	struct xfs_trans	*tp,
	struct xfs_inode	*dp,
	struct xfs_parent_defer	*parent,
	xfs_dir2_dataptr_t	diroffset,
	struct xfs_inode	*child)
{
	struct xfs_da_args	*args = &parent->args;

	xfs_init_parent_name_rec(&parent->rec, dp, diroffset);
	args->trans = tp;
	args->dp = child;
	args->hashval = xfs_da_hashname(args->name, args->namelen);
	return xfs_attr_defer_remove(args);
}

void
xfs_parent_cancel(
	xfs_mount_t		*mp,
	struct xfs_parent_defer *parent)
{
	xlog_drop_incompat_feat(mp->m_log);
	free(parent);
}

unsigned int
xfs_pptr_calc_space_res(
	struct xfs_mount	*mp,
	unsigned int	namelen)
{
	/*
	 * Pptrs are always the first attr in an attr tree, and never larger
	 * than a block
	 */
	return XFS_DAENTER_SPACE_RES(mp, XFS_ATTR_FORK) +
	       XFS_NEXTENTADD_SPACE_RES(mp, namelen, XFS_ATTR_FORK);
}

