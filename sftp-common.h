/* $OpenBSD: sftp-common.h,v 1.12 2015/01/14 13:54:13 djm Exp $ */

/*
 * Copyright (c) 2001 Markus Friedl.  All rights reserved.
 * Copyright (c) 2001 Damien Miller.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SFTP_COMMON_H
#define SFTP_COMMON_H

#include <sys/types.h>
#include <sys/stat.h>
/* Maximum packet that we are willing to send/accept */
#define SFTP_MAX_MSG_LENGTH	(256 * 1024)

struct sshbuf;

/* File attributes */
struct Attrib {
	u_int32_t	flags;
	u_int64_t	size;
	u_int32_t	uid;
	u_int32_t	gid;
	u_int32_t	perm;
	u_int32_t	atime;
	u_int32_t	mtime;
};
typedef struct Attrib Attrib;

/* portable attributes, etc. */

struct Stat {
	char *name;
	char *long_name;
	Attrib attrib;
};
typedef struct Stat Stat;
void free_stat(Stat *);

struct StatList {
    Stat * stats;
    size_t count;
};
typedef struct StatList StatList;
void free_stat_list(StatList *);

void	 attrib_clear(Attrib *);
void	 stat_to_attrib(const struct stat *, Attrib *);
void	 attrib_to_stat(const Attrib *, struct stat *);
int	 decode_attrib(struct sshbuf *, Attrib *);
int	 encode_attrib(struct sshbuf *, const Attrib *);
char	*ls_file(const char *, const struct stat *, int, int);

const char *fx2txt(int);

#endif
