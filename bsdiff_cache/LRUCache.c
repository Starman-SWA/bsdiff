#include "LRUCache.h"
 
OldFile* createOldFile(int fd, int fileSize, int pageSize) {
    OldFile* oldfile = (OldFile *)malloc(sizeof(OldFile));
    oldfile->fd = fd;
    oldfile->fileSize = fileSize;
    oldfile->pageSize = pageSize;
    return oldfile;
}

QNode* newQNode( unsigned pageNumber, OldFile* oldfile )
{
    QNode* temp = (QNode *)malloc( sizeof( QNode ) );
    temp->pageNumber = pageNumber;
    temp->prev = temp->next = NULL;
    int pos = lseek(oldfile->fd, pageNumber * oldfile->pageSize, SEEK_SET);
    //printf("lseek pos=%d ok\n", pos);
    temp->content = (uint8_t *)malloc(sizeof(uint8_t) * oldfile->pageSize);
    int rd = read(oldfile->fd, temp->content, oldfile->pageSize);
    //printf("read %d", rd);

    return temp;
}
 
Queue* createQueue( int numberOfFrames )
{
    Queue* queue = (Queue *)malloc( sizeof( Queue ) ); 
    queue->count = 0;
    queue->front = queue->rear = NULL;
    queue->numberOfFrames = numberOfFrames;
 
    return queue;
}
 

Hash* createHash( int capacity )
{
    Hash* hash = (Hash *) malloc( sizeof( Hash ) );
    hash->capacity = capacity;
    hash->array = (QNode **) malloc( hash->capacity * sizeof( QNode* ) );
    int i;
    for( i = 0; i < hash->capacity; ++i )
        hash->array[i] = NULL;
 
    return hash;
}
 
int AreAllFramesFull( Queue* queue )
{
    return queue->count == queue->numberOfFrames;
}
 

int isQueueEmpty( Queue* queue )
{
    return queue->rear == NULL;
}
 
void deQueue( Queue* queue )
{
    if( isQueueEmpty( queue ) )
        return;
 
    if (queue->front == queue->rear)
        queue->front = NULL;
 
    QNode* temp = queue->rear;
    queue->rear = queue->rear->prev;
 
    if (queue->rear)
        queue->rear->next = NULL;
 
    free(temp->content);
    free( temp );

    queue->count--;
}
 
void Enqueue( Queue* queue, Hash* hash, OldFile* oldfile, unsigned pageNumber, uint8_t** contentPtr )
{

    if ( AreAllFramesFull ( queue ) )
    {
        hash->array[ queue->rear->pageNumber ] = NULL;
        deQueue( queue );
    }
 
    QNode* temp = newQNode( pageNumber, oldfile );
    temp->next = queue->front;
 
    if ( isQueueEmpty( queue ) )
        queue->rear = queue->front = temp;
    else 
    {
        queue->front->prev = temp;
        queue->front = temp;
    }
 
    hash->array[ pageNumber ] = temp;
 
 
    queue->count++;

    *contentPtr = temp->content;
}


void ReferencePage( Queue* queue, Hash* hash, OldFile* oldfile, unsigned pageNumber, uint8_t** pageContent )
{
    QNode* reqPage = hash->array[ pageNumber ];
 
    if ( reqPage == NULL )
        Enqueue( queue, hash, oldfile, pageNumber, pageContent );
    else {
        *pageContent = reqPage->content;
        if (reqPage != queue->front)
        {
            reqPage->prev->next = reqPage->next;
            if (reqPage->next)
            reqPage->next->prev = reqPage->prev;
            if (reqPage == queue->rear)
            {
            queue->rear = reqPage->prev;
            queue->rear->next = NULL;
            }
            reqPage->next = queue->front;
            reqPage->prev = NULL;
            reqPage->next->prev = reqPage;
    
            queue->front = reqPage;
        }
    }
}
 

// int main()
// {
   
//     Queue* q = createQueue( 4 );        
//     Hash* hash = createHash( 10 );
//     ReferencePage( q, hash, 1);
//     ReferencePage( q, hash, 2);
//     ReferencePage( q, hash, 3);
//     ReferencePage( q, hash, 1);
//     ReferencePage( q, hash, 4);
//     ReferencePage( q, hash, 5);
//     printf ("%d ", q->front->pageNumber);
//     printf ("%d ", q->front->next->pageNumber);
//     printf ("%d ", q->front->next->next->pageNumber);
//     printf ("%d ", q->front->next->next->next->pageNumber);
 
//     return 0;
// }