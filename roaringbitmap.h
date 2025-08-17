#ifndef __ROARINGBITMAP_H__
#define __ROARINGBITMAP_H__

/* Created by ZEROMAX on 2017/3/20.*/

#include "postgres.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "fmgr.h"
#include "catalog/pg_type.h"
#include "utils/builtins.h"
#include "utils/array.h"
#include "utils/bytea.h"
#include "utils/memutils.h"
#include "utils/guc.h"
#include "lib/stringinfo.h"
#include "funcapi.h"
#include "libpq/pqformat.h"

/* must include "roaring.h" before redefine malloc functions */
#include "roaring.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

bool ArrayContainsNulls(ArrayType *array);

/* useful macros for accessing int4 arrays */
#define ARRPTR(x)  ( (int *) ARR_DATA_PTR(x) )
#define ARRNELEMS(x)  ArrayGetNItems(ARR_NDIM(x), ARR_DIMS(x))

/* reject arrays we can't handle; to wit, those containing nulls */
#define CHECKARRVALID(x) \
    do { \
        if (ARR_HASNULL(x) && ArrayContainsNulls(x)) \
            ereport(ERROR, \
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), \
                     errmsg("array must not contain nulls"))); \
    } while(0)

#define ARRISEMPTY(x)  (ARRNELEMS(x) == 0)

/* Malloc a buffer of size + alignment bytes and returns the aligned part.
The offset between the real pointer and returned value was stored in p[-1].
*/
static inline void *pg_aligned_malloc(size_t alignment, size_t size) {
    void *p;
    void *porg;
    assert(alignment <= 256);
    porg = palloc(size + alignment);
    p = (void *)((((size_t)porg + alignment) / alignment) * alignment);
    *((unsigned char *)p-1) = (unsigned char)((size_t)p - (size_t)porg);
    return p;
}

static inline void pg_aligned_free(void *memblock) {
    void *porg;
    if (memblock == NULL)
        return;
    porg = (void *)((size_t)memblock - *((unsigned char *)memblock-1));
    if (porg == memblock)
        porg = (void *)((size_t)porg - 256);
    pfree(porg);
}

static inline void *pg_calloc(size_t nmemb, size_t size) {
    return palloc0(nmemb * size);
}

static inline void pg_free(void *ptr) {
    if (ptr == NULL) {
        free(ptr);
    } else {
        pfree(ptr);
    }
}

static inline void *pg_realloc(void *ptr, size_t size) {
    return ptr == NULL ? palloc(size) : repalloc(ptr, size);
}

#include "roaring.c"
#include "roaring_buffer_reader.c"

#endif
