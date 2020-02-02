#ifndef _MY_MALLOC_H_
#define _MY_MALLOC_H_

#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

//Thread Safe malloc/free: locking version
void *ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);

//Thread Safe malloc/free: non-locking version
void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);

//data structure
typedef struct metadata
{
    size_t size;
    struct metadata *prev;
    struct metadata *next;

} metadata;

//function pointer
typedef void *(*funcptr)(intptr_t);
//lock
metadata *head = NULL;
metadata *tail = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//no lock, use thread local storage
__thread metadata *head_tls = NULL;
__thread metadata *tail_tls = NULL;

#endif