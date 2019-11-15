
-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION roaringbitmap" to load this file. \quit

--
-- Type definition for Roaring Bitmaps
--

CREATE TYPE roaringbitmap;

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

--
-- type cast for bytea
--
CREATE OR REPLACE FUNCTION roaringbitmap(bytea)
  RETURNS roaringbitmap
  AS 'MODULE_PATHNAME','rb_from_bytea'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE CAST (roaringbitmap AS bytea) WITHOUT FUNCTION;

CREATE CAST (bytea AS roaringbitmap) WITH FUNCTION roaringbitmap(bytea);

--
-- Operator Functions
--
CREATE OR REPLACE FUNCTION rb_and(roaringbitmap, roaringbitmap)
  RETURNS roaringbitmap 
  AS 'MODULE_PATHNAME', 'rb_and'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_or(roaringbitmap, roaringbitmap)
  RETURNS roaringbitmap 
  AS 'MODULE_PATHNAME', 'rb_or'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_xor(roaringbitmap, roaringbitmap)
  RETURNS roaringbitmap 
  AS 'MODULE_PATHNAME', 'rb_xor'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_andnot(roaringbitmap, roaringbitmap)
  RETURNS roaringbitmap 
  AS 'MODULE_PATHNAME', 'rb_andnot'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_add(roaringbitmap, integer)
  RETURNS roaringbitmap 
  AS 'MODULE_PATHNAME', 'rb_add'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_add(integer, roaringbitmap)
  RETURNS roaringbitmap 
  AS 'SELECT rb_add($2, $1);'
  LANGUAGE SQL STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_remove(roaringbitmap, integer)
  RETURNS roaringbitmap 
  AS 'MODULE_PATHNAME', 'rb_remove'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_shiftright(roaringbitmap, bigint)
  RETURNS roaringbitmap
  AS 'MODULE_PATHNAME', 'rb_shiftright'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_shiftleft(roaringbitmap, bigint)
  RETURNS roaringbitmap 
  AS 'SELECT rb_shiftright($1, -$2);'
  LANGUAGE SQL STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_contains(roaringbitmap, roaringbitmap)
  RETURNS bool
  AS  'MODULE_PATHNAME', 'rb_contains'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_contains(roaringbitmap, integer)
  RETURNS bool 
  AS 'MODULE_PATHNAME', 'rb_exsit'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_containedby(roaringbitmap, roaringbitmap)
  RETURNS bool
  AS  'MODULE_PATHNAME', 'rb_containedby'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_containedby(integer, roaringbitmap)
  RETURNS bool 
  AS 'SELECT rb_contains($2, $1);'
  LANGUAGE SQL STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_intersect(roaringbitmap, roaringbitmap)
  RETURNS bool
  AS  'MODULE_PATHNAME', 'rb_intersect'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_equals(roaringbitmap, roaringbitmap)
  RETURNS bool
  AS  'MODULE_PATHNAME', 'rb_equals'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_not_equals(roaringbitmap, roaringbitmap)
  RETURNS bool
  AS  'MODULE_PATHNAME', 'rb_not_equals'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

--
-- Functions
--

CREATE OR REPLACE FUNCTION rb_build(integer[])
  RETURNS roaringbitmap 
  AS 'MODULE_PATHNAME', 'rb_build'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_index(roaringbitmap, integer)
  RETURNS bigint 
  AS 'MODULE_PATHNAME', 'rb_index'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_cardinality(roaringbitmap)
  RETURNS bigint 
  AS 'MODULE_PATHNAME', 'rb_cardinality'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_and_cardinality(roaringbitmap, roaringbitmap)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb_and_cardinality'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_or_cardinality(roaringbitmap, roaringbitmap)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb_or_cardinality'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_xor_cardinality(roaringbitmap, roaringbitmap)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb_xor_cardinality'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_andnot_cardinality(roaringbitmap, roaringbitmap)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb_andnot_cardinality'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_is_empty(roaringbitmap)
  RETURNS bool
  AS  'MODULE_PATHNAME', 'rb_is_empty'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_fill(roaringbitmap, range_start bigint, range_end bigint)
  RETURNS roaringbitmap
  AS 'MODULE_PATHNAME', 'rb_fill'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_clear(roaringbitmap, range_start bigint, range_end bigint)
  RETURNS roaringbitmap
  AS 'MODULE_PATHNAME', 'rb_clear'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_flip(roaringbitmap, range_start bigint, range_end bigint)
  RETURNS roaringbitmap
  AS 'MODULE_PATHNAME', 'rb_flip'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_range(roaringbitmap, range_start bigint, range_end bigint)
  RETURNS roaringbitmap
  AS 'MODULE_PATHNAME', 'rb_range'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_range_cardinality(roaringbitmap, range_start bigint, range_end bigint)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb_range_cardinality'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_min(roaringbitmap)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'rb_min'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_max(roaringbitmap)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'rb_max'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

 CREATE OR REPLACE FUNCTION rb_rank(roaringbitmap, integer)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb_rank'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_jaccard_dist(roaringbitmap, roaringbitmap)
  RETURNS float8
  AS 'MODULE_PATHNAME', 'rb_jaccard_dist'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_select(roaringbitmap, bitset_limit bigint,bitset_offset bigint=0,reverse boolean=false,range_start bigint=0,range_end bigint=4294967296)
  RETURNS roaringbitmap
  AS 'MODULE_PATHNAME', 'rb_select'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_to_array(roaringbitmap)
  RETURNS integer[]
  AS 'MODULE_PATHNAME', 'rb_to_array'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_iterate(roaringbitmap)
   RETURNS SETOF integer 
   AS 'MODULE_PATHNAME', 'rb_iterate'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

--
-- Operators
--

CREATE OPERATOR & (
  LEFTARG = roaringbitmap,
  RIGHTARG = roaringbitmap,
  PROCEDURE = rb_and
);

CREATE OPERATOR | (
  LEFTARG = roaringbitmap,
  RIGHTARG = roaringbitmap,
  PROCEDURE = rb_or
);

CREATE OPERATOR | (
  LEFTARG = roaringbitmap,
  RIGHTARG = int4,
  PROCEDURE = rb_add
);

CREATE OPERATOR | (
  LEFTARG = int4,
  RIGHTARG = roaringbitmap,
  PROCEDURE = rb_add
);

CREATE OPERATOR # (
  LEFTARG = roaringbitmap,
  RIGHTARG = roaringbitmap,
  PROCEDURE = rb_xor
);

CREATE OPERATOR << (
  LEFTARG = roaringbitmap,
  RIGHTARG = bigint,
  PROCEDURE = rb_shiftleft
);

CREATE OPERATOR >> (
  LEFTARG = roaringbitmap,
  RIGHTARG = bigint,
  PROCEDURE = rb_shiftright
);

CREATE OPERATOR - (
  LEFTARG = roaringbitmap,
  RIGHTARG = roaringbitmap,
  PROCEDURE = rb_andnot
);

CREATE OPERATOR - (
  LEFTARG = roaringbitmap,
  RIGHTARG = int4,
  PROCEDURE = rb_remove
);

CREATE OPERATOR @> (
  LEFTARG = roaringbitmap,
  RIGHTARG = roaringbitmap,
  PROCEDURE = rb_contains,
  COMMUTATOR = '<@',
  RESTRICT = contsel,
  JOIN = contjoinsel
);

CREATE OPERATOR @> (
  LEFTARG = roaringbitmap,
  RIGHTARG = int4,
  PROCEDURE = rb_contains,
  COMMUTATOR = '<@',
  RESTRICT = contsel,
  JOIN = contjoinsel
);

CREATE OPERATOR <@ (
  LEFTARG = roaringbitmap,
  RIGHTARG = roaringbitmap,
  PROCEDURE = rb_containedby,
  COMMUTATOR = '@>',
  RESTRICT = contsel,
  JOIN = contjoinsel
);

CREATE OPERATOR <@ (
  LEFTARG = int4,
  RIGHTARG = roaringbitmap,
  PROCEDURE = rb_containedby,
  COMMUTATOR = '@>',
  RESTRICT = contsel,
  JOIN = contjoinsel
);

CREATE OPERATOR && (
  LEFTARG = roaringbitmap,
  RIGHTARG = roaringbitmap,
  PROCEDURE = rb_intersect,
  COMMUTATOR = '&&',
  RESTRICT = contsel,
  JOIN = contjoinsel
);

CREATE OPERATOR = (
  LEFTARG = roaringbitmap,
  RIGHTARG = roaringbitmap,
  PROCEDURE = rb_equals,
  COMMUTATOR = '=',
  NEGATOR = '<>',
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = roaringbitmap,
  RIGHTARG = roaringbitmap,
  PROCEDURE = rb_not_equals,
  COMMUTATOR = '<>',
  NEGATOR = '=',
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

--
-- Aggragations
--

CREATE OR REPLACE FUNCTION rb_final(internal)
  RETURNS roaringbitmap
  AS 'MODULE_PATHNAME', 'rb_serialize'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_serialize(internal)
  RETURNS bytea
  AS 'MODULE_PATHNAME', 'rb_serialize'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_deserialize(bytea,internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb_deserialize'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_cardinality_final(internal)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb_cardinality_final'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;


CREATE OR REPLACE FUNCTION rb_or_trans(internal, roaringbitmap)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb_or_trans'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_or_combine(internal, internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb_or_combine'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE AGGREGATE rb_or_agg(roaringbitmap)(
  SFUNC = rb_or_trans,
  STYPE = internal,
  FINALFUNC = rb_final,
  COMBINEFUNC = rb_or_combine,
  SERIALFUNC = rb_serialize,
  DESERIALFUNC = rb_deserialize,
  PARALLEL = SAFE
);

CREATE AGGREGATE rb_or_cardinality_agg(roaringbitmap)(
  SFUNC = rb_or_trans,
  STYPE = internal,
  FINALFUNC = rb_cardinality_final,
  COMBINEFUNC = rb_or_combine,
  SERIALFUNC = rb_serialize,
  DESERIALFUNC = rb_deserialize,
  PARALLEL = SAFE
);


CREATE OR REPLACE FUNCTION rb_and_trans(internal, roaringbitmap)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb_and_trans'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_and_combine(internal, internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb_and_combine'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE AGGREGATE rb_and_agg(roaringbitmap)(
  SFUNC = rb_and_trans,
  STYPE = internal,
  FINALFUNC = rb_final,
  COMBINEFUNC = rb_and_combine,
  SERIALFUNC = rb_serialize,
  DESERIALFUNC = rb_deserialize,
  PARALLEL = SAFE
);


CREATE AGGREGATE rb_and_cardinality_agg(roaringbitmap)(
  SFUNC = rb_and_trans,
  STYPE = internal,
  FINALFUNC = rb_cardinality_final,
  COMBINEFUNC = rb_and_combine,
  SERIALFUNC = rb_serialize,
  DESERIALFUNC = rb_deserialize,
  PARALLEL = SAFE
);


CREATE OR REPLACE FUNCTION rb_xor_trans(internal, roaringbitmap)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb_xor_trans'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_xor_combine(internal, internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb_xor_combine'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE AGGREGATE rb_xor_agg(roaringbitmap)(
  SFUNC = rb_xor_trans,
  STYPE = internal,
  FINALFUNC = rb_final,
  COMBINEFUNC = rb_xor_combine,
  SERIALFUNC = rb_serialize,
  DESERIALFUNC = rb_deserialize,
  PARALLEL = SAFE
);


CREATE AGGREGATE rb_xor_cardinality_agg(roaringbitmap)(
  SFUNC = rb_xor_trans,
  STYPE = internal,
  FINALFUNC = rb_cardinality_final,
  COMBINEFUNC = rb_xor_combine,
  SERIALFUNC = rb_serialize,
  DESERIALFUNC = rb_deserialize,
  PARALLEL = SAFE
);

CREATE OR REPLACE FUNCTION rb_build_trans(internal, integer)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb_build_trans'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE  AGGREGATE rb_build_agg(integer)(
  SFUNC = rb_build_trans,
  STYPE = internal,
  FINALFUNC = rb_final,
  COMBINEFUNC = rb_or_combine,
  SERIALFUNC = rb_serialize,
  DESERIALFUNC = rb_deserialize,
  PARALLEL = SAFE
);
