// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 Oracle, Inc.
 * All Rights Reserved.
 */
#ifndef	__XFS_PARENT_H__
#define	__XFS_PARENT_H__

extern struct kmem_cache	*xfs_parent_intent_cache;

/* Metadata validators */
bool xfs_parent_namecheck(struct xfs_mount *mp,
		const struct xfs_parent_name_rec *rec, size_t reclen,
		unsigned int attr_flags);
bool xfs_parent_valuecheck(struct xfs_mount *mp, const void *value,
		size_t valuelen);

/*
 * Incore version of a parent pointer, also contains dirent name so callers
 * can pass/obtain all the parent pointer information in a single structure
 */
struct xfs_parent_name_irec {
	/* Key fields for looking up a particular parent pointer. */
	xfs_ino_t		p_ino;
	uint32_t		p_gen;
	xfs_dir2_dataptr_t	p_diroffset;

	/* Attributes of a parent pointer. */
	uint8_t			p_namelen;
	unsigned char		p_name[MAXNAMELEN];
};

void xfs_parent_irec_from_disk(struct xfs_parent_name_irec *irec,
		const struct xfs_parent_name_rec *rec,
		const void *value, int valuelen);

/*
 * Dynamically allocd structure used to wrap the needed data to pass around
 * the defer ops machinery
 */
struct xfs_parent_defer {
	struct xfs_parent_name_rec	rec;
	struct xfs_parent_name_rec	old_rec;
	struct xfs_da_args		args;
	bool				have_log;
};

/*
 * Parent pointer attribute prototypes
 */
void xfs_init_parent_name_rec(struct xfs_parent_name_rec *rec,
			      struct xfs_inode *ip,
			      uint32_t p_diroffset);
int __xfs_parent_init(struct xfs_mount *mp, bool grab_log,
		struct xfs_parent_defer **parentp);

static inline int
xfs_parent_start(
	struct xfs_mount	*mp,
	struct xfs_parent_defer	**pp)
{
	*pp = NULL;

	if (xfs_has_parent(mp))
		return __xfs_parent_init(mp, true, pp);
	return 0;
}

static inline int
xfs_parent_start_locked(
	struct xfs_mount	*mp,
	struct xfs_parent_defer	**pp)
{
	*pp = NULL;

	if (xfs_has_parent(mp))
		return __xfs_parent_init(mp, false, pp);
	return 0;
}

int xfs_parent_defer_add(struct xfs_trans *tp, struct xfs_parent_defer *parent,
			 struct xfs_inode *dp, struct xfs_name *parent_name,
			 xfs_dir2_dataptr_t diroffset, struct xfs_inode *child);
int xfs_parent_defer_replace(struct xfs_trans *tp,
		struct xfs_parent_defer *new_parent, struct xfs_inode *old_dp,
		xfs_dir2_dataptr_t old_diroffset, struct xfs_name *parent_name,
		struct xfs_inode *new_ip, xfs_dir2_dataptr_t new_diroffset,
		struct xfs_inode *child);
int xfs_parent_defer_remove(struct xfs_trans *tp, struct xfs_inode *dp,
			    struct xfs_parent_defer *parent,
			    xfs_dir2_dataptr_t diroffset,
			    struct xfs_inode *child);

void __xfs_parent_cancel(struct xfs_mount *mp, struct xfs_parent_defer *parent);

static inline void
xfs_parent_finish(
	struct xfs_mount	*mp,
	struct xfs_parent_defer	*p)
{
	if (p)
		__xfs_parent_cancel(mp, p);
}

unsigned int xfs_pptr_calc_space_res(struct xfs_mount *mp,
				     unsigned int namelen);

#endif	/* __XFS_PARENT_H__ */
