
#include "sftp.h"
#include "sftp-callback.h"

#include "xmalloc.h"
/*
callback_ptr create_sftp_callback(const char * name, int type, sftp_callback_func cbkfunc)
{
   callback_ptr pcbk = NULL;

   if ((name != NULL) && (type >= 0) && (cbkfunc != NULL))
   {
      pcbk = (callback_ptr) xmalloc(sizeof(struct sftp_callback));
      pcbk->name_     = xstrdup(name);
      pcbk->type_     = type;
      pcbk->callback_ = cbkfunc; 
   } 

   return pcbk;
}

void destroy_sftp_callback(callback_ptr pcbk)
{
   if (pcbk)
   {
      if (pcbk->name_)
      {
         free((void*)pcbk->name_);
         pcbk->name_ = NULL;
      }
      pcbk->type_    =  -1; 
      pcbk->callback_= NULL;
      
      free(pcbk);
   }
}
*/
