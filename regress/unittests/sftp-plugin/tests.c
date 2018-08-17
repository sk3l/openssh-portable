#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "sftp-plugin.h"

//:#define SSHDIR "/etc/ssh"

int main(int argc, char ** argv)
{
   int failures = 0;
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

   if (!failures)
   {
      printf("All tests passed\n");
      return 0;
   }

   printf("Some tests failed (as above)\n");
   return failures;
}
