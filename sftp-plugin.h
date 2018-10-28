#ifndef SFTP_PLUGIN_H
#define SFTP_PLUGIN_H

//#include "includes.h"

#include "sftp.h"
#include "sftp-callback.h"

#define MAX_CALLBACK_IDX SSH2_FXP_EXTENDED_REPLY

// Order of invocation of plugin, relative to main SFTP handlers
enum PLUGIN_SEQUENCE {
   PLUGIN_SEQ_UNKNOWN= 0,
   PLUGIN_SEQ_BEFORE = 1,
   PLUGIN_SEQ_INSTEAD= 2,
   PLUGIN_SEQ_AFTER  = 3
};

typedef struct Plugin Plugin;
struct Plugin {
   void * so_handle_;
   char * name_;
   enum PLUGIN_SEQUENCE sequence_;
   struct sftp_callbacks callbacks_;
};


// Initialize the list of SFTP plugins
int sftp_plugins_init();

// Release the list of SFTP plugins
int sftp_plugins_release();

// Return an array of SFTP plugins
int get_plugins(Plugin ** plugins, size_t * cnt);

// Methods for invocation of SFTP plugin methods
int call_open_file_plugins(uint32_t, const char *, uint32_t, uint32_t, attribs_ptr, enum PLUGIN_SEQUENCE);
int call_open_dir_plugins(uint32_t, const char *, enum PLUGIN_SEQUENCE);
int call_close_plugins(uint32_t, int, enum PLUGIN_SEQUENCE);
int call_read_plugins(uint32_t, int, uint64_t, uint32_t, enum PLUGIN_SEQUENCE);
int call_read_dir_plugins(uint32_t, int, enum PLUGIN_SEQUENCE);
int call_write_plugins(uint32_t, int, uint64_t, const char *, enum PLUGIN_SEQUENCE);
int call_remove_plugins(uint32_t, const char *, enum PLUGIN_SEQUENCE);
int call_rename_plugins(uint32_t, const char *, const char *, uint32_t, enum PLUGIN_SEQUENCE);
int call_mkdir_plugins(uint32_t, const char *, enum PLUGIN_SEQUENCE);
int call_rmdir_plugins(uint32_t, const char *, enum PLUGIN_SEQUENCE);
int call_stat_plugins(uint32_t, const char *, uint32_t, enum PLUGIN_SEQUENCE);
int call_lstat_plugins(uint32_t, const char *, uint32_t, enum PLUGIN_SEQUENCE);
int call_fstat_plugins(uint32_t, int, uint32_t, enum PLUGIN_SEQUENCE);
int call_setstat_plugins(uint32_t, const char *, attribs_ptr, enum PLUGIN_SEQUENCE);
int call_fsetstat_plugins(uint32_t, int, attribs_ptr, enum PLUGIN_SEQUENCE);
int call_read_link_plugins(uint32_t, const char *, enum PLUGIN_SEQUENCE);
int call_link_plugins(uint32_t, const char *, const char *, int, enum PLUGIN_SEQUENCE);
int call_lock_plugins(uint32_t, const char *, uint64_t, uint64_t,int, enum PLUGIN_SEQUENCE);
int call_unlock_plugins(uint32_t, const char *, uint64_t, uint64_t, enum PLUGIN_SEQUENCE);
int call_realpath_plugins(uint32_t, const char *, uint8_t, const char *, enum PLUGIN_SEQUENCE);

#endif
