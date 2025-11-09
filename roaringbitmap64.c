#include "roaringbitmap.h"


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
    return DirectFunctionCall1(bytearecv, PG_GETARG_DATUM(0));
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
