#ifndef ROARING64_GROUP_BY_SOURCE_H
#define ROARING64_GROUP_BY_SOURCE_H

/**
 * 64-bit variant of rb_group_elements_by_source.  Mirrors the API of
 * roaring_group_by_source.h but operates over roaring64_bitmap_t inputs.
 */

#include "postgres.h"

#include "funcapi.h"
#include "utils/array.h"

typedef struct roaring64_group_by_source_state_s
    roaring64_group_by_source_state_t;

/*
 * Deserialise the input bitmap array, run the k-way merge grouping
 * elements by source-set, and return an initialised
 * roaring64_group_by_source_state_t ready for repeated
 * roaring64_group_by_source_next_row() calls.  Must be called inside
 * SRF_IS_FIRSTCALL(), with funcctx already initialised.
 */
roaring64_group_by_source_state_t *roaring64_group_by_source_build_state(
    ArrayType *arr, FuncCallContext *funcctx, FunctionCallInfo fcinfo);

/*
 * Fetch the next (sources int[], members roaringbitmap64) row from
 * state.  Returns a HeapTuple, or NULL when exhausted.
 */
HeapTuple roaring64_group_by_source_next_row(
    roaring64_group_by_source_state_t *state);

#endif
