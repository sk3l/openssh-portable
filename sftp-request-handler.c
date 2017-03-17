
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

extern Handle * handles;
extern u_int num_handles;

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

static Handle * get_handle(const char *, int, int);

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
	   { "print symlink to fifo", NULL, SSH2_FXP_SYMLINK, post_symlink_to_fifo, 1 },
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

   len = sprintf(buff_fifo, "op=%-10s path='%.*s'\n", "open", (int)len, path);
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

   hptr = get_handle(handle, len, -1);
   if (hptr != NULL) {
	   debug("Dispatch close of path \"%s\" to event FIFO.", hptr->name);

      len = sprintf(buff_fifo, "op=%-10s path='%s'\n", "close", hptr->name);
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

   hptr = get_handle(handle, len, -1);
   if (hptr != NULL) {
	   debug("Dispatch read from path \"%s\" to event FIFO.", hptr->name);

      /* establish read offset  */
      off = get_u64(handle + len);

      /* establish read max size */
      max = get_u32(handle + len + sizeof(u_int64_t));

      len = sprintf(buff_fifo, "op=%-10s path='%s' off=%lld max=%d\n", "read", hptr->name, (long long)off, (int)max);
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

   hptr = get_handle(handle, len, -1);
   if (hptr != NULL) {
	   debug("Dispatch write to path \"%s\" to event FIFO.", hptr->name);

      /* establish read offset  */
      off = get_u64(handle + len);

      /* establish read max size */
      cnt = get_u32(handle + len + sizeof(u_int64_t));

      len = sprintf(buff_fifo, "op=%-10s path='%s' off=%lld cnt=%d\n", "write", hptr->name, (long long)off, (int)cnt);
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

	debug("Dispatch lstat name \"%.*s\" to event FIFO.", (int)len, path);

   len = sprintf(buff_fifo, "op=%-10s path='%.*s'\n", "lstat", (int)len, path);
	rc = write(fd_fifo, buff_fifo, len);
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

   len = sprintf(buff_fifo, "op=%-10s path='%.*s'\n", "opendir", (int)len, path);
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

   hptr = get_handle(handle, len, HANDLE_DIR);
   if (hptr != NULL) {
		debug("Dispatch readdir name \"%s\" to event FIFO.", hptr->name);

      len = sprintf(buff_fifo, "op=%-10s path='%s'\n", "readdir", hptr->name);
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
	logit("Processing custom handler override for remove.");
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

	len = sprintf(buff_fifo, "op=%-10s path='%.*s'\n", "mkdir", (int)len, path);
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

	len = sprintf(buff_fifo, "op=%-10s path='%.*s'\n", "rmdir", (int)len, path);
	rc = write(fd_fifo, buff_fifo, len);
	if (rc < 1)
	{
	   error("Encountered error during FIFO write in post_opendir_to_fifo: %d", errno);
	   return;
	}
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

static Handle * get_handle(const char * idx_str, int len, int hkind) {
   Handle * hptr;
   u_int idx_handle = -1;

   /* Handles are conveyed as string in protocol, but they are really int32
    * index into global handles array.*/
   if (len != sizeof(int)) {
	   error("Encountered error in get_handle: handle len of %d", (int)len);
	   return NULL;
   }
   idx_handle = get_u32(idx_str);

   /* Handle check is from get_handle() func in sftp-server.c */
   if (idx_handle < num_handles) {
      hptr = &handles[idx_handle];
      if ((hkind >= 0) && (hptr->use != hkind)) {
	      error("Encountered error in get_handle: expected handle tpe %d, bu got %d",
               hkind, hptr->use);
	      return NULL;
      }

      return hptr;
   }
   else {
	   error("Encountered error in get_handle: handle idx %d out of bounds %d",
           idx_handle, num_handles);
      return NULL;
   }
}
