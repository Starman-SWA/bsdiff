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

#include <limits.h>
#include "bspatch.h"
#include "LRUCache.h"

static int64_t offtin(uint8_t *buf)
{
	int64_t y;

	y=buf[7]&0x7F;
	y=y*256;y+=buf[6];
	y=y*256;y+=buf[5];
	y=y*256;y+=buf[4];
	y=y*256;y+=buf[3];
	y=y*256;y+=buf[2];
	y=y*256;y+=buf[1];
	y=y*256;y+=buf[0];

	if(buf[7]&0x80) y=-y;

	return y;
}

void flush_buffer(struct newfile* newfile) {
	if (write(newfile->fd, newfile->buffer, newfile->pos) != newfile->pos) {
		errx(1, "cannot write to newfile\n");
	}
	newfile->pos = 0;
}

int bspatch( Queue* queue, Hash* hash, OldFile* oldfile, struct newfile *newfile, struct bspatch_stream* stream)
{
	IF_DEBUG(printf("LRU_SIZE: %d\n", LRU_SIZE);)
	IF_DEBUG(printf("NEWFILE_BUFSIZE: %lu\n", NEWFILE_BUFSIZE);)
	IF_DEBUG(printf("OLDFILE_MAXSIZE: %lu\n", OLDFILE_MAXSIZE);)
	IF_DEBUG(printf("OLDFILE_PAGESIZE: %lu\n", OLDFILE_PAGESIZE);)
	IF_DEBUG(printf("HASH_SIZE: %lu\n", HASH_SIZE);)
	uint8_t buf[8];
	int64_t oldpos,newpos;
	int64_t ctrl[3];
	int64_t i;
	uint8_t** old = (uint8_t **)malloc(sizeof(uint8_t*));
	struct timeval start, end;
	long timeuse;

	IF_DEBUG(printf("bspatch starting\n");)
	oldpos=0;newpos=0;
	while(newpos<(newfile->newsize)) {
		IF_DEBUG(gettimeofday(&start, NULL);)
		/* Read control data */
		for(i=0;i<=2;i++) {
			if (stream->read(stream, buf, 8))
				return -1;
			ctrl[i]=offtin(buf);
		};

		/* Sanity-check */
		if (ctrl[0]<0 || ctrl[0]>INT_MAX ||
			ctrl[1]<0 || ctrl[1]>INT_MAX ||
			newpos+ctrl[0]>(newfile->newsize))
			return -1;
		IF_DEBUG(gettimeofday(&end, NULL);)
		IF_DEBUG(timeuse =1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;)
		IF_DEBUG(printf("time1=%ld\n",timeuse);)
		IF_DEBUG(printf("current oldpos=%ld, newpos=%ld\n", oldpos, newpos);)
		IF_DEBUG(printf("current control block: (%ld, %ld, %ld)\n", ctrl[0], ctrl[1], ctrl[2]);)
		IF_DEBUG(printf("diff string: reading diff string of %ld bytes to new file\n", ctrl[0]);)

		int rest = ctrl[0];
		IF_DEBUG(gettimeofday(&start, NULL);)
		while (rest > 0) {
			int take = min(NEWFILE_BUFSIZE-newfile->pos, rest);
			rest -= take;
			/* Read diff string */
			if (stream->read(stream, &newfile->buffer[newfile->pos], take))
				return -1;

			int old_take = 0, old_rest = take, offset = oldpos % OLDFILE_PAGESIZE, pageNo = oldpos / OLDFILE_PAGESIZE;
			// pageNo*PAGESIZE + offset 为当前的读取位置
			i = 0; // newfile的i
			while (old_rest > 0) {
				old_take = min(old_rest, min(OLDFILE_PAGESIZE, OLDFILE_PAGESIZE-offset));
				old_rest -= old_take;
				/* Add old data to diff string */
				// for(i=0;i<take;i++)
				// 	if((oldpos+i>=0) && (oldpos+i<oldsize)) {
				// 		newfile->buffer[newfile->pos+i]+=old[oldpos+i];
				// 	}
				ReferencePage(queue, hash, oldfile, pageNo, old);
				for (int oi = 0; oi < old_take; ++oi) {
					newfile->buffer[newfile->pos + i++] += (*old)[offset + oi];
				}
				offset = 0;
				pageNo++;
			}

			
			newfile->pos += take;
			if (newfile->pos == NEWFILE_BUFSIZE) {
				flush_buffer(newfile);
			}
			oldpos += take;
		}
		IF_DEBUG(gettimeofday(&end, NULL);)
		IF_DEBUG(timeuse =1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;)
		IF_DEBUG(printf("time2=%ld\n",timeuse);)
		IF_DEBUG(gettimeofday(&start, NULL);)

		IF_DEBUG(printf("diff string: adding %ld bytes from old file to new file\n", ctrl[0]);)

		/* Adjust pointers */
		newpos+=ctrl[0];
		//oldpos+=ctrl[0];

		/* Sanity-check */
		if(newpos+ctrl[1]>(newfile->newsize))
			return -1;

		IF_DEBUG(printf("extra string: reading %ld bytes from delta file to new file\n", ctrl[1]);)

		rest = ctrl[1];
		while (rest > 0) {
			int take = min(NEWFILE_BUFSIZE-newfile->pos, rest);
			rest -= take;
			/* Read extra string */
			if (stream->read(stream, &newfile->buffer[newfile->pos], take))
				return -1;

			newfile->pos += take;
			if (newfile->pos == NEWFILE_BUFSIZE) {
				flush_buffer(newfile);
			}
		}

		IF_DEBUG(gettimeofday(&end, NULL);)
		IF_DEBUG(timeuse = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;)
		IF_DEBUG(printf("time3=%ld\n",timeuse);)

		/* Adjust pointers */
		newpos+=ctrl[1];
		oldpos+=ctrl[2];
	};

	if (newfile->pos != 0) {
		if (write(newfile->fd, newfile->buffer, newfile->pos) != newfile->pos) {
			errx(1, "cannot write to newfile\n");
		}
	}

	free(old);
	return 0;
}

#if defined(BSPATCH_EXECUTABLE)

#include <bzlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

static int bz2_read(const struct bspatch_stream* stream, void* buffer, int length)
{
	int n;
	int bz2err;
	BZFILE* bz2;

	bz2 = (BZFILE*)stream->opaque;
	n = BZ2_bzRead(&bz2err, bz2, buffer, length);
	if (n != length)
		return -1;

	return 0;
}

int main(int argc,char * argv[])
{
	IF_DEBUG(printf("main func:\n");)
	FILE * f;
	int fd, new_fd;
	int bz2err;
	uint8_t header[24];
	uint8_t *old, *new;
	int64_t oldsize, newsize;
	BZFILE* bz2;
	struct bspatch_stream stream;
	struct stat sb;
	struct newfile newfile;
	OldFile* oldfile;
	Queue* queue;
	Hash* hash;

	if(argc!=4) errx(1,"usage: %s oldfile newfile patchfile\n",argv[0]);

	IF_DEBUG(printf("opening patch file\n");)

	/* Open patch file */
	if ((f = fopen(argv[3], "r")) == NULL)
		err(1, "fopen(%s)", argv[3]);

	IF_DEBUG(printf("reading header\n");)

	/* Read header */
	if (fread(header, 1, 24, f) != 24) {
		if (feof(f))
			errx(1, "Corrupt patch\n");
		err(1, "fread(%s)", argv[3]);
	}

	IF_DEBUG(printf("checking for appropriate magic\n");)

	/* Check for appropriate magic */
	if (memcmp(header, "ENDSLEY/BSDIFF43", 16) != 0)
		errx(1, "Corrupt patch\n");

	IF_DEBUG(printf("reading lengths from header\n");)

	/* Read lengths from header */
	newsize=offtin(header+16);
	if(newsize<0)
		errx(1,"Corrupt patch\n");
	newfile.newsize = newsize;

	IF_DEBUG(printf("open old file, get size, malloc, read old file to buffer\n");)

	/* Close patch file and re-open it via libbzip2 at the right places */
	// if(((fd=open(argv[1],O_RDONLY,0))<0) ||
	// 	((oldsize=lseek(fd,0,SEEK_END))==-1) ||
	// 	((old=malloc(oldsize+1))==NULL) ||
	// 	(lseek(fd,0,SEEK_SET)!=0) ||
	// 	(read(fd,old,oldsize)!=oldsize) ||
	// 	(fstat(fd, &sb)) ||
	// 	(close(fd)==-1)) err(1,"%s",argv[1]);
	fd = open(argv[1],O_RDONLY,0);
	if (fd < 0) {
		err(1,"%s",argv[1]);
	}
	oldsize = lseek(fd,0,SEEK_END);
	oldfile = createOldFile(fd, oldsize, OLDFILE_PAGESIZE);
	queue = createQueue(LRU_SIZE);
	hash = createHash(HASH_SIZE);

	IF_DEBUG(printf("malloc buffer for new file\n");)

	//if((new=malloc(newsize+1))==NULL) err(1,NULL);

	IF_DEBUG(printf("open patch file by bz2\n");)

	if (NULL == (bz2 = BZ2_bzReadOpen(&bz2err, f, 0, 0, NULL, 0)))
		errx(1, "BZ2_bzReadOpen, bz2err=%d", bz2err);

	if ((new_fd = open(argv[2],O_CREAT|O_TRUNC|O_WRONLY/*,sb.st_mode*/,0)) < 0) {
		err(1,"%s",argv[2]);
	}
	newfile.fd = new_fd;
	newfile.pos = 0;
	if ((newfile.buffer = malloc(NEWFILE_BUFSIZE+1)) == NULL) {
		err(1,"%s",argv[2]);
	}

	stream.read = bz2_read;
	stream.opaque = bz2;
	if (bspatch(queue, hash, oldfile, &newfile, &stream))
		errx(1, "bspatch");

	/* Clean up the bzip2 reads */
	BZ2_bzReadClose(&bz2err, bz2);
	fclose(f);

	if (close(new_fd) == -1) {
		err(1,"%s",argv[2]);
	}

	IF_DEBUG(printf("write buffer to new file\n");)

	/* Write the new file */
	// if(((fd=open(argv[2],O_CREAT|O_TRUNC|O_WRONLY,sb.st_mode))<0) ||
	// 	(write(fd,new,newsize)!=newsize) || (close(fd)==-1))
	// 	err(1,"%s",argv[2]);

	free(newfile.buffer);
	free(old);
	close(fd);

	return 0;
}

#endif
