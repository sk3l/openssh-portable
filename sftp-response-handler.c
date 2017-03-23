
#include "sftp-handler.h"
#include "sftp-response-handler.h"

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"
#include "misc.h"
#include "sshbuf.h"
#include "sftp.h"
#include "xmalloc.h"

extern struct sshbuf * oqueue;

extern int fd_fifo;

extern Handle * handles;
extern u_int num_handles;

static char buff_fifo[34000]; /* max packet size from SFTP protocol */

static void post_response_to_fifo(u_int32_t id);

struct sftp_handler rsp_overrides[] = {
	   { "response to fifo", NULL, SSH2_FXP_OPEN, post_response_to_fifo, 0 }
};

static const char * rsp_strings [] = {
   "ok",
   "status",
   "handle",
   "data",
   "name",
   "attrs"
};

static void post_response_to_fifo(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
   u_int  mlen = 0;
	u_char resp_type;
   const u_char * resp_ptr = NULL,
                * resp_str = NULL,
                * resp_msg = NULL;

	logit("Processing custom handler override SFTP response messages.");

   resp_ptr = sshbuf_ptr(oqueue);
   
   len = get_u32(resp_ptr);
   if (len < 1)
   {
	   error("Encountered empty SFTP response message in post_response_to_fifo");
	   return;
   }

   resp_type = *(resp_ptr + sizeof(u_int));
      
   if (resp_type == 0)
      resp_str = rsp_strings[0];
   else if (resp_type == SSH2_FXP_STATUS)
   {
      resp_type -= 100;
      resp_str = rsp_strings[resp_type];

      rc = get_ssh_string(resp_ptr + 1 + 3*(sizeof(u_int)), &resp_msg, &mlen);
	   if (rc != 0)
	   {
	      error("Encountered error during SSH buff peek in post_response_to_fifo: %d", rc);
	      return;
	   }

      len = sprintf(buff_fifo, "id=%-10d resp=%-10s msg=%.*s\n", id, resp_str, (int)mlen, resp_msg);
   }
   else if (resp_type > SSH2_FXP_STATUS || resp_type <= SSH2_FXP_ATTRS)
   {
      resp_type -= 100;
      resp_str = rsp_strings[resp_type];
      len = sprintf(buff_fifo, "id=%-10d resp=%-10s\n", id, resp_str);
   }
   else
	{
	   error("Encountered unknown SFTP response type %d in post_response_to_fifo", resp_type);
	   return;
	}

	rc = write(fd_fifo, buff_fifo, len);
	if (rc < 1)
	{
	   error("Encountered error during FIFO write in post_open_to_fifo: %d", errno);
	}
}

