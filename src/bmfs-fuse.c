#include "libbmfs.h"

#define FUSE_USE_VERSION 30

#include <fuse.h>

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

/** These are options read from
 * the command line. */

struct bmfs_fuse_options
{
	/** The path of the disk file */
	const char *disk;
	/** A flag set when help is requested */
	int show_help;
};

#define BMFS_FUSE_OPTION(t, p) \
    { t, offsetof(struct bmfs_fuse_options, p), 1 }
static const struct fuse_opt option_spec[] = {
	BMFS_FUSE_OPTION("--disk=%s", disk),
	BMFS_FUSE_OPTION("-h", show_help),
	BMFS_FUSE_OPTION("--help", show_help),
	FUSE_OPT_END
};

/** Called when the fuse connection
 * is initialized. Not currently used.
 * */

static void *bmfs_fuse_init(struct fuse_conn_info *conn)
{
	(void) conn;
	return NULL;
}

/** Gets permissions and size of a
 * specific file in the directory.
 * Currently, all files in BMFS have
 * read and write permissions only.
 * */

static int bmfs_fuse_getattr(const char *path, struct stat *stbuf)
{
	memset(stbuf, 0, sizeof(struct stat));

	if (strcmp(path, "/") == 0)
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	}

	struct BMFSEntry entry;
	if (bmfs_findfile(path + 1, &entry, NULL))
	{
		stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_nlink = 1;
		stbuf->st_size = entry.FileSize;
		return 0;
	}

	return -ENOENT;
}

static int bmfs_fuse_utimens(const char *path, const struct timespec tv[2])
{
	(void) path;
	(void) tv;
	return 0;
}

/** Lists contents of a directory.
 * Since BMFS only has one directory,
 * this function basically lists all
 * files in the file system.
 * */

static int bmfs_fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                             off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	/* BMFS only supports the root directory */
	if (strcmp(path, "/") != 0)
		return -ENOENT;

	/* standard directory entries */
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	/* get the current directory entries */
	struct BMFSDir dir;
	if (bmfs_readdir(&dir, disk) != 0)
		return -ENOENT;

	/* list the entries */
	for (int i = 0; i < 64; i++)
	{
		if (dir.Entries[i].FileName[0] == 0)
			/* end of entries */
			break;
		else if (dir.Entries[i].FileName[0] == 1)
			/* empty entry */
			continue;
		/* found an entry */
		filler(buf, dir.Entries[i].FileName, NULL, 0);
	}

	return 0;
}

static int bmfs_fuse_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	(void) mode;
	(void) fi;
	return bmfs_create(path + 1, 1);
}

/** This function opens a file.
 * Except, the way fuse is implemented,
 * it really just checks that it exists.
 * */

static int bmfs_fuse_open(const char *path, struct fuse_file_info *fi)
{
	(void) fi;

	if (bmfs_findfile(path + 1, NULL, NULL))
		/* file was found */
		return 0;

	return -ENOENT;
}

/** Reads data from a file.
 * The data is written to the buf parameter.
 * The file is read starting at the offset
 * parameter.
 * */

static int bmfs_fuse_read(const char *path, char *buf, size_t size, off_t offset,
                          struct fuse_file_info *fi)
{
	(void) fi;
	return bmfs_read(path + 1, buf, size, offset);
}

static int bmfs_fuse_write(const char *path, const char *buf, size_t size, off_t offset,
                           struct fuse_file_info *fi)
{
	(void) fi;
	return bmfs_write(path + 1, buf, size, offset);
}

static struct fuse_operations bmfs_fuse_operations = {
	.init = bmfs_fuse_init,
	.getattr = bmfs_fuse_getattr,
	.utimens = bmfs_fuse_utimens,
	.readdir = bmfs_fuse_readdir,
	.create = bmfs_fuse_create,
	.open = bmfs_fuse_open,
	.read = bmfs_fuse_read,
	.write = bmfs_fuse_write
};

static void show_help(const char *argv0)
{
	fprintf(stderr, "usage: %s <disk> <mountpoint> [options]\n", argv0);
	fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	struct bmfs_fuse_options options = {
		/* .disk may be reallocated, can't
		 * use string literal */
		.disk = strdup("disk.image"),
		.show_help = 0
	};

	/* Parse options */
	if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
		return EXIT_FAILURE;

	if (options.show_help)
	{
		show_help(argv[0]);

		/* this displays fuse-specific help and exits */
		if (fuse_opt_add_arg(&args, "-ho") != 0)
			return EXIT_FAILURE;

		/* allow fuse_main() to run, so that the help
		 * message is displayed */
	}

	disk = fopen(options.disk, "r+b");
	if (disk == NULL)
	{
		fprintf(stderr, "%s: Failed to open '%s': %s\n", argv[0], options.disk, strerror(errno));
		return EXIT_FAILURE;
	}

	return fuse_main(args.argc, args.argv, &bmfs_fuse_operations, NULL);
}
