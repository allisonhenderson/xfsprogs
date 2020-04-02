/*
 * Copyright (c) 2015 Red Hat, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software Foundation
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

/* Initializes a xfs_parent_ptr from an xfs_parent_name_rec */
void
xfs_init_parent_ptr(struct xfs_parent_ptr		*xpp,
		     struct xfs_parent_name_rec	*rec)
{
	xpp->xpp_ino = be64_to_cpu(rec->p_ino);
	xpp->xpp_gen = be32_to_cpu(rec->p_gen);
	xpp->xpp_diroffset = be32_to_cpu(rec->p_diroffset);
}

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
