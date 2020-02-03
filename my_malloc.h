#ifndef _MY_MALLOC_H_
#define _MY_MALLOC_H_

#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
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
    struct metadata *next;
    struct metadata *prev;

} metadata;

//function pointer
typedef void *(*funcptr)(intptr_t);

//global variable
//lock
extern metadata *head;
extern metadata *tail;
extern pthread_mutex_t lock;

//no lock, use thread local storage
extern __thread metadata *head_tls;
extern __thread metadata *tail_tls;

#endif