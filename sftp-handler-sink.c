#include <fcntl.h>
#include <unistd.h>

#include "includes.h"
#include "log.h"
#include "sftp-handler-sink.h"

int fd_fifo;
static const char * path_fifo = "/tmp/sftp_evts";

int init_handler_sink() 
{
	logit("Initializing SFTP custom handler sink.");

   /* TO DO - for now, hard code the handler sink to a FIFO,
    * but should eventually be possible to register a file, socket, or other
    * handler sinks.
    */

	fd_fifo = open(path_fifo, O_WRONLY | O_NONBLOCK);
	if (fd_fifo < 0)
	{
	   error("Encountered error opening SFTP event FIFO: %d", errno);
	   return 0;
	}

   return 1;
}

int write_to_handler_sink(const void *buf, int count)
{
   return write(fd_fifo, buf, count);
} 
