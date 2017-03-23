#ifndef SFTP_RESPONSE_HANDLER_H
#define SFTP_RESPONSE_HANDLER_H

#include "includes.h"
#include <dirent.h>

/* redeclaration from sftp-server.c (need to make a common module)*/
typedef struct Handle Handle;
struct Handle {
	int use;
	DIR *dirp;
	int fd;
	int flags;
	char *name;
	u_int64_t bytes_read, bytes_write;
	int next_unused;
};

enum {
	HANDLE_UNUSED,
	HANDLE_DIR,
	HANDLE_FILE
};

#endif
