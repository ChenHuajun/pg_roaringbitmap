CREATE FUNCTION rb_group_elements_by_source(bitmaps roaringbitmap[])
  RETURNS TABLE (sources int[], members roaringbitmap)
  AS 'MODULE_PATHNAME', 'rb_group_elements_by_source'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION rb64_group_elements_by_source(bitmaps roaringbitmap64[])
  RETURNS TABLE (sources int[], members roaringbitmap64)
  AS 'MODULE_PATHNAME', 'rb64_group_elements_by_source'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

--
-- ============================================================================
-- btree / hash / gin operator classes + restriction estimators
-- ============================================================================
--
--   A. sort support functions + 4 comparison operators (btree prerequisite)
--   B. in-place patches to pre-existing objects
--   C. btree / hash / gin operator classes
--   D. RESTRICT estimators + attachment
--   E. trivial statistics for the type (typanalyze)
--

-- ---------------------------------------------------------------------------
-- A. sort support functions + comparison operators (btree prerequisite)
-- ---------------------------------------------------------------------------
--
CREATE FUNCTION rb_cmp(roaringbitmap, roaringbitmap)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'rb_cmp'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION rb_lt(roaringbitmap, roaringbitmap)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'rb_lt'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION rb_le(roaringbitmap, roaringbitmap)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'rb_le'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION rb_ge(roaringbitmap, roaringbitmap)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'rb_ge'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION rb_gt(roaringbitmap, roaringbitmap)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'rb_gt'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR < (
  LEFTARG = roaringbitmap,
  RIGHTARG = roaringbitmap,
  PROCEDURE = rb_lt,
  COMMUTATOR = '>',
  NEGATOR = '>=',
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR <= (
  LEFTARG = roaringbitmap,
  RIGHTARG = roaringbitmap,
  PROCEDURE = rb_le,
  COMMUTATOR = '>=',
  NEGATOR = '>',
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = roaringbitmap,
  RIGHTARG = roaringbitmap,
  PROCEDURE = rb_ge,
  COMMUTATOR = '<=',
  NEGATOR = '<',
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR > (
  LEFTARG = roaringbitmap,
  RIGHTARG = roaringbitmap,
  PROCEDURE = rb_gt,
  COMMUTATOR = '<',
  NEGATOR = '<=',
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

-- ---------------------------------------------------------------------------
-- B. in-place patches to pre-existing objects
-- ---------------------------------------------------------------------------
--
-- 1) = (roaringbitmap, roaringbitmap) gains HASHES and MERGES.  HASHES enables
--    hash join / hash aggregation / hash partitioning; MERGES is what makes the
--    planner consider a merge join on =.
--
-- 2) rb_containedby(integer, roaringbitmap) is a LANGUAGE SQL wrapper whose old
--    body 'SELECT rb_contains($2, $1);' change to 'SELECT $2 @> $1;'so "42 <@ rb"
--    could not use the GIN index.
--
-- 3) The operator functions behind &&, @>, <@ and = get COST 100.  Comparing two
--    bitmaps costs far more than the default cost of 1 suggests, and it also
--    compensates for the planner costing a Seq Scan without accounting for the
--    TOAST pages that have to be read to detoast every large bitmap; with the
--    default cost the planner never picks the GIN index on high-cardinality
--    columns.
--
ALTER OPERATOR = (roaringbitmap, roaringbitmap) SET (MERGES = true, HASHES = true);

CREATE OR REPLACE FUNCTION rb_containedby(integer, roaringbitmap)
  RETURNS boolean
  AS 'SELECT $2 @> $1;'
  LANGUAGE SQL STRICT IMMUTABLE PARALLEL SAFE;

ALTER FUNCTION rb_contains(roaringbitmap, roaringbitmap)     COST 100;
ALTER FUNCTION rb_contains(roaringbitmap, integer)           COST 100;
ALTER FUNCTION rb_containedby(roaringbitmap, roaringbitmap)  COST 100;
ALTER FUNCTION rb_containedby(integer, roaringbitmap)        COST 100;
ALTER FUNCTION rb_intersect(roaringbitmap, roaringbitmap)    COST 100;
ALTER FUNCTION rb_equals(roaringbitmap, roaringbitmap)       COST 100;

-- ---------------------------------------------------------------------------
-- C. btree / hash / gin operator classes
-- ---------------------------------------------------------------------------
--

-- C-1. btree operator class
CREATE OPERATOR CLASS roaringbitmap_ops
  DEFAULT FOR TYPE roaringbitmap USING btree AS
    OPERATOR 1 <,
    OPERATOR 2 <=,
    OPERATOR 3 =,
    OPERATOR 4 >=,
    OPERATOR 5 >,
    FUNCTION 1 rb_cmp(roaringbitmap, roaringbitmap);

-- C-2. hash operator class
CREATE FUNCTION rb_hash(roaringbitmap)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'rb_hash'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION rb_hash_extended(roaringbitmap, bigint)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb_hash_extended'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR CLASS roaringbitmap_ops
  DEFAULT FOR TYPE roaringbitmap USING hash AS
    OPERATOR 1 =,
    FUNCTION 1 rb_hash(roaringbitmap),
    FUNCTION 2 rb_hash_extended(roaringbitmap, bigint);

-- C-3. gin operator class
CREATE FUNCTION rb_ginextract_value(roaringbitmap, internal, internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb_ginextract_value'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION rb_ginextract_query(roaringbitmap, internal, int2,
                                    internal, internal, internal, internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb_ginextract_query'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION rb_ginconsistent(internal, int2, roaringbitmap, int4,
                                 internal, internal, internal, internal)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'rb_ginconsistent'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION rb_gintriconsistent(internal, int2, roaringbitmap, int4,
                                    internal, internal, internal)
  RETURNS "char"
  AS 'MODULE_PATHNAME', 'rb_gintriconsistent'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR CLASS roaringbitmap_ops
  DEFAULT FOR TYPE roaringbitmap USING gin AS
    OPERATOR 1 && (roaringbitmap, roaringbitmap),
    OPERATOR 2 @> (roaringbitmap, roaringbitmap),
    OPERATOR 3 <@ (roaringbitmap, roaringbitmap),
    OPERATOR 4 = (roaringbitmap, roaringbitmap),
    OPERATOR 5 @> (roaringbitmap, int4),
    FUNCTION 1 btint4cmp(int4, int4),
    FUNCTION 2 rb_ginextract_value(roaringbitmap, internal, internal),
    FUNCTION 3 rb_ginextract_query(roaringbitmap, internal, int2,
                                   internal, internal, internal, internal),
    FUNCTION 4 rb_ginconsistent(internal, int2, roaringbitmap, int4,
                                internal, internal, internal, internal),
    FUNCTION 6 rb_gintriconsistent(internal, int2, roaringbitmap, int4,
                                   internal, internal, internal),
    STORAGE int4;

-- ---------------------------------------------------------------------------
-- D. restriction estimators (RESTRICT only, JOIN stays contjoinsel)
-- ---------------------------------------------------------------------------
--
CREATE FUNCTION rb_contain_sel(internal, oid, internal, integer)
  RETURNS float8
  AS 'MODULE_PATHNAME', 'rb_contain_sel'
  LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE FUNCTION rb_contained_sel(internal, oid, internal, integer)
  RETURNS float8
  AS 'MODULE_PATHNAME', 'rb_contained_sel'
  LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE FUNCTION rb_overlap_sel(internal, oid, internal, integer)
  RETURNS float8
  AS 'MODULE_PATHNAME', 'rb_overlap_sel'
  LANGUAGE C STABLE STRICT PARALLEL SAFE;

ALTER OPERATOR @> (roaringbitmap, roaringbitmap) SET (RESTRICT = rb_contain_sel);
ALTER OPERATOR @> (roaringbitmap, int4)          SET (RESTRICT = rb_contain_sel);
ALTER OPERATOR <@ (roaringbitmap, roaringbitmap) SET (RESTRICT = rb_contained_sel);
ALTER OPERATOR <@ (int4, roaringbitmap)          SET (RESTRICT = rb_contained_sel);
ALTER OPERATOR && (roaringbitmap, roaringbitmap) SET (RESTRICT = rb_overlap_sel);

-- ---------------------------------------------------------------------------
-- E. trivial statistics for the type (typanalyze)
-- ---------------------------------------------------------------------------
--
-- Section A gives the type "<" and "=", which would make std_typanalyze() pick
-- compute_scalar_stats(): ANALYZE would sort up to 30000 bitmaps and store an MCV
-- list plus a histogram that the planner then detoasts on every query.  On big
-- bitmaps that buys nothing, since values above WIDTH_THRESHOLD are dropped
-- before any comparison.
--
-- rb_typanalyze() keeps only what 1.2 and earlier collected, so the cost goes
-- back to one pass over the sample. 
--
CREATE FUNCTION rb_typanalyze(internal)
  RETURNS boolean
  AS 'MODULE_PATHNAME','rb_typanalyze'
  LANGUAGE C STABLE PARALLEL SAFE;

ALTER TYPE roaringbitmap SET (ANALYZE = rb_typanalyze);
