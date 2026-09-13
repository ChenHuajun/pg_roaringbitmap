#ifndef ROARING_GROUP_BY_SOURCE_H
#define ROARING_GROUP_BY_SOURCE_H

#include "postgres.h"

#include "funcapi.h"
#include "utils/array.h"

typedef struct roaring_group_by_source_state_s roaring_group_by_source_state_t;

/*
 * Deserialise the input bitmap array, run the k-way merge grouping elements
 * by source-set, and return an initialised roaring_group_by_source_state_t
 * ready for repeated roaring_group_by_source_next_row() calls.  Must be
 * called inside SRF_IS_FIRSTCALL(), with funcctx already initialised.
 */
roaring_group_by_source_state_t *roaring_group_by_source_build_state(
    ArrayType *arr, FuncCallContext *funcctx, FunctionCallInfo fcinfo);

/*
 * Fetch the next (sources int[], members roaringbitmap) row from state.
 * Returns a HeapTuple, or NULL when exhausted.
 */
HeapTuple roaring_group_by_source_next_row(roaring_group_by_source_state_t *state);

#endif
