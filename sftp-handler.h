#ifndef SFTP_HANDLER_H
#define SFTP_HANDLER_H

struct sftp_handler {
	const char *name;	         /* user-visible name for fine-grained perms */
   const char *ext_name;	   /* extended request name */
   u_int type;		            /* packet type, for non extended packets */
   void (*handler)(u_int32_t);
   int does_write;		      /* if nonzero, banned for readonly mode */
};

typedef struct sftp_handler * handler_ptr;
typedef handler_ptr *         handler_tbl;

void init_handler_overrides(handler_tbl handlers); 

#endif
