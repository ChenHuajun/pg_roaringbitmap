SET search_path = public;

DROP FUNCTION roaringbitmap_in(cstring)
   RETURNS roaringbitmap
   AS 'roaringbitmap.so','roaringbitmap_in'
   LANGUAGE C STRICT IMMUTABLE;

DROP FUNCTION roaringbitmap_out(roaringbitmap)
   RETURNS cstring
   AS 'roaringbitmap.so','roaringbitmap_out'
   LANGUAGE C STRICT IMMUTABLE;

DROP FUNCTION roaringbitmap_recv(internal)
   RETURNS roaringbitmap
   AS 'roaringbitmap.so','roaringbitmap_recv'
   LANGUAGE C STRICT IMMUTABLE;

DROP FUNCTION roaringbitmap_send(roaringbitmap)
   RETURNS bytea
   AS 'roaringbitmap.so','roaringbitmap_send'
   LANGUAGE C STRICT IMMUTABLE;

CREATE TYPE roaringbitmap (
    INTERNALLENGTH = VARIABLE,
    INPUT = roaringbitmap_in,
    OUTPUT = roaringbitmap_out,
    receive = roaringbitmap_recv,
    send = roaringbitmap_send,
    STORAGE = external
 );

DROP FUNCTION roaringbitmap(bytea)
   RETURNS roaringbitmap
   AS 'roaringbitmap.so','roaringbitmap'
   LANGUAGE C STRICT;

CREATE CAST (bytea AS roaringbitmap) WITH FUNCTION roaringbitmap(bytea);
CREATE CAST (roaringbitmap AS bytea) WITHOUT FUNCTION;

DROP FUNCTION rb_build(integer[])
   RETURNS roaringbitmap 
   AS 'roaringbitmap.so', 'rb_build'
   LANGUAGE C STRICT;

DROP FUNCTION rb_or(roaringbitmap, roaringbitmap)
   RETURNS roaringbitmap 
   AS 'roaringbitmap.so', 'rb_or'
   LANGUAGE C STRICT;

DROP FUNCTION rb_and(roaringbitmap, roaringbitmap)
   RETURNS roaringbitmap 
   AS 'roaringbitmap.so', 'rb_and'
   LANGUAGE C STRICT;

DROP FUNCTION rb_xor(roaringbitmap, roaringbitmap)
   RETURNS roaringbitmap 
   AS 'roaringbitmap.so', 'rb_xor'
   LANGUAGE C STRICT;

DROP FUNCTION rb_andnot(roaringbitmap, roaringbitmap)
   RETURNS roaringbitmap 
   AS 'roaringbitmap.so', 'rb_andnot'
   LANGUAGE C STRICT;

DROP FUNCTION rb_cardinality(roaringbitmap)
   RETURNS integer 
   AS 'roaringbitmap.so', 'rb_cardinality'
   LANGUAGE C STRICT;

DROP FUNCTION rb_iterate(roaringbitmap)
   RETURNS SETOF integer 
   AS 'roaringbitmap.so', 'rb_iterate'
   LANGUAGE C STRICT;

DROP FUNCTION rb_serialize(internal)
     RETURNS roaringbitmap
     AS 'roaringbitmap.so', 'rb_serialize'
     LANGUAGE C IMMUTABLE;

DROP FUNCTION rb_or_trans(internal, roaringbitmap)
     RETURNS internal
      AS 'roaringbitmap.so', 'rb_or_trans'
     LANGUAGE C IMMUTABLE;

CREATE AGGREGATE rb_or_agg(roaringbitmap)(
       SFUNC = rb_or_trans,
       STYPE = internal,
       FINALFUNC = rb_serialize
);

DROP FUNCTION rb_and_trans(internal, roaringbitmap)
     RETURNS internal
      AS 'roaringbitmap.so', 'rb_and_trans'
     LANGUAGE C IMMUTABLE;

CREATE AGGREGATE rb_and_agg(roaringbitmap)(
       SFUNC = rb_and_trans,
       STYPE = internal,
       FINALFUNC = rb_serialize
);

DROP FUNCTION rb_build_trans(internal, integer)
     RETURNS internal
      AS 'roaringbitmap.so', 'rb_build_trans'
     LANGUAGE C;

CREATE  AGGREGATE rb_build_agg(integer)(
       SFUNC = rb_build_trans,
       STYPE = internal,
       FINALFUNC = rb_serialize
);
