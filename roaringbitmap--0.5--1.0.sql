
-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION roaringbitmap" to load this file. \quit

CREATE OR REPLACE FUNCTION rb_contains(roaringbitmap, integer)
  RETURNS bool 
  AS 'MODULE_PATHNAME', 'rb_exists'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

--
-- Type definition for 64 bit Roaring Bitmaps
--

CREATE TYPE roaringbitmap64;

CREATE OR REPLACE FUNCTION roaringbitmap64_in(cstring)
  RETURNS roaringbitmap64
  AS 'MODULE_PATHNAME','roaringbitmap64_in'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION roaringbitmap64_out(roaringbitmap64)
  RETURNS cstring
  AS 'MODULE_PATHNAME','roaringbitmap64_out'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION roaringbitmap64_recv(internal)
  RETURNS roaringbitmap64
  AS 'MODULE_PATHNAME','roaringbitmap64_recv'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION roaringbitmap64_send(roaringbitmap64)
  RETURNS bytea
  AS 'MODULE_PATHNAME','roaringbitmap64_send'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE TYPE roaringbitmap64 (
  INTERNALLENGTH = VARIABLE,
  INPUT = roaringbitmap64_in,
  OUTPUT = roaringbitmap64_out,
  receive = roaringbitmap64_recv,
  send = roaringbitmap64_send,
  STORAGE = external
);

--
-- type cast for bytea
--
CREATE OR REPLACE FUNCTION roaringbitmap64(bytea)
  RETURNS roaringbitmap64
  AS 'MODULE_PATHNAME','rb64_from_bytea'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE CAST (roaringbitmap64 AS bytea) WITHOUT FUNCTION;

CREATE CAST (bytea AS roaringbitmap64) WITH FUNCTION roaringbitmap64(bytea);

--
-- type cast for roaringbitmap
--
CREATE OR REPLACE FUNCTION rb64_to_roaringbitmap(roaringbitmap64)
  RETURNS roaringbitmap
  AS 'MODULE_PATHNAME', 'rb64_to_roaringbitmap'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_from_roaringbitmap(roaringbitmap)
  RETURNS roaringbitmap64
  AS 'MODULE_PATHNAME', 'rb64_from_roaringbitmap'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE CAST (roaringbitmap64 AS roaringbitmap) WITH FUNCTION rb64_to_roaringbitmap(roaringbitmap64);
CREATE CAST (roaringbitmap AS roaringbitmap64) WITH FUNCTION rb64_from_roaringbitmap(roaringbitmap);

--
-- Operator Functions
--
CREATE OR REPLACE FUNCTION rb64_and(roaringbitmap64, roaringbitmap64)
  RETURNS roaringbitmap64 
  AS 'MODULE_PATHNAME', 'rb64_and'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_or(roaringbitmap64, roaringbitmap64)
  RETURNS roaringbitmap64 
  AS 'MODULE_PATHNAME', 'rb64_or'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_xor(roaringbitmap64, roaringbitmap64)
  RETURNS roaringbitmap64 
  AS 'MODULE_PATHNAME', 'rb64_xor'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_andnot(roaringbitmap64, roaringbitmap64)
  RETURNS roaringbitmap64 
  AS 'MODULE_PATHNAME', 'rb64_andnot'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_add(roaringbitmap64, bigint)
  RETURNS roaringbitmap64 
  AS 'MODULE_PATHNAME', 'rb64_add'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_add(bigint, roaringbitmap64)
  RETURNS roaringbitmap64 
  AS 'SELECT rb64_add($2, $1);'
  LANGUAGE SQL STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_remove(roaringbitmap64, bigint)
  RETURNS roaringbitmap64 
  AS 'MODULE_PATHNAME', 'rb64_remove'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_shiftright(roaringbitmap64, bigint)
  RETURNS roaringbitmap64
  AS 'MODULE_PATHNAME', 'rb64_shiftright'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_shiftleft(roaringbitmap64, bigint)
  RETURNS roaringbitmap64 
  AS 'SELECT rb64_shiftright($1, -$2);'
  LANGUAGE SQL STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_contains(roaringbitmap64, roaringbitmap64)
  RETURNS bool
  AS  'MODULE_PATHNAME', 'rb64_contains'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_contains(roaringbitmap64, bigint)
  RETURNS bool 
  AS 'MODULE_PATHNAME', 'rb64_exists'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_containedby(roaringbitmap64, roaringbitmap64)
  RETURNS bool
  AS  'MODULE_PATHNAME', 'rb64_containedby'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_containedby(bigint, roaringbitmap64)
  RETURNS bool 
  AS 'SELECT rb64_contains($2, $1);'
  LANGUAGE SQL STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_intersect(roaringbitmap64, roaringbitmap64)
  RETURNS bool
  AS  'MODULE_PATHNAME', 'rb64_intersect'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_equals(roaringbitmap64, roaringbitmap64)
  RETURNS bool
  AS  'MODULE_PATHNAME', 'rb64_equals'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_not_equals(roaringbitmap64, roaringbitmap64)
  RETURNS bool
  AS  'MODULE_PATHNAME', 'rb64_not_equals'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

--
-- Functions
--

CREATE OR REPLACE FUNCTION rb64_build(bigint[])
  RETURNS roaringbitmap64 
  AS 'MODULE_PATHNAME', 'rb64_build'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_index(roaringbitmap64, bigint)
  RETURNS bigint 
  AS 'MODULE_PATHNAME', 'rb64_index'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_cardinality(roaringbitmap64)
  RETURNS bigint 
  AS 'MODULE_PATHNAME', 'rb64_cardinality'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_and_cardinality(roaringbitmap64, roaringbitmap64)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb64_and_cardinality'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_or_cardinality(roaringbitmap64, roaringbitmap64)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb64_or_cardinality'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_xor_cardinality(roaringbitmap64, roaringbitmap64)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb64_xor_cardinality'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_andnot_cardinality(roaringbitmap64, roaringbitmap64)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb64_andnot_cardinality'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_is_empty(roaringbitmap64)
  RETURNS bool
  AS  'MODULE_PATHNAME', 'rb64_is_empty'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_fill(roaringbitmap64, range_start bigint, range_end bigint)
  RETURNS roaringbitmap64
  AS 'MODULE_PATHNAME', 'rb64_fill'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_clear(roaringbitmap64, range_start bigint, range_end bigint)
  RETURNS roaringbitmap64
  AS 'MODULE_PATHNAME', 'rb64_clear'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_flip(roaringbitmap64, range_start bigint, range_end bigint)
  RETURNS roaringbitmap64
  AS 'MODULE_PATHNAME', 'rb64_flip'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_range(roaringbitmap64, range_start bigint, range_end bigint)
  RETURNS roaringbitmap64
  AS 'MODULE_PATHNAME', 'rb64_range'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_range_cardinality(roaringbitmap64, range_start bigint, range_end bigint)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb64_range_cardinality'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_min(roaringbitmap64)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb64_min'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_max(roaringbitmap64)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb64_max'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

 CREATE OR REPLACE FUNCTION rb64_rank(roaringbitmap64, bigint)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb64_rank'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_jaccard_dist(roaringbitmap64, roaringbitmap64)
  RETURNS float8
  AS 'MODULE_PATHNAME', 'rb64_jaccard_dist'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_select(roaringbitmap64, bitset_limit bigint,bitset_offset bigint=0,reverse boolean=false,range_start bigint=0,range_end bigint=0)
  RETURNS roaringbitmap64
  AS 'MODULE_PATHNAME', 'rb64_select'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_to_array(roaringbitmap64)
  RETURNS bigint[]
  AS 'MODULE_PATHNAME', 'rb64_to_array'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_iterate(roaringbitmap64)
   RETURNS SETOF bigint
   AS 'MODULE_PATHNAME', 'rb64_iterate'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

--
-- Operators
--

CREATE OPERATOR & (
  LEFTARG = roaringbitmap64,
  RIGHTARG = roaringbitmap64,
  PROCEDURE = rb64_and
);

CREATE OPERATOR | (
  LEFTARG = roaringbitmap64,
  RIGHTARG = roaringbitmap64,
  PROCEDURE = rb64_or
);

CREATE OPERATOR | (
  LEFTARG = roaringbitmap64,
  RIGHTARG = bigint,
  PROCEDURE = rb64_add
);

CREATE OPERATOR | (
  LEFTARG = bigint,
  RIGHTARG = roaringbitmap64,
  PROCEDURE = rb64_add
);

CREATE OPERATOR # (
  LEFTARG = roaringbitmap64,
  RIGHTARG = roaringbitmap64,
  PROCEDURE = rb64_xor
);

CREATE OPERATOR << (
  LEFTARG = roaringbitmap64,
  RIGHTARG = bigint,
  PROCEDURE = rb64_shiftleft
);

CREATE OPERATOR >> (
  LEFTARG = roaringbitmap64,
  RIGHTARG = bigint,
  PROCEDURE = rb64_shiftright
);

CREATE OPERATOR - (
  LEFTARG = roaringbitmap64,
  RIGHTARG = roaringbitmap64,
  PROCEDURE = rb64_andnot
);

CREATE OPERATOR - (
  LEFTARG = roaringbitmap64,
  RIGHTARG = bigint,
  PROCEDURE = rb64_remove
);

CREATE OPERATOR @> (
  LEFTARG = roaringbitmap64,
  RIGHTARG = roaringbitmap64,
  PROCEDURE = rb64_contains,
  COMMUTATOR = '<@',
  RESTRICT = contsel,
  JOIN = contjoinsel
);

CREATE OPERATOR @> (
  LEFTARG = roaringbitmap64,
  RIGHTARG = bigint,
  PROCEDURE = rb64_contains,
  COMMUTATOR = '<@',
  RESTRICT = contsel,
  JOIN = contjoinsel
);

CREATE OPERATOR <@ (
  LEFTARG = roaringbitmap64,
  RIGHTARG = roaringbitmap64,
  PROCEDURE = rb64_containedby,
  COMMUTATOR = '@>',
  RESTRICT = contsel,
  JOIN = contjoinsel
);

CREATE OPERATOR <@ (
  LEFTARG = bigint,
  RIGHTARG = roaringbitmap64,
  PROCEDURE = rb64_containedby,
  COMMUTATOR = '@>',
  RESTRICT = contsel,
  JOIN = contjoinsel
);

CREATE OPERATOR && (
  LEFTARG = roaringbitmap64,
  RIGHTARG = roaringbitmap64,
  PROCEDURE = rb64_intersect,
  COMMUTATOR = '&&',
  RESTRICT = contsel,
  JOIN = contjoinsel
);

CREATE OPERATOR = (
  LEFTARG = roaringbitmap64,
  RIGHTARG = roaringbitmap64,
  PROCEDURE = rb64_equals,
  COMMUTATOR = '=',
  NEGATOR = '<>',
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = roaringbitmap64,
  RIGHTARG = roaringbitmap64,
  PROCEDURE = rb64_not_equals,
  COMMUTATOR = '<>',
  NEGATOR = '=',
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

--
-- Aggragations
--

CREATE OR REPLACE FUNCTION rb64_final(internal)
  RETURNS roaringbitmap64
  AS 'MODULE_PATHNAME', 'rb64_serialize'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_serialize(internal)
  RETURNS bytea
  AS 'MODULE_PATHNAME', 'rb64_serialize'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_deserialize(bytea,internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb64_deserialize'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_cardinality_final(internal)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb64_cardinality_final'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;


CREATE OR REPLACE FUNCTION rb64_or_trans(internal, roaringbitmap64)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb64_or_trans'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_or_combine(internal, internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb64_or_combine'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE AGGREGATE rb64_or_agg(roaringbitmap64)(
  SFUNC = rb64_or_trans,
  STYPE = internal,
  FINALFUNC = rb64_final,
  COMBINEFUNC = rb64_or_combine,
  SERIALFUNC = rb64_serialize,
  DESERIALFUNC = rb64_deserialize,
  PARALLEL = SAFE
);

CREATE AGGREGATE rb64_or_cardinality_agg(roaringbitmap64)(
  SFUNC = rb64_or_trans,
  STYPE = internal,
  FINALFUNC = rb64_cardinality_final,
  COMBINEFUNC = rb64_or_combine,
  SERIALFUNC = rb64_serialize,
  DESERIALFUNC = rb64_deserialize,
  PARALLEL = SAFE
);


CREATE OR REPLACE FUNCTION rb64_and_trans(internal, roaringbitmap64)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb64_and_trans'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_and_combine(internal, internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb64_and_combine'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE AGGREGATE rb64_and_agg(roaringbitmap64)(
  SFUNC = rb64_and_trans,
  STYPE = internal,
  FINALFUNC = rb64_final,
  COMBINEFUNC = rb64_and_combine,
  SERIALFUNC = rb64_serialize,
  DESERIALFUNC = rb64_deserialize,
  PARALLEL = SAFE
);


CREATE AGGREGATE rb64_and_cardinality_agg(roaringbitmap64)(
  SFUNC = rb64_and_trans,
  STYPE = internal,
  FINALFUNC = rb64_cardinality_final,
  COMBINEFUNC = rb64_and_combine,
  SERIALFUNC = rb64_serialize,
  DESERIALFUNC = rb64_deserialize,
  PARALLEL = SAFE
);


CREATE OR REPLACE FUNCTION rb64_xor_trans(internal, roaringbitmap64)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb64_xor_trans'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_xor_combine(internal, internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb64_xor_combine'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE AGGREGATE rb64_xor_agg(roaringbitmap64)(
  SFUNC = rb64_xor_trans,
  STYPE = internal,
  FINALFUNC = rb64_final,
  COMBINEFUNC = rb64_xor_combine,
  SERIALFUNC = rb64_serialize,
  DESERIALFUNC = rb64_deserialize,
  PARALLEL = SAFE
);


CREATE AGGREGATE rb64_xor_cardinality_agg(roaringbitmap64)(
  SFUNC = rb64_xor_trans,
  STYPE = internal,
  FINALFUNC = rb64_cardinality_final,
  COMBINEFUNC = rb64_xor_combine,
  SERIALFUNC = rb64_serialize,
  DESERIALFUNC = rb64_deserialize,
  PARALLEL = SAFE
);

CREATE OR REPLACE FUNCTION rb64_build_trans(internal, bigint)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb64_build_trans'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE  AGGREGATE rb64_build_agg(bigint)(
  SFUNC = rb64_build_trans,
  STYPE = internal,
  FINALFUNC = rb64_final,
  COMBINEFUNC = rb64_or_combine,
  SERIALFUNC = rb64_serialize,
  DESERIALFUNC = rb64_deserialize,
  PARALLEL = SAFE
);
