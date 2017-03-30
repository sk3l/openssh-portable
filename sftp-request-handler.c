
#include "includes.h"

#include "sftp-common.h"
#include "sftp-handler.h"
#include "sftp-handler-sink.h" 
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

static char buff_sink[SFTP_MAX_MSG_LENGTH]; /* max SFTP packet size */

static void post_open_to_sink(u_int32_t id);
static void post_close_to_sink(u_int32_t id);
static void post_read_to_sink(u_int32_t id);
static void post_write_to_sink(u_int32_t id);
static void post_stat_to_sink(u_int32_t id);
static void post_lstat_to_sink(u_int32_t id);
static void post_fstat_to_sink(u_int32_t id);
static void post_setstat_to_sink(u_int32_t id);
static void post_fsetstat_to_sink(u_int32_t id);
static void post_opendir_to_sink(u_int32_t id);
static void post_readdir_to_sink(u_int32_t id);
static void post_remove_to_sink(u_int32_t id);
static void post_mkdir_to_sink(u_int32_t id);
static void post_rmdir_to_sink(u_int32_t id);
static void post_realpath_to_sink(u_int32_t id);
static void post_rename_to_sink(u_int32_t id);
static void post_readlink_to_sink(u_int32_t id);
static void post_symlink_to_sink(u_int32_t id);

struct sftp_handler req_overrides[] = {
	   { NULL, NULL, 0, NULL, 0 },
	   { NULL, NULL, 0, NULL, 0 },
	   { NULL, NULL, 0, NULL, 0 },
	   /* NB. SSH2_FXP_OPEN does the readonly check in the handler itself */
	   { "open to sink", NULL, SSH2_FXP_OPEN, post_open_to_sink, 0 },
	   { "close to sink", NULL, SSH2_FXP_CLOSE, post_close_to_sink, 0 },
	   { "read to sink", NULL, SSH2_FXP_READ, post_read_to_sink, 0 },
	   { "write to sink", NULL, SSH2_FXP_WRITE, post_write_to_sink, 1 },
	   { "lstat to sink", NULL, SSH2_FXP_LSTAT, post_lstat_to_sink, 0 },
	   { "fstat to sink", NULL, SSH2_FXP_FSTAT, post_fstat_to_sink, 0 },
	   { "setstat to sink", NULL, SSH2_FXP_SETSTAT, post_setstat_to_sink, 1 },
	   { "fsetstat to sink", NULL, SSH2_FXP_FSETSTAT, post_fsetstat_to_sink, 1 },
	   { "opendir to sink", NULL, SSH2_FXP_OPENDIR, post_opendir_to_sink, 0 },
	   { "readdir to sink", NULL, SSH2_FXP_READDIR, post_readdir_to_sink, 0 },
	   { "remove to sink", NULL, SSH2_FXP_REMOVE, post_remove_to_sink, 1 },
	   { "mkdir to sink", NULL, SSH2_FXP_MKDIR, post_mkdir_to_sink, 1 },
	   { "rmdir to sink", NULL, SSH2_FXP_RMDIR, post_rmdir_to_sink, 1 },
	   { "realpath to sink", NULL, SSH2_FXP_REALPATH, post_realpath_to_sink, 0 },
	   { "stat to sink", NULL, SSH2_FXP_STAT, post_stat_to_sink, 0 },
	   { "rename to sink", NULL, SSH2_FXP_RENAME, post_rename_to_sink, 1 },
	   { "readlink to sink", NULL, SSH2_FXP_READLINK, post_readlink_to_sink, 0 },
	   { "symlink to sink", NULL, SSH2_FXP_SYMLINK, post_symlink_to_sink, 1 },
};

static void post_open_to_sink(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for open.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_open_to_sink: %d", rc);
	   return;
	}

	debug("Dispatch open name \"%.*s\" to SFTP handler sink.", (int)len, path);

	len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%.*s'\n", id, "open", (int)len, path);
	rc = write_to_handler_sink(buff_sink, len);
	if (rc < 1)
	{
	   error("Encountered error during write to SFTP handler sink in post_open_to_sink: %d", errno);
	}
}

static void post_close_to_sink(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * handle = NULL;
	Handle * hptr;

	logit("Processing custom handler override for close.");

	rc = sshbuf_peek_string_direct(iqueue, &handle, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_close_to_sink: %d", rc);
	   return;
	}

	hptr = get_sftp_handle(handle, len, -1);
	if (hptr != NULL) {
	   debug("Dispatch close of path \"%s\" to SFTP handler sink.", hptr->name);

	   len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%s'\n", id, "close", hptr->name);
	   rc = write_to_handler_sink(buff_sink, len);
	   if (rc < 1)
	   {
	      error("Encountered error during write to SFTP handler sink in post_close_to_sink: %d", errno);
	      return;
	   }
	} else {
	   error("Encountered error in post_close_to_sink: bad handle");
	}
}

static void post_read_to_sink(u_int32_t id)
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
	   error("Encountered error during SSH buff peek in post_read_to_sink: %d", rc);
	   return;
	}

	hptr = get_sftp_handle(handle, len, -1);
	if (hptr != NULL) {
	   debug("Dispatch read from path \"%s\" to SFTP handler sink.", hptr->name);

	   /* establish read offset  */
	   off = get_u64(handle + len);

	   /* establish read max size */
	   max = get_u32(handle + len + sizeof(u_int64_t));

	   len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%s' off=%lld max=%d\n", id, "read", hptr->name, (long long)off, (int)max);
	   rc = write_to_handler_sink(buff_sink, len);
	   if (rc < 1)
	   {
	      error("Encountered error during write to SFTP handler sink in post_read_to_sink: %d", errno);
	      return;
	   }
	} else {
	   error("Encountered error in post_read_to_sink: bad handle");
	}

}

static void post_write_to_sink(u_int32_t id)
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
	   error("Encountered error during SSH buff peek in post_write_to_sink: %d", rc);
	   return;
	}

	hptr = get_sftp_handle(handle, len, -1);
	if (hptr != NULL) {
	   debug("Dispatch write to path \"%s\" to SFTP handler sink.", hptr->name);

	   /* establish read offset  */
	   off = get_u64(handle + len);

	   /* establish read max size */
	   cnt = get_u32(handle + len + sizeof(u_int64_t));

	   len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%s' off=%lld cnt=%d\n", id, "write", hptr->name, (long long)off, (int)cnt);
	   rc = write_to_handler_sink(buff_sink, len);
	   if (rc < 1)
	   {
	      error("Encountered error during write to SFTP handler sink in post_write_to_sink: %d", errno);
	      return;
	   }
	} else {
	   error("Encountered error in post_write_to_sink: bad handle");
	}

}

static void post_stat_to_sink(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for stat.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_stat_to_sink: %d", rc);
	   return;
	}

	/* TO DO - parse and communicate file flags */

	debug("Dispatch stat name \"%.*s\" to SFTP handler sink.", (int)len, path);

	len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%.*s'\n", id, "stat", (int)len, path);
	rc = write_to_handler_sink(buff_sink, len);
	if (rc < 1)
	{
	   error("Encountered error during write to SFTP handler sink in post_stat_to_sink: %d", errno);
	   return;
	}

}

static void post_lstat_to_sink(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for lstat.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_lstat_to_sink: %d", rc);
	   return;
	}

	debug("Dispatch lstat name \"%.*s\" to SFTP handler sink.", (int)len, path);

	len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%.*s'\n", id, "lstat", (int)len, path);
	rc = write_to_handler_sink(buff_sink, len);
	if (rc < 1)
	{
	   error("Encountered error during write to SFTP handler sink in post_lstat_to_sink: %d", errno);
	   return;
	}
}

static void post_fstat_to_sink(u_int32_t id)
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
	   error("Encountered error during SSH buff peek in post_fstat_to_sink: %d", rc);
	   return;
	}

	hptr = get_sftp_handle(handle, len, HANDLE_DIR);
	if (hptr != NULL) {
		debug("Dispatch fstat name \"%s\" to SFTP handler sink.", hptr->name);

	   /* TO DO - parse and communicate file flags */

	   len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%s'\n", id, "fstat", hptr->name);
	   rc = write_to_handler_sink(buff_sink, len);
	   if (rc < 1)
	   {
	      error("Encountered error during write to SFTP handler sink in post_fstat_to_sink: %d", errno);
	      return;
	   }
	} else {
	   error("Encountered error in post_fstat_to_sink: bad handle idx %d", (int)idx_handle);
	}
}

static void post_setstat_to_sink(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for setstat.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_setstat_to_sink: %d", rc);
	   return;
	}

	debug("Dispatch setstat name \"%.*s\" to SFTP handler sink.", (int)len, path);

	/* TO DO - parse and communicate file attrs */

	len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%.*s'\n", id, "setstat", (int)len, path);
	rc = write_to_handler_sink(buff_sink, len);
	if (rc < 1)
	{
	   error("Encountered error during write to SFTP handler sink in post_setstat_to_sink: %d", errno);
	   return;
	}

}

static void post_fsetstat_to_sink(u_int32_t id)
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
	   error("Encountered error during SSH buff peek in post_fsetstat_to_sink: %d", rc);
	   return;
	}

	hptr = get_sftp_handle(handle, len, HANDLE_DIR);
	if (hptr != NULL) {
		debug("Dispatch fsetstat name \"%s\" to SFTP handler sink.", hptr->name);

	   len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%s'\n", id, "fsetstat", hptr->name);
	   rc = write_to_handler_sink(buff_sink, len);
	   if (rc < 1)
	   {
	      error("Encountered error during write to SFTP handler sink in post_fsetstat_to_sink: %d", errno);
	      return;
	   }
	} else {
	   error("Encountered error in post_fsetstat_to_sink: bad handle idx %d", (int)idx_handle);
	}
}

static void post_opendir_to_sink(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for opendir.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_opendir_to_sink: %d", rc);
	   return;
	}

	debug("Dispatch opendir name \"%.*s\" to SFTP handler sink.", (int)len, path);

	len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%.*s'\n", id, "opendir", (int)len, path);
	rc = write_to_handler_sink(buff_sink, len);
	if (rc < 1)
	{
	   error("Encountered error during write to SFTP handler sink in post_opendir_to_sink: %d", errno);
	   return;
	}
}

static void post_readdir_to_sink(u_int32_t id)
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
	   error("Encountered error during SSH buff peek in post_readdir_to_sink: %d", rc);
	   return;
	}

	hptr = get_sftp_handle(handle, len, HANDLE_DIR);
	if (hptr != NULL) {
		debug("Dispatch readdir name \"%s\" to SFTP handler sink.", hptr->name);

	   len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%s'\n", id, "readdir", hptr->name);
	   rc = write_to_handler_sink(buff_sink, len);
	   if (rc < 1)
	   {
	      error("Encountered error during write to SFTP handler sink in post_readdir_to_sink: %d", errno);
	      return;
	   }
	} else {
	   error("Encountered error in post_readdir_to_sink: bad handle idx %d", (int)idx_handle);
	}
}

static void post_remove_to_sink(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for remove.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_remove_to_sink: %d", rc);
	   return;
	}

	debug("Dispatch remove name \"%.*s\" to SFTP handler sink.", (int)len, path);

	len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%.*s'\n", id, "remove", (int)len, path);
	rc = write_to_handler_sink(buff_sink, len);
	if (rc < 1)
	{
	   error("Encountered error during write to SFTP handler sink in post_remove_to_sink: %d", errno);
	   return;
	}
}

static void post_mkdir_to_sink(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for mkdir.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_mkdir_to_sink: %d", rc);
	   return;
	}

	debug("Dispatch mkdir name \"%.*s\" to SFTP handler sink.", (int)len, path);

	/* TO DO - parse and communicate file attrs */

	len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%.*s'\n", id, "mkdir", (int)len, path);
	rc = write_to_handler_sink(buff_sink, len);
	if (rc < 1)
	{
	   error("Encountered error during write to SFTP handler sink in post_opendir_to_sink: %d", errno);
	   return;
	}
}

static void post_rmdir_to_sink(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for rmdir.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_rmdir_to_sink: %d", rc);
	   return;
	}

	debug("Dispatch rmdir name \"%.*s\" to SFTP handler sink.", (int)len, path);

	len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%.*s'\n", id, "rmdir", (int)len, path);
	rc = write_to_handler_sink(buff_sink, len);
	if (rc < 1)
	{
	   error("Encountered error during write to SFTP handler sink in post_rmdir_to_sink: %d", errno);
	   return;
	}
}

static void post_realpath_to_sink(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for realpath.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_realpath_to_sink: %d", rc);
	   return;
	}

	/* TO DO - parse and communicate control byte & component paths */

	debug("Dispatch realpath name \"%.*s\" to SFTP handler sink.", (int)len, path);

	len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%.*s'\n", id, "realpath", (int)len, path);
	rc = write_to_handler_sink(buff_sink, len);
	if (rc < 1)
	{
	   error("Encountered error during write to SFTP handler sink in post_realpath_to_sink: %d", errno);
	   return;
	}
}

static void post_rename_to_sink(u_int32_t id)
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
	   error("Encountered error during SSH buff peek in post_rename_to_sink: %d", rc);
	   return;
	}

	rc = get_ssh_string(buf + sizeof(u_int) + len, &npath, &nlen);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_rename_to_sink: %d", rc);
	   return;
	}

	debug("Dispatch rename from \"%.*s\" to \"%.*s\" to SFTP handler sink.", (int)len, path, (int)nlen, npath);

	len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%.*s' npath='%.*s'\n", id, "rename", (int)len, path, (int)nlen, npath);
	rc = write_to_handler_sink(buff_sink, len);
	if (rc < 1)
	{
	   error("Encountered error during write to SFTP handler sink in post_opendir_to_sink: %d", errno);
	   return;
	}
}

static void post_readlink_to_sink(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
	const u_char * path = NULL;

	logit("Processing custom handler override for readlink.");

	rc = sshbuf_peek_string_direct(iqueue, &path, &len);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_readlink_to_sink: %d", rc);
	   return;
	}

	debug("Dispatch readlink name \"%.*s\" to SFTP handler sink.", (int)len, path);

	len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%.*s'\n", id, "readlink", (int)len, path);
	rc = write_to_handler_sink(buff_sink, len);
	if (rc < 1)
	{
	   error("Encountered error during write to SFTP handler sink in post_readlink_to_sink: %d", errno);
	   return;
	}
}

static void post_symlink_to_sink(u_int32_t id)
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
	   error("Encountered error during SSH buff peek in post_symlink_to_sink: %d", rc);
	   return;
	}

	rc = get_ssh_string(buf + sizeof(u_int) + len, &npath, &nlen);
	if (rc != 0)
	{
	   error("Encountered error during SSH buff peek in post_symlink_to_sink: %d", rc);
	   return;
	}

	debug("Dispatch symlink to \"%.*s\" from \"%.*s\" to SFTP handler sink.", (int)nlen, npath, (int)len, path);

	len = sprintf(buff_sink, "id=%-10d rqst=%-10s path='%.*s' newpath='%.*s'\n", id, "symlink", (int)len, path, (int)nlen, npath);
	rc = write_to_handler_sink(buff_sink, len);
	if (rc < 1)
	{
	   error("Encountered error during write to SFTP handler sink in post_opendir_to_sink: %d", errno);
	   return;
	}
}

