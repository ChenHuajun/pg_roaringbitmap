
-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION varbitex" to load this file. \quit

--- data type --

CREATE OR REPLACE FUNCTION roaringbitmap_in(cstring)
   RETURNS roaringbitmap
   AS 'MODULE_PATHNAME','roaringbitmap_in'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION roaringbitmap_out(roaringbitmap)
   RETURNS cstring
   AS 'MODULE_PATHNAME','roaringbitmap_out'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION roaringbitmap_recv(internal)
   RETURNS roaringbitmap
   AS 'MODULE_PATHNAME','roaringbitmap_recv'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION roaringbitmap_send(roaringbitmap)
   RETURNS bytea
   AS 'MODULE_PATHNAME','roaringbitmap_send'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

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
   AS 'MODULE_PATHNAME', 'rb_build'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;


CREATE OR REPLACE FUNCTION rb_or(roaringbitmap, roaringbitmap)
   RETURNS roaringbitmap 
   AS 'MODULE_PATHNAME', 'rb_or'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;


CREATE OR REPLACE FUNCTION rb_or_cardinality(roaringbitmap, roaringbitmap)
   RETURNS integer
   AS 'MODULE_PATHNAME', 'rb_or_cardinality'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_and(roaringbitmap, roaringbitmap)
   RETURNS roaringbitmap 
   AS 'MODULE_PATHNAME', 'rb_and'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;


CREATE OR REPLACE FUNCTION rb_and_cardinality(roaringbitmap, roaringbitmap)
   RETURNS integer
   AS 'MODULE_PATHNAME', 'rb_and_cardinality'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;


CREATE OR REPLACE FUNCTION rb_xor(roaringbitmap, roaringbitmap)
   RETURNS roaringbitmap 
   AS 'MODULE_PATHNAME', 'rb_xor'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;


CREATE OR REPLACE FUNCTION rb_xor_cardinality(roaringbitmap, roaringbitmap)
   RETURNS integer
   AS 'MODULE_PATHNAME', 'rb_xor_cardinality'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;


CREATE OR REPLACE FUNCTION rb_andnot(roaringbitmap, roaringbitmap)
   RETURNS roaringbitmap 
   AS 'MODULE_PATHNAME', 'rb_andnot'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;


CREATE OR REPLACE FUNCTION rb_andnot_cardinality(roaringbitmap, roaringbitmap)
   RETURNS integer
   AS 'MODULE_PATHNAME', 'rb_andnot_cardinality'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;


CREATE OR REPLACE FUNCTION rb_cardinality(roaringbitmap)
   RETURNS integer 
   AS 'MODULE_PATHNAME', 'rb_cardinality'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;


CREATE OR REPLACE FUNCTION rb_is_empty(roaringbitmap)
  RETURNS bool
  AS  'MODULE_PATHNAME', 'rb_is_empty'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;


CREATE OR REPLACE FUNCTION rb_equals(roaringbitmap, roaringbitmap)
  RETURNS bool
  AS  'MODULE_PATHNAME', 'rb_equals'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;


CREATE OR REPLACE FUNCTION rb_intersect(roaringbitmap, roaringbitmap)
  RETURNS bool
  AS  'MODULE_PATHNAME', 'rb_intersect'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_remove(roaringbitmap, integer)
   RETURNS roaringbitmap
   AS 'MODULE_PATHNAME', 'rb_remove'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_flip(roaringbitmap, integer, integer)
   RETURNS roaringbitmap
   AS 'MODULE_PATHNAME', 'rb_flip'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_minimum(roaringbitmap)
   RETURNS integer
   AS 'MODULE_PATHNAME', 'rb_minimum'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_maximum(roaringbitmap)
   RETURNS integer
   AS 'MODULE_PATHNAME', 'rb_maximum'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

 CREATE OR REPLACE FUNCTION rb_rank(roaringbitmap, integer)
   RETURNS integer
   AS 'MODULE_PATHNAME', 'rb_rank'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_iterate(roaringbitmap)
   RETURNS SETOF integer 
   AS 'MODULE_PATHNAME', 'rb_iterate'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;


-- aggragations --

CREATE OR REPLACE FUNCTION rb_serialize(internal)
     RETURNS roaringbitmap
     AS 'MODULE_PATHNAME', 'rb_serialize'
     LANGUAGE C IMMUTABLE PARALLEL SAFE;


CREATE OR REPLACE FUNCTION rb_cardinality_trans(internal)
     RETURNS integer
     AS 'MODULE_PATHNAME', 'rb_cardinality_trans'
     LANGUAGE C IMMUTABLE PARALLEL SAFE;


CREATE OR REPLACE FUNCTION rb_or_trans(internal, roaringbitmap)
     RETURNS internal
      AS 'MODULE_PATHNAME', 'rb_or_trans'
     LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE AGGREGATE rb_or_agg(roaringbitmap)(
       SFUNC = rb_or_trans,
       STYPE = internal,
       FINALFUNC = rb_serialize,
       PARALLEL = SAFE
);


CREATE AGGREGATE rb_or_cardinality_agg(roaringbitmap)(
       SFUNC = rb_or_trans,
       STYPE = internal,
       FINALFUNC = rb_cardinality_trans,
       PARALLEL = SAFE
);


CREATE OR REPLACE FUNCTION rb_and_trans(internal, roaringbitmap)
     RETURNS internal
      AS 'MODULE_PATHNAME', 'rb_and_trans'
     LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE AGGREGATE rb_and_agg(roaringbitmap)(
       SFUNC = rb_and_trans,
       STYPE = internal,
       FINALFUNC = rb_serialize,
       PARALLEL = SAFE
);


CREATE AGGREGATE rb_and_cardinality_agg(roaringbitmap)(
       SFUNC = rb_and_trans,
       STYPE = internal,
       FINALFUNC = rb_cardinality_trans,
       PARALLEL = SAFE
);


CREATE OR REPLACE FUNCTION rb_xor_trans(internal, roaringbitmap)
     RETURNS internal
      AS 'MODULE_PATHNAME', 'rb_xor_trans'
     LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE AGGREGATE rb_xor_agg(roaringbitmap)(
       SFUNC = rb_xor_trans,
       STYPE = internal,
       FINALFUNC = rb_serialize,
       PARALLEL = SAFE
);


CREATE AGGREGATE rb_xor_cardinality_agg(roaringbitmap)(
       SFUNC = rb_xor_trans,
       STYPE = internal,
       FINALFUNC = rb_cardinality_trans,
       PARALLEL = SAFE
);

CREATE OR REPLACE FUNCTION rb_build_trans(internal, integer)
     RETURNS internal
      AS 'MODULE_PATHNAME', 'rb_build_trans'
     LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE  AGGREGATE rb_build_agg(integer)(
       SFUNC = rb_build_trans,
       STYPE = internal,
       FINALFUNC = rb_serialize,
       PARALLEL = SAFE
);
