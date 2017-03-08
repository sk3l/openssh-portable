#include "includes.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "log.h"
#include "sftp.h"
#include "sftp-handler.h"
#include "xmalloc.h"

static int fd_fifo;
static const char * path_fifo = "/tmp/sftp_evts";

static void post_stat_to_fifo(u_int32_t id);
static void post_lstat_to_fifo(u_int32_t id);

static struct sftp_handler overrides[] = {
   {"stat fifo", NULL, SSH2_FXP_STAT, post_stat_to_fifo, 0},
   {"lstat fifo", NULL, SSH2_FXP_STAT, post_lstat_to_fifo, 0}
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

   /* Append event handlers of interest */
   hptr = handlers[SSH2_FXP_LSTAT];

   new_hptr = (handler_ptr) xcalloc(handler_size, 3);   
   memcpy(new_hptr, hptr, handler_size);
   memcpy(&new_hptr[1], &overrides[1], handler_size);

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
   logit("Processing custom handler override for lstat.");
}
