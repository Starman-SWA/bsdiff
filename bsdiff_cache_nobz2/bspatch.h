/*-
 * Copyright 2003-2005 Colin Percival
 * Copyright 2012 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BSPATCH_H
# define BSPATCH_H

#define DEBUG 0

#if DEBUG
#define IF_DEBUG(x) x
#else
#define IF_DEBUG(x)
#endif

# include <stdint.h>
# include <stdio.h>
# include <unistd.h>
# include <err.h>
# include "LRUCache.h"
# include <sys/time.h>

#define min(a,b) ((a) < (b) ? (a) : (b))

//#define NEWFILE_BUFSIZE 1024*1024*64
//#define OLDFILE_MAXSIZE 1024*1024*1024*2
//#define OLDFILE_PAGESIZE 1024*1024*64
#define LRU_SIZE 64
//#define HASH_SIZE (OLDFILE_MAXSIZE/OLDFILE_PAGESIZE+1)
const uint64_t NEWFILE_BUFSIZE = 1024*1024*32;
const uint64_t OLDFILE_MAXSIZE = 1024ULL*1024ULL*1024ULL*2ULL;
const uint64_t OLDFILE_PAGESIZE = 1024*1024*1;
const uint64_t HASH_SIZE = (OLDFILE_MAXSIZE/OLDFILE_PAGESIZE+1);

struct bspatch_stream
{
	void* opaque;
	FILE* f;
	int (*read)(const struct bspatch_stream* stream, void* buffer, int length);
};

struct newfile {
	int fd;
	int64_t newsize;
	int64_t pos;
	uint8_t *buffer;
};

int bspatch(Queue* queue, Hash* hash, OldFile* oldfile, struct newfile *newfile, struct bspatch_stream* stream);
int bspatch_internal(Queue* queue, Hash* hash, OldFile* oldfile, struct newfile *newfile, struct bspatch_stream* stream);

#endif

