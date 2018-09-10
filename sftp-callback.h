#ifndef SFTP_CALLBACK_H
#define SFTP_CALLBACK_H

#include "includes.h"

/* Request callback functions */
typedef int (*sftp_cbk_open_file)(uint32_t, const char *, uint32_t, uint32_t, uint32_t);
typedef int (*sftp_cbk_open_dir) (uint32_t, const char *);
typedef int (*sftp_cbk_close)    (uint32_t, const char *);
typedef int (*sftp_cbk_read)     (uint32_t, const char *, uint64_t, uint32_t);
typedef int (*sftp_cbk_read_dir) (uint32_t, const char *);
typedef int (*sftp_cbk_write)    (uint32_t, const char *, uint64_t, const char *);
typedef int (*sftp_cbk_remove)   (uint32_t, const char *);
typedef int (*sftp_cbk_rename)   (uint32_t, const char *, const char *, uint32_t);
typedef int (*sftp_cbk_mkdir)    (uint32_t, const char *);
typedef int (*sftp_cbk_rmdir)    (uint32_t, const char *);
typedef int (*sftp_cbk_stat)     (uint32_t, const char *, uint32_t);
typedef int (*sftp_cbk_lstat)    (uint32_t, const char *, uint32_t);
typedef int (*sftp_cbk_fstat)    (uint32_t, const char *, uint32_t);
typedef int (*sftp_cbk_setstat)  (uint32_t, const char *, uint32_t);
typedef int (*sftp_cbk_fsetstat) (uint32_t, const char *, uint32_t);
typedef int (*sftp_cbk_link)     (uint32_t, const char *, const char *, int);
typedef int (*sftp_cbk_lock)     (uint32_t, const char *, uint64_t, uint64_t,int);
typedef int (*sftp_cbk_unlock)   (uint32_t, const char *, uint64_t, uint64_t);
typedef int (*sftp_cbk_realpath) (uint32_t, const char *, uint8_t, const char *);
/* Response callback functions */
typedef int (*sftp_cbk_status)   (uint32_t, uint32_t, const char *, const char *);
typedef int (*sftp_cbk_handle)   (uint32_t, const char *);
typedef int (*sftp_cbk_data)     (uint32_t, const char *, int);
typedef int (*sftp_cbk_name)     (uint32_t, uint32_t, const char *, int);
typedef int (*sftp_cbk_attrs)    (uint32_t, uint32_t, uint32_t);

/* Extension callback functions */

struct sftp_callbacks {
  sftp_cbk_open_file pf_open_file;
  sftp_cbk_open_dir  pf_open_dir;
  sftp_cbk_close     pf_close;
  sftp_cbk_read      pf_read;
  sftp_cbk_read_dir  pr_read_dir;
  sftp_cbk_write     pf_write;
  sftp_cbk_remove    pf_remove;
  sftp_cbk_rename    pf_rename;
  sftp_cbk_mkdir     pf_mkdir;
  sftp_cbk_rmdir     pf_rmdir;
  sftp_cbk_stat      pf_stat;
  sftp_cbk_lstat     pf_lstat;
  sftp_cbk_fstat     pf_fstat;
  sftp_cbk_setstat   pf_setstat;
  sftp_cbk_fsetstat  pf_fsetstat;
  sftp_cbk_link      pf_link;
  sftp_cbk_lock      pf_lock;
  sftp_cbk_unlock    pf_unlock;
  sftp_cbk_realpath  pf_realpath;
  sftp_cbk_status    pf_status;
  sftp_cbk_handle    pf_handle;
  sftp_cbk_data      pf_data;
  sftp_cbk_name      pf_name;
  sftp_cbk_attrs     pf_attrs;
};

typedef struct sftp_callbacks * callbacks_ptr;


#endif
