#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>  
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef _LRU_CACHE_H
#define _LRU_CACHE_H

/***
 * 要求：Hash->capacity * oldFile->pageSize >= OldFile->fileSize
*/

typedef struct QNode
{
    struct QNode *prev, *next;
    unsigned pageNumber;  
    uint8_t *content;
} QNode;
 
typedef struct Queue
{
    unsigned count;  
    unsigned numberOfFrames; 
    QNode *front, *rear;
} Queue;
 
typedef struct Hash
{
    int capacity; 
    QNode* *array;
} Hash;

typedef struct OldFile {
    int fd;
    int fileSize;
    int pageSize;
} OldFile;

OldFile* createOldFile(int fd, int fileSize, int pageSize);
Queue* createQueue( int numberOfFrames );
Hash* createHash( int capacity );
void ReferencePage( Queue* queue, Hash* hash, OldFile* oldfile, unsigned pageNumber, uint8_t** pageContent );

#endif