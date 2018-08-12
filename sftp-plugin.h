#ifndef SFTP_PLUGIN_H
#define SFTP_PLUGIN_H

//#include "includes.h"

#include "sftp.h"
#include "sftp-callback.h"

#define MAX_CALLBACK_IDX SSH2_FXP_EXTENDED_REPLY		

// Order of invocation of plugin, relative to main SFTP handlers
enum PLUGIN_SEQUENCE {
   PLUGIN_SEQ_BEFORE = 0,
   PLUGIN_SEQ_INSTEAD= 1,
   PLUGIN_SEQ_AFTER  = 2
};   

typedef struct Plugin Plugin;
struct Plugin {
   void * so_handle_;
   char * name_;
   enum PLUGIN_SEQUENCE sequence_;
   callback_ptr callbacks[MAX_CALLBACK_IDX];
};

// Initialize the list of SFTP plugins
int sftp_plugins_init();

// Release the list of SFTP plugins
int sftp_plugins_release();

// Return an array of SFTP plugins 
int get_plugins(Plugin ** plugins, int * cnt);

#endif
