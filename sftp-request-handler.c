
#include "sftp-handler.h"
#include "sftp-request-handler.h"

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"
#include "misc.h"
#include "sshbuf.h"
#include "sftp.h"
#include "xmalloc.h"

extern struct sshbuf * iqueue;

extern int fd_fifo;

static char buff_fifo[34000]; /* max packet size from SFTP protocol */

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

struct sftp_handler req_overrides[] = {
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
	   { "symlink to fifo", NULL, SSH2_FXP_SYMLINK, post_symlink_to_fifo, 1 },
};

static void post_open_to_fifo(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for open.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_open_to_fifo: %d", rc);
	   return;
	}

	debug("Dispatch open name \"%.*s\" to event FIFO.", (int)len, path);

	len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%.*s'\n", id, "open", (int)len, path);
	rc = write(fd_fifo, buff_fifo, len);
	if (rc < 1)
	{
	   error("Encountered error during FIFO write in post_open_to_fifo: %d", errno);
	}
}

static void post_close_to_fifo(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * handle = NULL;
	Handle * hptr;

	logit("Processing custom handler override for close.");

	rc = sshbuf_peek_string_direct(iqueue, &handle, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_close_to_fifo: %d", rc);
	   return;
	}

	hptr = get_sftp_handle(handle, len, -1);
	if (hptr != NULL) {
	   debug("Dispatch close of path \"%s\" to event FIFO.", hptr->name);

	   len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%s'\n", id, "close", hptr->name);
	   rc = write(fd_fifo, buff_fifo, len);
	   if (rc < 1)
	   {
	      error("Encountered error during FIFO write in post_close_to_fifo: %d", errno);
	      return;
	   }
	} else {
	   error("Encountered error in post_close_to_fifo: bad handle");
	}
}

static void post_read_to_fifo(u_int32_t id)
{
	int rc = 0;
	u_int64_t off = 0;
	u_int32_t max = 0;
	size_t len = 0;
	const u_char * handle = NULL;
	Handle * hptr;

	logit("Processing custom handler override for read.");

	rc = sshbuf_peek_string_direct(iqueue, &handle, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_read_to_fifo: %d", rc);
	   return;
	}

	hptr = get_sftp_handle(handle, len, -1);
	if (hptr != NULL) {
	   debug("Dispatch read from path \"%s\" to event FIFO.", hptr->name);

	   /* establish read offset  */
	   off = get_u64(handle + len);

	   /* establish read max size */
	   max = get_u32(handle + len + sizeof(u_int64_t));

	   len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%s' off=%lld max=%d\n", id, "read", hptr->name, (long long)off, (int)max);
	   rc = write(fd_fifo, buff_fifo, len);
	   if (rc < 1)
	   {
	      error("Encountered error during FIFO write in post_read_to_fifo: %d", errno);
	      return;
	   }
	} else {
	   error("Encountered error in post_read_to_fifo: bad handle");
	}

}

static void post_write_to_fifo(u_int32_t id)
{
	int rc = 0;
	u_int64_t off = 0;
	u_int32_t cnt = 0;
	size_t len = 0;
	const u_char * handle = NULL;
	Handle * hptr;

	logit("Processing custom handler override for write.");

	rc = sshbuf_peek_string_direct(iqueue, &handle, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_write_to_fifo: %d", rc);
	   return;
	}

	hptr = get_sftp_handle(handle, len, -1);
	if (hptr != NULL) {
	   debug("Dispatch write to path \"%s\" to event FIFO.", hptr->name);

	   /* establish read offset  */
	   off = get_u64(handle + len);

	   /* establish read max size */
	   cnt = get_u32(handle + len + sizeof(u_int64_t));

	   len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%s' off=%lld cnt=%d\n", id, "write", hptr->name, (long long)off, (int)cnt);
	   rc = write(fd_fifo, buff_fifo, len);
	   if (rc < 1)
	   {
	      error("Encountered error during FIFO write in post_write_to_fifo: %d", errno);
	      return;
	   }
	} else {
	   error("Encountered error in post_write_to_fifo: bad handle");
	}

}

static void post_stat_to_fifo(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for stat.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_stat_to_fifo: %d", rc);
	   return;
	}

	/* TO DO - parse and communicate file flags */

	debug("Dispatch stat name \"%.*s\" to event FIFO.", (int)len, path);

	len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%.*s'\n", id, "stat", (int)len, path);
	rc = write(fd_fifo, buff_fifo, len);
	if (rc < 1)
	{
	   error("Encountered error during FIFO write in post_stat_to_fifo: %d", errno);
	   return;
	}

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

	debug("Dispatch lstat name \"%.*s\" to event FIFO.", (int)len, path);

	len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%.*s'\n", id, "lstat", (int)len, path);
	rc = write(fd_fifo, buff_fifo, len);
	if (rc < 1)
	{
	   error("Encountered error during FIFO write in post_lstat_to_fifo: %d", errno);
	   return;
	}
}

static void post_fstat_to_fifo(u_int32_t id)
{
	int rc = 0;
	u_int32_t idx_handle = 0;
	size_t len = 0;
	const u_char * handle = NULL;
	Handle * hptr;

	logit("Processing custom handler override for fstat.");

	rc = sshbuf_peek_string_direct(iqueue, &handle, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_fstat_to_fifo: %d", rc);
	   return;
	}

	hptr = get_sftp_handle(handle, len, HANDLE_DIR);
	if (hptr != NULL) {
		debug("Dispatch fstat name \"%s\" to event FIFO.", hptr->name);

	   /* TO DO - parse and communicate file flags */

	   len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%s'\n", id, "fstat", hptr->name);
	   rc = write(fd_fifo, buff_fifo, len);
	   if (rc < 1)
	   {
	      error("Encountered error during FIFO write in post_fstat_to_fifo: %d", errno);
	      return;
	   }
	} else {
	   error("Encountered error in post_fstat_to_fifo: bad handle idx %d", (int)idx_handle);
	}
}

static void post_setstat_to_fifo(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for setstat.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_setstat_to_fifo: %d", rc);
	   return;
	}

	debug("Dispatch setstat name \"%.*s\" to event FIFO.", (int)len, path);

	/* TO DO - parse and communicate file attrs */

	len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%.*s'\n", id, "setstat", (int)len, path);
	rc = write(fd_fifo, buff_fifo, len);
	if (rc < 1)
	{
	   error("Encountered error during FIFO write in post_setstat_to_fifo: %d", errno);
	   return;
	}

}

static void post_fsetstat_to_fifo(u_int32_t id)
{
	int rc = 0;
	u_int32_t idx_handle = 0;
	size_t len = 0;
	const u_char * handle = NULL;
	Handle * hptr;

	logit("Processing custom handler override for fsetstat.");

	rc = sshbuf_peek_string_direct(iqueue, &handle, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_fsetstat_to_fifo: %d", rc);
	   return;
	}

	hptr = get_sftp_handle(handle, len, HANDLE_DIR);
	if (hptr != NULL) {
		debug("Dispatch fsetstat name \"%s\" to event FIFO.", hptr->name);

	   len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%s'\n", id, "fsetstat", hptr->name);
	   rc = write(fd_fifo, buff_fifo, len);
	   if (rc < 1)
	   {
	      error("Encountered error during FIFO write in post_fsetstat_to_fifo: %d", errno);
	      return;
	   }
	} else {
	   error("Encountered error in post_fsetstat_to_fifo: bad handle idx %d", (int)idx_handle);
	}
}

static void post_opendir_to_fifo(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for opendir.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_opendir_to_fifo: %d", rc);
	   return;
	}

	debug("Dispatch opendir name \"%.*s\" to event FIFO.", (int)len, path);

	len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%.*s'\n", id, "opendir", (int)len, path);
	rc = write(fd_fifo, buff_fifo, len);
	if (rc < 1)
	{
	   error("Encountered error during FIFO write in post_opendir_to_fifo: %d", errno);
	   return;
	}
}

static void post_readdir_to_fifo(u_int32_t id)
{
	int rc = 0;
	u_int32_t idx_handle = 0;
	size_t len = 0;
	const u_char * handle = NULL;
	Handle * hptr;

	logit("Processing custom handler override for readdir.");

	rc = sshbuf_peek_string_direct(iqueue, &handle, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_readdir_to_fifo: %d", rc);
	   return;
	}

	hptr = get_sftp_handle(handle, len, HANDLE_DIR);
	if (hptr != NULL) {
		debug("Dispatch readdir name \"%s\" to event FIFO.", hptr->name);

	   len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%s'\n", id, "readdir", hptr->name);
	   rc = write(fd_fifo, buff_fifo, len);
	   if (rc < 1)
	   {
	      error("Encountered error during FIFO write in post_readdir_to_fifo: %d", errno);
	      return;
	   }
	} else {
	   error("Encountered error in post_readdir_to_fifo: bad handle idx %d", (int)idx_handle);
	}
}

static void post_remove_to_fifo(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for remove.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_remove_to_fifo: %d", rc);
	   return;
	}

	debug("Dispatch remove name \"%.*s\" to event FIFO.", (int)len, path);

	len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%.*s'\n", id, "remove", (int)len, path);
	rc = write(fd_fifo, buff_fifo, len);
	if (rc < 1)
	{
	   error("Encountered error during FIFO write in post_remove_to_fifo: %d", errno);
	   return;
	}
}

static void post_mkdir_to_fifo(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for mkdir.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_mkdir_to_fifo: %d", rc);
	   return;
	}

	debug("Dispatch mkdir name \"%.*s\" to event FIFO.", (int)len, path);

	/* TO DO - parse and communicate file attrs */

	len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%.*s'\n", id, "mkdir", (int)len, path);
	rc = write(fd_fifo, buff_fifo, len);
	if (rc < 1)
	{
	   error("Encountered error during FIFO write in post_opendir_to_fifo: %d", errno);
	   return;
	}
}

static void post_rmdir_to_fifo(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for rmdir.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_rmdir_to_fifo: %d", rc);
	   return;
	}

	debug("Dispatch rmdir name \"%.*s\" to event FIFO.", (int)len, path);

	len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%.*s'\n", id, "rmdir", (int)len, path);
	rc = write(fd_fifo, buff_fifo, len);
	if (rc < 1)
	{
	   error("Encountered error during FIFO write in post_rmdir_to_fifo: %d", errno);
	   return;
	}
}

static void post_realpath_to_fifo(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for realpath.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_realpath_to_fifo: %d", rc);
	   return;
	}

	/* TO DO - parse and communicate control byte & component paths */

	debug("Dispatch realpath name \"%.*s\" to event FIFO.", (int)len, path);

	len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%.*s'\n", id, "realpath", (int)len, path);
	rc = write(fd_fifo, buff_fifo, len);
	if (rc < 1)
	{
	   error("Encountered error during FIFO write in post_realpath_to_fifo: %d", errno);
	   return;
	}
}

static void post_rename_to_fifo(u_int32_t id)
{
	int rc = 0;
	u_int len  = 0,
	      nlen = 0 ;
	const u_char * buf  = NULL,
	             * path = NULL,
	             * npath= NULL;

	logit("Processing custom handler override for rename.");

	/* Since we must extract multiple str (from, to path) we cannot simply peek.
	 * We must manually parse both strings, as sshbuf peek only enables viewing
	 * the next available str in the buffer, and not subsequent ones. */
	buf = sshbuf_ptr(iqueue);

	rc = get_ssh_string(buf, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_rename_to_fifo: %d", rc);
	   return;
	}

	rc = get_ssh_string(buf + sizeof(u_int) + len, &npath, &nlen);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_rename_to_fifo: %d", rc);
	   return;
	}

	debug("Dispatch rename from \"%.*s\" to \"%.*s\" to event FIFO.", (int)len, path, (int)nlen, npath);

	len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%.*s' npath='%.*s'\n", id, "rename", (int)len, path, (int)nlen, npath);
	rc = write(fd_fifo, buff_fifo, len);
	if (rc < 1)
	{
	   error("Encountered error during FIFO write in post_opendir_to_fifo: %d", errno);
	   return;
	}
}

static void post_readlink_to_fifo(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for readlink.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_readlink_to_fifo: %d", rc);
	   return;
	}

	debug("Dispatch readlink name \"%.*s\" to event FIFO.", (int)len, path);

	len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%.*s'\n", id, "readlink", (int)len, path);
	rc = write(fd_fifo, buff_fifo, len);
	if (rc < 1)
	{
	   error("Encountered error during FIFO write in post_readlink_to_fifo: %d", errno);
	   return;
	}
}

static void post_symlink_to_fifo(u_int32_t id)
{
	int rc = 0;
	u_int len  = 0,
	      nlen = 0 ;
	const u_char * buf  = NULL,
	             * path = NULL,
	             * npath= NULL;

	logit("Processing custom handler override for symlink.");

	/* Since we must extract multiple str (from, to path) we cannot simply peek.
	 * We must manually parse both strings, as sshbuf peek only enables viewing
	 * the next available str in the buffer, and not subsequent ones. */
	buf = sshbuf_ptr(iqueue);

	rc = get_ssh_string(buf, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_symlink_to_fifo: %d", rc);
	   return;
	}

	rc = get_ssh_string(buf + sizeof(u_int) + len, &npath, &nlen);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_symlink_to_fifo: %d", rc);
	   return;
	}

	debug("Dispatch symlink to \"%.*s\" from \"%.*s\" to event FIFO.", (int)nlen, npath, (int)len, path);

	len = sprintf(buff_fifo, "id=%-10d rqst=%-10s path='%.*s' newpath='%.*s'\n", id, "symlink", (int)len, path, (int)nlen, npath);
	rc = write(fd_fifo, buff_fifo, len);
	if (rc < 1)
	{
	   error("Encountered error during FIFO write in post_opendir_to_fifo: %d", errno);
	   return;
	}
}

