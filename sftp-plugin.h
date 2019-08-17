#ifndef SFTP_PLUGIN_H
#define SFTP_PLUGIN_H

#include <sys/types.h>

#include "sftp-common.h"
#include "sftp-callback.h"

#define MAX_CALLBACK_IDX SSH2_FXP_EXTENDED_REPLY

#define MAX_PLUGIN_CNT 8

#define PLUGIN_CBK_SUCCESS 0
#define PLUGIN_CBK_FAILURE 1

// Order of invocation of plugin, relative to main SFTP handlers
enum PLUGIN_SEQUENCE {
   PLUGIN_SEQ_UNKNOWN= 0,
   PLUGIN_SEQ_BEFORE = 1,
   PLUGIN_SEQ_INSTEAD= 2,
   PLUGIN_SEQ_AFTER  = 3
};

const char * plugin_sequence_enum_to_str(enum PLUGIN_SEQUENCE);

typedef struct Plugin Plugin;
struct Plugin {
   void * so_handle_;
   char * name_;
   enum PLUGIN_SEQUENCE sequence_;
   struct sftp_callbacks callbacks_;
};

// Collect count of number of callbacks invoked, and number of failures
typedef struct callback_stats callback_stats;
struct callback_stats {
    u_int8_t invocation_cnt_;
    u_int8_t failure_cnt_;
};

// Initialize the list of SFTP plugins
int sftp_plugins_init();

// Release the list of SFTP plugins
int sftp_plugins_release();

// Return an array of SFTP plugins
int get_plugins(Plugin ** plugins, size_t * cnt);

// Methods for invocation of SFTP plugin methods
int call_open_file_plugins(u_int32_t, const char *, u_int32_t, u_int32_t, int *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_open_dir_plugins(u_int32_t, const char *, int *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_close_plugins(u_int32_t, const char *, int, enum PLUGIN_SEQUENCE, callback_stats *);
int call_read_plugins(u_int32_t, const char *, int, u_int64_t, u_int32_t, u_char *, int *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_read_dir_plugins(u_int32_t, const char *, StatList *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_write_plugins(u_int32_t, const char *, int, u_int64_t, u_int32_t, u_char *, int *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_remove_plugins(u_int32_t, const char *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_rename_plugins(u_int32_t, const char *, const char *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_mkdir_plugins(u_int32_t, const char *, Attrib *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_rmdir_plugins(u_int32_t, const char *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_stat_plugins(u_int32_t, const char *, Attrib *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_lstat_plugins(u_int32_t, const char *, Attrib *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_fstat_plugins(u_int32_t, const char *, Attrib *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_setstat_plugins(u_int32_t, const char *, Attrib *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_fsetstat_plugins(u_int32_t, const char *, Attrib *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_read_link_plugins(u_int32_t, const char *, Stat *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_symlink_plugins(u_int32_t, const char *, const char *, enum PLUGIN_SEQUENCE, callback_stats *);
int call_lock_plugins(u_int32_t, const char *, u_int64_t, u_int64_t,int, enum PLUGIN_SEQUENCE, callback_stats *);
int call_unlock_plugins(u_int32_t, const char *, u_int64_t, u_int64_t, enum PLUGIN_SEQUENCE, callback_stats *);
int call_realpath_plugins(u_int32_t, const char *, Stat *, enum PLUGIN_SEQUENCE, callback_stats *);

#endif
