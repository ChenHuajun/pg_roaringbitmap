#include "roaringbitmap.h"
#include "roaring_group_by_source.h"

#include "access/gin.h"
#include "access/stratnum.h"
#include "commands/vacuum.h"
#include "common/hashfn.h"
#include "utils/selfuncs.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

/* Created by ZEROMAX on 2017/3/20.*/

#define MAX_BITMAP_RANGE_END UINT64_C(0x100000000)
#define INT4_MIN -2147483648
#define INT4_MAX 2147483647

/* GUC variables */


static const struct config_enum_entry output_format_options[] =
{
    {"array", RBITMAP_OUTPUT_ARRAY, false},
    {"bytea", RBITMAP_OUTPUT_BYTEA, false},
    {NULL, 0, false}
};

int    rbitmap_output_format = RBITMAP_OUTPUT_BYTEA;        /* output format */

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
    /* Reject sizes whose multiplication would overflow, otherwise a bogus
     * small buffer would be handed back to CRoaring. Returning NULL is how
     * the memory hook reports allocation failure. */
    if (nmemb != 0 && size > SIZE_MAX / nmemb)
        return NULL;
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

static roaring_memory_t rb_memory_hook = {
    .malloc = palloc,
    .realloc = pg_realloc,
    .calloc = pg_calloc,
    .free = pg_free,
    .aligned_malloc = pg_aligned_malloc,
    .aligned_free = pg_aligned_free,
};

void        _PG_init(void);
/*
 * Module load callback
 */
void
_PG_init(void)
{
    /* Define custom GUC variables. */
    DefineCustomEnumVariable("roaringbitmap.output_format",
                             "Selects output format of roaringbitmap.",
                             NULL,
                             &rbitmap_output_format,
                             RBITMAP_OUTPUT_BYTEA,
                             output_format_options,
                             PGC_USERSET,
                             0,
                             NULL,
                             NULL,
                             NULL);
    roaring_init_memory_hook(rb_memory_hook);
}


bool
ArrayContainsNulls(ArrayType *array) {
    int nelems;
    uint8 *bitmap;
    int bitmask;

    /* Easy answer if there's no null bitmap */
    if (!ARR_HASNULL(array))
        return false;

    nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

    bitmap = ARR_NULLBITMAP(array);

    /* check whole bytes of the bitmap byte-at-a-time */
    while (nelems >= 8) {
        if (*bitmap != 0xFF)
            return true;
        bitmap++;
        nelems -= 8;
    }

    /* check last partial byte */
    bitmask = 1;
    while (nelems > 0) {
        if ((*bitmap & bitmask) == 0)
            return true;
        bitmask <<= 1;
        nelems--;
    }

    return false;
}

/*
 * Deserialize a roaringbitmap varlena into a roaring_bitmap_t, raising an
 * error on malformed input.  The caller owns the returned bitmap and must
 * roaring_bitmap_free() it.
 */
static roaring_bitmap_t *
rb_bitmap_deserialize(bytea *data)
{
    roaring_bitmap_t *r;

    r = roaring_bitmap_portable_deserialize_safe(VARDATA(data),
                                                 VARSIZE(data) - VARHDRSZ);
    if (!r)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    return r;
}


//rb_group_elements_by_source
Datum rb_group_elements_by_source(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(rb_group_elements_by_source);

Datum
rb_group_elements_by_source(PG_FUNCTION_ARGS)
{
    FuncCallContext *funcctx;
    MemoryContext    oldcontext;

    if (SRF_IS_FIRSTCALL())
    {
        if (PG_ARGISNULL(0))
        {
            funcctx = SRF_FIRSTCALL_INIT();
            SRF_RETURN_DONE(funcctx);
        }

        ArrayType *arr = PG_GETARG_ARRAYTYPE_P(0);
        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

        funcctx->user_fctx = roaring_group_by_source_build_state(arr, funcctx, fcinfo);

        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();

    HeapTuple tuple = roaring_group_by_source_next_row((roaring_group_by_source_state_t *) funcctx->user_fctx);
    if (tuple == NULL)
        SRF_RETURN_DONE(funcctx);

    SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
}

//rb_from_bytea
Datum rb_from_bytea(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(rb_from_bytea);

Datum
rb_from_bytea(PG_FUNCTION_ARGS) {
    bytea *serializedbytes = PG_GETARG_BYTEA_P(0);
    roaring_bitmap_t *r1;
    size_t expectedsize;
    const char *reason;

    r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes), VARSIZE(serializedbytes) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if(!roaring_bitmap_internal_validate(r1, &reason)) {
        roaring_bitmap_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("bitmap format is error: %s", reason)));
    }

    expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}


//roaringbitmap_in
Datum roaringbitmap_in(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(roaringbitmap_in);

Datum
roaringbitmap_in(PG_FUNCTION_ARGS) {
    char       *ptr = PG_GETARG_CSTRING(0);
    long        l;
    char       *badp;
    roaring_bitmap_t *r1;
    size_t expectedsize;
    bytea *serializedbytes;
    Datum dd;
    const char *reason;

    if(*ptr == '\\' && *(ptr+1) == 'x') {
       /* bytea input */
        dd = DirectFunctionCall1(byteain, PG_GETARG_DATUM(0));

        serializedbytes = DatumGetByteaP(dd);
        r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes), VARSIZE(serializedbytes) - VARHDRSZ);
        if (!r1)
            ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                     errmsg("bitmap format is error")));

        if(!roaring_bitmap_internal_validate(r1, &reason)) {
            roaring_bitmap_free(r1);
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                     errmsg("bitmap format is error: %s", reason)));
        }

        expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
        serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
        roaring_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
        roaring_bitmap_free(r1);

        SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
        PG_RETURN_BYTEA_P(serializedbytes);
    }
    /* else int array input */

    /* Find the head char '{' */
    while (*ptr && isspace((unsigned char) *ptr))
            ptr++;
    if (*ptr !='{')
        ereport(ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
             errmsg("malformed bitmap literal")));
    ptr++;

    r1 = roaring_bitmap_create();

    while (*ptr && isspace((unsigned char) *ptr))
        ptr++;

    if (*ptr != '}') {
        while (*ptr) {
            /* Parse int element */
            errno = 0;
            l = strtol(ptr, &badp, 10);

            /* We made no progress parsing the string, so bail out */
            if (ptr == badp){
                roaring_bitmap_free(r1);
                ereport(ERROR,
                    (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                     errmsg("invalid input syntax for %s: \"%s\"",
                            "integer", ptr)));
            }

            if (errno == ERANGE
                || l < INT4_MIN || l > INT4_MAX
                ){
                    roaring_bitmap_free(r1);
                    ereport(ERROR,
                            (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
                             errmsg("value \"%s\" is out of range for type %s", ptr,
                                    "integer")));
                }

            /* Add int element to bitmap */
            roaring_bitmap_add(r1, l);

            /* Skip any trailing whitespace after the int element */
            ptr = badp;
            while (*ptr && isspace((unsigned char) *ptr))
                ptr++;

            /* Find the element terminator ',' */
            if (*ptr != ',')
                break;
            ptr++;

            /* Skip any trailing whitespace after the terminator */
            while (*ptr && isspace((unsigned char) *ptr))
                ptr++;
        }

        /* Find the tail char '}' */
        if (*ptr !='}'){
            roaring_bitmap_free(r1);
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("malformed bitmap literal")));
        }
    }

    /* Check if input end */
    ptr++;
    while (*ptr && isspace((unsigned char) *ptr))
        ptr++;

    if (*ptr !='\0'){
        roaring_bitmap_free(r1);
        ereport(ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
             errmsg("malformed bitmap literal")));
    }

    expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//roaringbitmap_out
Datum roaringbitmap_out(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(roaringbitmap_out);

Datum
roaringbitmap_out(PG_FUNCTION_ARGS) {
    bytea *serializedbytes;
    roaring_uint32_iterator_t iterator;
    StringInfoData buf;
    roaring_bitmap_t *r1;
    
    if(rbitmap_output_format == RBITMAP_OUTPUT_BYTEA){
        return DirectFunctionCall1(byteaout, PG_GETARG_DATUM(0));
    }
    
    serializedbytes = PG_GETARG_BYTEA_P(0);
    r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes), VARSIZE(serializedbytes) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    initStringInfo(&buf);

    appendStringInfoChar(&buf, '{');

    roaring_iterator_init(r1, &iterator);
    if(iterator.has_value) {
        appendStringInfo(&buf, "%d", (int)iterator.current_value);
        roaring_uint32_iterator_advance(&iterator);

        while(iterator.has_value) {
            appendStringInfo(&buf, ",%d", (int)iterator.current_value);
            roaring_uint32_iterator_advance(&iterator);
        }
    }

    appendStringInfoChar(&buf, '}');

    roaring_bitmap_free(r1);

    PG_RETURN_CSTRING(buf.data);
}

//roaringbitmap_recv
Datum roaringbitmap_recv(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(roaringbitmap_recv);

Datum
roaringbitmap_recv(PG_FUNCTION_ARGS) {
    StringInfo    buf = (StringInfo) PG_GETARG_POINTER(0);
    int           nbytes;
    bytea *serializedbytes;
    roaring_bitmap_t *r1;
    size_t expectedsize;
    const char *reason;

    nbytes = buf->len - buf->cursor;
    if (nbytes <= 0)
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
                 errmsg("empty roaring bitmap binary data")));

    r1 = roaring_bitmap_portable_deserialize_safe(pq_getmsgbytes(buf, nbytes), nbytes);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if(!roaring_bitmap_internal_validate(r1, &reason)) {
        roaring_bitmap_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("bitmap format is error: %s", reason)));
    }

    expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}


//roaringbitmap_send
Datum roaringbitmap_send(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(roaringbitmap_send);

Datum
roaringbitmap_send(PG_FUNCTION_ARGS) {
    bytea *bp = PG_GETARG_BYTEA_P(0);
    StringInfoData buf;

    pq_begintypsend(&buf);
    pq_sendbytes(&buf, VARDATA(bp), VARSIZE(bp) - VARHDRSZ);
    PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}


//bitmap_or
PG_FUNCTION_INFO_V1(rb_or);
Datum rb_or(PG_FUNCTION_ARGS);

Datum
rb_or(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring_bitmap_t *r1;
    roaring_bitmap_t *r2;
    size_t expectedsize;
    bytea *serializedbytes;

    r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes2), VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring_bitmap_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }
    roaring_bitmap_or_inplace(r1, r2);
    roaring_bitmap_free(r2);
    expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}


//bitmap_or_cardinality
PG_FUNCTION_INFO_V1(rb_or_cardinality);
Datum rb_or_cardinality(PG_FUNCTION_ARGS);

Datum
rb_or_cardinality(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring_buffer_t *r1;
    roaring_buffer_t *r2;
    uint64 card1;
    bool ret;

    r1 = roaring_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring_buffer_or_cardinality(r1, r2, &card1);
    roaring_buffer_free(r1);
    roaring_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_INT64(card1);
}

//bitmap_and
PG_FUNCTION_INFO_V1(rb_and);
Datum rb_and(PG_FUNCTION_ARGS);

Datum
rb_and(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring_buffer_t *r1;
    roaring_buffer_t *r2;
    roaring_bitmap_t *r;
    size_t expectedsize;
    bytea *serializedbytes;

    r1 = roaring_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    r = roaring_buffer_and(r1, r2);
    roaring_buffer_free(r1);
    roaring_buffer_free(r2);
    if (!r) {
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    expectedsize = roaring_bitmap_portable_size_in_bytes(r);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r, VARDATA(serializedbytes));
    roaring_bitmap_free(r);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}


//bitmap_and_cardinality
PG_FUNCTION_INFO_V1(rb_and_cardinality);
Datum rb_and_cardinality(PG_FUNCTION_ARGS);

Datum
rb_and_cardinality(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring_buffer_t *r1;
    roaring_buffer_t *r2;
    uint64 card1;
    bool ret;

    r1 = roaring_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring_buffer_and_cardinality(r1, r2, &card1);
    roaring_buffer_free(r1);
    roaring_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_INT64(card1);
}


//bitmap_andnot
PG_FUNCTION_INFO_V1(rb_andnot);
Datum rb_andnot(PG_FUNCTION_ARGS);

Datum
rb_andnot(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring_buffer_t *r1;
    roaring_buffer_t *r2;
    roaring_bitmap_t *r;
    size_t expectedsize;
    bytea *serializedbytes;

    r1 = roaring_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    r = roaring_buffer_andnot(r1, r2);
    roaring_buffer_free(r1);
    roaring_buffer_free(r2);
    if (!r) {
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    expectedsize = roaring_bitmap_portable_size_in_bytes(r);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r, VARDATA(serializedbytes));
    roaring_bitmap_free(r);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}


//bitmap_andnot_cardinality
PG_FUNCTION_INFO_V1(rb_andnot_cardinality);
Datum rb_andnot_cardinality(PG_FUNCTION_ARGS);

Datum
rb_andnot_cardinality(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring_buffer_t *r1;
    roaring_buffer_t *r2;
    uint64 card1;
    bool ret;

    r1 = roaring_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring_buffer_andnot_cardinality(r1, r2, &card1);
    roaring_buffer_free(r1);
    roaring_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_INT64(card1);
}


//bitmap_xor
PG_FUNCTION_INFO_V1(rb_xor);
Datum rb_xor(PG_FUNCTION_ARGS);

Datum
rb_xor(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring_bitmap_t *r1;
    roaring_bitmap_t *r2;
    size_t expectedsize;
    bytea *serializedbytes;

    r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes2), VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring_bitmap_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    roaring_bitmap_xor_inplace(r1, r2);
    roaring_bitmap_free(r2);
    expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}


//bitmap_xor_cardinality
PG_FUNCTION_INFO_V1(rb_xor_cardinality);
Datum rb_xor_cardinality(PG_FUNCTION_ARGS);

Datum
rb_xor_cardinality(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring_buffer_t *r1;
    roaring_buffer_t *r2;
    uint64 card1;
    bool ret;

    r1 = roaring_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring_buffer_xor_cardinality(r1, r2, &card1);
    roaring_buffer_free(r1);
    roaring_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_INT64(card1);
}


//bitmap cardinality
PG_FUNCTION_INFO_V1(rb_cardinality);
Datum rb_cardinality(PG_FUNCTION_ARGS);

Datum
rb_cardinality(PG_FUNCTION_ARGS) {
    bytea *data = PG_GETARG_BYTEA_P(0);
    roaring_buffer_t *r1;
    uint64 card;

    r1 = roaring_buffer_create(VARDATA(data),
                               VARSIZE(data) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    card = roaring_buffer_get_cardinality(r1);
    roaring_buffer_free(r1);

    PG_RETURN_INT64(card);
}


//bitmap is empty
PG_FUNCTION_INFO_V1(rb_is_empty);
Datum rb_is_empty(PG_FUNCTION_ARGS);

Datum
rb_is_empty(PG_FUNCTION_ARGS) {
    bytea *data = PG_GETARG_BYTEA_P(0);
    roaring_buffer_t *r1;
    bool isempty;

    r1 = roaring_buffer_create(VARDATA(data),
                               VARSIZE(data) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    isempty = roaring_buffer_is_empty(r1);
    roaring_buffer_free(r1);

    PG_RETURN_BOOL(isempty);
}

//bitmap contains one value
PG_FUNCTION_INFO_V1(rb_exists);
Datum rb_exists(PG_FUNCTION_ARGS);

Datum
rb_exists(PG_FUNCTION_ARGS) {
    bytea *data = PG_GETARG_BYTEA_P(0);
    uint32 value = PG_GETARG_UINT32(1);
    roaring_buffer_t *r1;
    bool isexist;
    bool ret;

    r1 = roaring_buffer_create(VARDATA(data),
                               VARSIZE(data) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    ret = roaring_buffer_contains(r1, value, &isexist);
    roaring_buffer_free(r1);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_BOOL(isexist);
}

//rb_exsit is renamed to rb_exists in v1.0
//keep the old rb_exsit definition for backward compatible
PG_FUNCTION_INFO_V1(rb_exsit);
Datum rb_exsit(PG_FUNCTION_ARGS);

Datum
rb_exsit(PG_FUNCTION_ARGS) {
    return rb_exists(fcinfo);
}

//bitmap equals
PG_FUNCTION_INFO_V1(rb_equals);
Datum rb_equals(PG_FUNCTION_ARGS);

Datum
rb_equals(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring_buffer_t *r1;
    roaring_buffer_t *r2;
    bool isequal;
    bool ret;

    r1 = roaring_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring_buffer_equals(r1, r2, &isequal);
    roaring_buffer_free(r1);
    roaring_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_BOOL(isequal);
}

//bitmap not equals
PG_FUNCTION_INFO_V1(rb_not_equals);
Datum rb_not_equals(PG_FUNCTION_ARGS);

Datum
rb_not_equals(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring_buffer_t *r1;
    roaring_buffer_t *r2;
    bool isequal;
    bool ret;

    r1 = roaring_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring_buffer_equals(r1, r2, &isequal);
    roaring_buffer_free(r1);
    roaring_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_BOOL(!isequal);
}

//bitmap intersect
PG_FUNCTION_INFO_V1(rb_intersect);
Datum rb_intersect(PG_FUNCTION_ARGS);

Datum
rb_intersect(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring_buffer_t *r1;
    roaring_buffer_t *r2;
    bool isintersect;
    bool ret;

    r1 = roaring_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring_buffer_intersect(r1, r2, &isintersect);
    roaring_buffer_free(r1);
    roaring_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_BOOL(isintersect);
}

//bitmap contains
PG_FUNCTION_INFO_V1(rb_contains);
Datum rb_contains(PG_FUNCTION_ARGS);

Datum
rb_contains(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring_buffer_t *r1;
    roaring_buffer_t *r2;
    bool iscontain;
    bool ret;

    r1 = roaring_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring_buffer_is_subset(r2, r1, &iscontain);
    roaring_buffer_free(r1);
    roaring_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_BOOL(iscontain);
}

//bitmap contained
PG_FUNCTION_INFO_V1(rb_containedby);
Datum rb_containedby(PG_FUNCTION_ARGS);

Datum
rb_containedby(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring_buffer_t *r1;
    roaring_buffer_t *r2;
    bool iscontained;
    bool ret;

    r1 = roaring_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring_buffer_is_subset(r1, r2, &iscontained);
    roaring_buffer_free(r1);
    roaring_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_BOOL(iscontained);
}

//bitmap jaccard distance
PG_FUNCTION_INFO_V1(rb_jaccard_dist);
Datum rb_jaccard_dist(PG_FUNCTION_ARGS);

Datum
rb_jaccard_dist(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring_buffer_t *r1;
    roaring_buffer_t *r2;
    double jaccard_dist;
    bool ret;

    r1 = roaring_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring_buffer_jaccard_index(r1, r2, &jaccard_dist);
    roaring_buffer_free(r1);
    roaring_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_FLOAT8(jaccard_dist);
}

//bitmap add
PG_FUNCTION_INFO_V1(rb_add);
Datum rb_add(PG_FUNCTION_ARGS);

Datum
rb_add(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    uint32 value = PG_GETARG_UINT32(1);
    size_t expectedsize;
    bytea *serializedbytes;

    roaring_bitmap_t *r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    roaring_bitmap_add(r1, value);

    expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap remove
PG_FUNCTION_INFO_V1(rb_remove);
Datum rb_remove(PG_FUNCTION_ARGS);

Datum
rb_remove(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    uint32 value = PG_GETARG_UINT32(1);
    size_t expectedsize;
    bytea *serializedbytes;

    roaring_bitmap_t *r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    roaring_bitmap_remove(r1, value);

    expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap minimum
PG_FUNCTION_INFO_V1(rb_min);
Datum rb_min(PG_FUNCTION_ARGS);

Datum
rb_min(PG_FUNCTION_ARGS) {
    bytea *data = PG_GETARG_BYTEA_P(0);
    roaring_buffer_t *r1;
    uint32 min;
    bool ret;

    r1 = roaring_buffer_create(VARDATA(data),
                               VARSIZE(data) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if(roaring_buffer_is_empty(r1))
    {
        roaring_buffer_free(r1);
        PG_RETURN_NULL();
    }

    ret = roaring_buffer_minimum(r1, &min);
    roaring_buffer_free(r1);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_UINT32(min);
}


//bitmap maximum
PG_FUNCTION_INFO_V1(rb_max);
Datum rb_max(PG_FUNCTION_ARGS);

Datum
rb_max(PG_FUNCTION_ARGS) {
    bytea *data = PG_GETARG_BYTEA_P(0);
    roaring_buffer_t *r1;
    uint32 max;
    bool ret;

    r1 = roaring_buffer_create(VARDATA(data),
                               VARSIZE(data) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if(roaring_buffer_is_empty(r1))
    {
        roaring_buffer_free(r1);
        PG_RETURN_NULL();
    }

    ret = roaring_buffer_maximum(r1, &max);
    roaring_buffer_free(r1);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_UINT32(max);
}

//bitmap rank
PG_FUNCTION_INFO_V1(rb_rank);
Datum rb_rank(PG_FUNCTION_ARGS);

Datum
rb_rank(PG_FUNCTION_ARGS) {
    bytea *data = PG_GETARG_BYTEA_P(0);
    uint32 value = PG_GETARG_UINT32(1);
    roaring_buffer_t *r1;
    uint64 rank;
    bool ret;

    r1 = roaring_buffer_create(VARDATA(data),
                               VARSIZE(data) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    ret = roaring_buffer_rank(r1, value, &rank);
    roaring_buffer_free(r1);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_INT64((int64)rank);
}

//bitmap index
PG_FUNCTION_INFO_V1(rb_index);
Datum rb_index(PG_FUNCTION_ARGS);

Datum
rb_index(PG_FUNCTION_ARGS) {
    bytea *data = PG_GETARG_BYTEA_P(0);
    uint32 value = PG_GETARG_UINT32(1);
    roaring_buffer_t *r1;
    uint64 rank;
    int64 result;
    bool ret,isexist;

    r1 = roaring_buffer_create(VARDATA(data),
                               VARSIZE(data) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    ret = roaring_buffer_contains(r1, value, &isexist);
    if(!ret)
    {
        roaring_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    result = -1;
    if(isexist)
    {
        ret = roaring_buffer_rank(r1, value, &rank);
        roaring_buffer_free(r1);
        if(!ret)
            ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                    errmsg("bitmap format is error")));

        result = (int64)rank - 1;
    }

    PG_RETURN_INT64(result);
}

//bitmap fill
PG_FUNCTION_INFO_V1(rb_fill);
Datum rb_fill(PG_FUNCTION_ARGS);

Datum
rb_fill(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    int64 rangestart = PG_GETARG_INT64(1);
    int64 rangeend = PG_GETARG_INT64(2);
    roaring_bitmap_t *r1;
    size_t expectedsize;
    bytea *serializedbytes;

    if (rangestart < 0)
        rangestart = 0;
    if (rangeend < 0)
        rangeend = 0;
    if (rangeend > MAX_BITMAP_RANGE_END) {
        rangeend = MAX_BITMAP_RANGE_END;
    }

    r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if (rangestart < rangeend) {
        roaring_bitmap_add_range(r1,rangestart, rangeend);
    }

    expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap clear
PG_FUNCTION_INFO_V1(rb_clear);
Datum rb_clear(PG_FUNCTION_ARGS);

Datum
rb_clear(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    int64 rangestart = PG_GETARG_INT64(1);
    int64 rangeend = PG_GETARG_INT64(2);
    roaring_bitmap_t *r1;
    size_t expectedsize;
    bytea *serializedbytes;

    if (rangestart < 0)
        rangestart = 0;
    if (rangeend < 0)
        rangeend = 0;
    if (rangeend > MAX_BITMAP_RANGE_END) {
        rangeend = MAX_BITMAP_RANGE_END;
    }

    r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if (rangestart < rangeend) {
        roaring_bitmap_remove_range(r1,rangestart, rangeend);
    }

    expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap flip
PG_FUNCTION_INFO_V1(rb_flip);
Datum rb_flip(PG_FUNCTION_ARGS);

Datum
rb_flip(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    int64 rangestart = PG_GETARG_INT64(1);
    int64 rangeend = PG_GETARG_INT64(2);
    roaring_bitmap_t *r1;
    size_t expectedsize;
    bytea *serializedbytes;

    if (rangestart < 0)
        rangestart = 0;
    if (rangeend < 0)
        rangeend = 0;
    if (rangeend > MAX_BITMAP_RANGE_END) {
        rangeend = MAX_BITMAP_RANGE_END;
    }

    r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if (rangestart < rangeend) {
        roaring_bitmap_flip_inplace(r1, rangestart, rangeend);
    }

    expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap shiftright
PG_FUNCTION_INFO_V1(rb_shiftright);
Datum rb_shiftright(PG_FUNCTION_ARGS);

Datum
rb_shiftright(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    int64 distance = PG_GETARG_INT64(1);
    uint64 value;
    roaring_bitmap_t *r1;
    roaring_bitmap_t *r2;
    roaring_uint32_iterator_t iterator;
    size_t expectedsize;
    bytea *serializedbytes;

    r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if(distance != 0)
    {
        r2 = roaring_bitmap_create();
        if (!r2) {
            roaring_bitmap_free(r1);
            ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                     errmsg("failed to create bitmap")));
        }

        roaring_iterator_init(r1, &iterator);
        if(distance > 0){
            while(iterator.has_value) {
                value = iterator.current_value + distance;
                if(value >= MAX_BITMAP_RANGE_END)
                    break;
                roaring_bitmap_add(r2, (uint32)value);
                roaring_uint32_iterator_advance(&iterator);
            }
        }else{
            roaring_uint32_iterator_move_equalorlarger(&iterator, -distance);
            while(iterator.has_value) {
                value = iterator.current_value + distance;
                if(value >= MAX_BITMAP_RANGE_END)
                    break;
                roaring_bitmap_add(r2, (uint32)value);
                roaring_uint32_iterator_advance(&iterator);
            }
        }
        roaring_bitmap_free(r1);
        r1 = r2;
    }

    expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap range
PG_FUNCTION_INFO_V1(rb_range);
Datum rb_range(PG_FUNCTION_ARGS);

Datum
rb_range(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    int64 rangestart = PG_GETARG_INT64(1);
    int64 rangeend = PG_GETARG_INT64(2);
    roaring_bitmap_t *r1;
    roaring_bitmap_t *r2;
    roaring_uint32_iterator_t iterator;
    size_t expectedsize;
    bytea *serializedbytes;

    if (rangestart < 0)
        rangestart = 0;
    if (rangeend < 0)
        rangeend = 0;
    if (rangeend > MAX_BITMAP_RANGE_END) {
        rangeend = MAX_BITMAP_RANGE_END;
    }
    if (rangestart > MAX_BITMAP_RANGE_END) {
        rangestart = MAX_BITMAP_RANGE_END;
    }

    r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    
    r2 = roaring_bitmap_create();
    if (!r2) {
        roaring_bitmap_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("failed to create bitmap")));
    }

    /* rangestart >= rangeend yields an empty bitmap; it also guarantees that
     * rangestart fits in the uint32_t parameter of move_equalorlarger() */
    if (rangestart < rangeend) {
        roaring_iterator_init(r1, &iterator);
        roaring_uint32_iterator_move_equalorlarger(&iterator, rangestart);
        while(iterator.has_value) {
            if(iterator.current_value >= rangeend)
                break;
            roaring_bitmap_add(r2, iterator.current_value);
            roaring_uint32_iterator_advance(&iterator);
        }
    }

    expectedsize = roaring_bitmap_portable_size_in_bytes(r2);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r2, VARDATA(serializedbytes));
    roaring_bitmap_free(r1);
    roaring_bitmap_free(r2);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap range_cardinality
PG_FUNCTION_INFO_V1(rb_range_cardinality);
Datum rb_range_cardinality(PG_FUNCTION_ARGS);

Datum
rb_range_cardinality(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    int64 rangestart = PG_GETARG_INT64(1);
    int64 rangeend = PG_GETARG_INT64(2);
    roaring_bitmap_t *r1;
    roaring_uint32_iterator_t iterator;
    uint64 card1;

    if (rangestart < 0)
        rangestart = 0;
    if (rangeend < 0)
        rangeend = 0;
    if (rangeend > MAX_BITMAP_RANGE_END) {
        rangeend = MAX_BITMAP_RANGE_END;
    }
    if (rangestart > MAX_BITMAP_RANGE_END) {
        rangestart = MAX_BITMAP_RANGE_END;
    }

    r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    card1 = 0;
    /* rangestart >= rangeend yields 0; it also guarantees that rangestart
     * fits in the uint32_t parameter of move_equalorlarger() */
    if (rangestart < rangeend) {
        roaring_iterator_init(r1, &iterator);
        roaring_uint32_iterator_move_equalorlarger(&iterator, rangestart);
        while(iterator.has_value) {
            if(iterator.current_value >= rangeend)
                break;
            card1++;
            roaring_uint32_iterator_advance(&iterator);
        }
    }

    roaring_bitmap_free(r1);
    PG_RETURN_INT64(card1);
}

//bitmap range
PG_FUNCTION_INFO_V1(rb_select);
Datum rb_select(PG_FUNCTION_ARGS);

Datum
rb_select(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    int64 limit = PG_GETARG_INT64(1);
    int64 offset = PG_GETARG_INT64(2);
    bool reverse = PG_GETARG_BOOL(3);
    int64 rangestart = PG_GETARG_INT64(4);
    int64 rangeend = PG_GETARG_INT64(5);
    int64 count = 0;
    int64 total_count = 0;
    roaring_bitmap_t *r1;
    roaring_bitmap_t *r2;
    roaring_uint32_iterator_t iterator;
    size_t expectedsize;
    bytea *serializedbytes;

    if (rangestart < 0)
        rangestart = 0;
    if (rangeend < 0)
        rangeend = 0;
    if (rangeend > MAX_BITMAP_RANGE_END) {
        rangeend = MAX_BITMAP_RANGE_END;
    }
    if (rangestart > MAX_BITMAP_RANGE_END) {
        rangestart = MAX_BITMAP_RANGE_END;
    }

    if (offset < 0)
        offset = 0;

    r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_bitmap_create();
    if (!r2) {
        roaring_bitmap_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("failed to create bitmap")));
    }

    /* rangestart >= rangeend yields an empty bitmap; it also guarantees that
     * rangestart fits in the uint32_t parameter of move_equalorlarger() */
    if (limit > 0 && rangestart < rangeend) {
        roaring_iterator_init(r1, &iterator);
        roaring_uint32_iterator_move_equalorlarger(&iterator, rangestart);
        if (!reverse) {
            while (iterator.has_value) {
                if (iterator.current_value >= rangeend
                        || count - offset >= limit)
                    break;
                if (count >= offset) {
                    roaring_bitmap_add(r2, iterator.current_value);
                }
                roaring_uint32_iterator_advance(&iterator);
                count++;
            }
        } else {
            while (iterator.has_value) {
                if (iterator.current_value >= rangeend)
                    break;
                roaring_uint32_iterator_advance(&iterator);
                total_count++;
            }

            if (total_count > offset) {
                /* calulate new offset for reverse */
                offset = total_count - offset - limit;
                if(offset < 0)
                    offset = 0;
                roaring_iterator_init(r1, &iterator);
                roaring_uint32_iterator_move_equalorlarger(&iterator,rangestart);
                count = 0;
                while (iterator.has_value) {
                    if (iterator.current_value >= rangeend
                            || count - offset >= limit)
                        break;
                    if (count >= offset) {
                        roaring_bitmap_add(r2, iterator.current_value);
                    }
                    roaring_uint32_iterator_advance(&iterator);
                    count++;
                }
            }
        }
    }

    expectedsize = roaring_bitmap_portable_size_in_bytes(r2);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r2, VARDATA(serializedbytes));
    roaring_bitmap_free(r1);
    roaring_bitmap_free(r2);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap build
PG_FUNCTION_INFO_V1(rb_build);
Datum rb_build(PG_FUNCTION_ARGS);

Datum
rb_build(PG_FUNCTION_ARGS) {
    ArrayType *a = (ArrayType *) PG_GETARG_ARRAYTYPE_P(0);
    int na, n;
    int *da;
    roaring_bitmap_t *r1;
    size_t expectedsize;
    bytea *serializedbytes;

    CHECKARRVALID(a);

    na = ARRNELEMS(a);
    da = (int *)ARRPTR(a);

    r1 = roaring_bitmap_create();

    for (n = 0; n < na; n++) {
        roaring_bitmap_add(r1, da[n]);
    }

    expectedsize = roaring_bitmap_portable_size_in_bytes(r1);

    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap to int[]
PG_FUNCTION_INFO_V1(rb_to_array);
Datum rb_to_array(PG_FUNCTION_ARGS);

Datum
rb_to_array(PG_FUNCTION_ARGS)
{
    bytea *serializedbytes = PG_GETARG_BYTEA_P(0);
    roaring_bitmap_t *r1;
    roaring_uint32_iterator_t *iterator;
    ArrayType *result;
    Datum *out_datums;
    uint64_t card1;
    uint32_t counter = 0;

    r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes), VARSIZE(serializedbytes) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    card1 = roaring_bitmap_get_cardinality(r1);

    if (card1 == 0)
    {
        result = construct_empty_array(INT4OID);
    }
    else
    {
        out_datums = (Datum *)palloc(sizeof(Datum) * card1);

        iterator = roaring_iterator_create(r1);
        while (iterator->has_value)
        {
            out_datums[counter] = Int32GetDatum(iterator->current_value);
            counter++;
            roaring_uint32_iterator_advance(iterator);
        }
        roaring_uint32_iterator_free(iterator);

        result = construct_array(out_datums, card1, INT4OID, sizeof(int32), true, 'i');
    }

    roaring_bitmap_free(r1);
    PG_RETURN_POINTER(result);
}

//bitmap list
PG_FUNCTION_INFO_V1(rb_iterate);
Datum rb_iterate(PG_FUNCTION_ARGS);

Datum
rb_iterate(PG_FUNCTION_ARGS) {
    FuncCallContext *funcctx;
    MemoryContext oldcontext;
    roaring_uint32_iterator_t *fctx;
    bytea *data;
    roaring_bitmap_t *r1;

    if (SRF_IS_FIRSTCALL()) {

        funcctx = SRF_FIRSTCALL_INIT();

        /*
         * Fetched before the context switch below on purpose: the detoasted
         * copy then belongs to the caller's per-tuple context, which postgres
         * resets once the input tuple is done, while the deserialized bitmap
         * lives in multi_call_memory_ctx for the whole SRF stream. Moving this
         * line after the switch would pin one full copy of the input bitmap
         * for the entire scan.
         */
        data = PG_GETARG_BYTEA_P(0);

        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

        r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(data), VARSIZE(data) - VARHDRSZ);
        if (!r1)
            ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                     errmsg("bitmap format is error")));

        fctx = roaring_iterator_create(r1);

        funcctx->user_fctx = fctx;

        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();

    fctx = funcctx->user_fctx;

    if (fctx->has_value) {
        Datum result;
        result = Int32GetDatum(fctx->current_value);
        roaring_uint32_iterator_advance(fctx);
        SRF_RETURN_NEXT(funcctx, result);
    } else {
        roaring_uint32_iterator_free(fctx);
        SRF_RETURN_DONE(funcctx);
    }
}

//bitmap or trans
PG_FUNCTION_INFO_V1(rb_or_trans);
Datum rb_or_trans(PG_FUNCTION_ARGS);

Datum
rb_or_trans(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    MemoryContext oldcontext;
    bytea *bb;
    roaring_bitmap_t *r1;
    roaring_bitmap_t *r2;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb_or_trans outside transition context")));

    if (PG_ARGISNULL(1)) {
        if (PG_ARGISNULL(0)) {
            PG_RETURN_NULL();
        }
        r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);
    } else {
        bb = PG_GETARG_BYTEA_P(1);

        oldcontext = MemoryContextSwitchTo(aggctx);

        r2 = roaring_bitmap_portable_deserialize_safe(VARDATA(bb), VARSIZE(bb) - VARHDRSZ);

        if (PG_ARGISNULL(0)) {
            r1 = r2;
        } else {
            r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);
            roaring_bitmap_or_inplace(r1, r2);
            roaring_bitmap_free(r2);
        }

        MemoryContextSwitchTo(oldcontext);
    }

    PG_RETURN_POINTER(r1);
}

//bitmap or combine
PG_FUNCTION_INFO_V1(rb_or_combine);
Datum rb_or_combine(PG_FUNCTION_ARGS);

Datum
rb_or_combine(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    MemoryContext oldcontext;
    roaring_bitmap_t *r1;
    roaring_bitmap_t *r2;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb_or_combine outside transition context")));

    if (PG_ARGISNULL(1)) {
        if (PG_ARGISNULL(0)) {
            PG_RETURN_NULL();
        }
        r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);
    } else {
        r2 = (roaring_bitmap_t *) PG_GETARG_POINTER(1);

        oldcontext = MemoryContextSwitchTo(aggctx);

        if (PG_ARGISNULL(0)) {
            r1 = roaring_bitmap_copy(r2);
        } else {
            r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);
            roaring_bitmap_or_inplace(r1, r2);
        }

        MemoryContextSwitchTo(oldcontext);
    }

    PG_RETURN_POINTER(r1);
}

//bitmap and trans
PG_FUNCTION_INFO_V1(rb_and_trans);
Datum rb_and_trans(PG_FUNCTION_ARGS);

Datum
rb_and_trans(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    MemoryContext oldcontext;
    bytea *bb;
    roaring_bitmap_t *r1;
    roaring_bitmap_t *r2;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb_and_trans outside transition context")));

    if (PG_ARGISNULL(1)) {
        if (PG_ARGISNULL(0)) {
            PG_RETURN_NULL();
        }
        r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);
    } else {
        if (PG_ARGISNULL(0) ) {
            /*
             * Must be PG_GETARG_BYTEA_P(), not _PP: everything below goes
             * through VARDATA()/VARSIZE(), which assume a 4-byte varlena header
             * and a 4-byte aligned payload. _PP may hand back a short-header or
             * unaligned datum, and CRoaring reads the buffer as aligned
             * uint16/uint32, so this would corrupt data or crash.
             */
            bb = PG_GETARG_BYTEA_P(1);

            oldcontext = MemoryContextSwitchTo(aggctx);
            r2 = roaring_bitmap_portable_deserialize_safe(VARDATA(bb), VARSIZE(bb) - VARHDRSZ);
            MemoryContextSwitchTo(oldcontext);
            r1 = r2;
        } else {
            r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);
            if (!roaring_bitmap_is_empty(r1)) {
                bb = PG_GETARG_BYTEA_P(1);
                r2 = roaring_bitmap_portable_deserialize_safe(VARDATA(bb), VARSIZE(bb) - VARHDRSZ);

                oldcontext = MemoryContextSwitchTo(aggctx);
                roaring_bitmap_and_inplace(r1, r2);
                MemoryContextSwitchTo(oldcontext);

                roaring_bitmap_free(r2);
            }
        }
    }

    PG_RETURN_POINTER(r1);
}

//bitmap and combine
PG_FUNCTION_INFO_V1(rb_and_combine);
Datum rb_and_combine(PG_FUNCTION_ARGS);

Datum
rb_and_combine(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    MemoryContext oldcontext;
    roaring_bitmap_t *r1;
    roaring_bitmap_t *r2;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb_and_combine outside transition context")));

    if (PG_ARGISNULL(1)) {
        if (PG_ARGISNULL(0)) {
            PG_RETURN_NULL();
        }
        r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);
    } else {
        r2 = (roaring_bitmap_t *) PG_GETARG_POINTER(1);

        oldcontext = MemoryContextSwitchTo(aggctx);

        if (PG_ARGISNULL(0)) {
            r1 = roaring_bitmap_copy(r2);
        } else {
            r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);
            roaring_bitmap_and_inplace(r1, r2);
        }

        MemoryContextSwitchTo(oldcontext);
    }

    PG_RETURN_POINTER(r1);
}

//bitmap xor trans
PG_FUNCTION_INFO_V1(rb_xor_trans);
Datum rb_xor_trans(PG_FUNCTION_ARGS);

Datum
rb_xor_trans(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    MemoryContext oldcontext;
    bytea *bb;
    roaring_bitmap_t *r1;
    roaring_bitmap_t *r2;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb_xor_trans outside transition context")));

    if (PG_ARGISNULL(1)) {
        if (PG_ARGISNULL(0)) {
            PG_RETURN_NULL();
        }
        r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);
    } else {
        bb = PG_GETARG_BYTEA_P(1);

        oldcontext = MemoryContextSwitchTo(aggctx);

        r2 = roaring_bitmap_portable_deserialize_safe(VARDATA(bb), VARSIZE(bb) - VARHDRSZ);

        if (PG_ARGISNULL(0)) {
            r1 = r2;
        } else {
            r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);
            roaring_bitmap_xor_inplace(r1, r2);
            roaring_bitmap_free(r2);
        }

        MemoryContextSwitchTo(oldcontext);
    }

    PG_RETURN_POINTER(r1);
}

//bitmap xor combine
PG_FUNCTION_INFO_V1(rb_xor_combine);
Datum rb_xor_combine(PG_FUNCTION_ARGS);

Datum
rb_xor_combine(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    MemoryContext oldcontext;
    roaring_bitmap_t *r1;
    roaring_bitmap_t *r2;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb_xor_combine outside transition context")));

    if (PG_ARGISNULL(1)) {
        if (PG_ARGISNULL(0)) {
            PG_RETURN_NULL();
        }
        r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);
    } else {
        r2 = (roaring_bitmap_t *) PG_GETARG_POINTER(1);

        oldcontext = MemoryContextSwitchTo(aggctx);

        if (PG_ARGISNULL(0)) {
            r1 = roaring_bitmap_copy(r2);
        } else {
            r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);
            roaring_bitmap_xor_inplace(r1, r2);
        }

        MemoryContextSwitchTo(oldcontext);
    }

    PG_RETURN_POINTER(r1);
}

//bitmap build trans
PG_FUNCTION_INFO_V1(rb_build_trans);
Datum rb_build_trans(PG_FUNCTION_ARGS);

Datum
rb_build_trans(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    MemoryContext oldcontext;
    roaring_bitmap_t *r1;
    int i2;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb_build_trans outside transition context")));

    if (PG_ARGISNULL(1)) {
        if (PG_ARGISNULL(0)) {
            PG_RETURN_NULL();
        }
        r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);
    } else {
        i2 = PG_GETARG_UINT32(1);

        oldcontext = MemoryContextSwitchTo(aggctx);

        if (PG_ARGISNULL(0)) {
            r1 = roaring_bitmap_create();
        } else {
            r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);
        }
        roaring_bitmap_add(r1, i2);

        MemoryContextSwitchTo(oldcontext);
    }

    PG_RETURN_POINTER(r1);
}


//bitmap Serialize
PG_FUNCTION_INFO_V1(rb_serialize);
Datum rb_serialize(PG_FUNCTION_ARGS);

Datum
rb_serialize(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    roaring_bitmap_t *r1;
    size_t expectedsize;
    bytea *serializedbytes;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb_serialize outside aggregate context")));

    // Is the first argument a NULL?
    if (PG_ARGISNULL(0)) {
        PG_RETURN_NULL();
    } else {
        r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);

        expectedsize = roaring_bitmap_portable_size_in_bytes(r1);
        serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
        roaring_bitmap_portable_serialize(r1, VARDATA(serializedbytes));

        SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
        PG_RETURN_BYTEA_P(serializedbytes);
    }
}

//bitmap Deserialize
PG_FUNCTION_INFO_V1(rb_deserialize);
Datum rb_deserialize(PG_FUNCTION_ARGS);

Datum
rb_deserialize(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    bytea *serializedbytes;
    roaring_bitmap_t *r1;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb_deserialize outside aggregate context")));

    // Is the first argument a NULL?
    if (PG_ARGISNULL(0)) {
        PG_RETURN_NULL();
    } else {
        serializedbytes = PG_GETARG_BYTEA_P(0);
        r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes), VARSIZE(serializedbytes) - VARHDRSZ);
        if (!r1)
            ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                     errmsg("bitmap format is error")));
        // PostgreSQL's combine_aggregates() only init fcinfo->isnull once,
        // set fcinfo->isnull here to avoid bug https://github.com/ChenHuajun/pg_roaringbitmap/issues/6
        fcinfo->isnull = false;
        PG_RETURN_POINTER(r1);
    }
}

//bitmap Cardinality trans
PG_FUNCTION_INFO_V1(rb_cardinality_final);
Datum rb_cardinality_final(PG_FUNCTION_ARGS);

Datum
rb_cardinality_final(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    roaring_bitmap_t *r1;
    uint64 card1;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb_cardinality_final outside aggregate context")));

    // Is the first argument a NULL?
    if (PG_ARGISNULL(0)) {
        PG_RETURN_NULL();
    } else {
        r1 = (roaring_bitmap_t *) PG_GETARG_POINTER(0);

        card1 = roaring_bitmap_get_cardinality(r1);

        PG_RETURN_INT64(card1);
    }
}

//bitmap run optimize
PG_FUNCTION_INFO_V1(rb_runoptimize);
Datum rb_runoptimize(PG_FUNCTION_ARGS);

Datum
rb_runoptimize(PG_FUNCTION_ARGS) {
    bytea *serializedbytes = PG_GETARG_BYTEA_P(0);
    roaring_bitmap_t *r;
    size_t expectedsize;

    r = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes), VARSIZE(serializedbytes) - VARHDRSZ);
    if (!r)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                        errmsg("bitmap format is error")));

    roaring_bitmap_run_optimize(r);

    expectedsize = roaring_bitmap_portable_size_in_bytes(r);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r, VARDATA(serializedbytes));

    roaring_bitmap_free(r);
    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

/*
 * ============================================================================
 * opclass support functions
 *
 * These back the btree / hash / gin operator classes and the restriction
 * selectivity estimators.
 * ============================================================================
 */

/* ---------------------------------------------------------------------------
 * btree support: rb_cmp + the four comparison operators
 * ---------------------------------------------------------------------------
 */

/*
 * Compare two bitmaps as ascending, deduplicated sequences of unsigned
 * (uint32) elements.  This is lexicographic order: compare element by element,
 * the first differing element decides, and the shorter sequence sorts first.
 *
 * Elements are compared as uint32, matching how they are stored, how
 * rb_to_array() lists them (0, ..., 2147483647, -2147483648, ..., -1) and how
 * rb_min() / rb_max() report them.  Hence {-2147483648} > {2147483647}.
 *
 * It must satisfy rb_cmp(a, b) == 0 iff rb_equals(a, b), independent of the
 * internal container/run-optimization encoding.
 */
static int
rb_bitmap_compare(bytea *a, bytea *b)
{
    roaring_bitmap_t *ra;
    roaring_bitmap_t *rb;
    roaring_uint32_iterator_t *ia;
    roaring_uint32_iterator_t *ib;
    int         result = 0;

    ra = rb_bitmap_deserialize(a);
    rb = rb_bitmap_deserialize(b);

    ia = roaring_iterator_create(ra);
    ib = roaring_iterator_create(rb);

    while (ia->has_value && ib->has_value)
    {
        /*
         * current_value is uint32_t, comparing it directly is the unsigned order.
         */
        if (ia->current_value < ib->current_value)
        {
            result = -1;
            break;
        }
        if (ia->current_value > ib->current_value)
        {
            result = 1;
            break;
        }

        roaring_uint32_iterator_advance(ia);
        roaring_uint32_iterator_advance(ib);
    }

    if (result == 0)
    {
        /* The sequence that runs out first is the smaller one. */
        if (ia->has_value && !ib->has_value)
            result = 1;
        else if (!ia->has_value && ib->has_value)
            result = -1;
    }

    roaring_uint32_iterator_free(ia);
    roaring_uint32_iterator_free(ib);
    roaring_bitmap_free(ra);
    roaring_bitmap_free(rb);

    return result;
}

//bitmap compare
PG_FUNCTION_INFO_V1(rb_cmp);
Datum rb_cmp(PG_FUNCTION_ARGS);

Datum
rb_cmp(PG_FUNCTION_ARGS) {
    PG_RETURN_INT32(rb_bitmap_compare(PG_GETARG_BYTEA_P(0),
                                      PG_GETARG_BYTEA_P(1)));
}

//bitmap less than
PG_FUNCTION_INFO_V1(rb_lt);
Datum rb_lt(PG_FUNCTION_ARGS);

Datum
rb_lt(PG_FUNCTION_ARGS) {
    PG_RETURN_BOOL(rb_bitmap_compare(PG_GETARG_BYTEA_P(0),
                                     PG_GETARG_BYTEA_P(1)) < 0);
}

//bitmap less than or equal to
PG_FUNCTION_INFO_V1(rb_le);
Datum rb_le(PG_FUNCTION_ARGS);

Datum
rb_le(PG_FUNCTION_ARGS) {
    PG_RETURN_BOOL(rb_bitmap_compare(PG_GETARG_BYTEA_P(0),
                                     PG_GETARG_BYTEA_P(1)) <= 0);
}

//bitmap greater than or equal to
PG_FUNCTION_INFO_V1(rb_ge);
Datum rb_ge(PG_FUNCTION_ARGS);

Datum
rb_ge(PG_FUNCTION_ARGS) {
    PG_RETURN_BOOL(rb_bitmap_compare(PG_GETARG_BYTEA_P(0),
                                     PG_GETARG_BYTEA_P(1)) >= 0);
}

//bitmap greater than
PG_FUNCTION_INFO_V1(rb_gt);
Datum rb_gt(PG_FUNCTION_ARGS);

Datum
rb_gt(PG_FUNCTION_ARGS) {
    PG_RETURN_BOOL(rb_bitmap_compare(PG_GETARG_BYTEA_P(0),
                                     PG_GETARG_BYTEA_P(1)) > 0);
}

/* ---------------------------------------------------------------------------
 * hash support: rb_hash + rb_hash_extended
 * ---------------------------------------------------------------------------
 *
 * We hash the normalized element sequence (after deserialization), never the
 * serialized bytes, so that runoptimize / container encoding never changes
 * the hash of an equal set.  The combine step mirrors hash_array().
 */

//bitmap hash
PG_FUNCTION_INFO_V1(rb_hash);
Datum rb_hash(PG_FUNCTION_ARGS);

Datum
rb_hash(PG_FUNCTION_ARGS) {
    bytea      *data = PG_GETARG_BYTEA_P(0);
    roaring_bitmap_t *r;
    roaring_uint32_iterator_t *it;
    uint32      result = 1;

    r = rb_bitmap_deserialize(data);
    it = roaring_iterator_create(r);

    while (it->has_value)
    {
        uint32      elthash = hash_bytes_uint32(it->current_value);

        result = (result << 5) - result + elthash;
        roaring_uint32_iterator_advance(it);
    }

    roaring_uint32_iterator_free(it);
    roaring_bitmap_free(r);

    PG_RETURN_UINT32(result);
}

//bitmap extended hash
PG_FUNCTION_INFO_V1(rb_hash_extended);
Datum rb_hash_extended(PG_FUNCTION_ARGS);

Datum
rb_hash_extended(PG_FUNCTION_ARGS) {
    bytea      *data = PG_GETARG_BYTEA_P(0);
    uint64      seed = PG_GETARG_INT64(1);
    roaring_bitmap_t *r;
    roaring_uint32_iterator_t *it;
    uint64      result = 1;

    r = rb_bitmap_deserialize(data);
    it = roaring_iterator_create(r);

    while (it->has_value)
    {
        uint64      elthash = hash_bytes_uint32_extended(it->current_value, seed);

        result = (result << 5) - result + elthash;
        roaring_uint32_iterator_advance(it);
    }

    roaring_uint32_iterator_free(it);
    roaring_bitmap_free(r);

    PG_RETURN_UINT64(result);
}

/* ---------------------------------------------------------------------------
 * gin support
 * ---------------------------------------------------------------------------
 *
 * The strategy numbers must match the gin operator class defined in
 * roaringbitmap--${VERSION}.sql; extractQuery, consistent and triconsistent all key
 * off them:
 *
 *   1  &&  (roaringbitmap, roaringbitmap)
 *   2  @>  (roaringbitmap, roaringbitmap)
 *   3  <@  (roaringbitmap, roaringbitmap)
 *   4  =   (roaringbitmap, roaringbitmap)
 *   5  @>  (roaringbitmap, int4)
 *
 * Strategies 2 and 5 share the @> operator and differ only in the type of the
 * right operand.  GIN passes that operand's raw Datum to extractQuery without
 * any type coercion, so the strategy number is the only signal for telling the
 * two apart.
 */
#define RB_GIN_OVERLAP_STRATEGY         1   /* && (roaringbitmap, roaringbitmap) */
#define RB_GIN_CONTAINS_STRATEGY        2   /* @> (roaringbitmap, roaringbitmap) */
#define RB_GIN_CONTAINED_STRATEGY       3   /* <@ (roaringbitmap, roaringbitmap) */
#define RB_GIN_EQUAL_STRATEGY           4   /* =  (roaringbitmap, roaringbitmap) */
#define RB_GIN_CONTAINS_INT_STRATEGY    5   /* @> (roaringbitmap, int4) */

/*
 * Extract every member of a bitmap as a palloc'd array of int4 Datums.
 * Sets *nentries; returns NULL with *nentries == 0 for the empty bitmap, which
 * makes GIN record an empty item.
 */
static Datum *
rb_bitmap_to_keys(bytea *data, int32 *nentries)
{
    roaring_bitmap_t *r;
    roaring_uint32_iterator_t *it;
    Datum      *entries;
    uint64      card;
    uint64      i = 0;

    r = rb_bitmap_deserialize(data);
    card = roaring_bitmap_get_cardinality(r);

    /*
     * GIN reports the number of extracted keys in an int32, so a bitmap with
     * more members than that cannot be indexed.
     */
    if (card > PG_INT32_MAX)
    {
        roaring_bitmap_free(r);
        ereport(ERROR,
                (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                 errmsg("bitmap has too many members to be indexed: " UINT64_FORMAT,
                        card)));
    }

    *nentries = (int32) card;

    if (card == 0)
    {
        roaring_bitmap_free(r);
        return NULL;
    }

    entries = (Datum *) palloc(sizeof(Datum) * card);

    it = roaring_iterator_create(r);
    while (it->has_value)
    {
        entries[i++] = Int32GetDatum(it->current_value);
        roaring_uint32_iterator_advance(it);
    }
    roaring_uint32_iterator_free(it);

    roaring_bitmap_free(r);

    return entries;
}

//gin extract value
PG_FUNCTION_INFO_V1(rb_ginextract_value);
Datum rb_ginextract_value(PG_FUNCTION_ARGS);

Datum
rb_ginextract_value(PG_FUNCTION_ARGS) {
    int32      *nkeys = (int32 *) PG_GETARG_POINTER(1);
    bool      **nullFlags = (bool **) PG_GETARG_POINTER(2);

    /* roaringbitmap members are never NULL. */
    *nullFlags = NULL;

    PG_RETURN_POINTER(rb_bitmap_to_keys(PG_GETARG_BYTEA_P(0), nkeys));
}

//gin extract query
PG_FUNCTION_INFO_V1(rb_ginextract_query);
Datum rb_ginextract_query(PG_FUNCTION_ARGS);

Datum
rb_ginextract_query(PG_FUNCTION_ARGS) {
    int32      *nkeys = (int32 *) PG_GETARG_POINTER(1);
    StrategyNumber strategy = PG_GETARG_UINT16(2);
    bool      **nullFlags = (bool **) PG_GETARG_POINTER(5);
    int32      *searchMode = (int32 *) PG_GETARG_POINTER(6);
    Datum      *entries = NULL;
    int32       nentries = 0;

    /* roaringbitmap members are never NULL. */
    *nullFlags = NULL;

    switch (strategy)
    {
        case RB_GIN_OVERLAP_STRATEGY:    /* && (roaringbitmap, roaringbitmap) */
            entries = rb_bitmap_to_keys(PG_GETARG_BYTEA_P(0), &nentries);
            *searchMode = GIN_SEARCH_MODE_DEFAULT;
            break;

        case RB_GIN_CONTAINS_STRATEGY:   /* @> (roaringbitmap, roaringbitmap) */
            entries = rb_bitmap_to_keys(PG_GETARG_BYTEA_P(0), &nentries);
            if (nentries > 0)
                *searchMode = GIN_SEARCH_MODE_DEFAULT;
            else
                *searchMode = GIN_SEARCH_MODE_ALL;  /* every set contains {} */
            break;

        case RB_GIN_CONTAINED_STRATEGY:  /* <@ (roaringbitmap, roaringbitmap) */
            entries = rb_bitmap_to_keys(PG_GETARG_BYTEA_P(0), &nentries);
            *searchMode = GIN_SEARCH_MODE_INCLUDE_EMPTY; /* {} is contained in all */
            break;

        case RB_GIN_EQUAL_STRATEGY:      /* = (roaringbitmap, roaringbitmap) */
            entries = rb_bitmap_to_keys(PG_GETARG_BYTEA_P(0), &nentries);
            if (nentries > 0)
                *searchMode = GIN_SEARCH_MODE_DEFAULT;
            else
                *searchMode = GIN_SEARCH_MODE_INCLUDE_EMPTY;
            break;

        case RB_GIN_CONTAINS_INT_STRATEGY:   /* @> (roaringbitmap, int4) */
            entries = (Datum *) palloc(sizeof(Datum));
            entries[0] = Int32GetDatum(PG_GETARG_INT32(0));
            nentries = 1;
            *searchMode = GIN_SEARCH_MODE_DEFAULT;
            break;

        default:
            elog(ERROR, "rb_ginextract_query: unknown strategy number: %d",
                 strategy);
    }

    *nkeys = nentries;

    PG_RETURN_POINTER(entries);
}

//gin consistent
PG_FUNCTION_INFO_V1(rb_ginconsistent);
Datum rb_ginconsistent(PG_FUNCTION_ARGS);

Datum
rb_ginconsistent(PG_FUNCTION_ARGS) {
    bool       *check = (bool *) PG_GETARG_POINTER(0);
    StrategyNumber strategy = PG_GETARG_UINT16(1);
    int32       nkeys = PG_GETARG_INT32(3);
    bool       *recheck = (bool *) PG_GETARG_POINTER(5);
    bool        res;
    int32       i;

    switch (strategy)
    {
        case RB_GIN_OVERLAP_STRATEGY:    /* && (roaringbitmap, roaringbitmap) */
            *recheck = false;
            res = false;
            for (i = 0; i < nkeys; i++)
            {
                if (check[i])
                {
                    res = true;
                    break;
                }
            }
            break;

        case RB_GIN_CONTAINS_STRATEGY:       /* @> (roaringbitmap, roaringbitmap) */
        case RB_GIN_CONTAINS_INT_STRATEGY:   /* @> (roaringbitmap, int4) */
            *recheck = false;
            res = true;
            for (i = 0; i < nkeys; i++)
            {
                if (!check[i])
                {
                    res = false;
                    break;
                }
            }
            break;

        case RB_GIN_CONTAINED_STRATEGY:  /* <@ (roaringbitmap, roaringbitmap) */
            *recheck = true;
            res = true;                 /* upper-bound filter only */
            break;

        case RB_GIN_EQUAL_STRATEGY:      /* = (roaringbitmap, roaringbitmap) */
            *recheck = true;
            res = true;
            for (i = 0; i < nkeys; i++)
            {
                if (!check[i])
                {
                    res = false;
                    break;
                }
            }
            break;

        default:
            elog(ERROR, "rb_ginconsistent: unknown strategy number: %d",
                 strategy);
            res = false;
    }

    PG_RETURN_BOOL(res);
}

//gin triconsistent
PG_FUNCTION_INFO_V1(rb_gintriconsistent);
Datum rb_gintriconsistent(PG_FUNCTION_ARGS);

Datum
rb_gintriconsistent(PG_FUNCTION_ARGS) {
    GinTernaryValue *check = (GinTernaryValue *) PG_GETARG_POINTER(0);
    StrategyNumber strategy = PG_GETARG_UINT16(1);
    int32       nkeys = PG_GETARG_INT32(3);
    GinTernaryValue res;
    int32       i;

    switch (strategy)
    {
        case RB_GIN_OVERLAP_STRATEGY:    /* && (roaringbitmap, roaringbitmap) */
            res = GIN_FALSE;
            for (i = 0; i < nkeys; i++)
            {
                if (check[i] == GIN_TRUE)
                {
                    res = GIN_TRUE;
                    break;
                }
                else if (check[i] == GIN_MAYBE && res != GIN_MAYBE)
                    res = GIN_MAYBE;
            }
            break;

        case RB_GIN_CONTAINS_STRATEGY:       /* @> (roaringbitmap, roaringbitmap) */
        case RB_GIN_CONTAINS_INT_STRATEGY:   /* @> (roaringbitmap, int4) */
            res = GIN_TRUE;
            for (i = 0; i < nkeys; i++)
            {
                if (check[i] == GIN_FALSE)
                {
                    res = GIN_FALSE;
                    break;
                }
                if (check[i] == GIN_MAYBE && res != GIN_MAYBE)
                    res = GIN_MAYBE;
            }
            break;

        case RB_GIN_CONTAINED_STRATEGY:  /* <@ (roaringbitmap, roaringbitmap) */
            res = GIN_MAYBE;            /* always needs a recheck */
            break;

        case RB_GIN_EQUAL_STRATEGY:      /* = (roaringbitmap, roaringbitmap) */
            res = GIN_MAYBE;            /* all keys hit is necessary, not sufficient */
            for (i = 0; i < nkeys; i++)
            {
                if (check[i] == GIN_FALSE)
                {
                    res = GIN_FALSE;
                    break;
                }
            }
            break;

        default:
            elog(ERROR, "rb_gintriconsistent: unknown strategy number: %d",
                 strategy);
            res = GIN_FALSE;
    }

    PG_RETURN_GIN_TERNARY_VALUE(res);
}

/* ---------------------------------------------------------------------------
 * restriction selectivity
 * ---------------------------------------------------------------------------
 */

/* Default selectivity constants, mirroring core arraycontsel(). */
#define RB_DEFAULT_CONTAIN_SEL   0.005
#define RB_DEFAULT_OVERLAP_SEL   0.01

/*
 * Shared body for the three restriction estimators.
 *
 * The estimate is a constant placeholder, but the (Var op Const) shape is still
 * resolved so that NULL constants yield 0.0 and non-variable clauses fall back
 * to the default.  A statistics-driven estimate is future work.
 */
static float8
rb_containment_restriction_sel(FunctionCallInfo fcinfo, float8 default_sel)
{
    PlannerInfo *root = (PlannerInfo *) PG_GETARG_POINTER(0);
    List       *args = (List *) PG_GETARG_POINTER(2);
    int         varRelid = PG_GETARG_INT32(3);
    VariableStatData vardata;
    Node       *other;
    bool        varonleft;

    if (!get_restriction_variable(root, args, varRelid,
                                  &vardata, &other, &varonleft))
        return default_sel;

    if (!IsA(other, Const))
    {
        ReleaseVariableStats(vardata);
        return default_sel;
    }

    if (((Const *) other)->constisnull)
    {
        ReleaseVariableStats(vardata);
        return 0.0;
    }

    ReleaseVariableStats(vardata);
    return default_sel;
}

//bitmap contain selectivity
PG_FUNCTION_INFO_V1(rb_contain_sel);
Datum rb_contain_sel(PG_FUNCTION_ARGS);

Datum
rb_contain_sel(PG_FUNCTION_ARGS) {
    PG_RETURN_FLOAT8(rb_containment_restriction_sel(fcinfo,
                                                    RB_DEFAULT_CONTAIN_SEL));
}

//bitmap contained selectivity
PG_FUNCTION_INFO_V1(rb_contained_sel);
Datum rb_contained_sel(PG_FUNCTION_ARGS);

Datum
rb_contained_sel(PG_FUNCTION_ARGS) {
    PG_RETURN_FLOAT8(rb_containment_restriction_sel(fcinfo,
                                                    RB_DEFAULT_CONTAIN_SEL));
}

//bitmap overlap selectivity
PG_FUNCTION_INFO_V1(rb_overlap_sel);
Datum rb_overlap_sel(PG_FUNCTION_ARGS);

Datum
rb_overlap_sel(PG_FUNCTION_ARGS) {
    PG_RETURN_FLOAT8(rb_containment_restriction_sel(fcinfo,
                                                    RB_DEFAULT_OVERLAP_SEL));
}

/* ---------------------------------------------------------------------------
 * statistics support
 * ---------------------------------------------------------------------------
 *
 * The btree operator class, introduced in version 1.3, gives the type both "<" 
 * and "=", so a plain std_typanalyze() would collect full scalar statistics: 
 * up to 300 * default_statistics_target bitmaps sorted with rb_cmp(), plus an 
 * MCV list and a 101-entry histogram per column. For bitmaps that buys almost 
 * nothing --​ cannot support collection of most common values (MCV) for elements
 * inside bitmaps and values above WIDTH_THRESHOLD are dropped before any comparison
 * -- while the planner has to detoast and walk a large pg_statistic row on every query.
 *
 * rb_typanalyze therefore keeps the pre-1.3 behaviour: null fraction, average
 * stored width and an unknown distinct count.  No comparison, no sort, no extra
 * storage, and the planner falls back to the default selectivities it used
 * before the opclasses existed.
 *
 */

static void rb_compute_stats(VacAttrStatsP stats,
                             AnalyzeAttrFetchFunc fetchfunc,
                             int samplerows, double totalrows);

/*
 * rb_typanalyze -- collect only the trivial statistics for roaringbitmap.
 *
 * Returning false would drop the column from ANALYZE altogether, losing even
 * the null fraction and the average width.  std_typanalyze() is called first so
 * that attstattarget / minrows keep the backend's handling; only the half that
 * collects values is replaced.
 */
PG_FUNCTION_INFO_V1(rb_typanalyze);
Datum rb_typanalyze(PG_FUNCTION_ARGS);

Datum
rb_typanalyze(PG_FUNCTION_ARGS) {
    VacAttrStats *stats = (VacAttrStats *) PG_GETARG_POINTER(0);

	/*
	 * Call the standard typanalyze function.  It may fail to find needed
	 * operators, in which case we also can't do anything, so just fail.
	 */
	if (!std_typanalyze(stats))
		PG_RETURN_BOOL(false);

    stats->compute_stats = rb_compute_stats;

    PG_RETURN_BOOL(true);
}

/*
 * rb_compute_stats -- null fraction and average width, nothing else.
 *
 * Same output as the backend's static compute_trivial_stats(), which cannot be
 * called from an extension.  Nothing is compared, hashed, sorted or copied, and
 * VARSIZE_ANY() reads only the varlena header, so a TOASTed value contributes
 * its external pointer size rather than its full size.  The pg_statistic row is
 * therefore a constant ~100 bytes with no datums in it, whatever the bitmaps
 * look like.
 */
static void
rb_compute_stats(VacAttrStatsP stats, AnalyzeAttrFetchFunc fetchfunc,
                 int samplerows, double totalrows)
{
    int         i;
    int         null_cnt = 0;
    int         nonnull_cnt = 0;
    double      total_width = 0;
    /* roaringbitmap is a varlena type: not byval and typlen == -1 */
    bool        is_varwidth = (!stats->attrtype->typbyval &&
                               stats->attrtype->typlen < 0);

    for (i = 0; i < samplerows; i++)
    {
        Datum       value;
        bool        isnull;

#if PG_VERSION_NUM >= 180000
        vacuum_delay_point(true);
#else
        vacuum_delay_point();
#endif

        value = fetchfunc(stats, i, &isnull);

        if (isnull)
        {
            null_cnt++;
            continue;
        }
        nonnull_cnt++;

        total_width += VARSIZE_ANY(DatumGetPointer(value));
    }

    if (nonnull_cnt > 0)
    {
        stats->stats_valid = true;
        stats->stanullfrac = (double) null_cnt / (double) samplerows;
        if (is_varwidth)
            stats->stawidth = total_width / (double) nonnull_cnt;
        else
            stats->stawidth = stats->attrtype->typlen;
        stats->stadistinct = 0.0;       /* "unknown" */
    }
    else if (null_cnt > 0)
    {
        /* We found only nulls; assume the column is entirely null */
        stats->stats_valid = true;
        stats->stanullfrac = 1.0;
        if (is_varwidth)
            stats->stawidth = 0;        /* "unknown" */
        else
            stats->stawidth = stats->attrtype->typlen;
        stats->stadistinct = 0.0;       /* "unknown" */
    }
}
