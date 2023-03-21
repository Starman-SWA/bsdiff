#include "LRUCache.h"

int main()
{
    int fd = open("./seq.txt", O_RDONLY);
    int oldsize = lseek(fd, 0, SEEK_END);
    printf("oldsize=%d\n", oldsize);

    OldFile* oldfile = createOldFile(fd, oldsize, 32);
    Queue* q = createQueue( 4 );        
    Hash* hash = createHash( 32 );
    uint8_t** buffer[6];
    for (int i = 0; i < 6; ++i) {
        buffer[i] = (uint8_t**)malloc(sizeof(uint8_t*));
    }
    ReferencePage( q, hash, oldfile, 1, buffer[0]);
    printf("buffer0:\n");
    for (int i = 0; i < 32; ++i) {
        printf("%d ", (*buffer[0])[i]);
    }
    printf("\n");
    ReferencePage( q, hash, oldfile, 2, buffer[1]);
    printf("buffer1:\n");
    for (int i = 0; i < 32; ++i) {
        printf("%d ", (*buffer[1])[i]);
    }
    printf("\n");
    ReferencePage( q, hash, oldfile, 3, buffer[2]);
    printf("buffer2:\n");
    for (int i = 0; i < 32; ++i) {
        printf("%d ", (*buffer[2])[i]);
    }
    printf("\n");
    ReferencePage( q, hash, oldfile, 1, buffer[3]);
    printf("buffer3:\n");
    for (int i = 0; i < 32; ++i) {
        printf("%d ", (*buffer[3])[i]);
    }
    printf("\n");
    ReferencePage( q, hash, oldfile, 4, buffer[4]);
    printf("buffer4:\n");
    for (int i = 0; i < 32; ++i) {
        printf("%d ", (*buffer[4])[i]);
    }
    printf("\n");
    ReferencePage( q, hash, oldfile, 5, buffer[5]);
    printf("buffer5:\n");
    for (int i = 0; i < 32; ++i) {
        printf("%d ", (*buffer[5])[i]);
    }
    printf("\n");
    printf ("%d ", q->front->pageNumber);
    printf ("%d ", q->front->next->pageNumber);
    printf ("%d ", q->front->next->next->pageNumber);
    printf ("%d ", q->front->next->next->next->pageNumber);
 
    close(fd);
    return 0;
}