#include "roaringbitmap.h"

#include "roaring64_group_by_source.h"

#include "access/gin.h"
#include "access/stratnum.h"
#include "commands/vacuum.h"
#include "common/hashfn.h"
#include "utils/selfuncs.h"

/*
 * Deserialize a roaringbitmap64 varlena into a roaring64_bitmap_t, raising an
 * error on malformed input.  The caller owns the returned bitmap and must
 * roaring64_bitmap_free() it.
 */
static roaring64_bitmap_t *
rb64_bitmap_deserialize(bytea *data)
{
    roaring64_bitmap_t *r;

    r = roaring64_bitmap_portable_deserialize_safe(VARDATA(data),
                                                   VARSIZE(data) - VARHDRSZ);
    if (!r)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    return r;
}

//rb64_from_bytea
Datum rb64_from_bytea(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(rb64_from_bytea);

Datum
rb64_from_bytea(PG_FUNCTION_ARGS) {
    bytea *serializedbytes = PG_GETARG_BYTEA_P(0);
    roaring64_bitmap_t *r1;
    size_t expectedsize;
    bytea *serializedbytes2;
    const char *reason;

    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes), VARSIZE(serializedbytes) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if(!roaring64_bitmap_internal_validate(r1, &reason)) {
        roaring64_bitmap_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("bitmap format is error: %s", reason)));
    }

    expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);
    serializedbytes2 = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes2));
    roaring64_bitmap_free(r1);

    SET_VARSIZE(serializedbytes2, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes2);
}


//roaringbitmap64_in
Datum roaringbitmap64_in(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(roaringbitmap64_in);

Datum
roaringbitmap64_in(PG_FUNCTION_ARGS) {
    char       *ptr = PG_GETARG_CSTRING(0);
    long        l;
    char       *badp;
    roaring64_bitmap_t *r1;
    size_t expectedsize;
    bytea *serializedbytes;
    Datum dd;
    const char *reason;

    if(*ptr == '\\' && *(ptr+1) == 'x') {
       /* bytea input */
        dd = DirectFunctionCall1(byteain, PG_GETARG_DATUM(0));

        serializedbytes = DatumGetByteaP(dd);
        r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes), VARSIZE(serializedbytes) - VARHDRSZ);
        if (!r1)
            ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                     errmsg("bitmap format is error")));

        if(!roaring64_bitmap_internal_validate(r1, &reason)) {
            roaring64_bitmap_free(r1);
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                     errmsg("bitmap format is error: %s", reason)));
        }

        expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);
        serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
        roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
        roaring64_bitmap_free(r1);

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

    r1 = roaring64_bitmap_create();

    while (*ptr && isspace((unsigned char) *ptr))
        ptr++;

    if (*ptr != '}') {
        while (*ptr) {
            /* Parse int element */
            errno = 0;
            l = strtol(ptr, &badp, 10);

            /* We made no progress parsing the string, so bail out */
            if (ptr == badp){
                roaring64_bitmap_free(r1);
                ereport(ERROR,
                    (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                     errmsg("invalid input syntax for %s: \"%s\"",
                            "bigint", ptr)));
            }

            if (errno == ERANGE
                ){
                    roaring64_bitmap_free(r1);
                    ereport(ERROR,
                            (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
                             errmsg("value \"%s\" is out of range for type %s", ptr,
                                    "bigint")));
                }

            /* Add int element to bitmap */
            roaring64_bitmap_add(r1, l);

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
            roaring64_bitmap_free(r1);
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
        roaring64_bitmap_free(r1);
        ereport(ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
             errmsg("malformed bitmap literal")));
    }

    expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//roaringbitmap_out
Datum roaringbitmap64_out(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(roaringbitmap64_out);

Datum
roaringbitmap64_out(PG_FUNCTION_ARGS) {
    bytea *serializedbytes;
    roaring64_iterator_t *iterator;
    StringInfoData buf;
    roaring64_bitmap_t *r1;
    
    if(rbitmap_output_format == RBITMAP_OUTPUT_BYTEA){
        return DirectFunctionCall1(byteaout, PG_GETARG_DATUM(0));
    }
    
    serializedbytes = PG_GETARG_BYTEA_P(0);
    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes), VARSIZE(serializedbytes) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    initStringInfo(&buf);

    appendStringInfoChar(&buf, '{');

    iterator = roaring64_iterator_create(r1);
    if(roaring64_iterator_has_value(iterator)) {
        appendStringInfo(&buf, "%ld", (long)roaring64_iterator_value(iterator));
        roaring64_iterator_advance(iterator);

        while(roaring64_iterator_has_value(iterator)) {
            appendStringInfo(&buf, ",%ld", (long)roaring64_iterator_value(iterator));
            roaring64_iterator_advance(iterator);
        }
    }

    appendStringInfoChar(&buf, '}');

    PG_RETURN_CSTRING(buf.data);
}

//roaringbitmap64_recv
Datum roaringbitmap64_recv(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(roaringbitmap64_recv);

Datum
roaringbitmap64_recv(PG_FUNCTION_ARGS) {
    StringInfo    buf = (StringInfo) PG_GETARG_POINTER(0);
    int           nbytes;
    bytea *serializedbytes;
    roaring64_bitmap_t *r1;
    size_t expectedsize;
    const char *reason;

    nbytes = buf->len - buf->cursor;
    if (nbytes <= 0)
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
                 errmsg("empty roaring bitmap binary data")));

    r1 = roaring64_bitmap_portable_deserialize_safe(pq_getmsgbytes(buf, nbytes), nbytes);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if(!roaring64_bitmap_internal_validate(r1, &reason)) {
        roaring64_bitmap_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("bitmap format is error: %s", reason)));
    }

    expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}


//roaringbitmap64_send
Datum roaringbitmap64_send(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(roaringbitmap64_send);

Datum
roaringbitmap64_send(PG_FUNCTION_ARGS) {
    bytea *bp = PG_GETARG_BYTEA_P(0);
    StringInfoData buf;

    pq_begintypsend(&buf);
    pq_sendbytes(&buf, VARDATA(bp), VARSIZE(bp) - VARHDRSZ);
    PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}


//bitmap_or
PG_FUNCTION_INFO_V1(rb64_or);
Datum rb64_or(PG_FUNCTION_ARGS);

Datum
rb64_or(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring64_bitmap_t *r1;
    roaring64_bitmap_t *r2;
    size_t expectedsize;
    bytea *serializedbytes;

    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes2), VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring64_bitmap_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }
    roaring64_bitmap_or_inplace(r1, r2);
    roaring64_bitmap_free(r2);
    expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}


//bitmap_or_cardinality
PG_FUNCTION_INFO_V1(rb64_or_cardinality);
Datum rb64_or_cardinality(PG_FUNCTION_ARGS);

Datum
rb64_or_cardinality(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring64_buffer_t *r1;
    roaring64_buffer_t *r2;
    uint64 card1;
    bool ret;

    r1 = roaring64_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring64_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring64_buffer_or_cardinality(r1, r2, &card1);
    roaring64_buffer_free(r1);
    roaring64_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_INT64(card1);
}

//bitmap_and
PG_FUNCTION_INFO_V1(rb64_and);
Datum rb64_and(PG_FUNCTION_ARGS);

Datum
rb64_and(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring64_bitmap_t *r1;
    roaring64_bitmap_t *r2;
    size_t expectedsize;
    bytea *serializedbytes;

    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes2), VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring64_bitmap_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    roaring64_bitmap_and_inplace(r1, r2);
    roaring64_bitmap_free(r2);
    expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}


//bitmap_and_cardinality
PG_FUNCTION_INFO_V1(rb64_and_cardinality);
Datum rb64_and_cardinality(PG_FUNCTION_ARGS);

Datum
rb64_and_cardinality(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring64_buffer_t *r1;
    roaring64_buffer_t *r2;
    uint64 card1;
    bool ret;

    r1 = roaring64_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring64_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring64_buffer_and_cardinality(r1, r2, &card1);
    roaring64_buffer_free(r1);
    roaring64_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_INT64(card1);
}


//bitmap_andnot
PG_FUNCTION_INFO_V1(rb64_andnot);
Datum rb64_andnot(PG_FUNCTION_ARGS);

Datum
rb64_andnot(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring64_bitmap_t *r1;
    roaring64_bitmap_t *r2;
    size_t expectedsize;
    bytea *serializedbytes;

    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes2), VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring64_bitmap_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    roaring64_bitmap_andnot_inplace(r1, r2);
    roaring64_bitmap_free(r2);
    expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}


//bitmap_andnot_cardinality
PG_FUNCTION_INFO_V1(rb64_andnot_cardinality);
Datum rb64_andnot_cardinality(PG_FUNCTION_ARGS);

Datum
rb64_andnot_cardinality(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring64_buffer_t *r1;
    roaring64_buffer_t *r2;
    uint64 card1;
    bool ret;

    r1 = roaring64_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring64_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring64_buffer_andnot_cardinality(r1, r2, &card1);
    roaring64_buffer_free(r1);
    roaring64_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_INT64(card1);
}


//bitmap_xor
PG_FUNCTION_INFO_V1(rb64_xor);
Datum rb64_xor(PG_FUNCTION_ARGS);

Datum
rb64_xor(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring64_bitmap_t *r1;
    roaring64_bitmap_t *r2;
    size_t expectedsize;
    bytea *serializedbytes;

    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes2), VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring64_bitmap_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    roaring64_bitmap_xor_inplace(r1, r2);
    roaring64_bitmap_free(r2);
    expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}


//bitmap_xor_cardinality
PG_FUNCTION_INFO_V1(rb64_xor_cardinality);
Datum rb64_xor_cardinality(PG_FUNCTION_ARGS);

Datum
rb64_xor_cardinality(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring64_buffer_t *r1;
    roaring64_buffer_t *r2;
    uint64 card1;
    bool ret;

    r1 = roaring64_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring64_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring64_buffer_xor_cardinality(r1, r2, &card1);
    roaring64_buffer_free(r1);
    roaring64_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_INT64(card1);
}


//bitmap cardinality
PG_FUNCTION_INFO_V1(rb64_cardinality);
Datum rb64_cardinality(PG_FUNCTION_ARGS);

Datum
rb64_cardinality(PG_FUNCTION_ARGS) {
    bytea *data = PG_GETARG_BYTEA_P(0);
    roaring64_buffer_t *r1;
    uint64 card;

    r1 = roaring64_buffer_create(VARDATA(data),
                               VARSIZE(data) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    card = roaring64_buffer_get_cardinality(r1);
    roaring64_buffer_free(r1);

    PG_RETURN_INT64(card);
}


//bitmap is empty
PG_FUNCTION_INFO_V1(rb64_is_empty);
Datum rb64_is_empty(PG_FUNCTION_ARGS);

Datum
rb64_is_empty(PG_FUNCTION_ARGS) {
    bytea *data = PG_GETARG_BYTEA_P(0);
    roaring64_buffer_t *r1;
    bool isempty;

    r1 = roaring64_buffer_create(VARDATA(data),
                               VARSIZE(data) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    isempty = roaring64_buffer_is_empty(r1);
    roaring64_buffer_free(r1);

    PG_RETURN_BOOL(isempty);
}

//bitmap contains one value
PG_FUNCTION_INFO_V1(rb64_exists);
Datum rb64_exists(PG_FUNCTION_ARGS);

Datum
rb64_exists(PG_FUNCTION_ARGS) {
    bytea *data = PG_GETARG_BYTEA_P(0);
    uint64 value = (uint64)PG_GETARG_INT64(1);
    roaring64_buffer_t *r1;
    bool isexist;
    bool ret;

    r1 = roaring64_buffer_create(VARDATA(data),
                               VARSIZE(data) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    ret = roaring64_buffer_contains(r1, value, &isexist);
    roaring64_buffer_free(r1);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_BOOL(isexist);
}

//bitmap equals
PG_FUNCTION_INFO_V1(rb64_equals);
Datum rb64_equals(PG_FUNCTION_ARGS);

Datum
rb64_equals(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring64_buffer_t *r1;
    roaring64_buffer_t *r2;
    bool isequal;
    bool ret;

    r1 = roaring64_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring64_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring64_buffer_equals(r1, r2, &isequal);
    roaring64_buffer_free(r1);
    roaring64_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_BOOL(isequal);
}

//bitmap not equals
PG_FUNCTION_INFO_V1(rb64_not_equals);
Datum rb64_not_equals(PG_FUNCTION_ARGS);

Datum
rb64_not_equals(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring64_buffer_t *r1;
    roaring64_buffer_t *r2;
    bool isequal;
    bool ret;

    r1 = roaring64_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring64_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring64_buffer_equals(r1, r2, &isequal);
    roaring64_buffer_free(r1);
    roaring64_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_BOOL(!isequal);
}

//bitmap intersect
PG_FUNCTION_INFO_V1(rb64_intersect);
Datum rb64_intersect(PG_FUNCTION_ARGS);

Datum
rb64_intersect(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring64_buffer_t *r1;
    roaring64_buffer_t *r2;
    bool isintersect;
    bool ret;

    r1 = roaring64_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring64_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring64_buffer_intersect(r1, r2, &isintersect);
    roaring64_buffer_free(r1);
    roaring64_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_BOOL(isintersect);
}

//bitmap contains
PG_FUNCTION_INFO_V1(rb64_contains);
Datum rb64_contains(PG_FUNCTION_ARGS);

Datum
rb64_contains(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring64_buffer_t *r1;
    roaring64_buffer_t *r2;
    bool iscontain;
    bool ret;

    r1 = roaring64_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring64_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring64_buffer_is_subset(r2, r1, &iscontain);
    roaring64_buffer_free(r1);
    roaring64_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_BOOL(iscontain);
}

//bitmap contained
PG_FUNCTION_INFO_V1(rb64_containedby);
Datum rb64_containedby(PG_FUNCTION_ARGS);

Datum
rb64_containedby(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring64_buffer_t *r1;
    roaring64_buffer_t *r2;
    bool iscontained;
    bool ret;

    r1 = roaring64_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring64_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring64_buffer_is_subset(r1, r2, &iscontained);
    roaring64_buffer_free(r1);
    roaring64_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_BOOL(iscontained);
}

//bitmap jaccard distance
PG_FUNCTION_INFO_V1(rb64_jaccard_dist);
Datum rb64_jaccard_dist(PG_FUNCTION_ARGS);

Datum
rb64_jaccard_dist(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    bytea *serializedbytes2 = PG_GETARG_BYTEA_P(1);
    roaring64_buffer_t *r1;
    roaring64_buffer_t *r2;
    double jaccard_dist;
    bool ret;

    r1 = roaring64_buffer_create(VARDATA(serializedbytes1),
                               VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_buffer_create(VARDATA(serializedbytes2),
                               VARSIZE(serializedbytes2) - VARHDRSZ);
    if (!r2) {
        roaring64_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    ret = roaring64_buffer_jaccard_index(r1, r2, &jaccard_dist);
    roaring64_buffer_free(r1);
    roaring64_buffer_free(r2);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_FLOAT8(jaccard_dist);
}

//bitmap add
PG_FUNCTION_INFO_V1(rb64_add);
Datum rb64_add(PG_FUNCTION_ARGS);

Datum
rb64_add(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    uint64 value = (uint64)PG_GETARG_INT64(1);
    size_t expectedsize;
    bytea *serializedbytes;

    roaring64_bitmap_t *r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1),
                                                                        VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    roaring64_bitmap_add(r1, value);

    expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap remove
PG_FUNCTION_INFO_V1(rb64_remove);
Datum rb64_remove(PG_FUNCTION_ARGS);

Datum
rb64_remove(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    uint64 value = (uint64)PG_GETARG_INT64(1);
    size_t expectedsize;
    bytea *serializedbytes;

    roaring64_bitmap_t *r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    roaring64_bitmap_remove(r1, value);

    expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap minimum
PG_FUNCTION_INFO_V1(rb64_min);
Datum rb64_min(PG_FUNCTION_ARGS);

Datum
rb64_min(PG_FUNCTION_ARGS) {
    bytea *data = PG_GETARG_BYTEA_P(0);
    roaring64_buffer_t *r1;
    uint64 min;
    bool ret;

    r1 = roaring64_buffer_create(VARDATA(data),
                               VARSIZE(data) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if(roaring64_buffer_is_empty(r1))
    {
        roaring64_buffer_free(r1);
        PG_RETURN_NULL();
    }

    ret = roaring64_buffer_minimum(r1, &min);
    roaring64_buffer_free(r1);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_UINT64(min);
}


//bitmap maximum
PG_FUNCTION_INFO_V1(rb64_max);
Datum rb64_max(PG_FUNCTION_ARGS);

Datum
rb64_max(PG_FUNCTION_ARGS) {
    bytea *data = PG_GETARG_BYTEA_P(0);
    roaring64_buffer_t *r1;
    uint64 max;
    bool ret;

    r1 = roaring64_buffer_create(VARDATA(data),
                               VARSIZE(data) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if(roaring64_buffer_is_empty(r1))
    {
        roaring64_buffer_free(r1);
        PG_RETURN_NULL();
    }

    ret = roaring64_buffer_maximum(r1, &max);
    roaring64_buffer_free(r1);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_UINT64(max);
}

//bitmap rank
PG_FUNCTION_INFO_V1(rb64_rank);
Datum rb64_rank(PG_FUNCTION_ARGS);

Datum
rb64_rank(PG_FUNCTION_ARGS) {
    bytea *data = PG_GETARG_BYTEA_P(0);
    uint64 value = (uint64)PG_GETARG_INT64(1);
    roaring64_buffer_t *r1;
    uint64 rank;
    bool ret;

    r1 = roaring64_buffer_create(VARDATA(data),
                               VARSIZE(data) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    ret = roaring64_buffer_rank(r1, value, &rank);
    roaring64_buffer_free(r1);
    if(!ret)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    PG_RETURN_INT64((int64)rank);
}

//bitmap index
PG_FUNCTION_INFO_V1(rb64_index);
Datum rb64_index(PG_FUNCTION_ARGS);

Datum
rb64_index(PG_FUNCTION_ARGS) {
    bytea *data = PG_GETARG_BYTEA_P(0);
    uint64 value = (uint64)PG_GETARG_INT64(1);
    roaring64_buffer_t *r1;
    uint64 rank;
    int64 result;
    bool ret,isexist;

    r1 = roaring64_buffer_create(VARDATA(data),
                               VARSIZE(data) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    ret = roaring64_buffer_contains(r1, value, &isexist);
    if(!ret)
    {
        roaring64_buffer_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    }

    result = -1;
    if(isexist)
    {
        ret = roaring64_buffer_rank(r1, value, &rank);
        roaring64_buffer_free(r1);
        if(!ret)
            ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                    errmsg("bitmap format is error")));

        result = (int64)rank - 1;
    } else {
        roaring64_buffer_free(r1);
    }

    PG_RETURN_INT64(result);
}

//bitmap fill
PG_FUNCTION_INFO_V1(rb64_fill);
Datum rb64_fill(PG_FUNCTION_ARGS);

Datum
rb64_fill(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    uint64_t rangestart = (uint64_t)PG_GETARG_INT64(1);
    uint64_t rangeend = (uint64_t)PG_GETARG_INT64(2);
    roaring64_bitmap_t *r1;
    size_t expectedsize;
    bytea *serializedbytes;

    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if (rangestart < rangeend || rangeend == 0) {
        roaring64_bitmap_add_range_closed(r1,rangestart, rangeend - 1);
    }

    expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap clear
PG_FUNCTION_INFO_V1(rb64_clear);
Datum rb64_clear(PG_FUNCTION_ARGS);

Datum
rb64_clear(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    uint64_t rangestart = (uint64_t)PG_GETARG_INT64(1);
    uint64_t rangeend = (uint64_t)PG_GETARG_INT64(2);
    roaring64_bitmap_t *r1;
    size_t expectedsize;
    bytea *serializedbytes;

    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if (rangestart < rangeend || rangeend == 0) {
        roaring64_bitmap_remove_range_closed(r1,rangestart, rangeend - 1);
    }

    expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap flip
PG_FUNCTION_INFO_V1(rb64_flip);
Datum rb64_flip(PG_FUNCTION_ARGS);

Datum
rb64_flip(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    uint64_t rangestart = (uint64_t)PG_GETARG_INT64(1);
    uint64_t rangeend = (uint64_t)PG_GETARG_INT64(2);
    roaring64_bitmap_t *r1;
    size_t expectedsize;
    bytea *serializedbytes;

    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if (rangestart < rangeend || rangeend == 0) {
        roaring64_bitmap_flip_closed_inplace(r1, rangestart, rangeend - 1);
    }

    expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap shiftright
PG_FUNCTION_INFO_V1(rb64_shiftright);
Datum rb64_shiftright(PG_FUNCTION_ARGS);

Datum
rb64_shiftright(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    int64 distance = PG_GETARG_INT64(1);
    uint64 value;
    roaring64_bitmap_t *r1;
    roaring64_bitmap_t *r2;
    roaring64_iterator_t *iterator;
    size_t expectedsize;
    bytea *serializedbytes;

    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    if(distance != 0)
    {
        r2 = roaring64_bitmap_create();
        if (!r2) {
            roaring64_bitmap_free(r1);
            ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                     errmsg("failed to create bitmap")));
        }

        iterator = roaring64_iterator_create(r1);
        if(distance > 0){
            while(roaring64_iterator_has_value(iterator)) {
                // if overflow, break
                if(roaring64_iterator_value(iterator) > UINT64_MAX - distance)
                    break;
                value = roaring64_iterator_value(iterator) + distance;
                roaring64_bitmap_add(r2, value);
                roaring64_iterator_advance(iterator);
            }
        }else{
            roaring64_iterator_move_equalorlarger(iterator, - distance);
            while(roaring64_iterator_has_value(iterator)) {
                value = roaring64_iterator_value(iterator) + distance;
                roaring64_bitmap_add(r2, value);
                roaring64_iterator_advance(iterator);
            }
        }
        roaring64_bitmap_free(r1);
        r1 = r2;
    }

    expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap range
PG_FUNCTION_INFO_V1(rb64_range);
Datum rb64_range(PG_FUNCTION_ARGS);

Datum
rb64_range(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    uint64_t rangestart = (uint64_t)PG_GETARG_INT64(1);
    uint64_t rangeend = (uint64_t)PG_GETARG_INT64(2);
    roaring64_bitmap_t *r1;
    roaring64_bitmap_t *r2;
    roaring64_iterator_t *iterator;
    size_t expectedsize;
    bytea *serializedbytes;

    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));
    
    r2 = roaring64_bitmap_create();
    if (!r2) {
        roaring64_bitmap_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("failed to create bitmap")));
    }

    iterator = roaring64_iterator_create(r1);
    roaring64_iterator_move_equalorlarger(iterator, rangestart);

    while(roaring64_iterator_has_value(iterator)) {
        if(rangeend != 0 && roaring64_iterator_value(iterator) >= rangeend)
            break;
        roaring64_bitmap_add(r2, roaring64_iterator_value(iterator));
        roaring64_iterator_advance(iterator);
    }

    expectedsize = roaring64_bitmap_portable_size_in_bytes(r2);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r2, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);
    roaring64_bitmap_free(r2);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap range_cardinality
PG_FUNCTION_INFO_V1(rb64_range_cardinality);
Datum rb64_range_cardinality(PG_FUNCTION_ARGS);

Datum
rb64_range_cardinality(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    uint64_t rangestart = (uint64_t)PG_GETARG_INT64(1);
    uint64_t rangeend = (uint64_t)PG_GETARG_INT64(2);
    roaring64_bitmap_t *r1;
    roaring64_iterator_t *iterator;
    uint64 card1;

    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    card1 = 0;
    iterator = roaring64_iterator_create(r1);
    roaring64_iterator_move_equalorlarger(iterator, rangestart);
    while(roaring64_iterator_has_value(iterator)) {
        if(rangeend != 0 && roaring64_iterator_value(iterator) >= rangeend)
            break;
        card1++;
        roaring64_iterator_advance(iterator);
    }

    roaring64_bitmap_free(r1);
    PG_RETURN_INT64(card1);
}

//bitmap range
PG_FUNCTION_INFO_V1(rb64_select);
Datum rb64_select(PG_FUNCTION_ARGS);

Datum
rb64_select(PG_FUNCTION_ARGS) {
    bytea *serializedbytes1 = PG_GETARG_BYTEA_P(0);
    int64 limit = PG_GETARG_INT64(1);
    int64 offset = PG_GETARG_INT64(2);
    bool reverse = PG_GETARG_BOOL(3);
    uint64_t rangestart = (uint64_t)PG_GETARG_INT64(4);
    uint64_t rangeend = (uint64_t)PG_GETARG_INT64(5);
    int64 count = 0;
    int64 total_count = 0;
    roaring64_bitmap_t *r1;
    roaring64_bitmap_t *r2;
    roaring64_iterator_t *iterator;
    size_t expectedsize;
    bytea *serializedbytes;

    if (offset < 0)
        offset = 0;

    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes1), VARSIZE(serializedbytes1) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_bitmap_create();
    if (!r2) {
        roaring64_bitmap_free(r1);
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("failed to create bitmap")));
    }

    if (limit > 0) {
        iterator = roaring64_iterator_create(r1);
        roaring64_iterator_move_equalorlarger(iterator, rangestart);
        if (!reverse) {
            while (roaring64_iterator_has_value(iterator)) {
                if ((rangeend != 0 && roaring64_iterator_value(iterator) >= rangeend)
                        || count - offset >= limit)
                    break;
                if (count >= offset) {
                    roaring64_bitmap_add(r2, roaring64_iterator_value(iterator));
                }
                roaring64_iterator_advance(iterator);
                count++;
            }
        } else {
            while (roaring64_iterator_has_value(iterator)) {
                if (rangeend != 0 && roaring64_iterator_value(iterator) >= rangeend)
                    break;
                roaring64_iterator_advance(iterator);
                total_count++;
            }

            if (total_count > offset) {
                /* calulate new offset for reverse */
                offset = total_count - offset - limit;
                if(offset < 0)
                    offset = 0;
                roaring64_iterator_reinit(r1, iterator);
                roaring64_iterator_move_equalorlarger(iterator,rangestart);
                count = 0;
                while (roaring64_iterator_has_value(iterator)) {
                    if ((rangeend != 0 && roaring64_iterator_value(iterator) >= rangeend)
                            || count - offset >= limit)
                        break;
                    if (count >= offset) {
                        roaring64_bitmap_add(r2, roaring64_iterator_value(iterator));
                    }
                    roaring64_iterator_advance(iterator);
                    count++;
                }
            }
        }
    }

    expectedsize = roaring64_bitmap_portable_size_in_bytes(r2);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r2, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);
    roaring64_bitmap_free(r2);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap build
PG_FUNCTION_INFO_V1(rb64_build);
Datum rb64_build(PG_FUNCTION_ARGS);

Datum
rb64_build(PG_FUNCTION_ARGS) {
    ArrayType *a = (ArrayType *) PG_GETARG_ARRAYTYPE_P(0);
    int na, n;
    int64 *da;
    roaring64_bitmap_t *r1;
    size_t expectedsize;
    bytea *serializedbytes;

    CHECKARRVALID(a);

    na = ARRNELEMS(a);
    da = (int64 *)ARRPTR(a);

    r1 = roaring64_bitmap_create();

    for (n = 0; n < na; n++) {
        roaring64_bitmap_add(r1, da[n]);
    }

    expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);

    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap to int[]
PG_FUNCTION_INFO_V1(rb64_to_array);
Datum rb64_to_array(PG_FUNCTION_ARGS);

Datum
rb64_to_array(PG_FUNCTION_ARGS)
{
    bytea *serializedbytes = PG_GETARG_BYTEA_P(0);
    roaring64_bitmap_t *r1;
    roaring64_iterator_t *iterator;
    ArrayType *result;
    Datum *out_datums;
    uint64_t card1;
    long counter = 0;

    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes), VARSIZE(serializedbytes) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    card1 = roaring64_bitmap_get_cardinality(r1);

    if (card1 == 0)
    {
        result = construct_empty_array(INT8OID);
    }
    else
    {
        out_datums = (Datum *)palloc(sizeof(Datum) * card1);

        iterator = roaring64_iterator_create(r1);
        while (roaring64_iterator_has_value(iterator))
        {
            out_datums[counter] = Int64GetDatum(roaring64_iterator_value(iterator));
            counter++;
            roaring64_iterator_advance(iterator);
        }
        roaring64_iterator_free(iterator);

        result = construct_array(out_datums, card1, INT8OID, sizeof(int64), true, 'i');
    }

    roaring64_bitmap_free(r1);
    PG_RETURN_POINTER(result);
}

//bitmap list
PG_FUNCTION_INFO_V1(rb64_iterate);
Datum rb64_iterate(PG_FUNCTION_ARGS);

Datum
rb64_iterate(PG_FUNCTION_ARGS) {
    FuncCallContext *funcctx;
    MemoryContext oldcontext;
    roaring64_iterator_t *fctx;
    bytea *data;
    roaring64_bitmap_t *r1;

    if (SRF_IS_FIRSTCALL()) {

        funcctx = SRF_FIRSTCALL_INIT();

        data = PG_GETARG_BYTEA_P(0);

        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(data), VARSIZE(data) - VARHDRSZ);
        if (!r1)
            ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                     errmsg("bitmap format is error")));

        fctx = roaring64_iterator_create(r1);

        funcctx->user_fctx = fctx;

        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();

    fctx = funcctx->user_fctx;

    if (roaring64_iterator_has_value(fctx)) {
        Datum result;
        result = roaring64_iterator_value(fctx);
        roaring64_iterator_advance(fctx);
        SRF_RETURN_NEXT(funcctx, result);
    } else {
        roaring64_iterator_free(fctx);
        SRF_RETURN_DONE(funcctx);
    }
}

//convert roaringbitmap64 to roaringbitmap
PG_FUNCTION_INFO_V1(rb64_to_roaringbitmap);
Datum rb64_to_roaringbitmap(PG_FUNCTION_ARGS);

Datum
rb64_to_roaringbitmap(PG_FUNCTION_ARGS)
{
    bytea *serializedbytes = PG_GETARG_BYTEA_P(0);
    roaring64_bitmap_t *r1;
    roaring64_iterator_t *iterator;
    roaring_bitmap_t *r2;
    size_t expectedsize;

    r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes), VARSIZE(serializedbytes) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring_bitmap_create();
    iterator = roaring64_iterator_create(r1);
    while (roaring64_iterator_has_value(iterator))
    {
        int64_t value = roaring64_iterator_value(iterator);
        if(value > INT32_MAX || value < INT32_MIN)
            ereport(ERROR,
                (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
                errmsg("value \"%ld\" is out of range for type %s", value,
                    "integer")));
        roaring_bitmap_add(r2, (int32_t)value);
        roaring64_iterator_advance(iterator);
    }
    roaring64_iterator_free(iterator);

    expectedsize = roaring_bitmap_portable_size_in_bytes(r2);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring_bitmap_portable_serialize(r2, VARDATA(serializedbytes));
    roaring64_bitmap_free(r1);
    roaring_bitmap_free(r2);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//convert roaringbitmap to roaringbitmap64
PG_FUNCTION_INFO_V1(rb64_from_roaringbitmap);
Datum rb64_from_roaringbitmap(PG_FUNCTION_ARGS);

Datum
rb64_from_roaringbitmap(PG_FUNCTION_ARGS)
{
    bytea *serializedbytes = PG_GETARG_BYTEA_P(0);
    roaring_bitmap_t *r1;
    roaring_uint32_iterator_t *iterator;
    roaring64_bitmap_t *r2;
    size_t expectedsize;

    r1 = roaring_bitmap_portable_deserialize_safe(VARDATA(serializedbytes), VARSIZE(serializedbytes) - VARHDRSZ);
    if (!r1)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                 errmsg("bitmap format is error")));

    r2 = roaring64_bitmap_create();
    iterator = roaring_iterator_create(r1);
    while (iterator->has_value)
    {
        roaring64_bitmap_add(r2, (int64_t)(int32_t)iterator->current_value);
        roaring_uint32_iterator_advance(iterator);
    }
    roaring_uint32_iterator_free(iterator);

    expectedsize = roaring64_bitmap_portable_size_in_bytes(r2);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r2, VARDATA(serializedbytes));
    roaring_bitmap_free(r1);
    roaring64_bitmap_free(r2);

    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//bitmap or trans
PG_FUNCTION_INFO_V1(rb64_or_trans);
Datum rb64_or_trans(PG_FUNCTION_ARGS);

Datum
rb64_or_trans(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    MemoryContext oldcontext;
    bytea *bb;
    roaring64_bitmap_t *r1;
    roaring64_bitmap_t *r2;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb64_or_trans outside transition context")));

    if (PG_ARGISNULL(1)) {
        if (PG_ARGISNULL(0)) {
            PG_RETURN_NULL();
        }
        r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);
    } else {
        bb = PG_GETARG_BYTEA_P(1);

        oldcontext = MemoryContextSwitchTo(aggctx);

        r2 = roaring64_bitmap_portable_deserialize_safe(VARDATA(bb), VARSIZE(bb) - VARHDRSZ);

        if (PG_ARGISNULL(0)) {
            r1 = r2;
        } else {
            r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);
            roaring64_bitmap_or_inplace(r1, r2);
            roaring64_bitmap_free(r2);
        }

        MemoryContextSwitchTo(oldcontext);
    }

    PG_RETURN_POINTER(r1);
}

//bitmap or combine
PG_FUNCTION_INFO_V1(rb64_or_combine);
Datum rb64_or_combine(PG_FUNCTION_ARGS);

Datum
rb64_or_combine(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    MemoryContext oldcontext;
    roaring64_bitmap_t *r1;
    roaring64_bitmap_t *r2;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb64_or_combine outside transition context")));

    if (PG_ARGISNULL(1)) {
        if (PG_ARGISNULL(0)) {
            PG_RETURN_NULL();
        }
        r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);
    } else {
        r2 = (roaring64_bitmap_t *) PG_GETARG_POINTER(1);

        oldcontext = MemoryContextSwitchTo(aggctx);

        if (PG_ARGISNULL(0)) {
            r1 = roaring64_bitmap_copy(r2);
        } else {
            r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);
            roaring64_bitmap_or_inplace(r1, r2);
        }

        MemoryContextSwitchTo(oldcontext);
    }

    PG_RETURN_POINTER(r1);
}

//bitmap and trans
PG_FUNCTION_INFO_V1(rb64_and_trans);
Datum rb64_and_trans(PG_FUNCTION_ARGS);

Datum
rb64_and_trans(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    MemoryContext oldcontext;
    bytea *bb;
    roaring64_bitmap_t *r1;
    roaring64_bitmap_t *r2;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb64_and_trans outside transition context")));

    if (PG_ARGISNULL(1)) {
        if (PG_ARGISNULL(0)) {
            PG_RETURN_NULL();
        }
        r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);
    } else {
        if (PG_ARGISNULL(0) ) {
            /* postgres will crash when use PG_GETARG_BYTEA_PP here */
            bb = PG_GETARG_BYTEA_P(1);

            oldcontext = MemoryContextSwitchTo(aggctx);
            r2 = roaring64_bitmap_portable_deserialize_safe(VARDATA(bb), VARSIZE(bb) - VARHDRSZ);
            MemoryContextSwitchTo(oldcontext);
            r1 = r2;
        } else {
            r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);
            if (!roaring64_bitmap_is_empty(r1)) {
                bb = PG_GETARG_BYTEA_P(1);
                r2 = roaring64_bitmap_portable_deserialize_safe(VARDATA(bb), VARSIZE(bb) - VARHDRSZ);

                oldcontext = MemoryContextSwitchTo(aggctx);
                roaring64_bitmap_and_inplace(r1, r2);
                MemoryContextSwitchTo(oldcontext);

                roaring64_bitmap_free(r2);
            }
        }
    }

    PG_RETURN_POINTER(r1);
}

//bitmap and combine
PG_FUNCTION_INFO_V1(rb64_and_combine);
Datum rb64_and_combine(PG_FUNCTION_ARGS);

Datum
rb64_and_combine(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    MemoryContext oldcontext;
    roaring64_bitmap_t *r1;
    roaring64_bitmap_t *r2;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb64_and_combine outside transition context")));

    if (PG_ARGISNULL(1)) {
        if (PG_ARGISNULL(0)) {
            PG_RETURN_NULL();
        }
        r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);
    } else {
        r2 = (roaring64_bitmap_t *) PG_GETARG_POINTER(1);

        oldcontext = MemoryContextSwitchTo(aggctx);

        if (PG_ARGISNULL(0)) {
            r1 = roaring64_bitmap_copy(r2);
        } else {
            r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);
            roaring64_bitmap_and_inplace(r1, r2);
        }

        MemoryContextSwitchTo(oldcontext);
    }

    PG_RETURN_POINTER(r1);
}

//bitmap xor trans
PG_FUNCTION_INFO_V1(rb64_xor_trans);
Datum rb64_xor_trans(PG_FUNCTION_ARGS);

Datum
rb64_xor_trans(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    MemoryContext oldcontext;
    bytea *bb;
    roaring64_bitmap_t *r1;
    roaring64_bitmap_t *r2;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb64_xor_trans outside transition context")));

    if (PG_ARGISNULL(1)) {
        if (PG_ARGISNULL(0)) {
            PG_RETURN_NULL();
        }
        r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);
    } else {
        bb = PG_GETARG_BYTEA_P(1);

        oldcontext = MemoryContextSwitchTo(aggctx);

        r2 = roaring64_bitmap_portable_deserialize_safe(VARDATA(bb), VARSIZE(bb) - VARHDRSZ);

        if (PG_ARGISNULL(0)) {
            r1 = r2;
        } else {
            r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);
            roaring64_bitmap_xor_inplace(r1, r2);
            roaring64_bitmap_free(r2);
        }

        MemoryContextSwitchTo(oldcontext);
    }

    PG_RETURN_POINTER(r1);
}

//bitmap xor combine
PG_FUNCTION_INFO_V1(rb64_xor_combine);
Datum rb64_xor_combine(PG_FUNCTION_ARGS);

Datum
rb64_xor_combine(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    MemoryContext oldcontext;
    roaring64_bitmap_t *r1;
    roaring64_bitmap_t *r2;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb64_xor_combine outside transition context")));

    if (PG_ARGISNULL(1)) {
        if (PG_ARGISNULL(0)) {
            PG_RETURN_NULL();
        }
        r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);
    } else {
        r2 = (roaring64_bitmap_t *) PG_GETARG_POINTER(1);

        oldcontext = MemoryContextSwitchTo(aggctx);

        if (PG_ARGISNULL(0)) {
            r1 = roaring64_bitmap_copy(r2);
        } else {
            r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);
            roaring64_bitmap_xor_inplace(r1, r2);
        }

        MemoryContextSwitchTo(oldcontext);
    }

    PG_RETURN_POINTER(r1);
}

//bitmap build trans
PG_FUNCTION_INFO_V1(rb64_build_trans);
Datum rb64_build_trans(PG_FUNCTION_ARGS);

Datum
rb64_build_trans(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    MemoryContext oldcontext;
    roaring64_bitmap_t *r1;
    uint64 i2;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb64_build_trans outside transition context")));

    if (PG_ARGISNULL(1)) {
        if (PG_ARGISNULL(0)) {
            PG_RETURN_NULL();
        }
        r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);
    } else {
        i2 = (uint64)PG_GETARG_INT64(1);

        oldcontext = MemoryContextSwitchTo(aggctx);

        if (PG_ARGISNULL(0)) {
            r1 = roaring64_bitmap_create();
        } else {
            r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);
        }
        roaring64_bitmap_add(r1, i2);

        MemoryContextSwitchTo(oldcontext);
    }

    PG_RETURN_POINTER(r1);
}


//bitmap Serialize
PG_FUNCTION_INFO_V1(rb64_serialize);
Datum rb64_serialize(PG_FUNCTION_ARGS);

Datum
rb64_serialize(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    roaring64_bitmap_t *r1;
    size_t expectedsize;
    bytea *serializedbytes;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb64_serialize outside aggregate context")));

    // Is the first argument a NULL?
    if (PG_ARGISNULL(0)) {
        PG_RETURN_NULL();
    } else {
        r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);

        expectedsize = roaring64_bitmap_portable_size_in_bytes(r1);
        serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
        roaring64_bitmap_portable_serialize(r1, VARDATA(serializedbytes));

        SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
        PG_RETURN_BYTEA_P(serializedbytes);
    }
}

//bitmap Deserialize
PG_FUNCTION_INFO_V1(rb64_deserialize);
Datum rb64_deserialize(PG_FUNCTION_ARGS);

Datum
rb64_deserialize(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    bytea *serializedbytes;
    roaring64_bitmap_t *r1;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb64_deserialize outside aggregate context")));

    // Is the first argument a NULL?
    if (PG_ARGISNULL(0)) {
        PG_RETURN_NULL();
    } else {
        serializedbytes = PG_GETARG_BYTEA_P(0);
        r1 = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes), VARSIZE(serializedbytes) - VARHDRSZ);
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
PG_FUNCTION_INFO_V1(rb64_cardinality_final);
Datum rb64_cardinality_final(PG_FUNCTION_ARGS);

Datum
rb64_cardinality_final(PG_FUNCTION_ARGS) {
    MemoryContext aggctx;
    roaring64_bitmap_t *r1;
    uint64 card1;

    // We must be called as a transition routine or we fail.
    if (!AggCheckCallContext(fcinfo, &aggctx))
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("rb64_cardinality_final outside aggregate context")));

    // Is the first argument a NULL?
    if (PG_ARGISNULL(0)) {
        PG_RETURN_NULL();
    } else {
        r1 = (roaring64_bitmap_t *) PG_GETARG_POINTER(0);

        card1 = roaring64_bitmap_get_cardinality(r1);

        PG_RETURN_INT64(card1);
    }
}

//bitmap run optimize
PG_FUNCTION_INFO_V1(rb64_runoptimize);
Datum rb64_runoptimize(PG_FUNCTION_ARGS);

Datum
rb64_runoptimize(PG_FUNCTION_ARGS) {
    bytea *serializedbytes = PG_GETARG_BYTEA_P(0);
    roaring64_bitmap_t *r;
    size_t expectedsize;

    r = roaring64_bitmap_portable_deserialize_safe(VARDATA(serializedbytes), VARSIZE(serializedbytes) - VARHDRSZ);
    if (!r)
        ereport(ERROR,
                (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                        errmsg("bitmap format is error")));

    roaring64_bitmap_run_optimize(r);

    expectedsize = roaring64_bitmap_portable_size_in_bytes(r);
    serializedbytes = (bytea *) palloc(VARHDRSZ + expectedsize);
    roaring64_bitmap_portable_serialize(r, VARDATA(serializedbytes));

    roaring64_bitmap_free(r);
    SET_VARSIZE(serializedbytes, VARHDRSZ + expectedsize);
    PG_RETURN_BYTEA_P(serializedbytes);
}

//rb64_group_elements_by_source
Datum rb64_group_elements_by_source(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(rb64_group_elements_by_source);

Datum
rb64_group_elements_by_source(PG_FUNCTION_ARGS)
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

        funcctx->user_fctx = roaring64_group_by_source_build_state(arr, funcctx, fcinfo);

        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();

    HeapTuple tuple = roaring64_group_by_source_next_row((roaring64_group_by_source_state_t *) funcctx->user_fctx);
    if (tuple == NULL)
        SRF_RETURN_DONE(funcctx);

    SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
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
 * btree support: rb64_cmp + the four comparison operators
 * ---------------------------------------------------------------------------
 */

/*
 * Compare two bitmaps as ascending, deduplicated sequences of unsigned
 * (uint64) elements.  This is lexicographic order: compare element by element,
 * the first differing element decides, and the shorter sequence sorts first.
 *
 * Elements are compared as uint64, matching how they are stored, how
 * rb64_to_array() lists them (0, ..., 9223372036854775807,
 * -9223372036854775808, ..., -1) and how rb64_min() / rb64_max() report them.
 * Hence {-9223372036854775808} > {9223372036854775807}.
 *
 * It must satisfy rb64_cmp(a, b) == 0 iff rb64_equals(a, b), independent of the
 * internal container/run-optimization encoding.
 */
static int
rb64_bitmap_compare(bytea *a, bytea *b)
{
    roaring64_bitmap_t *ra;
    roaring64_bitmap_t *rb;
    roaring64_iterator_t *ia;
    roaring64_iterator_t *ib;
    int         result = 0;

    ra = rb64_bitmap_deserialize(a);
    rb = rb64_bitmap_deserialize(b);

    ia = roaring64_iterator_create(ra);
    ib = roaring64_iterator_create(rb);

    while (roaring64_iterator_has_value(ia) && roaring64_iterator_has_value(ib))
    {
        /*
         * roaring64_iterator_value() returns uint64_t, comparing it directly is
         * the unsigned order.
         */
        if (roaring64_iterator_value(ia) < roaring64_iterator_value(ib))
        {
            result = -1;
            break;
        }
        if (roaring64_iterator_value(ia) > roaring64_iterator_value(ib))
        {
            result = 1;
            break;
        }

        roaring64_iterator_advance(ia);
        roaring64_iterator_advance(ib);
    }

    if (result == 0)
    {
        /* The sequence that runs out first is the smaller one. */
        if (roaring64_iterator_has_value(ia) && !roaring64_iterator_has_value(ib))
            result = 1;
        else if (!roaring64_iterator_has_value(ia) && roaring64_iterator_has_value(ib))
            result = -1;
    }

    roaring64_iterator_free(ia);
    roaring64_iterator_free(ib);
    roaring64_bitmap_free(ra);
    roaring64_bitmap_free(rb);

    return result;
}

//bitmap compare
PG_FUNCTION_INFO_V1(rb64_cmp);
Datum rb64_cmp(PG_FUNCTION_ARGS);

Datum
rb64_cmp(PG_FUNCTION_ARGS) {
    PG_RETURN_INT32(rb64_bitmap_compare(PG_GETARG_BYTEA_P(0),
                                        PG_GETARG_BYTEA_P(1)));
}

//bitmap less than
PG_FUNCTION_INFO_V1(rb64_lt);
Datum rb64_lt(PG_FUNCTION_ARGS);

Datum
rb64_lt(PG_FUNCTION_ARGS) {
    PG_RETURN_BOOL(rb64_bitmap_compare(PG_GETARG_BYTEA_P(0),
                                       PG_GETARG_BYTEA_P(1)) < 0);
}

//bitmap less than or equal to
PG_FUNCTION_INFO_V1(rb64_le);
Datum rb64_le(PG_FUNCTION_ARGS);

Datum
rb64_le(PG_FUNCTION_ARGS) {
    PG_RETURN_BOOL(rb64_bitmap_compare(PG_GETARG_BYTEA_P(0),
                                       PG_GETARG_BYTEA_P(1)) <= 0);
}

//bitmap greater than or equal to
PG_FUNCTION_INFO_V1(rb64_ge);
Datum rb64_ge(PG_FUNCTION_ARGS);

Datum
rb64_ge(PG_FUNCTION_ARGS) {
    PG_RETURN_BOOL(rb64_bitmap_compare(PG_GETARG_BYTEA_P(0),
                                       PG_GETARG_BYTEA_P(1)) >= 0);
}

//bitmap greater than
PG_FUNCTION_INFO_V1(rb64_gt);
Datum rb64_gt(PG_FUNCTION_ARGS);

Datum
rb64_gt(PG_FUNCTION_ARGS) {
    PG_RETURN_BOOL(rb64_bitmap_compare(PG_GETARG_BYTEA_P(0),
                                       PG_GETARG_BYTEA_P(1)) > 0);
}

/* ---------------------------------------------------------------------------
 * hash support: rb64_hash + rb64_hash_extended
 * ---------------------------------------------------------------------------
 *
 * We hash the normalized element sequence (after deserialization), never the
 * serialized bytes, so that runoptimize / container encoding never changes
 * the hash of an equal set.  The combine step mirrors hash_array().
 *
 * PostgreSQL has no uint64 counterpart of hash_bytes_uint32(), so each element
 * is hashed as an opaque 8-byte little-endian value; only the hash operator
 * class consumes the result, and it only requires equal sets to hash equally.
 */

//bitmap hash
PG_FUNCTION_INFO_V1(rb64_hash);
Datum rb64_hash(PG_FUNCTION_ARGS);

Datum
rb64_hash(PG_FUNCTION_ARGS) {
    bytea      *data = PG_GETARG_BYTEA_P(0);
    roaring64_bitmap_t *r;
    roaring64_iterator_t *it;
    uint32      result = 1;

    r = rb64_bitmap_deserialize(data);
    it = roaring64_iterator_create(r);

    while (roaring64_iterator_has_value(it))
    {
        uint64      value = roaring64_iterator_value(it);
        uint32      elthash = hash_bytes((const unsigned char *) &value,
                                         sizeof(uint64));

        result = (result << 5) - result + elthash;
        roaring64_iterator_advance(it);
    }

    roaring64_iterator_free(it);
    roaring64_bitmap_free(r);

    PG_RETURN_UINT32(result);
}

//bitmap extended hash
PG_FUNCTION_INFO_V1(rb64_hash_extended);
Datum rb64_hash_extended(PG_FUNCTION_ARGS);

Datum
rb64_hash_extended(PG_FUNCTION_ARGS) {
    bytea      *data = PG_GETARG_BYTEA_P(0);
    uint64      seed = PG_GETARG_INT64(1);
    roaring64_bitmap_t *r;
    roaring64_iterator_t *it;
    uint64      result = 1;

    r = rb64_bitmap_deserialize(data);
    it = roaring64_iterator_create(r);

    while (roaring64_iterator_has_value(it))
    {
        uint64      value = roaring64_iterator_value(it);
        uint64      elthash = hash_bytes_extended((const unsigned char *) &value,
                                                  sizeof(uint64), seed);

        result = (result << 5) - result + elthash;
        roaring64_iterator_advance(it);
    }

    roaring64_iterator_free(it);
    roaring64_bitmap_free(r);

    PG_RETURN_UINT64(result);
}

/* ---------------------------------------------------------------------------
 * gin support
 * ---------------------------------------------------------------------------
 *
 * Strategy numbers are fixed by CREATE OPERATOR CLASS ; GIN passes the right
 * operand's raw Datum to extractQuery without any type coercion, so the strategy
 * number is the only signal for telling (roaringbitmap64) apart from (int8).
 */
#define RB64_GIN_OVERLAP_STRATEGY         1   /* && (roaringbitmap64, roaringbitmap64) */
#define RB64_GIN_CONTAINS_STRATEGY        2   /* @> (roaringbitmap64, roaringbitmap64) */
#define RB64_GIN_CONTAINED_STRATEGY       3   /* <@ (roaringbitmap64, roaringbitmap64) */
#define RB64_GIN_EQUAL_STRATEGY           4   /* =  (roaringbitmap64, roaringbitmap64) */
#define RB64_GIN_CONTAINS_INT_STRATEGY    5   /* @> (roaringbitmap64, bigint) */

/*
 * Extract every member of a bitmap as a palloc'd array of int8 Datums.
 * Sets *nentries; returns NULL with *nentries == 0 for the empty bitmap, which
 * makes GIN record an empty item.
 */
static Datum *
rb64_bitmap_to_keys(bytea *data, int32 *nentries)
{
    roaring64_bitmap_t *r;
    roaring64_iterator_t *it;
    Datum      *entries;
    uint64      card;
    uint64      i = 0;

    r = rb64_bitmap_deserialize(data);
    card = roaring64_bitmap_get_cardinality(r);

    /*
     * GIN reports the number of extracted keys in an int32, so a bitmap with
     * more members than that cannot be indexed.
     */
    if (card > PG_INT32_MAX)
    {
        roaring64_bitmap_free(r);
        ereport(ERROR,
                (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                 errmsg("bitmap has too many members to be indexed: " UINT64_FORMAT,
                        card)));
    }

    *nentries = (int32) card;

    if (card == 0)
    {
        roaring64_bitmap_free(r);
        return NULL;
    }

    entries = (Datum *) palloc(sizeof(Datum) * card);

    it = roaring64_iterator_create(r);
    while (roaring64_iterator_has_value(it))
    {
        entries[i++] = Int64GetDatum(roaring64_iterator_value(it));
        roaring64_iterator_advance(it);
    }
    roaring64_iterator_free(it);

    roaring64_bitmap_free(r);

    return entries;
}

//gin extract value
PG_FUNCTION_INFO_V1(rb64_ginextract_value);
Datum rb64_ginextract_value(PG_FUNCTION_ARGS);

Datum
rb64_ginextract_value(PG_FUNCTION_ARGS) {
    int32      *nkeys = (int32 *) PG_GETARG_POINTER(1);
    bool      **nullFlags = (bool **) PG_GETARG_POINTER(2);

    /* roaringbitmap64 members are never NULL. */
    *nullFlags = NULL;

    PG_RETURN_POINTER(rb64_bitmap_to_keys(PG_GETARG_BYTEA_P(0), nkeys));
}

//gin extract query
PG_FUNCTION_INFO_V1(rb64_ginextract_query);
Datum rb64_ginextract_query(PG_FUNCTION_ARGS);

Datum
rb64_ginextract_query(PG_FUNCTION_ARGS) {
    int32      *nkeys = (int32 *) PG_GETARG_POINTER(1);
    StrategyNumber strategy = PG_GETARG_UINT16(2);
    bool      **nullFlags = (bool **) PG_GETARG_POINTER(5);
    int32      *searchMode = (int32 *) PG_GETARG_POINTER(6);
    Datum      *entries = NULL;
    int32       nentries = 0;

    /* roaringbitmap64 members are never NULL. */
    *nullFlags = NULL;

    switch (strategy)
    {
        case RB64_GIN_OVERLAP_STRATEGY:    /* && (roaringbitmap64, roaringbitmap64) */
            entries = rb64_bitmap_to_keys(PG_GETARG_BYTEA_P(0), &nentries);
            *searchMode = GIN_SEARCH_MODE_DEFAULT;
            break;

        case RB64_GIN_CONTAINS_STRATEGY:   /* @> (roaringbitmap64, roaringbitmap64) */
            entries = rb64_bitmap_to_keys(PG_GETARG_BYTEA_P(0), &nentries);
            if (nentries > 0)
                *searchMode = GIN_SEARCH_MODE_DEFAULT;
            else
                *searchMode = GIN_SEARCH_MODE_ALL;  /* every set contains {} */
            break;

        case RB64_GIN_CONTAINED_STRATEGY:  /* <@ (roaringbitmap64, roaringbitmap64) */
            entries = rb64_bitmap_to_keys(PG_GETARG_BYTEA_P(0), &nentries);
            *searchMode = GIN_SEARCH_MODE_INCLUDE_EMPTY; /* {} is contained in all */
            break;

        case RB64_GIN_EQUAL_STRATEGY:      /* = (roaringbitmap64, roaringbitmap64) */
            entries = rb64_bitmap_to_keys(PG_GETARG_BYTEA_P(0), &nentries);
            if (nentries > 0)
                *searchMode = GIN_SEARCH_MODE_DEFAULT;
            else
                *searchMode = GIN_SEARCH_MODE_INCLUDE_EMPTY;
            break;

        case RB64_GIN_CONTAINS_INT_STRATEGY:   /* @> (roaringbitmap64, bigint) */
            entries = (Datum *) palloc(sizeof(Datum));
            entries[0] = Int64GetDatum(PG_GETARG_INT64(0));
            nentries = 1;
            *searchMode = GIN_SEARCH_MODE_DEFAULT;
            break;

        default:
            elog(ERROR, "rb64_ginextract_query: unknown strategy number: %d",
                 strategy);
    }

    *nkeys = nentries;

    PG_RETURN_POINTER(entries);
}

//gin consistent
PG_FUNCTION_INFO_V1(rb64_ginconsistent);
Datum rb64_ginconsistent(PG_FUNCTION_ARGS);

Datum
rb64_ginconsistent(PG_FUNCTION_ARGS) {
    bool       *check = (bool *) PG_GETARG_POINTER(0);
    StrategyNumber strategy = PG_GETARG_UINT16(1);
    int32       nkeys = PG_GETARG_INT32(3);
    bool       *recheck = (bool *) PG_GETARG_POINTER(5);
    bool        res;
    int32       i;

    switch (strategy)
    {
        case RB64_GIN_OVERLAP_STRATEGY:    /* && (roaringbitmap64, roaringbitmap64) */
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

        case RB64_GIN_CONTAINS_STRATEGY:       /* @> (roaringbitmap64, roaringbitmap64) */
        case RB64_GIN_CONTAINS_INT_STRATEGY:   /* @> (roaringbitmap64, bigint) */
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

        case RB64_GIN_CONTAINED_STRATEGY:  /* <@ (roaringbitmap64, roaringbitmap64) */
            *recheck = true;
            res = true;                 /* upper-bound filter only */
            break;

        case RB64_GIN_EQUAL_STRATEGY:      /* = (roaringbitmap64, roaringbitmap64) */
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
            elog(ERROR, "rb64_ginconsistent: unknown strategy number: %d",
                 strategy);
            res = false;
    }

    PG_RETURN_BOOL(res);
}

//gin triconsistent
PG_FUNCTION_INFO_V1(rb64_gintriconsistent);
Datum rb64_gintriconsistent(PG_FUNCTION_ARGS);

Datum
rb64_gintriconsistent(PG_FUNCTION_ARGS) {
    GinTernaryValue *check = (GinTernaryValue *) PG_GETARG_POINTER(0);
    StrategyNumber strategy = PG_GETARG_UINT16(1);
    int32       nkeys = PG_GETARG_INT32(3);
    GinTernaryValue res;
    int32       i;

    switch (strategy)
    {
        case RB64_GIN_OVERLAP_STRATEGY:    /* && (roaringbitmap64, roaringbitmap64) */
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

        case RB64_GIN_CONTAINS_STRATEGY:       /* @> (roaringbitmap64, roaringbitmap64) */
        case RB64_GIN_CONTAINS_INT_STRATEGY:   /* @> (roaringbitmap64, bigint) */
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

        case RB64_GIN_CONTAINED_STRATEGY:  /* <@ (roaringbitmap64, roaringbitmap64) */
            res = GIN_MAYBE;            /* always needs a recheck */
            break;

        case RB64_GIN_EQUAL_STRATEGY:      /* = (roaringbitmap64, roaringbitmap64) */
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
            elog(ERROR, "rb64_gintriconsistent: unknown strategy number: %d",
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
#define RB64_DEFAULT_CONTAIN_SEL   0.005
#define RB64_DEFAULT_OVERLAP_SEL   0.01

/*
 * Shared body for the three restriction estimators.
 *
 * The estimate is a constant placeholder, but the (Var op Const) shape is still
 * resolved so that NULL constants yield 0.0 and non-variable clauses fall back
 * to the default.  A statistics-driven estimate is future work.
 */
static float8
rb64_containment_restriction_sel(FunctionCallInfo fcinfo, float8 default_sel)
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
PG_FUNCTION_INFO_V1(rb64_contain_sel);
Datum rb64_contain_sel(PG_FUNCTION_ARGS);

Datum
rb64_contain_sel(PG_FUNCTION_ARGS) {
    PG_RETURN_FLOAT8(rb64_containment_restriction_sel(fcinfo,
                                                      RB64_DEFAULT_CONTAIN_SEL));
}

//bitmap contained selectivity
PG_FUNCTION_INFO_V1(rb64_contained_sel);
Datum rb64_contained_sel(PG_FUNCTION_ARGS);

Datum
rb64_contained_sel(PG_FUNCTION_ARGS) {
    PG_RETURN_FLOAT8(rb64_containment_restriction_sel(fcinfo,
                                                      RB64_DEFAULT_CONTAIN_SEL));
}

//bitmap overlap selectivity
PG_FUNCTION_INFO_V1(rb64_overlap_sel);
Datum rb64_overlap_sel(PG_FUNCTION_ARGS);

Datum
rb64_overlap_sel(PG_FUNCTION_ARGS) {
    PG_RETURN_FLOAT8(rb64_containment_restriction_sel(fcinfo,
                                                      RB64_DEFAULT_OVERLAP_SEL));
}

/* ---------------------------------------------------------------------------
 * statistics support
 * ---------------------------------------------------------------------------
 *
 * The btree operator class, introduced in version 1.3, gives the type both "<"
 * and "=", so a plain std_typanalyze() would collect full scalar statistics:
 * up to 300 * default_statistics_target bitmaps sorted with rb64_cmp(), plus an
 * MCV list and a 101-entry histogram per column. For bitmaps that buys almost
 * nothing --  cannot support collection of most common values (MCV) for elements
 * inside bitmaps and values above WIDTH_THRESHOLD are dropped before any comparison
 * -- while the planner has to detoast and walk a large pg_statistic row on every query.
 *
 * rb64_typanalyze therefore keeps the pre-1.3 behaviour: null fraction, average
 * stored width and an unknown distinct count.  No comparison, no sort, no extra
 * storage, and the planner falls back to the default selectivities it used
 * before the opclasses existed.
 *
 */

static void rb64_compute_stats(VacAttrStatsP stats,
                               AnalyzeAttrFetchFunc fetchfunc,
                               int samplerows, double totalrows);

/*
 * rb64_typanalyze -- collect only the trivial statistics for roaringbitmap64.
 *
 * Returning false would drop the column from ANALYZE altogether, losing even
 * the null fraction and the average width.  std_typanalyze() is called first so
 * that attstattarget / minrows keep the backend's handling; only the half that
 * collects values is replaced.
 */
PG_FUNCTION_INFO_V1(rb64_typanalyze);
Datum rb64_typanalyze(PG_FUNCTION_ARGS);

Datum
rb64_typanalyze(PG_FUNCTION_ARGS) {
    VacAttrStats *stats = (VacAttrStats *) PG_GETARG_POINTER(0);

	/*
	 * Call the standard typanalyze function.  It may fail to find needed
	 * operators, in which case we also can't do anything, so just fail.
	 */
	if (!std_typanalyze(stats))
		PG_RETURN_BOOL(false);

    stats->compute_stats = rb64_compute_stats;

    PG_RETURN_BOOL(true);
}

/*
 * rb64_compute_stats -- null fraction and average width, nothing else.
 *
 * Same output as the backend's static compute_trivial_stats(), which cannot be
 * called from an extension.  Nothing is compared, hashed, sorted or copied, and
 * VARSIZE_ANY() reads only the varlena header, so a TOASTed value contributes
 * its external pointer size rather than its full size.  The pg_statistic row is
 * therefore a constant ~100 bytes with no datums in it, whatever the bitmaps
 * look like.
 */
static void
rb64_compute_stats(VacAttrStatsP stats, AnalyzeAttrFetchFunc fetchfunc,
                   int samplerows, double totalrows)
{
    int         i;
    int         null_cnt = 0;
    int         nonnull_cnt = 0;
    double      total_width = 0;
    /* roaringbitmap64 is a varlena type: not byval and typlen == -1 */
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
