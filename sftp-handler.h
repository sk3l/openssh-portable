#ifndef SFTP_HANDLER_H
#define SFTP_HANDLER_H

#include "includes.h"

typedef void (*handler_fnc)(u_int);

struct sftp_handler {
	const char *name;	         /* user-visible name for fine-grained perms */
   const char *ext_name;	   /* extended request name */
   u_int type;		            /* packet type, for non extended packets */
   handler_fnc handler;
   int does_write;		      /* if nonzero, banned for readonly mode */
};

typedef struct sftp_handler * handler_ptr;
typedef handler_ptr *         handler_list;

handler_ptr alloc_handler(
   const char * name, const char * extname, int type, handler_fnc, int writable);

int init_handler_overrides(handler_list * handlers);

int get_ssh_string(const char * buf, const u_char ** str, u_int * len);

#endif
