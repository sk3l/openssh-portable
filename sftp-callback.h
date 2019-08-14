#ifndef SFTP_CALLBACK_H
#define SFTP_CALLBACK_H

#include <sys/types.h>

#include "sftp-common.h"

void set_cbk_attribs(
    Attrib *, u_int32_t, u_int64_t, u_int32_t, u_int32_t, u_int32_t,u_int32_t, u_int32_t);

/* Request callback functions */
typedef int (*sftp_cbk_open_file)(u_int32_t, const char *, int *, u_int32_t, u_int32_t, Attrib *);
typedef int (*sftp_cbk_open_dir) (u_int32_t, const char *, int *);
typedef int (*sftp_cbk_close)    (u_int32_t, const char *, int);
typedef int (*sftp_cbk_read)     (u_int32_t, const char *, int, u_int64_t, u_int32_t, u_char *, int *);
typedef int (*sftp_cbk_read_dir) (u_int32_t, const char *, StatList *);
typedef int (*sftp_cbk_write)    (u_int32_t, const char *, int, u_int64_t, u_int32_t, u_char *, int *);
typedef int (*sftp_cbk_remove)   (u_int32_t, const char *);
typedef int (*sftp_cbk_rename)   (u_int32_t, const char *, const char *);
typedef int (*sftp_cbk_mkdir)    (u_int32_t, const char *, Attrib *);
typedef int (*sftp_cbk_rmdir)    (u_int32_t, const char *);
typedef int (*sftp_cbk_stat)     (u_int32_t, const char *, Attrib *);
typedef int (*sftp_cbk_lstat)    (u_int32_t, const char *, Attrib *);
typedef int (*sftp_cbk_fstat)    (u_int32_t, const char *, Attrib *);
typedef int (*sftp_cbk_setstat)  (u_int32_t, const char *, Attrib *);
typedef int (*sftp_cbk_fsetstat) (u_int32_t, const char *, Attrib *);
typedef int (*sftp_cbk_read_link)(u_int32_t, const char *, Stat *);
typedef int (*sftp_cbk_symlink)  (u_int32_t, const char *, const char *);
typedef int (*sftp_cbk_lock)     (u_int32_t, const char *, u_int64_t, u_int64_t,int);
typedef int (*sftp_cbk_unlock)   (u_int32_t, const char *, u_int64_t, u_int64_t);
typedef int (*sftp_cbk_realpath) (u_int32_t, const char *, Stat *);

/* Response callback functions */
typedef int (*sftp_cbk_status)   (u_int32_t, u_int32_t, const char *, const char *);
typedef int (*sftp_cbk_handle)   (u_int32_t, const char *);
typedef int (*sftp_cbk_data)     (u_int32_t, const char *, int);
typedef int (*sftp_cbk_name)     (u_int32_t, u_int32_t, const char *, int);
typedef int (*sftp_cbk_attrs)    (u_int32_t, u_int32_t, u_int32_t);

typedef void (*sftp_cbk_func)    ();

struct sftp_callbacks {
    sftp_cbk_open_file cf_open_file;
    sftp_cbk_open_dir  cf_open_dir;
    sftp_cbk_close     cf_close;
    sftp_cbk_read      cf_read;
    sftp_cbk_read_dir  cf_read_dir;
    sftp_cbk_write     cf_write;
    sftp_cbk_remove    cf_remove;
    sftp_cbk_rename    cf_rename;
    sftp_cbk_mkdir     cf_mkdir;
    sftp_cbk_rmdir     cf_rmdir;
    sftp_cbk_stat      cf_stat;
    sftp_cbk_lstat     cf_lstat;
    sftp_cbk_fstat     cf_fstat;
    sftp_cbk_setstat   cf_setstat;
    sftp_cbk_fsetstat  cf_fsetstat;
    sftp_cbk_read_link cf_read_link;
    sftp_cbk_symlink   cf_symlink;
    sftp_cbk_lock      cf_lock;
    sftp_cbk_unlock    cf_unlock;
    sftp_cbk_realpath  cf_realpath;
    sftp_cbk_status    cf_status;
    sftp_cbk_handle    cf_handle;
    sftp_cbk_data      cf_data;
    sftp_cbk_name      cf_name;
    sftp_cbk_attrs     cf_attrs;
};
typedef struct sftp_callbacks * callbacks_ptr;

enum SFTP_CALLBACK_FUNC {
    CBACK_FUNC_OPEN_FILE,
    CBACK_FUNC_OPEN_DIR,
    CBACK_FUNC_CLOSE,
    CBACK_FUNC_READ,
    CBACK_FUNC_READ_DIR,
    CBACK_FUNC_WRITE,
    CBACK_FUNC_REMOVE,
    CBACK_FUNC_RENAME,
    CBACK_FUNC_MKDIR,
    CBACK_FUNC_RMDIR,
    CBACK_FUNC_STAT,
    CBACK_FUNC_LSTAT,
    CBACK_FUNC_FSTAT,
    CBACK_FUNC_SETSTAT,
    CBACK_FUNC_FSETSTAT,
    CBACK_FUNC_READ_LINK,
    CBACK_FUNC_SYMLINK,
    CBACK_FUNC_LOCK,
    CBACK_FUNC_UNLOCK,
    CBACK_FUNC_REALPATH,
    CBACK_FUNC_STATUS,
    CBACK_FUNC_HANDLE,
    CBACK_FUNC_DATA,
    CBACK_FUNC_NAME,
    CBACK_FUNC_ATTRS,
    CBACK_FUNC_SENTRY   /* LAST CALLBACK enum*/
};

// Given SFTP callback function enum, return name of callback symbol name
const char * get_sftp_callback_sym(enum SFTP_CALLBACK_FUNC scf);

int set_sftp_callback_func(callbacks_ptr cp, enum SFTP_CALLBACK_FUNC scf, sftp_cbk_func f);

#endif
