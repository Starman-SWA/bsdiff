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

#include "bsdiff.h"
#include "graph.h"

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MIN(x,y) (((x)<(y)) ? (x) : (y))

static void split(int64_t *I,int64_t *V,int64_t start,int64_t len,int64_t h)
{
	int64_t i,j,k,x,tmp,jj,kk;

	if(len<16) {
		for(k=start;k<start+len;k+=j) {
			j=1;x=V[I[k]+h];
			for(i=1;k+i<start+len;i++) {
				if(V[I[k+i]+h]<x) {
					x=V[I[k+i]+h];
					j=0;
				};
				if(V[I[k+i]+h]==x) {
					tmp=I[k+j];I[k+j]=I[k+i];I[k+i]=tmp;
					j++;
				};
			};
			for(i=0;i<j;i++) V[I[k+i]]=k+j-1;
			if(j==1) I[k]=-1;
		};
		return;
	};

	x=V[I[start+len/2]+h];
	jj=0;kk=0;
	for(i=start;i<start+len;i++) {
		if(V[I[i]+h]<x) jj++;
		if(V[I[i]+h]==x) kk++;
	};
	jj+=start;kk+=jj;

	i=start;j=0;k=0;
	while(i<jj) {
		if(V[I[i]+h]<x) {
			i++;
		} else if(V[I[i]+h]==x) {
			tmp=I[i];I[i]=I[jj+j];I[jj+j]=tmp;
			j++;
		} else {
			tmp=I[i];I[i]=I[kk+k];I[kk+k]=tmp;
			k++;
		};
	};

	while(jj+j<kk) {
		if(V[I[jj+j]+h]==x) {
			j++;
		} else {
			tmp=I[jj+j];I[jj+j]=I[kk+k];I[kk+k]=tmp;
			k++;
		};
	};

	if(jj>start) split(I,V,start,jj-start,h);

	for(i=0;i<kk-jj;i++) V[I[jj+i]]=kk-1;
	if(jj==kk-1) I[jj]=-1;

	if(start+len>kk) split(I,V,kk,start+len-kk,h);
}

static void qsufsort(int64_t *I,int64_t *V,const uint8_t *old,int64_t oldsize)
{
	int64_t buckets[256];
	int64_t i,h,len;

	for(i=0;i<256;i++) buckets[i]=0;
	for(i=0;i<oldsize;i++) buckets[old[i]]++;
	for(i=1;i<256;i++) buckets[i]+=buckets[i-1];
	for(i=255;i>0;i--) buckets[i]=buckets[i-1];
	buckets[0]=0;

	for(i=0;i<oldsize;i++) I[++buckets[old[i]]]=i;
	I[0]=oldsize;
	for(i=0;i<oldsize;i++) V[i]=buckets[old[i]];
	V[oldsize]=0;
	for(i=1;i<256;i++) if(buckets[i]==buckets[i-1]+1) I[buckets[i]]=-1;
	I[0]=-1;

	for(h=1;I[0]!=-(oldsize+1);h+=h) {
		len=0;
		for(i=0;i<oldsize+1;) {
			if(I[i]<0) {
				len-=I[i];
				i-=I[i];
			} else {
				if(len) I[i-len]=-len;
				len=V[I[i]]+1-i;
				split(I,V,i,len,h);
				i+=len;
				len=0;
			};
		};
		if(len) I[i-len]=-len;
	};

	for(i=0;i<oldsize+1;i++) I[V[i]]=i;
}

static int64_t matchlen(const uint8_t *old,int64_t oldsize,const uint8_t *new,int64_t newsize)
{
	int64_t i;

	for(i=0;(i<oldsize)&&(i<newsize);i++)
		if(old[i]!=new[i]) break;

	return i;
}

static int64_t search(const int64_t *I,const uint8_t *old,int64_t oldsize,
		const uint8_t *new,int64_t newsize,int64_t st,int64_t en,int64_t *pos,const int64_t scan)
{
	int64_t x,y;

	if(en-st<2) {
//		x=matchlen(old+I[st],oldsize-I[st],new,newsize);
//		y=matchlen(old+I[en],oldsize-I[en],new,newsize);

        while(st>=0 && I[st]>scan) --st;
        while(en<oldsize && I[en]>scan) ++en;

        x=0, y=0;
        if(st>=0) x=matchlen(old+I[st],oldsize-I[st],new,newsize);
        if(en<oldsize) y=matchlen(old+I[en],oldsize-I[en],new,newsize);

        if(x>y) {
            *pos=I[st];
            return x;
        } else {
            *pos=I[en];
            return y;
        }
	};

	x=st+(en-st)/2;
	if(memcmp(old+I[x],new,MIN(oldsize-I[x],newsize))<0) {
		return search(I,old,oldsize,new,newsize,x,en,pos,scan);
	} else {
		return search(I,old,oldsize,new,newsize,st,x,pos,scan);
	};
}

static void offtout(int64_t x,uint8_t *buf)
{
	int64_t y;

	if(x<0) y=-x; else y=x;

	buf[0]=y%256;y-=buf[0];
	y=y/256;buf[1]=y%256;y-=buf[1];
	y=y/256;buf[2]=y%256;y-=buf[2];
	y=y/256;buf[3]=y%256;y-=buf[3];
	y=y/256;buf[4]=y%256;y-=buf[4];
	y=y/256;buf[5]=y%256;y-=buf[5];
	y=y/256;buf[6]=y%256;y-=buf[6];
	y=y/256;buf[7]=y%256;

	if(x<0) buf[7]|=0x80;
}

static int64_t writedata(struct bsdiff_stream* stream, const void* buffer, int64_t length)
{
	int64_t result = 0;

	while (length > 0)
	{
		const int smallsize = (int)MIN(length, INT_MAX);
		const int writeresult = stream->write(stream, buffer, smallsize);
		if (writeresult == -1)
		{
			return -1;
		}

		result += writeresult;
		length -= smallsize;
		buffer = (uint8_t*)buffer + smallsize;
	}

	return result;
}

struct bsdiff_request
{
	const uint8_t* old;
	int64_t oldsize;
	const uint8_t* new;
	int64_t newsize;
	struct bsdiff_stream* stream;
	int64_t *I;
	uint8_t *buffer;
};

void op_node_insert(struct op_node *head, struct op_node *n) {
    if (!n) return;

    struct op_node *cur = head;
    while (cur->next) {
        cur = cur->next;
    }
    cur->next = n;
    n->next = NULL;
}

struct op_node* new_add(int64_t to, int64_t len, uint8_t *extra) {
    struct op_node *n = (struct op_node*)malloc(sizeof(struct op_node));
    if (!n) return NULL;

    n->next = NULL;
    n->op = (struct op*)malloc(sizeof(struct op));
    if (!n->op) {
        free(n);
        return NULL;
    }

    n->op->optype = ADD;
    n->op->to = to;
    n->op->len = len;
    n->op->extra = (uint8_t*)malloc(sizeof(uint8_t)*len);
    if (!n->op->extra) {
        free(n->op);
        free(n);
        return NULL;
    }
    memcpy(n->op->extra, extra, len);

    return n;
}

struct op_node* new_copy(int64_t from, int64_t to, int64_t len, uint8_t *diff, const uint8_t *old) {
    struct op_node *n = (struct op_node*)malloc(sizeof(struct op_node));
    if (!n) return NULL;

    n->next = NULL;
    n->op = (struct op*)malloc(sizeof(struct op));
    if (!n->op) {
        free(n);
        return NULL;
    }

    n->op->optype = COPY;
    n->op->from = from;
    n->op->to = to;
    n->op->len = len;
    n->op->diff = (uint8_t*)malloc(sizeof(uint8_t)*len);
    if (!n->op->diff) {
        free(n->op);
        free(n);
        return NULL;
    }
    memcpy(n->op->diff, diff, len);
    n->op->old = old;

    return n;
}

int sortandwrite(const struct bsdiff_request req, struct op_node* copy_head, struct op_node* add_head) {
    FILE* fp;
    fp = fopen("difftest.txt", "w");
    if (fp == NULL) exit(1);

    int copynum = 0;
    struct op_node* copy_cur = copy_head->next;
    while (copy_cur) {
        ++copynum;
        copy_cur = copy_cur->next;
    }

    struct op_node** copyarr = (struct op_node**)malloc(sizeof(struct op_node*) * copynum);
    if (!copyarr) return -1;

    // convert "copy"s from linked list to array
    copy_cur = copy_head->next;
    for (int i = 0; i < copynum; ++i) {
        copyarr[i] = copy_cur;
        copy_cur = copy_cur->next;
    }

    printf("copynum:%d\n", copynum);

    struct VNode** graph = buildGraph(copyarr, copynum);
    struct op_node* new_add_head = removeRings(graph, copynum, copyarr);
    int* copyorder = toposort(graph, copynum);

    int64_t actualcopynum = 0;
    while (copyorder[actualcopynum] != -1) ++actualcopynum;

    struct op_node *add_cur = add_head;
    while (add_cur->next) add_cur = add_cur->next;
    add_cur->next = new_add_head->next;
    free(new_add_head);

    int64_t actualaddnum = 0;
    add_cur = add_head->next;
    while (add_cur) {++actualaddnum; add_cur=add_cur->next;}

    uint8_t buf[24];
    offtout(actualcopynum, buf);
    writedata(req.stream, buf, 8);
    offtout(actualaddnum, buf);
    writedata(req.stream, buf, 8);

    struct op* op;
    for (int64_t i = 0; i < actualcopynum; ++i) {
        op = copyarr[copyorder[i]]->op;
        offtout(op->from, buf);
        offtout(op->to, buf+8);
        offtout(op->len, buf+16);
        fprintf(fp, "COPY %ld %ld %ld\n", op->from, op->to, op->len);
        for (int64_t j = 0; j < op->len; j++) {
            fprintf(fp, "%d ", op->diff[j]);
        }
        fprintf(fp, "\n");
        for (int64_t j = 0; j < op->len; j++) {
            fprintf(fp, "%d ", req.old[op->from+j]);
        }
        fprintf(fp, "\n");
        writedata(req.stream, buf, 24);
        writedata(req.stream, op->diff, op->len);
    }

    add_cur = add_head->next;
    for (int64_t i = 0; i < actualaddnum; ++i) {
        op = add_cur->op;
        offtout(op->to, buf);
        offtout(op->len, buf+8);
        fprintf(fp, "ADD %ld %ld\n", op->to, op->len);
        for (int64_t j = 0; j < op->len; j++) {
            fprintf(fp, "%d ", op->extra[j]);
        }
        fprintf(fp, "\n");
        writedata(req.stream, buf, 16);
        writedata(req.stream, op->extra, op->len);
        add_cur = add_cur->next;
    }

    printf("copynum:%ld, addnum:%ld\n", actualcopynum, actualaddnum);


    for (int i = 0; i < copynum; ++i) {
        free(copyarr[i]);
    }
    for (int i = 0; i < copynum; ++i) {
        struct AdjNode* adj = graph[i]->firstAdj, *tmp;
        while (adj) {
            tmp = adj;
            adj = adj->next;
            free(tmp);
        }
        free(graph[i]);
    }

    fclose(fp);

    return 0;
}

void free_op_linkedlist(struct op_node* head) {
    struct op_node* cur, *tmp;
    struct op* op;

    tmp = head;
    cur = head->next;
    free(tmp);
    while (cur) {
        op = cur->op;

        if (op->extra) free(op->extra);
        if (op->diff) free(op->diff);

        tmp = cur;
        cur = cur->next;
        free(tmp);
    }
}

static int bsdiff_internal(const struct bsdiff_request req)
{
	int64_t *I,*V;
	int64_t scan,pos,len;
	int64_t lastscan,lastpos,lastoffset;
	int64_t oldscore,scsc;
	int64_t s,Sf,lenf,Sb,lenb;
	int64_t overlap,Ss,lens;
	int64_t i;
	uint8_t *buffer;
	uint8_t buf[8 * 3];
    int64_t copynum = 0, addnum = 0;

    struct op_node *add_head, *copy_head;
    if ((add_head = (struct op_node*)malloc(sizeof(struct op_node))) == NULL) return -1;
    add_head->next = NULL;
    if ((copy_head = (struct op_node*)malloc(sizeof(struct op_node))) == NULL) return -1;
    copy_head->next = NULL;

	if((V=req.stream->malloc((req.oldsize+1)*sizeof(int64_t)))==NULL) return -1;
	I = req.I;

	qsufsort(I,V,req.old,req.oldsize);
	req.stream->free(V);

	buffer = req.buffer;

	/* Compute the differences, writing ctrl as we go */
	scan=0;len=0;pos=0;
	lastscan=0;lastpos=0;lastoffset=0;
	while(scan<req.newsize) {
		oldscore=0;

		for(scsc=scan+=len;scan<req.newsize;scan++) {
			len=search(I,req.old,req.oldsize,req.new+scan,req.newsize-scan,
					0,req.oldsize,&pos,scan);

			for(;scsc<scan+len;scsc++)
			if((scsc+lastoffset<req.oldsize) &&
				(req.old[scsc+lastoffset] == req.new[scsc]))
				oldscore++;

			if(((len==oldscore) && (len!=0)) || 
				(len>oldscore+8)) break;

			if((scan+lastoffset<req.oldsize) &&
				(req.old[scan+lastoffset] == req.new[scan]))
				oldscore--;
		};

		if((len!=oldscore) || (scan==req.newsize)) {
			s=0;Sf=0;lenf=0;
			for(i=0;(lastscan+i<scan)&&(lastpos+i<req.oldsize);) {
				if(req.old[lastpos+i]==req.new[lastscan+i]) s++;
				i++;
				if(s*2-i>Sf*2-lenf) { Sf=s; lenf=i; };
			};

			lenb=0;
			if(scan<req.newsize) {
				s=0;Sb=0;
				for(i=1;(scan>=lastscan+i)&&(pos>=i);i++) {
					if(req.old[pos-i]==req.new[scan-i]) s++;
					if(s*2-i>Sb*2-lenb) { Sb=s; lenb=i; };
				};
			};

			if(lastscan+lenf>scan-lenb) {
				overlap=(lastscan+lenf)-(scan-lenb);
				s=0;Ss=0;lens=0;
				for(i=0;i<overlap;i++) {
					if(req.new[lastscan+lenf-overlap+i]==
					   req.old[lastpos+lenf-overlap+i]) s++;
					if(req.new[scan-lenb+i]==
					   req.old[pos-lenb+i]) s--;
					if(s>Ss) { Ss=s; lens=i+1; };
				};

				lenf+=lens-overlap;
				lenb-=lens;
			};

//			offtout(lenf,buf);
//			offtout((scan-lenb)-(lastscan+lenf),buf+8);
//			offtout((pos-lenb)-(lastpos+lenf),buf+16);
//
//			/* Write control data */
//			if (writedata(req.stream, buf, sizeof(buf)))
//				return -1;
//
            if (lenf > 0) {
                //			/* Write diff data */
                for(i=0;i<lenf;i++)
                    buffer[i]=req.new[lastscan+i]-req.old[lastpos+i];
//			if (writedata(req.stream, buffer, lenf))
//				return -1;

                op_node_insert(copy_head,
                               new_copy(lastpos, lastscan, lenf, buffer, req.old));
                copynum++;
            }

            if ((scan-lenb)-(lastscan+lenf) > 0) {
                //			/* Write extra data */
                for(i=0;i<(scan-lenb)-(lastscan+lenf);i++)
                    buffer[i]=req.new[lastscan+lenf+i];
//			if (writedata(req.stream, buffer, (scan-lenb)-(lastscan+lenf)))
//				return -1;

                op_node_insert(add_head,
                               new_add(lastscan+lenf, (scan-lenb)-(lastscan+lenf), buffer));
                addnum++;
            }


			lastscan=scan-lenb;
			lastpos=pos-lenb;
			lastoffset=pos-scan;
		};
	};

    printf("presort: copynum:%ld, addnum:%ld\n", copynum, addnum);
    if (sortandwrite(req, copy_head, add_head))
        return -1;

    free_op_linkedlist(add_head);

	return 0;
}

int bsdiff(const uint8_t* old, int64_t oldsize, const uint8_t* new, int64_t newsize, struct bsdiff_stream* stream)
{
	int result;
	struct bsdiff_request req;

	if((req.I=stream->malloc((oldsize+1)*sizeof(int64_t)))==NULL)
		return -1;

	if((req.buffer=stream->malloc(newsize+1))==NULL)
	{
		stream->free(req.I);
		return -1;
	}

	req.old = old;
	req.oldsize = oldsize;
	req.new = new;
	req.newsize = newsize;
	req.stream = stream;

	result = bsdiff_internal(req);

	stream->free(req.buffer);
	stream->free(req.I);

	return result;
}

#if defined(BSDIFF_EXECUTABLE)

#include <sys/types.h>

#include <bzlib.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int bz2_write(struct bsdiff_stream* stream, const void* buffer, int size)
{
	int bz2err;
	BZFILE* bz2;

	bz2 = (BZFILE*)stream->opaque;
	BZ2_bzWrite(&bz2err, bz2, (void*)buffer, size);
	if (bz2err != BZ_STREAM_END && bz2err != BZ_OK)
		return -1;

	return 0;
}

int main(int argc,char *argv[])
{
	int fd;
	int bz2err;
	uint8_t *old,*new;
	off_t oldsize,newsize;
	uint8_t buf[8];
	FILE * pf;
	struct bsdiff_stream stream;
	BZFILE* bz2;

	memset(&bz2, 0, sizeof(bz2));
	stream.malloc = malloc;
	stream.free = free;
	stream.write = bz2_write;

	if(argc!=4) errx(1,"usage: %s oldfile newfile patchfile\n",argv[0]);

	/* Allocate oldsize+1 bytes instead of oldsize bytes to ensure
		that we never try to malloc(0) and get a NULL pointer */
	if(((fd=open(argv[1],O_RDONLY,0))<0) ||
		((oldsize=lseek(fd,0,SEEK_END))==-1) ||
		((old=malloc(oldsize+1))==NULL) ||
		(lseek(fd,0,SEEK_SET)!=0) ||
		(read(fd,old,oldsize)!=oldsize) ||
		(close(fd)==-1)) err(1,"%s",argv[1]);


	/* Allocate newsize+1 bytes instead of newsize bytes to ensure
		that we never try to malloc(0) and get a NULL pointer */
	if(((fd=open(argv[2],O_RDONLY,0))<0) ||
		((newsize=lseek(fd,0,SEEK_END))==-1) ||
		((new=malloc(newsize+1))==NULL) ||
		(lseek(fd,0,SEEK_SET)!=0) ||
		(read(fd,new,newsize)!=newsize) ||
		(close(fd)==-1)) err(1,"%s",argv[2]);

	/* Create the patch file */
	if ((pf = fopen(argv[3], "w")) == NULL)
		err(1, "%s", argv[3]);

	/* Write header (signature+newsize)*/
	offtout(newsize, buf);
	if (fwrite("ENDSLEY/BSDIFF43", 16, 1, pf) != 1 ||
		fwrite(buf, sizeof(buf), 1, pf) != 1)
		err(1, "Failed to write header");


	if (NULL == (bz2 = BZ2_bzWriteOpen(&bz2err, pf, 9, 0, 0)))
		errx(1, "BZ2_bzWriteOpen, bz2err=%d", bz2err);

	stream.opaque = bz2;
	if (bsdiff(old, oldsize, new, newsize, &stream))
		err(1, "bsdiff");

	BZ2_bzWriteClose(&bz2err, bz2, 0, NULL, NULL);
	if (bz2err != BZ_OK)
		err(1, "BZ2_bzWriteClose, bz2err=%d", bz2err);

	if (fclose(pf))
		err(1, "fclose");

	/* Free the memory we used */
	free(old);
	free(new);

	return 0;
}

#endif
