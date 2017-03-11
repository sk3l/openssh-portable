#include "includes.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "sshbuf.h"
#include "sftp.h"
#include "sftp-handler.h"
#include "xmalloc.h"

extern struct sshbuf * iqueue;

static int fd_fifo;
static const char * path_fifo = "/tmp/sftp_evts";

static void post_open_to_fifo(u_int32_t id);
static void post_close_to_fifo(u_int32_t id);
static void post_read_to_fifo(u_int32_t id);
static void post_write_to_fifo(u_int32_t id);
static void post_stat_to_fifo(u_int32_t id);
static void post_lstat_to_fifo(u_int32_t id);
static void post_fstat_to_fifo(u_int32_t id);
static void post_setstat_to_fifo(u_int32_t id);
static void post_fsetstat_to_fifo(u_int32_t id);
static void post_opendir_to_fifo(u_int32_t id);
static void post_readdir_to_fifo(u_int32_t id);
static void post_remove_to_fifo(u_int32_t id);
static void post_mkdir_to_fifo(u_int32_t id);
static void post_rmdir_to_fifo(u_int32_t id);
static void post_realpath_to_fifo(u_int32_t id);
static void post_rename_to_fifo(u_int32_t id);
static void post_readlink_to_fifo(u_int32_t id);
static void post_symlink_to_fifo(u_int32_t id);

struct sftp_handler overrides[] = {
	   { NULL, NULL, 0, NULL, 0 },
	   { NULL, NULL, 0, NULL, 0 },
	   { NULL, NULL, 0, NULL, 0 },
	   /* NB. SSH2_FXP_OPEN does the readonly check in the handler itself */
	   { "open to fifo", NULL, SSH2_FXP_OPEN, post_open_to_fifo, 0 },
	   { "close to fifo", NULL, SSH2_FXP_CLOSE, post_close_to_fifo, 0 },
	   { "read to fifo", NULL, SSH2_FXP_READ, post_read_to_fifo, 0 },
	   { "write to fifo", NULL, SSH2_FXP_WRITE, post_write_to_fifo, 1 },
	   { "lstat to fifo", NULL, SSH2_FXP_LSTAT, post_lstat_to_fifo, 0 },
	   { "fstat to fifo", NULL, SSH2_FXP_FSTAT, post_fstat_to_fifo, 0 },
	   { "setstat to fifo", NULL, SSH2_FXP_SETSTAT, post_setstat_to_fifo, 1 },
	   { "fsetstat to fifo", NULL, SSH2_FXP_FSETSTAT, post_fsetstat_to_fifo, 1 },
	   { "opendir to fifo", NULL, SSH2_FXP_OPENDIR, post_opendir_to_fifo, 0 },
	   { "readdir to fifo", NULL, SSH2_FXP_READDIR, post_readdir_to_fifo, 0 },
	   { "remove to fifo", NULL, SSH2_FXP_REMOVE, post_remove_to_fifo, 1 },
	   { "mkdir to fifo", NULL, SSH2_FXP_MKDIR, post_mkdir_to_fifo, 1 },
	   { "rmdir to fifo", NULL, SSH2_FXP_RMDIR, post_rmdir_to_fifo, 1 },
	   { "realpath to fifo", NULL, SSH2_FXP_REALPATH, post_realpath_to_fifo, 0 },
	   { "stat to fifo", NULL, SSH2_FXP_STAT, post_stat_to_fifo, 0 },
	   { "rename to fifo", NULL, SSH2_FXP_RENAME, post_rename_to_fifo, 1 },
	   { "readlink to fifo", NULL, SSH2_FXP_READLINK, post_readlink_to_fifo, 0 },
	   { "print symlink to fifo", NULL, SSH2_FXP_SYMLINK, post_symlink_to_fifo, 1 },
};

int init_handler_overrides(handler_list * htbl)
{
	u_int i;
	handler_list   hlist = NULL,
	           new_hlist = NULL;

	logit("Initializing custom handler overrides.");

	fd_fifo = open(path_fifo, O_WRONLY | O_NONBLOCK);
	if (fd_fifo < 0)
	{
	   error("Encountered error opening SFTP event FIFO: %d", errno);
	   return 0;
	}

	/* Insert event handlers overrides*/
	for (i = 0; i <= SSH2_FXP_CNT; i++) {
	   if (overrides[i].handler == NULL)
	      continue;

	   hlist = htbl[i];

	   new_hlist = (handler_list) xcalloc(sizeof(handler_ptr), 3);
	   new_hlist[0] = &overrides[i];
	   new_hlist[1] = hlist[0];

	   htbl[i] = new_hlist;
	   free(hlist);
	}
	return 1;
}

static void post_open_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for open.");
}

static void post_close_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for close.");
}

static void post_read_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for read.");
}

static void post_write_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for write.");
}

static void post_stat_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for stat.");
}

static void post_lstat_to_fifo(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for lstat.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_lstat_to_fifo: %d", rc);
	   return;
	}

	debug("Dispatch lstat name \"%s\" to event FIFO.", path);

	rc = write(fd_fifo, path, len);
	if (rc < 1)
	{
	   error("Encountered error during FIFO write in post_lstat_to_fifo: %d", errno);
	   return;
	}

}

static void post_fstat_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for fstat.");
}

static void post_setstat_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for setstat.");
}

static void post_fsetstat_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for fsetstat.");
}

static void post_opendir_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for opendir.");
}

static void post_readdir_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for readdir.");
}

static void post_remove_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for remove.");
}

static void post_mkdir_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for mkdir.");
}

static void post_rmdir_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for rmdir.");
}

static void post_realpath_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for realpath.");
}

static void post_rename_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for rename.");
}

static void post_readlink_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for readlink.");
}

static void post_symlink_to_fifo(u_int32_t id)
{
	logit("Processing custom handler override for symlink.");
}
