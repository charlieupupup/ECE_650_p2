#include "my_malloc.h"

/*
 *
 * init global variable
 * 
 */

metadata *head = NULL;
metadata *tail = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//no lock, use thread local storage
__thread metadata *head_tls = NULL;
__thread metadata *tail_tls = NULL;

/*
 *
 * free list
 * 
 */
void add_list(metadata *curr, metadata **head_sp, metadata **tail_sp)
{
    //empty list
    if (!(*head_sp))
    {
        *head_sp = curr;
        *tail_sp = curr;
        return;
    }

    //curr ahead of head
    if (*head_sp && (unsigned long)curr < (unsigned long)(*head_sp))
    {
        curr->next = *head_sp;
        (*head_sp)->prev = curr;
        *head_sp = curr;
        return;
    }

    //curr after head
    if (*head_sp && (unsigned long)curr >= (unsigned long)(*head_sp))
    {
        //loop through to find proper place
        metadata *tmp = *head_sp;
        while (tmp->next)
        {
            if ((unsigned long)tmp < (unsigned long)curr && (unsigned long)curr < (unsigned long)tmp->next)
            {
                break;
            }
            tmp = tmp->next;
        }

        //curr
        curr->prev = tmp;
        curr->next = tmp->next;

        //check tmp->next
        if (tmp->next)
        {
            tmp->next->prev = curr;
        }

        //tail
        if (*tail_sp == tmp)
        {
            *tail_sp = curr;
        }
        //tmp
        tmp->next = curr;
    }
}

void remove_list(metadata *curr, metadata **head_sp, metadata **tail_sp)
{
    if (!curr)
    {
        return;
    }

    if (*head_sp == curr)
    {
        *head_sp = curr->next;
    }

    if (*tail_sp == curr)
    {
        *tail_sp = curr->prev;
    }

    //prev
    if (curr->prev)
    {
        curr->prev->next = curr->next;
    }
    //next
    if (curr->next)
    {
        curr->next->prev = curr->prev;
    }

    //curr
    curr->prev = NULL;
    curr->next = NULL;
}
/*
 *
 * physical address
 * 
 */
//curr remain in free list, use next block
metadata *split(metadata *curr, size_t size)
{

    //curr
    curr->size -= size + sizeof(metadata);

    //next
    metadata *next = (void *)curr + sizeof(metadata) + curr->size;
    next->size = size;
    next->prev = NULL;
    next->next = NULL;

    return next;
}

void join(metadata *curr, metadata **head_sp, metadata **tail_sp)
{
    if (curr)
    {
        //next
        if (curr->next && (void *)curr + sizeof(metadata) + curr->size == curr->next)
        {
            curr->size += curr->next->size + sizeof(metadata);

            if (*tail_sp == curr->next)
            {
                *tail_sp = curr;
            }
            if (curr->next->next)
            {
                curr->next->next->prev = curr;
            }
            curr->next = curr->next->next;
        }
        //prev
        if (curr->prev && (void *)curr->prev + sizeof(metadata) + curr->prev->size == curr)
        {
            curr->prev->size += curr->size + sizeof(metadata);

            if (*tail_sp == curr)
            {
                *tail_sp = curr->prev;
            }

            if (curr->next)
            {
                curr->next->prev = curr->prev;
            }

            curr->prev->next = curr->next;
        }
    }
}

metadata *extend(size_t size, funcptr func)
{
    //corner case
    if (size <= 0)
    {
        return NULL;
    }

    //size
    size_t s = size + sizeof(metadata);
    void *curr = (*func)(s);

    if (curr == (void *)-1)
    {
        return NULL;
    }

    //init
    metadata *new_block = curr;
    new_block->size = size;
    new_block->prev = NULL;
    new_block->next = NULL;

    return new_block;
}

void *sbrk_lock(intptr_t size)
{
    pthread_mutex_lock(&lock);
    void *ptr = sbrk(size);
    pthread_mutex_unlock(&lock);
    return ptr;
}

/*
 *
 * search & malloc
 * 
 */

metadata *bf(size_t size, metadata **head_sp)
{
    metadata *best = NULL;
    metadata *curr = *head_sp;

    while (curr)
    {
        if (curr->size == size)
        {
            return curr;
        }

        if (curr->size >= size)
        {
            if (!best || best->size > curr->size)
            {
                best = curr;
            }
        }

        curr = curr->next;
    }

    return best;
}

void *my_malloc(size_t size, funcptr func, metadata **head_sp, metadata **tail_sp)
{
    if (!(*head_sp))
    {
        metadata *new = extend(size, func);
        return new + 1;
    }

    //find block
    metadata *curr = bf(size, head_sp);

    if (!curr)
    {
        metadata *new = extend(size, func);
        return new + 1;
    }

    if (curr && curr->size <= size + 40)
    {
        remove(curr, head_sp, tail_sp);
        return curr + 1;
    }
    else
    {

        metadata *next = split(curr, size);
        return next + 1;
    }
}

void my_free(void *ptr, metadata **head_sp, metadata **tail_sp)
{
    if (!ptr)
    {
        return;
    }

    metadata *curr = (metadata *)ptr - 1;
    add(curr, head_sp, tail_sp);
    join(curr, head_sp, tail_sp);
}

//lock
void *ts_malloc_lock(size_t size)
{
    pthread_mutex_lock(&lock);
    void *ptr = my_malloc(size, sbrk, &head, &tail);
    pthread_mutex_unlock(&lock);
    return ptr;
}
void ts_free_lock(void *ptr)
{
    pthread_mutex_lock(&lock);
    my_free(ptr, &head, &tail);
    pthread_mutex_unlock(&lock);
}

//no lock
void *ts_malloc_nolock(size_t size)
{
    void *ptr = my_malloc(size, sbrk_lock, &head_tls, &tail_tls);
    return ptr;
}
void ts_free_nolock(void *ptr)
{
    my_free(ptr, &head_tls, &tail_tls);
}
