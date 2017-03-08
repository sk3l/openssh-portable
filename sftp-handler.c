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

static struct sftp_handler overrides[] = {
   {"stat fifo", NULL, SSH2_FXP_STAT, post_stat_to_fifo, 0}
};

int init_handler_overrides(handler_tbl handlers)
{
   int rv = 0;
   handler_ptr     hptr = NULL,
               new_hptr = NULL;
   
   logit("Initializing custom handler overrides.");

   /* Initialize the event FIFO */
   rv = mkfifo(path_fifo, 666);
   if (rv != 0)
   {
      if (errno != EEXIST)
      {
         error("Encountered error creting SFTP event FIFO: %d", errno);
         return 0; 
      } 
   }

   fd_fifo = open(path_fifo, O_WRONLY | O_NONBLOCK);
   if (fd_fifo < 0)
   {
      error("Encountered error opening SFTP event FIFO: %d", errno);
      return 0; 
   }    

   /* Append event handlers of interest */
   hptr = handlers[SSH2_FXP_STAT];

   new_hptr = (handler_ptr) xcalloc(sizeof(struct sftp_handler), 3);   
   memcpy(new_hptr, hptr, sizeof(struct sftp_handler));
   memcpy(new_hptr + sizeof(struct sftp_handler), &overrides[0], sizeof(struct sftp_handler)); 

   handlers[SSH2_FXP_STAT] = new_hptr;
   free(hptr);

   return 1;
}

void post_stat_to_fifo(u_int32_t id)
{
   logit("Processing custom handler override for stat.");
}
