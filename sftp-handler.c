
#include "sftp-handler.h"
#include "sftp-handler-sink.h"

#include <fcntl.h>
#include <sys/stat.h>

#include "log.h"
#include "misc.h"
#include "sftp.h"
#include "xmalloc.h"

extern Handle * handles;
extern u_int num_handles;

extern struct sftp_handler req_overrides[];
extern struct sftp_handler rsp_overrides[];

int init_handler_overrides(handler_list * htbl)
{
	u_int i;
	handler_list   hlist = NULL,
	           new_hlist = NULL;

	if (!init_handler_sink())
	{
	   error("Encountered error initializing the SFTP handler sink: %d", errno);
	   return 0;
	}

	logit("Initializing custom handler overrides.");

	/* Insert event handlers overrides*/
	for (i = 0; i < SSH2_FXP_MAX; i++) {
	   if (req_overrides[i].handler == NULL)
	      continue;

	   hlist = htbl[i];

	   new_hlist = (handler_list) xcalloc(sizeof(handler_ptr), 4);
	   new_hlist[0] = &req_overrides[i];
	   new_hlist[1] = hlist[0];
	   new_hlist[2] = &rsp_overrides[0];

	   htbl[i] = new_hlist;
	   free(hlist);
	}
	return 1;
}

/* Helper func to lookup & retrieve ptr to Handle from SFTP server module. */
Handle * get_sftp_handle(const char * idx_str, int len, int hkind) {
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

/* Helper func to peek at string value contained in SFTP protocol payload.
 * Note structure of strings follows SSH convetion: (<int_len>+<value>) */ 
int get_ssh_string(const char * buf, const u_char ** str, u_int * lenp)
{
	if ((buf == NULL) || (str == NULL))
	{
	   return 1;
	}

	*lenp = get_u32(buf);
	*str  = buf + sizeof(u_int);

	return 0;
}
