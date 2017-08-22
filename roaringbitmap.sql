SET search_path = public;

--- data type --

CREATE OR REPLACE FUNCTION roaringbitmap_in(cstring)
   RETURNS roaringbitmap
   AS 'roaringbitmap.so','roaringbitmap_in'
   LANGUAGE C STRICT IMMUTABLE;

CREATE OR REPLACE FUNCTION roaringbitmap_out(roaringbitmap)
   RETURNS cstring
   AS 'roaringbitmap.so','roaringbitmap_out'
   LANGUAGE C STRICT IMMUTABLE;

CREATE OR REPLACE FUNCTION roaringbitmap_recv(internal)
   RETURNS roaringbitmap
   AS 'roaringbitmap.so','roaringbitmap_recv'
   LANGUAGE C STRICT IMMUTABLE;

CREATE OR REPLACE FUNCTION roaringbitmap_send(roaringbitmap)
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

-- functions --

CREATE OR REPLACE FUNCTION rb_build(integer[])
   RETURNS roaringbitmap 
   AS 'roaringbitmap.so', 'rb_build'
   LANGUAGE C STRICT;


CREATE OR REPLACE FUNCTION rb_or(roaringbitmap, roaringbitmap)
   RETURNS roaringbitmap 
   AS 'roaringbitmap.so', 'rb_or'
   LANGUAGE C STRICT;


CREATE OR REPLACE FUNCTION rb_or_cardinality(roaringbitmap, roaringbitmap)
   RETURNS integer
   AS 'roaringbitmap.so', 'rb_or_cardinality'
   LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION rb_and(roaringbitmap, roaringbitmap)
   RETURNS roaringbitmap 
   AS 'roaringbitmap.so', 'rb_and'
   LANGUAGE C STRICT;


CREATE OR REPLACE FUNCTION rb_and_cardinality(roaringbitmap, roaringbitmap)
   RETURNS integer
   AS 'roaringbitmap.so', 'rb_and_cardinality'
   LANGUAGE C STRICT;


CREATE OR REPLACE FUNCTION rb_xor(roaringbitmap, roaringbitmap)
   RETURNS roaringbitmap 
   AS 'roaringbitmap.so', 'rb_xor'
   LANGUAGE C STRICT;


CREATE OR REPLACE FUNCTION rb_xor_cardinality(roaringbitmap, roaringbitmap)
   RETURNS integer
   AS 'roaringbitmap.so', 'rb_xor_cardinality'
   LANGUAGE C STRICT;


CREATE OR REPLACE FUNCTION rb_andnot(roaringbitmap, roaringbitmap)
   RETURNS roaringbitmap 
   AS 'roaringbitmap.so', 'rb_andnot'
   LANGUAGE C STRICT;


CREATE OR REPLACE FUNCTION rb_andnot_cardinality(roaringbitmap, roaringbitmap)
   RETURNS integer
   AS 'roaringbitmap.so', 'rb_andnot_cardinality'
   LANGUAGE C STRICT;


CREATE OR REPLACE FUNCTION rb_cardinality(roaringbitmap)
   RETURNS integer 
   AS 'roaringbitmap.so', 'rb_cardinality'
   LANGUAGE C STRICT;


CREATE OR REPLACE FUNCTION rb_is_empty(roaringbitmap)
  RETURNS bool
  AS  'roaringbitmap.so', 'rb_is_empty'
   LANGUAGE C STRICT;


CREATE OR REPLACE FUNCTION rb_equals(roaringbitmap, roaringbitmap)
  RETURNS bool
  AS  'roaringbitmap.so', 'rb_equals'
   LANGUAGE C STRICT;


CREATE OR REPLACE FUNCTION rb_intersect(roaringbitmap, roaringbitmap)
  RETURNS bool
  AS  'roaringbitmap.so', 'rb_intersect'
   LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION rb_remove(roaringbitmap, integer)
   RETURNS roaringbitmap
   AS 'roaringbitmap.so', 'rb_remove'
   LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION rb_flip(roaringbitmap, integer, integer)
   RETURNS roaringbitmap
   AS 'roaringbitmap.so', 'rb_flip'
   LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION rb_minimum(roaringbitmap)
   RETURNS integer
   AS 'roaringbitmap.so', 'rb_minimum'
   LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION rb_maximum(roaringbitmap)
   RETURNS integer
   AS 'roaringbitmap.so', 'rb_maximum'
   LANGUAGE C STRICT;

 CREATE OR REPLACE FUNCTION rb_rank(roaringbitmap, integer)
   RETURNS integer
   AS 'roaringbitmap.so', 'rb_rank'
   LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION rb_iterate(roaringbitmap)
   RETURNS SETOF integer 
   AS 'roaringbitmap.so', 'rb_iterate'
   LANGUAGE C STRICT;


CREATE OR REPLACE FUNCTION rb_serialize(internal)
     RETURNS roaringbitmap
     AS 'roaringbitmap.so', 'rb_serialize'
     LANGUAGE C IMMUTABLE;


CREATE OR REPLACE FUNCTION rb_cardinality_trans(internal)
     RETURNS integer
     AS 'roaringbitmap.so', 'rb_cardinality_trans'
     LANGUAGE C IMMUTABLE;


CREATE OR REPLACE FUNCTION rb_or_trans(internal, roaringbitmap)
     RETURNS internal
      AS 'roaringbitmap.so', 'rb_or_trans'
     LANGUAGE C IMMUTABLE;

CREATE AGGREGATE rb_or_agg(roaringbitmap)(
       SFUNC = rb_or_trans,
       STYPE = internal,
       FINALFUNC = rb_serialize
);

-- aggragations --

CREATE AGGREGATE rb_or_cardinality_agg(roaringbitmap)(
       SFUNC = rb_or_trans,
       STYPE = internal,
       FINALFUNC = rb_cardinality_trans
);


CREATE OR REPLACE FUNCTION rb_and_trans(internal, roaringbitmap)
     RETURNS internal
      AS 'roaringbitmap.so', 'rb_and_trans'
     LANGUAGE C IMMUTABLE;

CREATE AGGREGATE rb_and_agg(roaringbitmap)(
       SFUNC = rb_and_trans,
       STYPE = internal,
       FINALFUNC = rb_serialize
);


CREATE AGGREGATE rb_and_cardinality_agg(roaringbitmap)(
       SFUNC = rb_and_trans,
       STYPE = internal,
       FINALFUNC = rb_cardinality_trans
);


CREATE OR REPLACE FUNCTION rb_xor_trans(internal, roaringbitmap)
     RETURNS internal
      AS 'roaringbitmap.so', 'rb_xor_trans'
     LANGUAGE C IMMUTABLE;

CREATE AGGREGATE rb_xor_agg(roaringbitmap)(
       SFUNC = rb_xor_trans,
       STYPE = internal,
       FINALFUNC = rb_serialize
);


CREATE AGGREGATE rb_xor_cardinality_agg(roaringbitmap)(
       SFUNC = rb_xor_trans,
       STYPE = internal,
       FINALFUNC = rb_cardinality_trans
);

CREATE OR REPLACE FUNCTION rb_build_trans(internal, integer)
     RETURNS internal
      AS 'roaringbitmap.so', 'rb_build_trans'
     LANGUAGE C;

CREATE  AGGREGATE rb_build_agg(integer)(
       SFUNC = rb_build_trans,
       STYPE = internal,
       FINALFUNC = rb_serialize
);
