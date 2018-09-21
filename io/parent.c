// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2005-2006 Silicon Graphics, Inc.
 * All Rights Reserved.
 */

#include "command.h"
#include "input.h"
#include "libfrog/paths.h"
#include "parent.h"
#include "handle.h"
#include "init.h"
#include "io.h"

static cmdinfo_t parent_cmd;
static char *mntpt;

static int
pptr_print(
	struct xfs_pptr_info	*pi,
	struct xfs_parent_ptr	*pptr,
	void			*arg,
	int			flags)
{
	char			buf[XFS_PPTR_MAXNAMELEN + 1];

	if (pi->pi_flags & XFS_PPTR_OFLAG_ROOT) {
		printf(_("Root directory.\n"));
		return 0;
	}

	memcpy(buf, pptr->xpp_name, pptr->xpp_namelen);
	buf[pptr->xpp_namelen] = 0;

	if (flags & XFS_PPPTR_OFLAG_SHORT) {
		printf("%llu/%u/%u/%s\n",
			(unsigned long long)pptr->xpp_ino,
			(unsigned int)pptr->xpp_gen,
			(unsigned int)pptr->xpp_namelen,
			buf);
	}
	else {
		printf(_("p_ino    = %llu\n"), (unsigned long long)pptr->xpp_ino);
		printf(_("p_gen    = %u\n"), (unsigned int)pptr->xpp_gen);
		printf(_("p_reclen = %u\n"), (unsigned int)pptr->xpp_namelen);
		printf(_("p_name   = \"%s\"\n\n"), buf);
	}
	return 0;
}

int
print_parents(
	struct xfs_handle	*handle,
	uint64_t		pino,
	char			*pname,
	int			flags)
{
	int			ret;

	if (handle)
		ret = handle_walk_pptrs(handle, sizeof(*handle), pino,
				pname, pptr_print, NULL, flags);
	else
		ret = fd_walk_pptrs(file->fd, pino, pname, pptr_print,
				NULL, flags);
	if (ret)
		perror(file->name);

	return 0;
}

static int
path_print(
	const char		*mntpt,
	struct path_list	*path,
	void			*arg) {

	char			buf[PATH_MAX];
	size_t			len = PATH_MAX;
	int			ret;

	ret = snprintf(buf, len, "%s", mntpt);
	if (ret != strlen(mntpt)) {
		errno = ENOMEM;
		return -1;
	}

	ret = path_list_to_string(path, buf + ret, len - ret);
	if (ret < 0)
		return ret;
	return 0;
}

int
print_paths(
	struct xfs_handle	*handle,
	uint64_t		pino,
	char			*pname,
	int			flags)
{
	int			ret;

	if (handle)
		ret = handle_walk_ppaths(handle, sizeof(*handle), pino,
				pname, path_print, NULL, flags);
 	else
		ret = fd_walk_ppaths(file->fd, pino, pname, path_print,
				NULL, flags);
	if (ret)
		perror(file->name);
	return 0;
}

static int
parent_f(
	int			argc,
	char			**argv)
{
	struct xfs_handle	handle;
	void			*hanp = NULL;
	size_t			hlen;
	struct fs_path		*fs;
	char			*p;
	uint64_t		ino = 0;
	uint32_t		gen = 0;
	int			c;
	int			listpath_flag = 0;
	int			ret;
	static int		tab_init;
	uint64_t		pino = 0;
	char			*pname = NULL;
	int			ppptr_flags = 0;

	if (!tab_init) {
		tab_init = 1;
		fs_table_initialise(0, NULL, 0, NULL);
	}
	fs = fs_table_lookup(file->name, FS_MOUNT_POINT);
	if (!fs) {
		fprintf(stderr, _("file argument, \"%s\", is not in a mounted XFS filesystem\n"),
			file->name);
		exitcode = 1;
		return 1;
	}
	mntpt = fs->fs_dir;

	while ((c = getopt(argc, argv, "pfi:n:")) != EOF) {
		switch (c) {
		case 'p':
			listpath_flag = 1;
			break;
		case 'i':
	                pino = strtoull(optarg, &p, 0);
	                if (*p != '\0' || pino == 0) {
	                        fprintf(stderr,
	                                _("Bad inode number '%s'.\n"),
	                                optarg);
	                        return 0;
			}

			break;
		case 'n':
			pname = optarg;
			break;
		case 'f':
			ppptr_flags |= XFS_PPPTR_OFLAG_SHORT;
			break;
		default:
			return command_usage(&parent_cmd);
		}
	}

	/*
	 * Always initialize the fshandle table because we need it for
	 * the ppaths functions to work.
	 */
	ret = path_to_fshandle((char *)mntpt, &hanp, &hlen);
	if (ret) {
		perror(mntpt);
		return 0;
 	}
 
	if (optind + 2 == argc) {
		ino = strtoull(argv[optind], &p, 0);
		if (*p != '\0' || ino == 0) {
			fprintf(stderr,
				_("Bad inode number '%s'.\n"),
				argv[optind]);
			return 0;
		}
		gen = strtoul(argv[optind + 1], &p, 0);
		if (*p != '\0') {
			fprintf(stderr,
				_("Bad generation number '%s'.\n"),
				argv[optind + 1]);
			return 0;
		}

		memcpy(&handle, hanp, sizeof(handle));
		handle.ha_fid.fid_len = sizeof(xfs_fid_t) -
				sizeof(handle.ha_fid.fid_len);
		handle.ha_fid.fid_pad = 0;
		handle.ha_fid.fid_ino = ino;
		handle.ha_fid.fid_gen = gen;

	}

	if (listpath_flag)
		exitcode = print_paths(ino ? &handle : NULL,
				pino, pname, ppptr_flags);
	else
		exitcode = print_parents(ino ? &handle : NULL,
				pino, pname, ppptr_flags);

	if (hanp)
		free_handle(hanp, hlen);

	return 0;
}

static void
parent_help(void)
{
printf(_(
"\n"
" list the current file's parents and their filenames\n"
"\n"
" -p -- list the current file's paths up to the root\n"
"\n"
"If ino and gen are supplied, use them instead.\n"
"\n"
" -i -- Only show parent pointer records containing the given inode\n"
"\n"
" -n -- Only show parent pointer records containing the given filename\n"
"\n"
" -f -- Print records in short format: ino/gen/namelen/filename\n"
"\n"));
}

void
parent_init(void)
{
	parent_cmd.name = "parent";
	parent_cmd.cfunc = parent_f;
	parent_cmd.argmin = 0;
	parent_cmd.argmax = -1;
	parent_cmd.args = _("[-p] [ino gen] [-i] [ino] [-n] [name] [-f]");
	parent_cmd.flags = CMD_NOMAP_OK;
	parent_cmd.oneline = _("print parent inodes");
	parent_cmd.help = parent_help;

	if (expert)
		add_command(&parent_cmd);
}
