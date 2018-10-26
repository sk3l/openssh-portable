#include "includes.h"

#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "sshbuf.h"
//#include "sftp-plugin.h"

//:#define SSHDIR "/etc/ssh"

#include "sftp-plugin.c"

int main(int argc, char ** argv)
{
   int failures = 0;

   struct sshbuf * fbuf;
   if ((fbuf = sshbuf_new()) == NULL)
		printf("%s: sshbuf_new failed", __func__);

   plugins_cnt = load_plugin_conf("sftp_plugin_conf_test", fbuf);

   init_plugins(fbuf);

   if(load_plugins_so() != 0)
   		printf("%s: load_plugin_so failed", __func__);

   sshbuf_free(fbuf);

   sftp_plugins_release();

/*
   printf("Calling sftp_plugins_init()\n");

   if (sftp_plugins_init() != 0)
   {
      printf("sftp_plugins_init() failed.\n");
      failures = 1;
   }

   printf("Calling sftp_plugins_release()\n");

   if (sftp_plugins_release() != 0)
   {
      printf("sftp_plugins_release() failed.\n");
      failures = 1;
   }
*/
   if (!failures)
   {
      printf("All tests passed\n");
      return 0;
   }

   printf("Some tests failed (as above)\n");
   return failures;
}
