#include <stdlib.h>
#include <stdio.h>

#include "sftp-plugin.h"

#include "log.h"
#include "pathnames.h"

#define SFTP_PLUGIN_CONF SSHDIR "/sftp_plugin_conf"

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

// Read & parse the SFTP plugin conf
static void load_plugin_conf()
{

}

// Open & scan the SFTP plugin shared objects
static void load_plugin_so()
{

}

int sftp_plugins_init()
{
   load_plugin_conf();

   load_plugin_so();

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
