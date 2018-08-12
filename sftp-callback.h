#ifndef SFTP_CALLBACK_H
#define SFTP_CALLBACK_H

#include "includes.h"

typedef int (*sftp_callback_func)(const char *);

struct sftp_callback {
	const char *name_;	         /* descriptive name for log print */
   int type_;		               /* packet type, for non extended packets */
   sftp_callback_func callback_; /* the actual plugin function to exexute */
};

typedef struct sftp_callback * callback_ptr;

callback_ptr create_sftp_callback(const char * name, int type, sftp_callback_func cbkfunc);

void destroy_sftp_callback(callback_ptr pcbk);

#endif
