
#include "sftp-handler.h"
#include "sftp-handler-sink.h"
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

static size_t RESPONSE_OFFSET = sizeof(u_int) + sizeof(char) + sizeof(u_int);

static void post_response_to_fifo(u_int32_t id)
{
	int rc = 0;
	size_t len = 0;
   u_int  mlen = 0;
	u_char resp_type;

   const u_char * resp_ptr = NULL,
                * resp_str = NULL,
                * resp_msg = NULL;

   Handle * hptr = NULL;

	logit("Processing custom handler override SFTP response messages.");

   resp_ptr = sshbuf_ptr(oqueue);
   
   len = get_u32(resp_ptr);
   if (len < 1)
   {
	   error("Encountered empty SFTP response message in post_response_to_fifo");
	   return;
   }

   resp_type = *(resp_ptr + sizeof(u_int));
      
   if (resp_type == SSH2_FX_OK)
   {
      resp_str = rsp_strings[0];
   }
   else if (resp_type == SSH2_FXP_STATUS)
   {
      resp_type -= 100;
      resp_str = rsp_strings[resp_type];

      rc = get_ssh_string(resp_ptr + RESPONSE_OFFSET + (sizeof(u_int)), &resp_msg, &mlen);
	   if (rc != 0)
	   {
	      error("Encountered error during SSH buff peek in post_response_to_fifo: %d", rc);
	      return;
	   }

      len = sprintf(buff_fifo, "id=%-10d resp=%-10s msg='%.*s'\n", id, resp_str, (int)mlen, resp_msg);
   }
   else if (resp_type == SSH2_FXP_HANDLE)
   {
      resp_type -= 100;
      resp_str = rsp_strings[resp_type];

      rc = get_ssh_string(resp_ptr + RESPONSE_OFFSET, &resp_msg, &mlen);
	   if (rc != 0)
	   {
	      error("Encountered error during SSH buff peek in post_response_to_fifo: %d", rc);
	      return;
	   }

	   hptr = get_sftp_handle(resp_msg, mlen, -1);
		if (hptr != NULL) {
         len = sprintf(buff_fifo, "id=%-10d resp=%-10s path='%s'\n", id, resp_str, hptr->name);
      } else {
	      error("Encountered error in post_close_to_fifo: bad handle");
         return;
	   }
   }
   else if (resp_type == SSH2_FXP_DATA)
   {
      resp_type -= 100;
      resp_str = rsp_strings[resp_type];

      mlen = get_u32(resp_ptr + RESPONSE_OFFSET);

      len = sprintf(buff_fifo, "id=%-10d resp=%-10s len=%d\n", id, resp_str, mlen);
   }
   else if (resp_type == SSH2_FXP_NAME)
   {
      resp_type -= 100;
      resp_str = rsp_strings[resp_type];

      mlen = get_u32(resp_ptr + RESPONSE_OFFSET);

      len = sprintf(buff_fifo, "id=%-10d resp=%-10s cnt=%d\n", id, resp_str, mlen);
   }
   else if (resp_type == SSH2_FXP_ATTRS)
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

	rc = write_to_handler_sink(buff_fifo, len);
	if (rc < 1)
	{
	   error("Encountered error during write to SFTP handler sink in post_open_to_fifo: %d", errno);
	}
}

