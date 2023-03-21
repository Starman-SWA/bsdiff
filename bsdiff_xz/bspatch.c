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

int bspatch(const uint8_t* old, int64_t oldsize, uint8_t* new, int64_t newsize, struct bspatch_stream* stream)
{
	uint8_t buf[8];
	int64_t oldpos,newpos;
	int64_t ctrl[3];
	int64_t i;

	oldpos=0;newpos=0;
	while(newpos<newsize) {
		/* Read control data */
		for(i=0;i<=2;i++) {
			if (stream->read(stream, buf, 8))
				return -1;
			ctrl[i]=offtin(buf);
		};

		/* Sanity-check */
		if (ctrl[0]<0 || ctrl[0]>INT_MAX ||
			ctrl[1]<0 || ctrl[1]>INT_MAX ||
			newpos+ctrl[0]>newsize)
			return -1;

		/* Read diff string */
		if (stream->read(stream, new + newpos, ctrl[0]))
			return -1;

		/* Add old data to diff string */
		for(i=0;i<ctrl[0];i++)
			if((oldpos+i>=0) && (oldpos+i<oldsize))
				new[newpos+i]+=old[oldpos+i];

		/* Adjust pointers */
		newpos+=ctrl[0];
		oldpos+=ctrl[0];

		/* Sanity-check */
		if(newpos+ctrl[1]>newsize)
			return -1;

		/* Read extra string */
		if (stream->read(stream, new + newpos, ctrl[1]))
			return -1;

		/* Adjust pointers */
		newpos+=ctrl[1];
		oldpos+=ctrl[2];
	};

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
#include <lzma.h>
#include <stdbool.h>
#include <errno.h>

static bool
init_decoder(lzma_stream *strm)
{
    // Initialize a .xz decoder. The decoder supports a memory usage limit
    // and a set of flags.
    //
    // The memory usage of the decompressor depends on the settings used
    // to compress a .xz file. It can vary from less than a megabyte to
    // a few gigabytes, but in practice (at least for now) it rarely
    // exceeds 65 MiB because that's how much memory is required to
    // decompress files created with "xz -9". Settings requiring more
    // memory take extra effort to use and don't (at least for now)
    // provide significantly better compression in most cases.
    //
    // Memory usage limit is useful if it is important that the
    // decompressor won't consume gigabytes of memory. The need
    // for limiting depends on the application. In this example,
    // no memory usage limiting is used. This is done by setting
    // the limit to UINT64_MAX.
    //
    // The .xz format allows concatenating compressed files as is:
    //
    //     echo foo | xz > foobar.xz
    //     echo bar | xz >> foobar.xz
    //
    // When decompressing normal standalone .xz files, LZMA_CONCATENATED
    // should always be used to support decompression of concatenated
    // .xz files. If LZMA_CONCATENATED isn't used, the decoder will stop
    // after the first .xz stream. This can be useful when .xz data has
    // been embedded inside another file format.
    //
    // Flags other than LZMA_CONCATENATED are supported too, and can
    // be combined with bitwise-or. See lzma/container.h
    // (src/liblzma/api/lzma/container.h in the source package or e.g.
    // /usr/include/lzma/container.h depending on the install prefix)
    // for details.
    lzma_ret ret = lzma_stream_decoder(
            strm, UINT64_MAX, LZMA_CONCATENATED);

    // Return successfully if the initialization went fine.
    if (ret == LZMA_OK)
        return true;

    // Something went wrong. The possible errors are documented in
    // lzma/container.h (src/liblzma/api/lzma/container.h in the source
    // package or e.g. /usr/include/lzma/container.h depending on the
    // install prefix).
    //
    // Note that LZMA_MEMLIMIT_ERROR is never possible here. If you
    // specify a very tiny limit, the error will be delayed until
    // the first headers have been parsed by a call to lzma_code().
    const char *msg;
    switch (ret) {
        case LZMA_MEM_ERROR:
            msg = "Memory allocation failed";
            break;

        case LZMA_OPTIONS_ERROR:
            msg = "Unsupported decompressor flags";
            break;

        default:
            // This is most likely LZMA_PROG_ERROR indicating a bug in
            // this program or in liblzma. It is inconvenient to have a
            // separate error message for errors that should be impossible
            // to occur, but knowing the error code is important for
            // debugging. That's why it is good to print the error code
            // at least when there is no good error message to show.
            msg = "Unknown error, possibly a bug";
            break;
    }

    fprintf(stderr, "Error initializing the decoder: %s (error code %u)\n",
            msg, ret);
    return false;
}

uint8_t inbuf[BUFSIZ];

static int xz_read(const struct bspatch_stream* stream, void* buffer, int length) {
    lzma_action action = LZMA_RUN;

    stream->strm->next_out = buffer;
    stream->strm->avail_out = length;

    while (stream->strm->avail_out > 0) {
        if (stream->strm->avail_in == 0) {
            stream->strm->next_in = inbuf;
            stream->strm->avail_in = fread(inbuf, 1, sizeof(inbuf),
                                           stream->fin);

            if (ferror(stream->fin)) {
                fprintf(stderr, "Patch file: Read error: %s\n",
                        strerror(errno));
                return -1;
            }
        }

        lzma_ret ret = lzma_code(stream->strm, action);

        if (ret != LZMA_OK) {
            if (ret == LZMA_STREAM_END)
                return 0;

            const char *msg;
            switch (ret) {
                case LZMA_MEM_ERROR:
                    msg = "Memory allocation failed";
                    break;

                case LZMA_FORMAT_ERROR:
                    msg = "The input is not in the .xz format";
                    break;

                case LZMA_OPTIONS_ERROR:
                    msg = "Unsupported compression options";
                    break;

                case LZMA_DATA_ERROR:
                    msg = "Compressed file is corrupt";
                    break;

                case LZMA_BUF_ERROR:
                    msg = "Compressed file is truncated or "
                          "otherwise corrupt";
                    break;

                default:
                    msg = "Unknown error, possibly a bug";
                    break;
            }

            fprintf(stderr, "Patch file: Decoder error: "
                            "%s (error code %u)\n",
                    msg, ret);
            return -1;
        }
    }
    return 0;
}

int main(int argc,char * argv[])
{
	FILE * f;
	int fd;
	uint8_t header[24];
	uint8_t *old, *new;
	int64_t oldsize, newsize;
	struct bspatch_stream stream;
	struct stat sb;
    lzma_stream strm = LZMA_STREAM_INIT;

    stream.strm = &strm;
    stream.strm->next_in = inbuf;
    stream.strm->avail_in = 0;

    if (!init_decoder(&strm)) {
        err(1, "Failed to init xz decoder");
    }

	if(argc!=4) errx(1,"usage: %s oldfile newfile patchfile\n",argv[0]);

	/* Open patch file */
	if ((f = fopen(argv[3], "rb")) == NULL)
		err(1, "fopen(%s)", argv[3]);

	/* Read header */
	if (fread(header, 1, 24, f) != 24) {
		if (feof(f))
			errx(1, "Corrupt patch\n");
		err(1, "fread(%s)", argv[3]);
	}

	/* Check for appropriate magic */
	if (memcmp(header, "ENDSLEY/BSDIFF43", 16) != 0)
		errx(1, "Corrupt patch\n");

	/* Read lengths from header */
	newsize=offtin(header+16);
	if(newsize<0)
		errx(1,"Corrupt patch\n");

	/* Close patch file and re-open it via libbzip2 at the right places */
	if(((fd=open(argv[1],O_RDONLY,0))<0) ||
		((oldsize=lseek(fd,0,SEEK_END))==-1) ||
		((old=malloc(oldsize+1))==NULL) ||
		(lseek(fd,0,SEEK_SET)!=0) ||
		(read(fd,old,oldsize)!=oldsize) ||
		(fstat(fd, &sb)) ||
		(close(fd)==-1)) err(1,"%s",argv[1]);
	if((new=malloc(newsize+1))==NULL) err(1,NULL);

	stream.read = xz_read;
    stream.fin = f;

	if (bspatch(old, oldsize, new, newsize, &stream))
		errx(1, "bspatch");

    lzma_ret ret = lzma_code(&strm, LZMA_FINISH);
    lzma_end(&strm);

	/* Clean up the bzip2 reads */
	fclose(f);

	/* Write the new file */
	if(((fd=open(argv[2],O_CREAT|O_TRUNC|O_WRONLY,sb.st_mode))<0) ||
		(write(fd,new,newsize)!=newsize) || (close(fd)==-1))
		err(1,"%s",argv[2]);

	free(new);
	free(old);

	return 0;
}

#endif
