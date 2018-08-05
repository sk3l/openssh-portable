#include <stdlib.h>
#include <stdio.h>

#include "sftp-plugin-conf.h"

static Plugin * plugins_;

const char * PLUGIN_SEQUENCE_STR [] = {
   "BEFORE",
   "INSTEAD",
   "AFTER"
};

static enum PLUGIN_SEQUENCE plugin_sequence_str_to_enum(const char * sequence)
{
   enum PLUGIN_SEQUENCE ps = PLUGIN_SEQ_BEFORE;
      
   return ps;
}

int sftp_plugins_init()
{
   return 0;
}

int sftp_plugins_release()
{
   return 0;
}

int get_plugins(Plugin ** plugins, int * cnt)
{
   return 0;
}
