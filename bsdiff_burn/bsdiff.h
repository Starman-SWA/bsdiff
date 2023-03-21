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

#ifndef BSDIFF_H
# define BSDIFF_H

# include <stddef.h>
# include <stdint.h>

struct bsdiff_stream
{
	void* opaque;

	void* (*malloc)(size_t size);
	void (*free)(void* ptr);
	int (*write)(struct bsdiff_stream* stream, const void* buffer, int size);
};

enum OP_TYPE {
    COPY, ADD
};

struct op {
    enum OP_TYPE optype; // COPY or ADD
    int64_t from; // for COPY
    int64_t to; // for COPY and ADD
    int64_t len; // for COPY and ADD
    uint8_t* diff; // for COPY diff string
    const uint8_t *old; // for COPY old file
    uint8_t* extra; // for ADD extra string
};

struct op_node {
    struct op* op;
    struct op_node* next;
};

int bsdiff(const uint8_t* old, int64_t oldsize, const uint8_t* new, int64_t newsize, struct bsdiff_stream* stream);
extern void op_node_insert(struct op_node *head, struct op_node *n);

#endif
