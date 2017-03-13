#include "includes.h"

#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"
#include "misc.h"
#include "sshbuf.h"
#include "sftp.h"
#include "sftp-handler.h"
#include "xmalloc.h"

extern struct sshbuf * iqueue;

/* redeclaration from sftp-server.c (need to make a common module)*/
typedef struct Handle Handle;
struct Handle {
	int use;
	DIR *dirp;
	int fd;
	int flags;
	char *name;
	u_int64_t bytes_read, bytes_write;
	int next_unused;
};
extern Handle * handles;
extern u_int num_handles;

enum {
	HANDLE_UNUSED,
	HANDLE_DIR,
	HANDLE_FILE
};

static int fd_fifo;
static char buff_fifo[34000]; /* max packet size from SFTP protocol */
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

static Handle * get_handle(const char *, int, int);

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

	debug("Dispatch open name \"%s\" to event FIFO.", path);

   len = sprintf(buff_fifo, "op=open path='%s'\n", path);
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

      len = sprintf(buff_fifo, "op=close path='%s'\n", hptr->name);
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

   len = sprintf(buff_fifo, "op=lstat path='%s'\n", path);
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

	debug("Dispatch opendir name \"%s\" to event FIFO.", path);

   len = sprintf(buff_fifo, "op=opendir path='%s'\n", path);
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

      len = sprintf(buff_fifo, "op=readdir path='%s'\n", hptr->name);
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
