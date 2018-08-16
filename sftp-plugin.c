#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sshbuf.h"
#include "sftp-plugin.h"

#include "log.h"
#include "pathnames.h"
#include "xmalloc.h"

#define SFTP_PLUGIN_CONF SSHDIR "/sftp_plugin_conf"

static struct sshbuf * fbuf;

static Plugin * plugins;

const char * PLUGIN_SEQUENCE_STR [] = {
   "UNKNOWN",
   "BEFORE",
   "INSTEAD",
   "AFTER"
};

static enum PLUGIN_SEQUENCE plugin_sequence_str_to_enum(const char * sequence, int len)
{
   if (sequence == NULL || (len < 1))
      return PLUGIN_SEQ_UNKNOWN;

   for (int i = PLUGIN_SEQ_BEFORE; i <= PLUGIN_SEQ_AFTER; ++i)
   {
      if (strncmp(sequence, PLUGIN_SEQUENCE_STR[i], len) == 0)
         return i;
   }

   return PLUGIN_SEQ_UNKNOWN;
}

// Given a C string and character (and u_char) predicate function,
// traverse through the string while predicate matches and len isn't reached.
// Return number of characters traversed.
typedef int (*ctype_func)(int c);
static int skip_chartype(char ** pchar, int len, ctype_func cf)
{
   int i = 0;
   while ((i < len) && cf(**pchar))
   {
       ++i;
       (*pchar)++;
   }

   return (len - i);
}

static int skip_u_chartype(u_char ** puchar, int len, ctype_func cf)
{
   int i = 0;
   while ((i < len) && cf(**puchar))
   {
       ++i;
       (*puchar)++;
   }

   return (len - i);
}

// Read & parse the SFTP plugin conf
static int load_plugin_conf()
{
   char * line = NULL, * lpos;
   size_t linesize = 0, linecnt = 0;
   int len, rc;
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

      len = skip_chartype(&lpos, linesize, isspace);

      if ((len == linesize) || (*lpos == '#'))
         continue;

      // Stash the line in our buffer
      if ((rc = sshbuf_put(fbuf, lpos, linesize-len)) != 0)
			fatal("%s: buffer error", __func__);

      // If a partial last line is encountered, add a line feed
      // (simplifies parsing logic to assume every entry is \n terminated)
      if (line[linesize-1] != '\n')
         if ((rc = sshbuf_put_u8(fbuf, '\n')) != 0)
	        fatal("%s: buffer error", __func__);

   }

   free(line);
   fclose(fconf);

   return linecnt;
}

// Scan the config buffer, initializing the plugin list
// Pattern of the plugin conf should be:
// <pluginname>[<whitespace>+][<sequence>]\n
static void init_plugins(int count)
{
   u_int buflen;
   u_char * pluginbeg, * pluginend;
   u_char delim;
   int pos = 0, pluginidx = 0;
   Plugin * pplugin = NULL;

   // Allocate a Plugin per conf line
   plugins = (Plugin*) xcalloc(sizeof(Plugin), count);

   buflen    = sshbuf_len(fbuf);
   pluginbeg = pluginend = sshbuf_mutable_ptr(fbuf);

   // Scan through conf data in sshbuf, initializing Plugins
   while (pos < buflen)
   {
      pplugin = &plugins[pluginidx];

      pos += skip_u_chartype(&pluginend, buflen-pos, isalnum);

      // Verify next u_char is whitespace
      // Otherwise, bail on parsing (invalid char)
      if (!isspace(*pluginend))
         fatal("%s: invalid char in plugin name", __func__);

      // Read plugin lib name
      delim = *pluginend;
      *pluginend = 0;
      pplugin->name_ = xstrdup(pluginbeg);

      if (delim == '\n')
          continue;

      pluginbeg = ++pluginend;
      pos += skip_u_chartype(&pluginend, buflen-pos, isblank);

      if (*pluginend == '\n')
          continue;

      // Read plugin sequence
      pos += skip_u_chartype(&pluginend, buflen-pos, isalnum);

      // Verify next u_char is whitespace
      // Otherwise, bail on parsing (invalid char)
      if (!isspace(*pluginend))
         fatal("%s: invalid char in plugin sequence", __func__);

      pplugin->sequence_ = plugin_sequence_str_to_enum(pluginbeg, pluginend-pluginbeg);

      if (pplugin->sequence_ == PLUGIN_SEQ_UNKNOWN)
         fatal("%s: invalid sequence string in plugin conf", __func__);

      pos += skip_u_chartype(&pluginend, buflen-pos, isblank);
      pluginbeg = ++pluginend;
   }
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
