#include "rbtree.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

static bool resize_tab_values(metadata_t **old, rbnode_t *node);
static rbnode_t *new_rbtree(metadata_t *node);
static rbnode_t *remove_node(rbnode_t *node, t_key key, rbnode_t *tmp);

static inline void flip_color(rbnode_t *node)
{
    assert(node);
    node->color = !(node->color);
    node->left->color = !(node->left->color);
    node->right->color = !(node->right->color);
}

static rbnode_t *rotate_left(rbnode_t *left)
{
    if (!left)
        return NULL;

    rbnode_t *right = left->right;
    left->right = right->left;
    right->left = left;
    right->color = left->color;
    left->color = RED;
    return right;
}

static rbnode_t *rotate_right(rbnode_t *right)
{
    if (!right)
        return NULL;

    rbnode_t *left = right->left;
    right->left = left->right;
    left->right = right;
    left->color = right->color;
    right->color = RED;
    return left;
}

static rbnode_t *balance(rbnode_t *node)
{
    if (IS_RED(node->right))
        node = rotate_left(node);
    if (IS_RED(node->left) && IS_RED(node->left->left))
        node = rotate_right(node);
    if (IS_RED(node->left) && IS_RED(node->right))
        flip_color(node);
    return node;
}

static rbnode_t *move_red_to_left(rbnode_t *node)
{
    flip_color(node);
    if (node && node->right && IS_RED(node->right->left)) {
        node->right = rotate_right(node->right);
        node = rotate_left(node);
        flip_color(node);
    }
    return node;
}

static rbnode_t *move_red_to_right(rbnode_t *node)
{
    flip_color(node);
    if (node && node->left && IS_RED(node->left->left)) {
        node = rotate_right(node);
        flip_color(node);
    }
    return node;
}

static bool insert_node(rbnode_t *node, metadata_t *new)
{
    size_t i = 0;
    metadata_t **tmp = node->tab_values;
    size_t size = node->size_tab;
    if (node->n_active == size) {
        i = node->n_active;
        if (!resize_tab_values(tmp, node))
            return false;
    } else {
        while (i < size && tmp[i])
            i++;
    }
    node->n_active++;
    ;
    node->tab_values[i] = new;
    return true;
}

static rbnode_t *insert_this(rbnode_t *node, metadata_t *new)
{
    if (!node)
        return new_rbtree(new);

    int res = MY_COMPARE(new->size, node->key);
    if (res == 0) {
        if (!insert_node(node, new))
            return NULL;
    } else if (res < 0)
        node->left = insert_this(node->left, new);
    else
        node->right = insert_this(node->right, new);
    if (IS_RED(node->right) && !IS_RED(node->left))
        node = rotate_left(node);
    if (IS_RED(node->left) && IS_RED(node->left->left))
        node = rotate_right(node);
    if (IS_RED(node->left) && IS_RED(node->right))
        flip_color(node);
    return node;
}

rbnode_t *insert_in_freed_list(rbnode_t *node, metadata_t *new)
{
    node = insert_this(node, new);
    if (node)
        node->color = BLACK;
    new->free = YFREE;
    return node;
}

static rbnode_t g_rbnode_basis = {
    0, YFREE, NULL, NULL, 0, NULL, SIZE_TAB_VALUES, 1, RED, NULL, NULL,
};

static void *alloc_tab(size_t size)
{
    void *new;
    size_t true_size = ALIGN_BYTES(META_SIZE + (sizeof(new) * size));
    new = get_heap(true_size);
    if (new)
        memset(GET_PAYLOAD(new), 0, true_size - META_SIZE);
    return new;
}

static rbnode_t *new_rbtree(metadata_t *node)
{
    rbnode_t *new;
    if (!(new = get_heap(ALIGN_BYTES(sizeof(*new)))))
        return NULL;
    memcpy(&(new->size_tab), &(g_rbnode_basis.size_tab), sizeof(size_t) * 5);
    new->key = node->size;
    if ((new->tab_values = GET_PAYLOAD(alloc_tab(SIZE_TAB_VALUES))) ==
        (void *) META_SIZE)
        return NULL;
    new->tab_values[0] = node;
    return new;
}

static bool resize_tab_values(metadata_t **old, rbnode_t *node)
{
    metadata_t **new;
    size_t size = (node->size_tab) << 1;
    if ((new = GET_PAYLOAD(alloc_tab(size))) == (void *) META_SIZE)
        return false;
    memcpy(new, old, (size >> 1) * sizeof(*new));
    ((metadata_t *) GET_NODE(node->tab_values))->free = YFREE;
    node->tab_values = new;
    node->size_tab = size;
    return true;
}

static rbnode_t *remove_min(rbnode_t *node)
{
    if (!node)
        return NULL;
    if (!node->left) {
        node->meta.free = YFREE;
        return NULL;
    }
    if (!IS_RED(node->left) && !IS_RED(node->left->left))
        node = move_red_to_left(node);
    node->left = remove_min(node->left);
    return balance(node);
}

static inline rbnode_t *min(rbnode_t *node)
{
    if (!node)
        return NULL;

    while (node->left)
        node = node->left;
    return node;
}

static rbnode_t *remove_key(rbnode_t *node, t_key key)
{
    if (!node)
        return NULL;

    rbnode_t *tmp = NULL;
    if (MY_COMPARE(key, node->key) == -1) {
        if (node->left) {
            if (!IS_RED(node->left) && !IS_RED(node->left->left))
                node = move_red_to_left(node);
            node->left = remove_key(node->left, key);
        }
    } else if (!(node = remove_node(node, key, tmp)))
        return NULL;
    return balance(node);
}

static rbnode_t *remove_node(rbnode_t *node, t_key key, rbnode_t *tmp)
{
    if (IS_RED(node->left))
        node = rotate_right(node);
    if (!MY_COMPARE(key, node->key) && !node->right) {
        node->meta.free = YFREE;
        return NULL;
    }
    if (node->right) {
        if (!IS_RED(node->right) && !IS_RED(node->right->left))
            node = move_red_to_right(node);
        if (!MY_COMPARE(key, node->key)) {
            tmp = min(node->right);
            node->tab_values = tmp->tab_values;
            node->size_tab = tmp->size_tab;
            node->key = tmp->key;
            node->right = remove_min(node->right);
            node->n_active = tmp->n_active;
        } else
            node->right = remove_key(node->right, key);
    }
    return node;
}

static rbnode_t *remove_k(rbnode_t *node, t_key key)
{
    node = remove_key(node, key);
    if (node)
        node->color = BLACK;
    return node;
}

static rbnode_t *get_key(rbnode_t *node, t_key key)
{
    while (node) {
        int cmp;
        if (!(cmp = MY_COMPARE(key, node->key)))
            return (node);
        node = ((cmp < 0) ? node->left : node->right);
    }
    return NULL;
}

rbnode_t *remove_from_freed_list(rbnode_t *node, metadata_t *meta)
{
    rbnode_t *tmp;
    if ((tmp = get_key(node, meta->size))) {
        meta->free = NFREE;
        metadata_t **tab = tmp->tab_values;
        size_t size = tmp->size_tab;
        for (size_t i = 0; i < size; i++) {
            if (tab[i] == meta) {
                tab[i] = NULL;
                tmp->n_active--;
                if (tmp->n_active == 0)
                    return remove_k(node, meta->size);
                return node;
            }
        }
    }
    return NULL;
}