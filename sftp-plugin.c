#include "includes.h"

#include <ctype.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sshbuf.h"
#include "sftp-plugin.h"

#include "log.h"
#include "pathnames.h"
#include "xmalloc.h"

#define SFTP_PLUGIN_CONF SSHDIR "/sftp_plugin_conf"


static Plugin * plugins;
static size_t plugins_cnt;

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
static size_t skip_chartype(char ** pchar, size_t len, ctype_func cf)
{
   size_t i = 0;
   while ((i < len) && cf(**pchar))
   {
       ++i;
       (*pchar)++;
   }

   return i;
}

static size_t skip_u_chartype(u_char ** puchar, size_t len, ctype_func cf)
{
   size_t i = 0;
   while ((i < len) && cf(**puchar))
   {
       ++i;
       (*puchar)++;
   }

   return i;
}

// Read & parse the SFTP plugin conf
static int load_plugin_conf(const char * confpath, struct sshbuf * fbuf)
{
   char * line = NULL, * lpos;
   size_t linecnt = 0, linesize = 0, linelen = 0;
   size_t len, rc;
   FILE * fconf;

   printf("Attempting to open SFTP pluing conf '%s'\n", confpath);
   if ((fconf = fopen(confpath, "r")) == NULL)
   {
      perror(SFTP_PLUGIN_CONF);
      exit(1);
   }

   while (getline(&line, &linesize, fconf) != -1)
   {
      lpos = line;
      linelen = strlen(line);

      len = skip_chartype(&lpos, linelen, isspace);

      if ((len == linelen) || (*lpos == '#'))
         continue;

      ++linecnt;
      // Stash the line in our buffer
      if ((rc = sshbuf_put(fbuf, lpos, linelen-len)) != 0)
			fatal("%s: buffer error", __func__);

      // If a partial last line is encountered, add a line feed
      // (simplifies parsing logic to assume every entry is \n terminated)
      if (line[linelen-1] != '\n')
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
static void init_plugins(struct sshbuf * fbuf)
{
   u_int buflen;
   u_char * pluginbeg, * pluginend;
   u_char delim;
   size_t pos = 0;
   //int pos = 0, pluginidx = 0, blankcnt = 0;
   int pluginidx = 0;
   Plugin * pplugin = NULL;

   // Allocate a Plugin per conf line
   plugins = (Plugin*) xcalloc(sizeof(Plugin), plugins_cnt);

   buflen    = sshbuf_len(fbuf);
   pluginbeg = pluginend = sshbuf_mutable_ptr(fbuf);

   // Scan through conf data in sshbuf, initializing Plugins
   while (pos < buflen)
   {
      pplugin = &plugins[pluginidx++];

      pos += skip_u_chartype(&pluginend, buflen-pos, isalnum);

      // Verify next u_char is whitespace
      // Otherwise, bail on parsing (invalid char)
      if (!isspace(*pluginend))
         fatal("%s: invalid char in plugin name", __func__);

      // Read plugin lib name
      delim = *pluginend;
      *pluginend = 0;
      pplugin->name_ = xstrdup(pluginbeg);

      pluginbeg = ++pluginend;
      ++pos;

      if (delim == '\n')
          continue;

      pos += skip_u_chartype(&pluginend, buflen-pos, isblank);

      if (*pluginend == '\n')
      {
          pluginbeg = ++pluginend;
          ++pos;
          continue;
      }

      // Read plugin sequence
      pos += skip_u_chartype(&pluginend, buflen-pos, isalnum);

      // Verify next u_char is whitespace
      // Otherwise, bail on parsing (invalid char)
      if (!isspace(*pluginend))
         fatal("%s: invalid char in plugin sequence", __func__);

      delim = *pluginend;
      *pluginend = 0;

      // Read plugin sequence name
      pplugin->sequence_ = plugin_sequence_str_to_enum(pluginbeg, pluginend-pluginbeg);
      if (pplugin->sequence_ == PLUGIN_SEQ_UNKNOWN)
         fatal("%s: invalid sequence string in plugin conf", __func__);

      pluginbeg = ++pluginend;
      ++pos;

      if (delim == '\n')
          continue;

      // Skip trailing space after sequence
      pos += skip_u_chartype(&pluginend, buflen-pos, isblank);

      pluginbeg = ++pluginend;
      ++pos;
   }
}

// Open & scan the shared objects for the plugins
static int load_plugins_so()
{
   // For each plugin, load the referenced .so
   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];

      pplugin->so_handle_ = dlopen(pplugin->name_, RTLD_NOW);
      if (pplugin->so_handle_ == NULL)
          return -1;

      // For each callback type, look up the matching symbol
      for (enum SFTP_CALLBACK_FUNC cf = CBACK_FUNC_OPEN_FILE;
              cf != CBACK_FUNC_SENTRY;
              ++cf)
      {
          sftp_cbk_func pfunc = dlsym(pplugin->so_handle_, pplugin->name_);
          if (pfunc == NULL)
              continue;     // TO DO - call dlerror()

          if (set_sftp_callback_func(&pplugin->callbacks_, cf, pfunc) != 0)
              return -1;
      }
   }

   return 0;
}

int sftp_plugins_init()
{
   struct sshbuf * fbuf;
   if ((fbuf = sshbuf_new()) == NULL)
		fatal("%s: sshbuf_new failed", __func__);

   plugins_cnt = load_plugin_conf(SFTP_PLUGIN_CONF, fbuf);

   init_plugins(fbuf);

   if(load_plugins_so() != 0)
   		fatal("%s: load_plugin_so failed", __func__);
   
   sshbuf_free(fbuf);

   return 0;
}

int sftp_plugins_release()
{
   // For each plugin, unload the referenced .so
   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      if (dlclose(plugins[i].so_handle_) != 0)
	     fatal("%s: dlclose failed for .so %s", __func__, plugins[i].name_);

      free(plugins[i].name_);
   }

   free(plugins);

   return 0;
}

int get_plugins(Plugin ** plugins, int * cnt)
{
   return 0;
}

/* SFTP plugin invocation methods
 * Call the chain of plugin methods for a given SFTP protocol event,
 * using the specified plugin sequence type (before, instead or after)
 */
int call_open_file_plugins(uint32_t rqstid,
        const char * filename,
        uint32_t access,
        uint32_t flags,
        uint32_t attrs,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_open_dir_plugins(uint32_t rqstid,
        const char * dirpath,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_close_plugins(uint32_t rqstid,
        const char * handle,
        enum PLUGIN_SEQUENCE seq)
{
    return 1;
}

int call_read_plugins(uint32_t rqstid,
        const char * handel,
        uint64_t offset,
        uint32_t length,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_read_dir_plugins(uint32_t rqstid,
        const char * handle,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_write_plugins(uint32_t rqstid,
        const char * handle,
        uint64_t offset,
        const char * data,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_remove_plugins(uint32_t rqstid,
        const char * filename,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_rename_plugins(uint32_t rqstid,
        const char * oldfilename,
        const char * newfilename,
        uint32_t flags,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_mkdir_plugins(uint32_t rqstid,
        const char * path,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_rmdir_plugins(uint32_t rqstid,
        const char * path,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_stat_plugins(uint32_t rqstid,
        const char * path,
        uint32_t flags,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_lstat_plugins(uint32_t rqstid,
        const char * path,
        uint32_t flags,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_fstat_plugins(uint32_t rqstid,
        const char * handle,
        uint32_t flags,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_setstat_plugins(uint32_t rqstid,
        const char * path,
        uint32_t attrs,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_fsetstat_plugins(uint32_t rqstid,
        const char * handle,
        uint32_t attrs,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_link_plugins(uint32_t rqstid,
        const char * newlink,
        const char * curlink,
        int symlink,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_lock_plugins(uint32_t rqstid,
        const char * handle,
        uint64_t offset,
        uint64_t length,
        int lockmask,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_unlock_plugins(uint32_t rqstid,
        const char * handle,
        uint64_t offset,
        uint64_t length,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int call_realpath_plugins(uint32_t rqstid,
        const char * origpath,
        uint8_t ctlbyte,
        const char * path,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}


