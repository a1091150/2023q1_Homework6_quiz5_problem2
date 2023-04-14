#include "xalloc.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "heap.h"
#include "rbtree.h"

static malloc_t g_info = {
    .root_rbtree = NULL,
    .last_node = NULL,
    .end_in_page = NULL,
    .first_block = NULL,
    .page_size = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .page_remaining = 0,
};

static inline rbnode_t *find_best(rbnode_t *node, size_t size)
{
    rbnode_t *tmp = NULL;
    while (node) {
        if (node->key >= size) {
            tmp = node;
            node = node->left;
        } else
            node = node->right;
    }
    return tmp;
}

static metadata_t *search_freed_block(rbnode_t *node, size_t size)
{
    rbnode_t *tmp = find_best(node, size);
    if (tmp) {
        size_t size_tab = tmp->size_tab;
        metadata_t **tab = tmp->tab_values;
        for (size_t i = 0; i < size_tab; i++) {
            if (tab[i])
                return tab[i];
        }
    }
    return NULL;
}

static void *split_block(metadata_t *node, size_t size)
{
    g_info.root_rbtree = remove_from_freed_list(g_info.root_rbtree, node);
    if (node->size > size + sizeof(size_t) &&
        node->size - size > sizeof(rbnode_t) + SIZE_DEFAULT_BLOCK) {
        metadata_t *new = (void *) node + size;
        new->size = node->size - size;
        new->free = YFREE;
        new->prev = node;
        new->next = node->next;
        node->next = new;
        node->size = size;
        if (new->next)
            new->next->prev = new;
        g_info.root_rbtree = insert_in_freed_list(g_info.root_rbtree, new);
    }
    return node;
}

void *malloc(size_t size)
{
    metadata_t *tmp;
    void *ptr;

    pthread_mutex_lock(&g_info.mutex);
    if (size < SIZE_DEFAULT_BLOCK)
        size = SIZE_DEFAULT_BLOCK;
    size = ALIGN_BYTES(size) + META_SIZE;
    if (!g_info.page_size)
        g_info.page_size = getpagesize();
    if ((tmp = search_freed_block(g_info.root_rbtree, size)))
        ptr = split_block(tmp, size);
    else
        ptr = get_heap(size);
    pthread_mutex_unlock(&g_info.mutex);
    return ptr ? (GET_PAYLOAD(ptr)) : NULL;
}

static void invalid_pointer(void *ptr)
{
    printf("Error in '%s': free(): invalid pointer: %p\n",
           ((__progname) ? (__progname) : ("Unknow")), ptr);
    abort();
}

static void double_free(void *ptr)
{
    printf("Error in '%s': free(): double free: %p\n",
           ((__progname) ? (__progname) : ("Unknow")), ptr);
    abort();
}

static metadata_t *fusion(metadata_t *first, metadata_t *second)
{
    first->size += second->size;
    first->next = second->next;
    if (first->next)
        first->next->prev = first;
    else
        g_info.last_node = first;
    return first;
}

static inline metadata_t *try_fusion(metadata_t *node)
{
    while (IS_FREE(node->prev)) {
        g_info.root_rbtree =
            remove_from_freed_list(g_info.root_rbtree, node->prev);
        node = fusion(node->prev, node);
    }
    while (IS_FREE(node->next)) {
        g_info.root_rbtree =
            remove_from_freed_list(g_info.root_rbtree, node->next);
        node = fusion(node, node->next);
    }
    return node;
}

static inline void change_break(metadata_t *node)
{
    size_t pages_to_remove;

    if (node->prev) {
        node->prev->next = NULL;
        g_info.last_node = node->prev;
        g_info.end_in_page = (void *) g_info.last_node + g_info.last_node->size;
    } else {
        g_info.end_in_page = g_info.last_node;
        g_info.last_node = NULL;
    }
    g_info.page_remaining += node->size;
    pages_to_remove = g_info.page_remaining / g_info.page_size;
    /* FIXME: sbrk is deprecated */
    brk((sbrk(0) - (pages_to_remove * g_info.page_size)));
    g_info.page_remaining =
        g_info.page_remaining - (pages_to_remove * g_info.page_size);
}

void free(void *ptr)
{
    if (!ptr)
        return;

    pthread_mutex_lock(&g_info.mutex);
    metadata_t *node = GET_NODE(ptr);
    if (ptr < g_info.first_block || ptr > g_info.end_in_page || !IS_VALID(node))
        invalid_pointer(ptr);
    if (node->free == YFREE)
        double_free(ptr);
    node = try_fusion(node);
    if (!node->next)
        change_break(node);
    else
        g_info.root_rbtree = insert_in_freed_list(g_info.root_rbtree, node);
    pthread_mutex_unlock(&g_info.mutex);
}

void *calloc(size_t nmemb, size_t size)
{
    if (!nmemb || !size)
        return NULL;

    void *ptr;
    if (!(ptr = malloc(size * nmemb)))
        return NULL;

    pthread_mutex_lock(&g_info.mutex);
    memset(ptr, 0, ALIGN_BYTES(size * nmemb));
    pthread_mutex_unlock(&g_info.mutex);
    return ptr;
}

void *free_realloc(void *ptr)
{
    free(ptr);
    return NULL;
}

void *realloc(void *ptr, size_t size)
{
    if (!ptr)
        return malloc(size);
    if (!size)
        return free_realloc(ptr);

    ptr = (void *) ptr - META_SIZE;
    metadata_t *tmp = (metadata_t *) ptr;
    metadata_t *new = ptr;
    if (size + META_SIZE > tmp->size) {
        if (!(new = malloc(size)))
            return NULL;

        size = ALIGN_BYTES(size);
        pthread_mutex_lock(&g_info.mutex);
        memcpy(new, (void *) ptr + META_SIZE,
               (size <= tmp->size) ? (size) : (tmp->size));
        pthread_mutex_unlock(&g_info.mutex);
        free((void *) ptr + META_SIZE);
    } else
        new = GET_PAYLOAD(new);
    return new;
}