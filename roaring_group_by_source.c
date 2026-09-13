/**
 * Implements rb_group_elements_by_source(bitmaps roaringbitmap[]).
 *
 * Given N bitmaps, performs a k-way merge and groups elements by which
 * combination of input bitmaps contains them.
 *
 * Returns a set of rows with the following columns:
 *   sources int[]         – 1-based indices of input bitmaps
 *   members roaringbitmap – all elements contained in this combination of input
 *                           bitmaps
 */

#include "roaring_group_by_source.h"

#include <stdint.h>

#include "utils/lsyscache.h"

#define RB_GROUP_BY_SOURCE_HASH_PREFIX roaring_group_by_source_group
#define RB_GROUP_BY_SOURCE_HASH_MEMBERS_TYPE roaring_bitmap_t *
#define RB_GROUP_BY_SOURCE_HASH_BULK_CTX_TYPE roaring_bulk_context_t
#include "roaring_group_by_source_common.h"

/**
 * Internal state for roaring_group_by_source_next_row()
 */
struct roaring_group_by_source_state_s {
    roaring_group_by_source_group_hash *ht;
    roaring_group_by_source_group_iterator iter;
    int nwords;
    TupleDesc tupdesc;
};

/**
 * roaring_group_by_source_deserialize:
 * deserialises each non-null element of the Datum array into bitmaps[].
 * NULL slots are left as NULL in bitmaps[].
 */
static void roaring_group_by_source_deserialize(int nelems,
                                                const Datum *elem_values,
                                                const bool *elem_nulls,
                                                roaring_bitmap_t **bitmaps) {
    for (int i = 0; i < nelems; i++) {
        if (elem_nulls[i])
            continue;
        bytea *data = (bytea *)PG_DETOAST_DATUM(elem_values[i]);
        bitmaps[i] = roaring_bitmap_portable_deserialize_safe(
            VARDATA(data), VARSIZE(data) - VARHDRSZ);
        if (!bitmaps[i])
            ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                            errmsg("invalid bitmap format")));
    }
}

/**
 * roaring_group_by_source_build_iterators:
 * creates an iterator for each non-null bitmap and seeds the min-heap in
 * preparation for the k-way merge.
 */
static void roaring_group_by_source_build_iterators(
    int nelems, roaring_bitmap_t **bitmaps, roaring_uint32_iterator_t **iters,
    roaring_group_by_source_heap_node_t *heap, int *heap_size_out) {
    int heap_size = 0;

    for (int i = 0; i < nelems; i++) {
        if (!bitmaps[i])
            continue;
        roaring_uint32_iterator_t *it = roaring_iterator_create(bitmaps[i]);
        iters[i] = it;
        if (it->has_value) {
            heap[heap_size].src = i;
            heap[heap_size].value = it->current_value;
            heap_size++;
        }
    }

    if (heap_size > 1)
        roaring_group_by_source_heap_build(heap, heap_size);

    *heap_size_out = heap_size;
}

/**
 * roaring_group_by_source_run_merge:
 * k-way merge loop, for each unique value, builds a bitmask of source iterators
 * containing it, looks up the bitmask in the hash table, and adds the value to
 * the corresponding members.
 */
static void roaring_group_by_source_run_merge(
    roaring_uint32_iterator_t **iters,
    roaring_group_by_source_heap_node_t *heap, int heap_size,
    roaring_group_by_source_group_hash *ht, int nwords) {
    // Reusable scratch buffer for the current element's source-set bitmask
    uint64_t *bitmask = (uint64_t *)palloc0(nwords * sizeof(uint64_t));

    while (heap_size > 0) {
        uint32_t current_val = heap[0].value;
        memset(bitmask, 0, nwords * sizeof(uint64_t));

        // Write the indexes of iters that contain current_val into the bitmask
        do {
            int src = heap[0].src;
            bitmask[src / 64] |= ((uint64_t)1) << (src % 64);

            roaring_uint32_iterator_t *it = iters[src];
            roaring_uint32_iterator_advance(it);
            if (it->has_value) {
                heap[0].value = it->current_value;
                roaring_group_by_source_heap_sift_down(heap, heap_size, 0);
            } else {
                heap[0] = heap[heap_size - 1];
                heap_size--;
                if (heap_size > 0)
                    roaring_group_by_source_heap_sift_down(heap, heap_size, 0);
            }
        } while (heap_size > 0 && heap[0].value == current_val);

        // Look up the bitmask in the hash table and add current_val to the
        // corresponding members bitmap, creating a new entry if not found
        bool found;
        roaring_group_by_source_group_entry_t *entry =
            roaring_group_by_source_group_insert(ht, bitmask, &found);
        if (!found) {
            uint64_t *key_copy = (uint64_t *)palloc(nwords * sizeof(uint64_t));
            memcpy(key_copy, bitmask, nwords * sizeof(uint64_t));
            entry->key = key_copy;
            entry->members = roaring_bitmap_create();
            memset(&entry->bulk_ctx, 0, sizeof(roaring_bulk_context_t));
        }
        roaring_bitmap_add_bulk(entry->members, &entry->bulk_ctx, current_val);
    }

    pfree(bitmask);
}

roaring_group_by_source_state_t *
roaring_group_by_source_build_state(ArrayType *arr, FuncCallContext *funcctx,
                                    FunctionCallInfo fcinfo) {
    int16_t elmlen;
    bool elmbyval;
    char elmalign;
    Oid elmtype = ARR_ELEMTYPE(arr);
    get_typlenbyvalalign(elmtype, &elmlen, &elmbyval, &elmalign);

    Datum *elem_values;
    bool *elem_nulls;
    int nelems;
    deconstruct_array(arr, elmtype, elmlen, elmbyval, elmalign, &elem_values,
                      &elem_nulls, &nelems);

    int nwords = (nelems + 63) / 64;
    if (nwords < 1)
        nwords = 1;

    int n = Max(nelems, 1);
    roaring_bitmap_t **bitmaps =
        (roaring_bitmap_t **)palloc0(sizeof(roaring_bitmap_t *) * n);
    roaring_uint32_iterator_t **iters = (roaring_uint32_iterator_t **)palloc0(
        sizeof(roaring_uint32_iterator_t *) * n);
    roaring_group_by_source_heap_node_t *heap =
        (roaring_group_by_source_heap_node_t *)palloc(
            sizeof(roaring_group_by_source_heap_node_t) * n);
    int heap_size;

    roaring_group_by_source_deserialize(nelems, elem_values, elem_nulls,
                                        bitmaps);
    pfree(elem_values);
    pfree(elem_nulls);
    roaring_group_by_source_build_iterators(nelems, bitmaps, iters, heap,
                                            &heap_size);

    roaring_group_by_source_group_private_t *priv =
        (roaring_group_by_source_group_private_t *)palloc(
            sizeof(roaring_group_by_source_group_private_t));
    priv->nwords = nwords;

    roaring_group_by_source_group_hash *ht =
        roaring_group_by_source_group_create(funcctx->multi_call_memory_ctx,
                                             256, priv);

    roaring_group_by_source_run_merge(iters, heap, heap_size, ht, nwords);

    for (int i = 0; i < nelems; i++) {
        if (iters[i])
            roaring_uint32_iterator_free(iters[i]);
        if (bitmaps[i])
            roaring_bitmap_free(bitmaps[i]);
    }
    pfree(bitmaps);
    pfree(iters);
    pfree(heap);

    TupleDesc tupdesc;
    if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
        ereport(ERROR, (errcode(ERRCODE_DATATYPE_MISMATCH),
                        errmsg("return type must be a row type")));
    BlessTupleDesc(tupdesc);

    roaring_group_by_source_state_t *state =
        (roaring_group_by_source_state_t *)palloc0(
            sizeof(roaring_group_by_source_state_t));

    state->ht = ht;
    state->nwords = nwords;
    state->tupdesc = tupdesc;
    roaring_group_by_source_group_start_iterate(ht, &state->iter);

    return state;
}

HeapTuple
roaring_group_by_source_next_row(roaring_group_by_source_state_t *state) {
    roaring_group_by_source_group_entry_t *entry =
        roaring_group_by_source_group_iterate(state->ht, &state->iter);
    if (entry == NULL)
        return NULL;

    // Convert bitmask to int[] of 1-based source indices
    ArrayType *src_array = roaring_group_by_source_bitmask_to_sources_array(
        entry->key, state->nwords);

    // Serialize the members bitmap
    size_t portable_size =
        roaring_bitmap_portable_size_in_bytes(entry->members);
    bytea *serialized = (bytea *)palloc(VARHDRSZ + portable_size);
    roaring_bitmap_portable_serialize(entry->members, VARDATA(serialized));
    SET_VARSIZE(serialized, VARHDRSZ + portable_size);

    Datum vals[2] = {PointerGetDatum(src_array), PointerGetDatum(serialized)};
    const bool nulls[2] = {false, false};

    return heap_form_tuple(state->tupdesc, vals, nulls);
}
