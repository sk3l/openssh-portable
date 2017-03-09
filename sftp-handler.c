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

static void post_stat_to_fifo(u_int32_t id);
static void post_lstat_to_fifo(u_int32_t id);

static struct sftp_handler overrides[] = {
   {"stat fifo", NULL, SSH2_FXP_STAT, post_stat_to_fifo, 0},
   {"lstat fifo", NULL, SSH2_FXP_LSTAT, post_lstat_to_fifo, 0}
};

int init_handler_overrides(handler_tbl handlers)
{
   handler_ptr     hptr = NULL,
               new_hptr = NULL;

   u_int32_t handler_size = sizeof(struct sftp_handler);

   logit("Initializing custom handler overrides.");

   fd_fifo = open(path_fifo, O_WRONLY | O_NONBLOCK);
   if (fd_fifo < 0)
   {
      error("Encountered error opening SFTP event FIFO: %d", errno);
      return 0;
   }

   /* Insert event handlers of interest */
   hptr = handlers[SSH2_FXP_LSTAT];

   new_hptr = (handler_ptr) xcalloc(handler_size, 3);
   memcpy(&new_hptr[0], &overrides[1], handler_size);
   memcpy(&new_hptr[1], hptr, handler_size);

   handlers[SSH2_FXP_LSTAT] = new_hptr;
   free(hptr);

   return 1;
}

void post_stat_to_fifo(u_int32_t id)
{
   logit("Processing custom handler override for stat.");
}

void post_lstat_to_fifo(u_int32_t id)
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
