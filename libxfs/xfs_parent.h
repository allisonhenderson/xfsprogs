/*
 * Copyright (c) 2018 Oracle, Inc.
 * All Rights Reserved.
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
 * along with this program; if not, write the Free Software Foundation Inc.
 */
#ifndef	__XFS_PARENT_H__
#define	__XFS_PARENT_H__

#include "xfs_da_format.h"
#include "xfs_format.h"

/*
 * Parent pointer attribute prototypes
 */
void xfs_init_parent_name_rec(struct xfs_parent_name_rec *rec,
			      struct xfs_inode *ip,
			      uint32_t p_diroffset);
void xfs_init_parent_name_irec(struct xfs_parent_name_irec *irec,
			       struct xfs_parent_name_rec *rec);
void xfs_init_parent_ptr(struct xfs_parent_ptr *xpp,
			 struct xfs_parent_name_rec *rec);
#endif	/* __XFS_PARENT_H__ */
