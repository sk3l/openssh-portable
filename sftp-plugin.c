#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "sshbuf.h"
#include "sftp-plugin.h"

#include "log.h"
#include "pathnames.h"
#include "xmalloc.h"

#define SFTP_PLUGIN_CONF SSHDIR "/sftp_plugin_conf"

static struct sshbuf * fbuf;

static Plugin * plugins;

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

static void skip_space(char ** pstr, int len)
{
   while ((len > 0) && isspace(**pstr))
   {
      --len;
      (*pstr)++;
   }
}

// Read & parse the SFTP plugin conf
static int load_plugin_conf()
{
	char * line = NULL, * lpos;
	size_t linesize = 0, linecnt = 0;
   int i, rc;
   FILE * fconf;

   if ((fconf = fopen(SFTP_PLUGIN_CONF, "r")) == NULL)
   {
      perror(SFTP_PLUGIN_CONF);
      exit(1);
   } 

   while (getline(&line, &linesize, fconf) != -1)
   {
      ++linecnt;
      lpos = line;

      // Skip whitespace
      skip_space(&lpos, linesize);

      // Skip empty lines, comment lines
      if ((i == linesize) || (*lpos == '#'))
         continue;

      // Stash the line in our buffer
      if ((rc = sshbuf_put(fbuf, lpos, linesize-i)) != 0)
			fatal("%s: buffer error", __func__);
   }

   free(line);
   fclose(fconf);

   return linecnt;
}

// Scan the config buffer, initializing the plugin list
static void init_plugins(int count)
{
   u_int buflen;
   const u_char * pluginpos;
   int i, pluginidx;

   plugins = (Plugin*) xcalloc(sizeof(Plugin), count);

   buflen    = sshbuf_len(fbuf);
   pluginpos = sshbuf_ptr(fbuf);
   pluginidx = 0;
  
   // Pattern of the plugin conf should be:
   // <pluginname> <whitespace>+ [<sequence>] \n 

}

// Open & scan the shared objects for the plugins
static void load_plugin_so()
{

}

int sftp_plugins_init()
{
   int count;

   if ((fbuf = sshbuf_new()) == NULL)
		fatal("%s: sshbuf_new failed", __func__);

   count = load_plugin_conf();

   init_plugins(count);

   load_plugin_so();

   return 0;
}

int sftp_plugins_release()
{
   sshbuf_free(fbuf);

   free(plugins);

   return 0;
}

int get_plugins(Plugin ** plugins, int * cnt)
{
   return 0;
}
