#ifndef __XALLOC
#define __XALLOC
#include <pthread.h>
#include "rbtree.h"
typedef struct {
    rbnode_t *root_rbtree;
    metadata_t *last_node;
    void *end_in_page;
    void *first_block;
    int page_size;
    pthread_mutex_t mutex;
    size_t page_remaining;
} malloc_t;

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *free_realloc(void *ptr);
void *realloc(void *ptr, size_t size);
#endif /* __XALLOC */