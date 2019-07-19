#include "sftp-callback.h"
#include "sftp-plugin.h"

int sftp_cf_open_file(u_int32_t rqstid,
        const char * filename,
        u_int32_t access,
        u_int32_t flags,
        u_int32_t attrs,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_open_dir(u_int32_t rqstid,
        const char * dirpath,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_close(u_int32_t rqstid,
        const char * handle,
        enum PLUGIN_SEQUENCE seq)
{
    return 1;
}

int sftp_cf_read(u_int32_t rqstid,
        const char * handel,
        u_int64_t offset,
        u_int32_t length,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_read_dir(u_int32_t rqstid,
        const char * handle,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_write(u_int32_t rqstid,
        const char * handle,
        u_int64_t offset,
        const char * data,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_remove(u_int32_t rqstid,
        const char * filename,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_rename(u_int32_t rqstid,
        const char * oldfilename,
        const char * newfilename,
        u_int32_t flags,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_mkdir(u_int32_t rqstid,
        const char * path,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_rmdir(u_int32_t rqstid,
        const char * path,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_stat(u_int32_t rqstid,
        const char * path,
        u_int32_t flags,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_lstat(u_int32_t rqstid,
        const char * path,
        u_int32_t flags,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_fstat(u_int32_t rqstid,
        const char * handle,
        u_int32_t flags,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_setstat(u_int32_t rqstid,
        const char * path,
        u_int32_t attrs,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_fsetstat(u_int32_t rqstid,
        const char * handle,
        u_int32_t attrs,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_link(u_int32_t rqstid,
        const char * newlink,
        const char * curlink,
        int symlink,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_lock(u_int32_t rqstid,
        const char * handle,
        u_int64_t offset,
        u_int64_t length,
        int lockmask,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_unlock(u_int32_t rqstid,
        const char * handle,
        u_int64_t offset,
        u_int64_t length,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_realpath(u_int32_t rqstid,
        const char * origpath,
        u_int8_t ctlbyte,
        const char * path,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

