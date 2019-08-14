#include "includes.h"

#include "sftp.h"
#include "sftp-callback.h"

#include "xmalloc.h"

static const char * SFTP_CALLBACK_SYM [] = {
    "sftp_cf_open_file",
    "sftp_cf_open_dir",
    "sftp_cf_close",
    "sftp_cf_read",
    "sftp_cf_read_dir",
    "sftp_cf_write",
    "sftp_cf_remove",
    "sftp_cf_rename",
    "sftp_cf_mkdir",
    "sftp_cf_rmdir",
    "sftp_cf_stat",
    "sftp_cf_lstat",
    "sftp_cf_fstat",
    "sftp_cf_setstat",
    "sftp_cf_fsetstat",
    "sftp_cf_read_link",
    "sftp_cf_symlink",
    "sftp_cf_lock",
    "sftp_cf_unlock",
    "sftp_cf_realpath",
    "sftp_cf_status",
    "sftp_cf_handle",
    "sftp_cf_data",
    "sftp_cf_name",
    "sftp_cf_attrs"
};

const char * get_sftp_callback_sym(enum SFTP_CALLBACK_FUNC scf)
{
    return SFTP_CALLBACK_SYM[scf];
}

// Assign function ptr to the appropriate SFTP callback
int set_sftp_callback_func(callbacks_ptr cp, enum SFTP_CALLBACK_FUNC scf, sftp_cbk_func f)
{
    if (f == 0)
        return -1;

    switch (scf)
    {
        case CBACK_FUNC_OPEN_FILE:
            cp->cf_open_file = (sftp_cbk_open_file) f;
        break;
        case CBACK_FUNC_OPEN_DIR:
            cp->cf_open_dir = (sftp_cbk_open_dir) f;
        break;
        case CBACK_FUNC_CLOSE:
            cp->cf_close = (sftp_cbk_close) f;
        break;
        case CBACK_FUNC_READ:
            cp->cf_read = (sftp_cbk_read) f;
        break;
        case CBACK_FUNC_READ_DIR:
            cp->cf_read_dir = (sftp_cbk_read_dir) f;
        break;
        case CBACK_FUNC_WRITE:
            cp->cf_write = (sftp_cbk_write) f;
        break;
        case CBACK_FUNC_REMOVE:
            cp->cf_remove = (sftp_cbk_remove) f;
        break;
        case CBACK_FUNC_RENAME:
            cp->cf_rename = (sftp_cbk_rename) f;
        break;
        case CBACK_FUNC_MKDIR:
            cp->cf_mkdir = (sftp_cbk_mkdir) f;
        break;
        case CBACK_FUNC_RMDIR:
            cp->cf_rmdir = (sftp_cbk_rmdir) f;
        break;
        case CBACK_FUNC_STAT:
            cp->cf_stat = (sftp_cbk_stat) f;
        break;
        case CBACK_FUNC_LSTAT:
            cp->cf_lstat = (sftp_cbk_lstat) f;
        break;
        case CBACK_FUNC_FSTAT:
            cp->cf_fstat = (sftp_cbk_fstat) f;
        break;
        case CBACK_FUNC_SETSTAT:
            cp->cf_setstat = (sftp_cbk_setstat) f;
        break;
        case CBACK_FUNC_FSETSTAT:
            cp->cf_fsetstat = (sftp_cbk_fsetstat) f;
        break;
        case CBACK_FUNC_READ_LINK:
            cp->cf_read_link = (sftp_cbk_read_link) f;
        break;
         case CBACK_FUNC_SYMLINK:
            cp->cf_symlink = (sftp_cbk_symlink) f;
        break;
        case CBACK_FUNC_LOCK:
            cp->cf_lock = (sftp_cbk_lock) f;
        break;
        case CBACK_FUNC_UNLOCK:
            cp->cf_unlock = (sftp_cbk_unlock) f;
        break;
        case CBACK_FUNC_REALPATH:
            cp->cf_realpath = (sftp_cbk_realpath) f;
        break;
        case CBACK_FUNC_STATUS:
            cp->cf_status = (sftp_cbk_status) f;
        break;
        case CBACK_FUNC_HANDLE:
            cp->cf_handle = (sftp_cbk_handle) f;
        break;
        case CBACK_FUNC_DATA:
            cp->cf_data = (sftp_cbk_data) f;
        break;
        case CBACK_FUNC_NAME:
            cp->cf_name = (sftp_cbk_name) f;
        break;
        case CBACK_FUNC_ATTRS:
            cp->cf_attrs = (sftp_cbk_attrs) f;
        break;
        default:
            return -1;
        break;
    }

    return 0;
}

