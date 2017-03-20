
#include "sftp-handler.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"
#include "misc.h"
#include "sftp.h"
#include "xmalloc.h"

int fd_fifo;
static const char * path_fifo = "/tmp/sftp_evts";

extern struct sftp_handler req_overrides[];

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
	   if (req_overrides[i].handler == NULL)
	      continue;

	   hlist = htbl[i];

	   new_hlist = (handler_list) xcalloc(sizeof(handler_ptr), 3);
	   new_hlist[0] = &req_overrides[i];
	   new_hlist[1] = hlist[0];

	   htbl[i] = new_hlist;
	   free(hlist);
	}
	return 1;
}

int get_ssh_string(const char * buf, const u_char ** str, u_int * lenp)
{
   if ((buf == NULL) || (str == NULL) || (*str == NULL))
   {
      return 1;
   }

   *lenp = get_u32(buf);
   *str  = buf + sizeof(u_int);

   return 0;
}
