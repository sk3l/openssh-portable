#include "sftp-callback.h"
#include "sftp-plugin.h"

int sftp_cf_open_file(uint32_t rqstid,
        const char * filename,
        uint32_t access,
        uint32_t flags,
        uint32_t attrs,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_open_dir(uint32_t rqstid,
        const char * dirpath,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_close(uint32_t rqstid,
        const char * handle,
        enum PLUGIN_SEQUENCE seq)
{
    return 1;
}

int sftp_cf_read(uint32_t rqstid,
        const char * handel,
        uint64_t offset,
        uint32_t length,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_read_dir(uint32_t rqstid,
        const char * handle,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_write(uint32_t rqstid,
        const char * handle,
        uint64_t offset,
        const char * data,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_remove(uint32_t rqstid,
        const char * filename,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_rename(uint32_t rqstid,
        const char * oldfilename,
        const char * newfilename,
        uint32_t flags,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_mkdir(uint32_t rqstid,
        const char * path,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_rmdir(uint32_t rqstid,
        const char * path,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_stat(uint32_t rqstid,
        const char * path,
        uint32_t flags,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_lstat(uint32_t rqstid,
        const char * path,
        uint32_t flags,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_fstat(uint32_t rqstid,
        const char * handle,
        uint32_t flags,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_setstat(uint32_t rqstid,
        const char * path,
        uint32_t attrs,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_fsetstat(uint32_t rqstid,
        const char * handle,
        uint32_t attrs,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_link(uint32_t rqstid,
        const char * newlink,
        const char * curlink,
        int symlink,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_lock(uint32_t rqstid,
        const char * handle,
        uint64_t offset,
        uint64_t length,
        int lockmask,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_unlock(uint32_t rqstid,
        const char * handle,
        uint64_t offset,
        uint64_t length,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

int sftp_cf_realpath(uint32_t rqstid,
        const char * origpath,
        uint8_t ctlbyte,
        const char * path,
        enum PLUGIN_SEQUENCE seq)
{
    return 0;
}

