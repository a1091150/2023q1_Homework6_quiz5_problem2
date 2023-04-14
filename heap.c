#include "heap.h"
#include <errno.h>
#include <unistd.h>
static size_t page_remaining = 0;
static void *first_block = NULL;
static void *end_in_page = NULL;
static metadata_t *last_node = NULL;
static int page_size = 0;

metadata_t *fusion(metadata_t *first, metadata_t *second)
{
    first->size += second->size;
    first->next = second->next;
    if (first->next)
        first->next->prev = first;
    else
        last_node = first;
    return first;
}

void change_break(metadata_t *node)
{
    size_t pages_to_remove;

    if (node->prev) {
        node->prev->next = NULL;
        last_node = node->prev;
        end_in_page = (void *) last_node + last_node->size;
    } else {
        end_in_page = last_node;
        last_node = NULL;
    }
    page_remaining += node->size;
    pages_to_remove = page_remaining / page_size;
    /* FIXME: sbrk is deprecated */
    brk((sbrk(0) - (pages_to_remove * page_size)));
    page_remaining = page_remaining - (pages_to_remove * page_size);
}

static size_t get_new_page(size_t size)
{
    size_t pages = ((size / page_size) + 1) * page_size;
    /* FIXME: sbrk is deprecated */
    if (!end_in_page) {
        if ((end_in_page = sbrk(0)) == (void *) -1)
            return (size_t) -1;
        first_block = end_in_page;
    }
    if (sbrk(pages) == (void *) -1) {
        errno = ENOMEM;
        return (size_t) -1;
    }
    return pages;
}

static void *get_in_page(size_t size)
{
    metadata_t *new = end_in_page;
    new->size = size;
    new->free = NFREE;
    new->next = NULL;
    new->prev = last_node;
    if (last_node)
        last_node->next = new;
    last_node = new;
    end_in_page = (void *) new + size;
    return new;
}
void *get_heap(size_t size)
{
    size_t tmp;

    if (!page_size)
        page_size = getpagesize();

    if (page_remaining < size) {
        if ((tmp = get_new_page(size)) == (size_t) -1)
            return NULL;
        page_remaining += tmp;
    }
    page_remaining -= size;
    return get_in_page(size);
}