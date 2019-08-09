#define _GNU_SOURCE
#include <ctype.h>
#include <dlfcn.h>

#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

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

const char* plugin_sequence_enum_to_str(enum PLUGIN_SEQUENCE seq) {
    if (seq > PLUGIN_SEQ_AFTER)
        seq = PLUGIN_SEQ_UNKNOWN;

    return PLUGIN_SEQUENCE_STR[seq];
}

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

typedef int (*ctype_func)(int c);

static int isvariablechar(int c)
{
    return (c == '_') || isalnum(c);
}

// Given a C string and character (and u_char) predicate function,
// traverse through the string while predicate matches and len isn't reached.
// Return number of characters traversed.

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

      // Enforce limit on number of configured plugins
      if (++linecnt > MAX_PLUGIN_CNT) {
         perror(SFTP_PLUGIN_CONF);
         exit(1);
      }

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

      pos += skip_u_chartype(&pluginend, buflen-pos, isvariablechar);

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
      pos += skip_u_chartype(&pluginend, buflen-pos, isvariablechar);

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

char * make_soname(const char * libname)
{
    char * soname = NULL;
    const char * prefix = "lib";
    const char * suffix = ".so";

    size_t solen = 0;
    size_t llen  = strlen(libname);
    size_t plen  = strlen(prefix);
    size_t slen  = strlen(suffix);

    if (llen < 1)
        return NULL;

    // Allocate space for soname prefix and suffix
    // i.e. "lib" + libname + ".so" + '\0'
    solen = plen  + llen + slen + 1;

    soname = xmalloc(solen);
    if (soname != NULL)
    {
        strcpy(soname, prefix);
        strcpy(soname+plen, libname);
        strcpy(soname+plen+llen, suffix);
        soname[solen-1] = 0;
    }
    return soname;
}

// Open & scan the shared objects for the plugins
static int load_plugins_so()
{
   // For each plugin, load the referenced .so
   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      const char * symstr = NULL;
      char * dlerrstr = NULL;

      char * soname = make_soname(pplugin->name_);
      if (soname == NULL)
          fatal("%s: error while converting lib name '%s' to soname",
                  __func__, pplugin->name_);

      dlerror();
      pplugin->so_handle_ = dlopen(soname, RTLD_NOW);
      dlerrstr  = dlerror();
      if (dlerrstr != NULL)
          return -1;

      free(soname);

      // For each callback type, look up the matching symbol
      for (enum SFTP_CALLBACK_FUNC cf = CBACK_FUNC_OPEN_FILE;
              cf != CBACK_FUNC_SENTRY;
              ++cf)
      {
          sftp_cbk_func pfunc;

          symstr = get_sftp_callback_sym(cf);
          if (symstr == NULL)
              return -1;

          dlerror();
          *(void **)(&pfunc)  = dlsym(pplugin->so_handle_, symstr);
          dlerrstr = dlerror();
          if (dlerrstr != NULL)
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

int get_plugins(Plugin ** pplugins, size_t * cnt)
{
   pplugins = &plugins;
   cnt = &plugins_cnt;
   return 0;
}

/* SFTP plugin invocation methods
 * Call the chain of plugin methods for a given SFTP protocol event,
 * using the specified plugin sequence type (before, instead or after)
 */
int call_open_file_plugins(u_int32_t rqstid,
        const char * filename,
        u_int32_t access,
        u_int32_t flags,
        cbk_attribs_ptr attrs,
        int * fd,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats || !fd)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_open_file != NULL)
      {
         int rc = pplugin->callbacks_.cf_open_file(rqstid, filename, access, flags, attrs, fd);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_open_dir_plugins(u_int32_t rqstid,
        const char * dirpath,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_open_dir != NULL)
      {
         int rc = pplugin->callbacks_.cf_open_dir(rqstid, dirpath);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
       }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_close_plugins(u_int32_t rqstid,
        const char * handle,
        int fd,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_close != NULL)
      {
         int rc = pplugin->callbacks_.cf_close(rqstid, handle, fd);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
       }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_read_plugins(u_int32_t rqstid,
        const char * handle,
        u_int64_t offset,
        u_int32_t length,
        u_char * data,
        int * dlen,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats || !data || !dlen)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_read != NULL)
      {
         int rc = pplugin->callbacks_.cf_read(rqstid, handle, offset, length, data, dlen);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_read_dir_plugins(u_int32_t rqstid,
        const char * handle,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_read_dir != NULL)
      {
         int rc = pplugin->callbacks_.cf_read_dir(rqstid, handle);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
     }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_write_plugins(u_int32_t rqstid,
        const char * handle,
        u_int64_t offset,
        const char * data,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_write != NULL)
      {
         int rc = pplugin->callbacks_.cf_write(rqstid, handle, offset, data);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
     }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_remove_plugins(u_int32_t rqstid,
        const char * filename,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_remove != NULL)
      {
         int rc = pplugin->callbacks_.cf_remove(rqstid, filename);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_rename_plugins(u_int32_t rqstid,
        const char * oldfilename,
        const char * newfilename,
        u_int32_t flags,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_rename != NULL)
      {
         int rc = pplugin->callbacks_.cf_rename(rqstid, oldfilename, newfilename, flags);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_mkdir_plugins(u_int32_t rqstid,
        const char * path,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_mkdir != NULL)
      {
         int rc = pplugin->callbacks_.cf_mkdir(rqstid, path);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_rmdir_plugins(u_int32_t rqstid,
        const char * path,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_rmdir != NULL)
      {
         int rc = pplugin->callbacks_.cf_rmdir(rqstid, path);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_stat_plugins(u_int32_t rqstid,
        const char * path,
        u_int32_t flags,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_stat != NULL)
      {
         int rc = pplugin->callbacks_.cf_stat(rqstid, path, flags);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_lstat_plugins(u_int32_t rqstid,
        const char * path,
        u_int32_t flags,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_lstat != NULL)
      {
         int rc = pplugin->callbacks_.cf_lstat(rqstid, path, flags);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_fstat_plugins(u_int32_t rqstid,
        const char * handle,
        u_int32_t flags,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_fstat != NULL)
      {
         int rc = pplugin->callbacks_.cf_fstat(rqstid, handle, flags);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_setstat_plugins(u_int32_t rqstid,
        const char * path,
        cbk_attribs_ptr attrs,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_setstat != NULL)
      {
         int rc = pplugin->callbacks_.cf_setstat(rqstid, path, attrs);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_fsetstat_plugins(u_int32_t rqstid,
        const char * handle,
        cbk_attribs_ptr attrs,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_fsetstat != NULL)
      {
         int rc = pplugin->callbacks_.cf_fsetstat(rqstid, handle, attrs);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_read_link_plugins(u_int32_t rqstid,
        const char * path,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_read_link != NULL)
      {
         int rc = pplugin->callbacks_.cf_read_link(rqstid, path);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_link_plugins(u_int32_t rqstid,
        const char * newlink,
        const char * curlink,
        int symlink,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_link != NULL)
      {
         int rc = pplugin->callbacks_.cf_link(rqstid, newlink, curlink, symlink);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_lock_plugins(u_int32_t rqstid,
        const char * handle,
        u_int64_t offset,
        u_int64_t length,
        int lockmask,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_lock != NULL)
      {
         int rc = pplugin->callbacks_.cf_lock(rqstid, handle, offset, length, lockmask);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_unlock_plugins(u_int32_t rqstid,
        const char * handle,
        u_int64_t offset,
        u_int64_t length,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_unlock != NULL)
      {
         int rc = pplugin->callbacks_.cf_unlock(rqstid, handle, offset, length);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

int call_realpath_plugins(u_int32_t rqstid,
        const char * origpath,
        u_int8_t ctlbyte,
        const char * path,
        enum PLUGIN_SEQUENCE seq,
        callback_stats * cbkstats)
{
   if (!cbkstats)
      return PLUGIN_CBK_FAILURE;

   cbkstats->invocation_cnt_ = 0;
   cbkstats->failure_cnt_ = 0;

   for (size_t i = 0; i < plugins_cnt; ++i)
   {
      Plugin * pplugin = &plugins[i];
      if (pplugin->sequence_ == seq && pplugin->callbacks_.cf_realpath != NULL)
      {
         int rc = pplugin->callbacks_.cf_realpath(rqstid, origpath, ctlbyte, path);
         if (rc != PLUGIN_CBK_SUCCESS)
            cbkstats->failure_cnt_++;
         cbkstats->invocation_cnt_++;
      }
   }
   return PLUGIN_CBK_SUCCESS;
}

