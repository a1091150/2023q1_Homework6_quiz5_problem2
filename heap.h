#ifndef __HEAP
#define __HEAP
#include <stddef.h>
#include <stdint.h>
typedef struct metadata {
    size_t size;
    size_t free;
    struct metadata *next, *prev;
} metadata_t;

#if __SIZE_WIDTH__ == 64
#define YFREE 0xDEADBEEF5EBA571E
#define NFREE 0x5EBA571EDEADBEEF
#define ALIGN_BYTES(x) ((((x - 1) >> 4) << 4) + 16)
#else
#define YFREE 0x5EBA571E
#define NFREE 0xDEADBEEF
#define ALIGN_BYTES(x) ((((x - 1) >> 3) << 3) + 8)
#endif

#define SIZE_TAB_VALUES (256)
#define META_SIZE ALIGN_BYTES((sizeof(metadata_t)))
#define GET_PAYLOAD(x) ((void *) ((size_t) x + META_SIZE))
#define GET_NODE(x) ((void *) ((size_t) x - META_SIZE))
#define SIZE_DEFAULT_BLOCK (32)
#define IS_VALID(x) \
    (((metadata_t *) x)->free == YFREE || ((metadata_t *) x)->free == NFREE)


void *get_heap(size_t size);
void change_break(metadata_t *node);
metadata_t *fusion(metadata_t *first, metadata_t *second);
#endif