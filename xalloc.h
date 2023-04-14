#ifndef __XALLOC
#define __XALLOC
#include <pthread.h>
#include "rbtree.h"
typedef struct {
    rbnode_t *root_rbtree;
    pthread_mutex_t mutex;
} malloc_t;

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *free_realloc(void *ptr);
void *realloc(void *ptr, size_t size);
#endif /* __XALLOC */