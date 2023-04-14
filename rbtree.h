#ifndef __RBBTREE
#define __RBBTREE
#include <stddef.h>
#include "heap.h"

#define IS_FREE(x) ((x) ? (((metadata_t *) x)->free == YFREE) : (0))
#define IS_RED(x) ((x) ? (((rbnode_t *) x)->color == RED) : (0))
#define MY_COMPARE(k1, k2) (((k1 == k2) ? (0) : ((k1 < k2) ? (-1) : (1))))

typedef enum rbcolor { BLACK = 0, RED = 1 } rbcolor_t;

typedef size_t t_key;
typedef metadata_t t_value;

typedef struct rbnode {
    metadata_t meta;
    size_t key;
    t_value **tab_values;
    size_t size_tab;
    size_t n_active;
    rbcolor_t color;
    struct rbnode *left, *right;
} rbnode_t;

extern const char *__progname;

rbnode_t *remove_from_freed_list(rbnode_t *node, metadata_t *meta);
rbnode_t *insert_in_freed_list(rbnode_t *node, metadata_t *new);

#endif /* __RBBTREE */