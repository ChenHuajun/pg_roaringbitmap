/**
 * Type-independent common code shared between the 32-bit and 64-bit
 * implementations of rb_group_elements_by_source.
 *
 * Typical usage in a .c file:
 *
 *   #define RB_GROUP_BY_SOURCE_HASH_PREFIX        roaring_group_by_source_group
 *   #define RB_GROUP_BY_SOURCE_HASH_MEMBERS_TYPE  roaring_bitmap_t *
 *   #define RB_GROUP_BY_SOURCE_HASH_BULK_CTX_TYPE roaring_bulk_context_t
 *   #include "roaring_group_by_source_common.h"
 */

#ifndef ROARING_GROUP_BY_SOURCE_COMMON_H
#define ROARING_GROUP_BY_SOURCE_COMMON_H

#include "postgres.h"

#include <stdint.h>
#include <string.h>

#include "catalog/pg_type.h"
#include "utils/array.h"

#include "roaring.h"

/**
 * Heap implementation for k-way merge.
 */
typedef struct roaring_group_by_source_heap_node_s {
    int src;        // 0-based index of iterator into the input bitmap array
    uint64_t value; // iterator's current value
} roaring_group_by_source_heap_node_t;

static inline void roaring_group_by_source_heap_sift_down(
    roaring_group_by_source_heap_node_t *heap, int size, int idx) {
    for (;;) {
        int left = (idx << 1) + 1;
        if (left >= size)
            break;
        int right = left + 1;
        int smallest = left;
        if (right < size && heap[right].value < heap[left].value)
            smallest = right;
        if (!(heap[smallest].value < heap[idx].value))
            break;
        roaring_group_by_source_heap_node_t tmp = heap[idx];
        heap[idx] = heap[smallest];
        heap[smallest] = tmp;
        idx = smallest;
    }
}

static inline void
roaring_group_by_source_heap_build(roaring_group_by_source_heap_node_t *heap,
                                   int size) {
    for (int i = (size >> 1) - 1; i >= 0; i--)
        roaring_group_by_source_heap_sift_down(heap, size, i);
}

/**
 * private_data carried on the simplehash table.
 */
typedef struct roaring_group_by_source_group_private_s {
    int nwords;
} roaring_group_by_source_group_private_t;

/**
 * Convert a packed bitmask into a Postgres ArrayType containing the 1-based
 * indices of set bits. Used to emit the 'sources int[]' column.
 */
static inline ArrayType *
roaring_group_by_source_bitmask_to_sources_array(const uint64_t *key,
                                                 int nwords) {
    int max_sources = nwords * 64;
    Datum *src_buf = (Datum *)palloc(max_sources * sizeof(Datum));
    int nsources = 0;
    for (int w = 0; w < nwords; w++) {
        uint64_t v = key[w];
        int base = w * 64;
        while (v) {
            int bitpos = roaring_trailing_zeroes(v);
            src_buf[nsources++] = Int32GetDatum(base + bitpos + 1);
            v &= v - 1;
        }
    }
    ArrayType *src_array =
        construct_array(src_buf, nsources, INT4OID, sizeof(int32_t), true, 'i');
    pfree(src_buf);
    return src_array;
}

static inline uint32_t roaring_group_by_source_hash_key(const uint64_t *words,
                                                        int nwords) {
    uint64_t h = 0;
    for (int i = 0; i < nwords; i++) {
        h ^= words[i];
        h ^= h >> 30;
        h *= 0xbf58476d1ce4e5b9ULL;
        h ^= h >> 27;
        h *= 0x94d049bb133111ebULL;
        h ^= h >> 31;
    }
    return (uint32_t)h;
}

#endif /* ROARING_GROUP_BY_SOURCE_COMMON_H */

/**
 * Re-includable simplehash template, parameterised on:
 *
 *   RB_GROUP_BY_SOURCE_HASH_PREFIX         – the SH_PREFIX
 *   RB_GROUP_BY_SOURCE_HASH_MEMBERS_TYPE   – type of the members-bitmap
 *                                            pointer field (e.g.
 *                                            roaring_bitmap_t * or
 *                                            roaring64_bitmap_t *)
 *   RB_GROUP_BY_SOURCE_HASH_BULK_CTX_TYPE  – type of the bulk-add context
 *                                            field
 *
 * Each consuming translation unit predefines these three parameters and
 * then includes this header. This part emits the entry struct, sets
 * up the SH_* macros, includes lib/simplehash.h, and #undefs all three
 * parameter macros after.
 */
#ifdef RB_GROUP_BY_SOURCE_HASH_PREFIX

#ifndef RB_GROUP_BY_SOURCE_HASH_MEMBERS_TYPE
#error "roaring_group_by_source_common.h: RB_GROUP_BY_SOURCE_HASH_MEMBERS_TYPE must be defined before include"
#endif
#ifndef RB_GROUP_BY_SOURCE_HASH_BULK_CTX_TYPE
#error "roaring_group_by_source_common.h: RB_GROUP_BY_SOURCE_HASH_BULK_CTX_TYPE must be defined before include"
#endif

/* Helpers to construct the entry-type identifier as <prefix>_entry_t. */
#define RB_GROUP_BY_SOURCE_HASH_PASTE_(a, b) a##b
#define RB_GROUP_BY_SOURCE_HASH_PASTE(a, b) RB_GROUP_BY_SOURCE_HASH_PASTE_(a, b)
#define RB_GROUP_BY_SOURCE_HASH_ENTRY_T                                        \
    RB_GROUP_BY_SOURCE_HASH_PASTE(RB_GROUP_BY_SOURCE_HASH_PREFIX, _entry_t)

typedef struct {
    uint64_t *key; /* palloc'd bitmask of input bitmap indexes */
    RB_GROUP_BY_SOURCE_HASH_MEMBERS_TYPE members;
    RB_GROUP_BY_SOURCE_HASH_BULK_CTX_TYPE bulk_ctx;
    char status; /* required by simplehash */
} RB_GROUP_BY_SOURCE_HASH_ENTRY_T;

#define SH_PREFIX RB_GROUP_BY_SOURCE_HASH_PREFIX
#define SH_ELEMENT_TYPE RB_GROUP_BY_SOURCE_HASH_ENTRY_T
#define SH_KEY_TYPE uint64_t *
#define SH_KEY key
#define SH_HASH_KEY(tb, k)                                                     \
    roaring_group_by_source_hash_key(                                          \
        (k), ((roaring_group_by_source_group_private_t *)(tb)->private_data)   \
                 ->nwords)
#define SH_EQUAL(tb, a, b)                                                     \
    (memcmp((a), (b),                                                          \
            ((roaring_group_by_source_group_private_t *)(tb)->private_data)    \
                    ->nwords *                                                 \
                sizeof(uint64_t)) == 0)
#define SH_SCOPE static inline
#define SH_DECLARE
#define SH_DEFINE
#include "lib/simplehash.h"

#undef RB_GROUP_BY_SOURCE_HASH_PREFIX
#undef RB_GROUP_BY_SOURCE_HASH_MEMBERS_TYPE
#undef RB_GROUP_BY_SOURCE_HASH_BULK_CTX_TYPE
#undef RB_GROUP_BY_SOURCE_HASH_PASTE_
#undef RB_GROUP_BY_SOURCE_HASH_PASTE
#undef RB_GROUP_BY_SOURCE_HASH_ENTRY_T

#endif /* RB_GROUP_BY_SOURCE_HASH_PREFIX */
