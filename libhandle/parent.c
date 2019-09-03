/*
 * Copyright (C) 2017 Oracle.  All Rights Reserved.
 *
 * Author: Darrick J. Wong <darrick.wong@oracle.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software Foundation,
 * Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "platform_defs.h"
#include "xfs.h"
#include "xfs_arch.h"
#include "list.h"
#include "paths.h"
#include "handle.h"
#include "parent.h"

/* Allocate a buffer large enough for some parent pointer records. */
static inline struct xfs_pptr_info *
xfs_pptr_alloc(
      size_t                  nr_ptrs)
{
      struct xfs_pptr_info    *pi;

      pi = malloc(XFS_PPTR_INFO_SIZEOF(nr_ptrs));
      if (!pi)
              return NULL;
      memset(pi, 0, sizeof(struct xfs_pptr_info));
      pi->pi_ptrs_size = nr_ptrs;
      return pi;
}

/* Walk all parents of the given file handle. */
static int
handle_walk_parents(
	int			fd,
	struct xfs_handle	*handle,
	walk_pptr_fn		fn,
	void			*arg)
{
	struct xfs_pptr_info	*pi;
	struct xfs_parent_ptr	*p;
	unsigned int		i;
	ssize_t			ret = -1;

	pi = xfs_pptr_alloc(4);
	if (!pi)
		return -1;

	if (handle) {
		memcpy(&pi->pi_handle, handle, sizeof(struct xfs_handle));
		pi->pi_flags = XFS_PPTR_IFLAG_HANDLE;
	}

	ret = ioctl(fd, XFS_IOC_GETPPOINTER, pi);
	while (!ret) {
		if (pi->pi_flags & XFS_PPTR_OFLAG_ROOT) {
			ret = fn(pi, NULL, arg);
			break;
		}

		for (i = 0; i < pi->pi_ptrs_used; i++) {
			p = XFS_PPINFO_TO_PP(pi, i);
			ret = fn(pi, p, arg);
			if (ret)
				goto out_pi;
		}

		if (pi->pi_flags & XFS_PPTR_OFLAG_DONE)
			break;

		ret = ioctl(fd, XFS_IOC_GETPPOINTER, pi);
	}

out_pi:
	free(pi);
	return ret;
}

/* Walk all parent pointers of this handle. */
int
handle_walk_pptrs(
	void			*hanp,
	size_t			hlen,
	walk_pptr_fn		fn,
	void			*arg)
{
	char			*mntpt;
	int			fd;

	if (hlen != sizeof(struct xfs_handle)) {
		errno = EINVAL;
		return -1;
	}

	fd = handle_to_fsfd(hanp, &mntpt);
	if (fd < 0)
		return -1;

	return handle_walk_parents(fd, hanp, fn, arg);
}

/* Walk all parent pointers of this fd. */
int
fd_walk_pptrs(
	int			fd,
	walk_pptr_fn		fn,
	void			*arg)
{
	return handle_walk_parents(fd, NULL, fn, arg);
}

struct walk_ppaths_info {
	walk_ppath_fn			fn;
	void				*arg;
	char				*mntpt;
	struct path_list		*path;
	int				fd;
};

struct walk_ppath_level_info {
	struct xfs_handle		newhandle;
	struct path_component		*pc;
	struct walk_ppaths_info		*wpi;
};

static int handle_walk_parent_paths(struct walk_ppaths_info *wpi,
		struct xfs_handle *handle);

static int
handle_walk_parent_path_ptr(
	struct xfs_pptr_info		*pi,
	struct xfs_parent_ptr		*p,
	void				*arg)
{
	struct walk_ppath_level_info	*wpli = arg;
	struct walk_ppaths_info		*wpi = wpli->wpi;
	unsigned int			i;
	int				ret = 0;

	if (pi->pi_flags & XFS_PPTR_OFLAG_ROOT)
		return wpi->fn(wpi->mntpt, wpi->path, wpi->arg);

	for (i = 0; i < pi->pi_ptrs_used; i++) {
		p = XFS_PPINFO_TO_PP(pi, i);
		ret = path_component_change(wpli->pc, p->xpp_name,
				p->xpp_namelen);
		if (ret)
			break;
		wpli->newhandle.ha_fid.fid_ino = p->xpp_ino;
		wpli->newhandle.ha_fid.fid_gen = p->xpp_gen;
		path_list_add_parent_component(wpi->path, wpli->pc);
		ret = handle_walk_parent_paths(wpi, &wpli->newhandle);
		path_list_del_component(wpi->path, wpli->pc);
		if (ret)
			break;
	}

	return ret;
}

/*
 * Recursively walk all parents of the given file handle; if we hit the
 * fs root then we call the associated function with the constructed path.
 */
static int
handle_walk_parent_paths(
	struct walk_ppaths_info		*wpi,
	struct xfs_handle		*handle)
{
	struct walk_ppath_level_info	*wpli;
	int				ret;

	wpli = malloc(sizeof(struct walk_ppath_level_info));
	if (!wpli)
		return -1;
	wpli->pc = path_component_init("");
	if (!wpli->pc) {
		free(wpli);
		return -1;
	}
	wpli->wpi = wpi;
	memcpy(&wpli->newhandle, handle, sizeof(struct xfs_handle));

	ret = handle_walk_parents(wpi->fd, handle, handle_walk_parent_path_ptr,
			wpli);

	path_component_free(wpli->pc);
	free(wpli);
	return ret;
}

/*
 * Call the given function on all known paths from the vfs root to the inode
 * described in the handle.
 */
int
handle_walk_ppaths(
	void			*hanp,
	size_t			hlen,
	walk_ppath_fn		fn,
	void			*arg)
{
	struct walk_ppaths_info	wpi;
	ssize_t			ret;

	if (hlen != sizeof(struct xfs_handle)) {
		errno = EINVAL;
		return -1;
	}

	wpi.fd = handle_to_fsfd(hanp, &wpi.mntpt);
	if (wpi.fd < 0)
		return -1;
	wpi.path = path_list_init();
	if (!wpi.path)
		return -1;
	wpi.fn = fn;
	wpi.arg = arg;

	ret = handle_walk_parent_paths(&wpi, hanp);
	path_list_free(wpi.path);

	return ret;
}

/*
 * Call the given function on all known paths from the vfs root to the inode
 * referred to by the file description.
 */
int
fd_walk_ppaths(
	int			fd,
	walk_ppath_fn		fn,
	void			*arg)
{
	struct walk_ppaths_info	wpi;
	void			*hanp;
	size_t			hlen;
	int			fsfd;
	int			ret;

	ret = fd_to_handle(fd, &hanp, &hlen);
	if (ret)
		return ret;

	fsfd = handle_to_fsfd(hanp, &wpi.mntpt);
	if (fsfd < 0)
		return -1;
	wpi.fd = fd;
	wpi.path = path_list_init();
	if (!wpi.path)
		return -1;
	wpi.fn = fn;
	wpi.arg = arg;

	ret = handle_walk_parent_paths(&wpi, hanp);
	path_list_free(wpi.path);

	return ret;
}

struct path_walk_info {
	char			*buf;
	size_t			len;
};

/* Helper that stringifies the first full path that we find. */
static int
handle_to_path_walk(
	const char		*mntpt,
	struct path_list	*path,
	void			*arg)
{
	struct path_walk_info	*pwi = arg;
	int			ret;

	ret = snprintf(pwi->buf, pwi->len, "%s", mntpt);
	if (ret != strlen(mntpt)) {
		errno = ENOMEM;
		return -1;
	}

	ret = path_list_to_string(path, pwi->buf + ret, pwi->len - ret);
	if (ret < 0)
		return ret;

	return WALK_PPATHS_ABORT;
}

/* Return any eligible path to this file handle. */
int
handle_to_path(
	void			*hanp,
	size_t			hlen,
	char			*path,
	size_t			pathlen)
{
	struct path_walk_info	pwi;

	pwi.buf = path;
	pwi.len = pathlen;
	return handle_walk_ppaths(hanp, hlen, handle_to_path_walk, &pwi);
}

/* Return any eligible path to this file description. */
int
fd_to_path(
	int			fd,
	char			*path,
	size_t			pathlen)
{
	struct path_walk_info	pwi;

	pwi.buf = path;
	pwi.len = pathlen;
	return fd_walk_ppaths(fd, handle_to_path_walk, &pwi);
}
