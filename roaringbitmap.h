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

#include "roaring.h"
#include "roaring_buffer_reader.h"
#include "roaring64_buffer_reader.h"

bool ArrayContainsNulls(ArrayType *array);

/* useful macros for accessing int4 and int8 arrays */
#define ARRPTR(x)  (ARR_DATA_PTR(x))
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

/* GUC variable for output format */
typedef enum
{
    RBITMAP_OUTPUT_ARRAY,                /* output as int array */
    RBITMAP_OUTPUT_BYTEA                 /* output as bytea */
}RBITMAPOutputFormat;

extern int    rbitmap_output_format;     /* output format */

/*
 * rb_range() and rb64_range() either copy the in_card members in range into a
 * new bitmap, about one step per member, or remove the out_card members outside
 * it from the input, about one step per container. Return true when copying is
 * cheaper, estimating the containers outside the range from the members per
 * container inside it; span is the number of 2^16 blocks the range covers.
 */
static inline bool
rb_range_copy_is_cheaper(uint64 in_card, uint64 out_card, uint64 span)
{
    return in_card == 0 ||
        (double) in_card * in_card < (double) out_card * Min(span, in_card);
}

#endif
