#ifndef SFTP_PLUGIN_CONF_H
#define SFTP_PLUGIN_CONF_H

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
};

int sftp_plugins_init();
int sftp_plugins_release();

int get_plugins(Plugin ** plugins, int * cnt);

#endif
