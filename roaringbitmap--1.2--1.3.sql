CREATE FUNCTION rb_group_elements_by_source(bitmaps roaringbitmap[])
  RETURNS TABLE (sources int[], members roaringbitmap)
  AS 'MODULE_PATHNAME', 'rb_group_elements_by_source'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION rb64_group_elements_by_source(bitmaps roaringbitmap64[])
  RETURNS TABLE (sources int[], members roaringbitmap64)
  AS 'MODULE_PATHNAME', 'rb64_group_elements_by_source'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

-- rb_jaccard_dist() and rb64_jaccard_dist() have always returned the Jaccard
-- similarity coefficient despite their names.  They are left untouched for
-- backward compatibility and these correctly named functions are added in
-- their place; the old names are deprecated and may be removed later.
CREATE FUNCTION rb_jaccard_index(roaringbitmap, roaringbitmap)
  RETURNS float8
  AS 'MODULE_PATHNAME', 'rb_jaccard_index'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION rb64_jaccard_index(roaringbitmap64, roaringbitmap64)
  RETURNS float8
  AS 'MODULE_PATHNAME', 'rb64_jaccard_index'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

COMMENT ON FUNCTION rb_jaccard_dist(roaringbitmap, roaringbitmap) IS
  'DEPRECATED: despite its name this function returns the Jaccard similarity coefficient, not a distance. Use rb_jaccard_index(roaringbitmap, roaringbitmap) instead. Kept only for backward compatibility and may be removed in a future release.';

COMMENT ON FUNCTION rb64_jaccard_dist(roaringbitmap64, roaringbitmap64) IS
  'DEPRECATED: despite its name this function returns the Jaccard similarity coefficient, not a distance. Use rb64_jaccard_index(roaringbitmap64, roaringbitmap64) instead. Kept only for backward compatibility and may be removed in a future release.';

--
-- ============================================================================
-- btree / hash / gin operator classes + restriction estimators(for roaringbitmap)
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
--    body 'SELECT rb_contains($2, $1);' cannot use the GIN index; the new body
--    'SELECT $2 @> $1;' lets "42 <@ rb" be matched by the gin operator class.
--
-- 3) The operator functions behind &&, @>, <@ and = get COST 100.  Comparing two
--    bitmaps costs far more than the default cost of 1 suggests, and it also
--    compensates for the planner costing a Seq Scan without accounting for the
--    TOAST pages that have to be read to detoast every large bitmap; with the
--    default cost the planner never picks the GIN index on high-cardinality
--    columns.
--
-- HASHES and MERGES can only be changed in place from PostgreSQL 17 on.  Older
-- servers have no way to add them without dropping and recreating the operator,
-- which would have to CASCADE to dependent user objects, so there the upgraded
-- = simply keeps the definition it had in 1.2.
--
DO $rb13_merge$
BEGIN
  IF current_setting('server_version_num')::int >= 170000 THEN
    EXECUTE 'ALTER OPERATOR = (roaringbitmap, roaringbitmap) SET (MERGES = true, HASHES = true)';
  END IF;
END
$rb13_merge$;

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

-- ============================================================================
-- btree / hash / gin operator classes + restriction estimators(for roaringbitmap64)
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
CREATE FUNCTION rb64_cmp(roaringbitmap64, roaringbitmap64)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'rb64_cmp'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION rb64_lt(roaringbitmap64, roaringbitmap64)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'rb64_lt'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION rb64_le(roaringbitmap64, roaringbitmap64)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'rb64_le'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION rb64_ge(roaringbitmap64, roaringbitmap64)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'rb64_ge'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION rb64_gt(roaringbitmap64, roaringbitmap64)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'rb64_gt'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR < (
  LEFTARG = roaringbitmap64,
  RIGHTARG = roaringbitmap64,
  PROCEDURE = rb64_lt,
  COMMUTATOR = '>',
  NEGATOR = '>=',
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR <= (
  LEFTARG = roaringbitmap64,
  RIGHTARG = roaringbitmap64,
  PROCEDURE = rb64_le,
  COMMUTATOR = '>=',
  NEGATOR = '>',
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = roaringbitmap64,
  RIGHTARG = roaringbitmap64,
  PROCEDURE = rb64_ge,
  COMMUTATOR = '<=',
  NEGATOR = '<',
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR > (
  LEFTARG = roaringbitmap64,
  RIGHTARG = roaringbitmap64,
  PROCEDURE = rb64_gt,
  COMMUTATOR = '<',
  NEGATOR = '<=',
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

-- ---------------------------------------------------------------------------
-- B. in-place patches to pre-existing objects
-- ---------------------------------------------------------------------------
--
-- 1) = (roaringbitmap64, roaringbitmap64) gains HASHES and MERGES, for the same
--    reasons as B.
--
-- 2) rb64_containedby(bigint, roaringbitmap64) is a LANGUAGE SQL wrapper whose
--    old body 'SELECT rb64_contains($2, $1);' cannot use the GIN index; the new
--    body 'SELECT $2 @> $1;' lets "42 <@ rb64" be matched by the gin operator
--    class.
--
-- 3) The operator functions behind &&, @>, <@ and = get COST 100, for the same
--    reasons as B.
--
-- As for the 32-bit type: HASHES and MERGES can only be changed in place from
-- PostgreSQL 17 on, so older servers keep the 1.2 definition of =.
--
DO $rb64_13_merge$
BEGIN
  IF current_setting('server_version_num')::int >= 170000 THEN
    EXECUTE 'ALTER OPERATOR = (roaringbitmap64, roaringbitmap64) SET (MERGES = true, HASHES = true)';
  END IF;
END
$rb64_13_merge$;

CREATE OR REPLACE FUNCTION rb64_containedby(bigint, roaringbitmap64)
  RETURNS boolean
  AS 'SELECT $2 @> $1;'
  LANGUAGE SQL STRICT IMMUTABLE PARALLEL SAFE;

ALTER FUNCTION rb64_contains(roaringbitmap64, roaringbitmap64)    COST 100;
ALTER FUNCTION rb64_contains(roaringbitmap64, bigint)             COST 100;
ALTER FUNCTION rb64_containedby(roaringbitmap64, roaringbitmap64) COST 100;
ALTER FUNCTION rb64_containedby(bigint, roaringbitmap64)          COST 100;
ALTER FUNCTION rb64_intersect(roaringbitmap64, roaringbitmap64)   COST 100;
ALTER FUNCTION rb64_equals(roaringbitmap64, roaringbitmap64)      COST 100;

-- ---------------------------------------------------------------------------
-- C. btree / hash / gin operator classes
-- ---------------------------------------------------------------------------
--

-- C-1. btree operator class
CREATE OPERATOR CLASS roaringbitmap64_ops
  DEFAULT FOR TYPE roaringbitmap64 USING btree AS
    OPERATOR 1 <,
    OPERATOR 2 <=,
    OPERATOR 3 =,
    OPERATOR 4 >=,
    OPERATOR 5 >,
    FUNCTION 1 rb64_cmp(roaringbitmap64, roaringbitmap64);

-- C-2. hash operator class
CREATE FUNCTION rb64_hash(roaringbitmap64)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'rb64_hash'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION rb64_hash_extended(roaringbitmap64, bigint)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb64_hash_extended'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR CLASS roaringbitmap64_ops
  DEFAULT FOR TYPE roaringbitmap64 USING hash AS
    OPERATOR 1 =,
    FUNCTION 1 rb64_hash(roaringbitmap64),
    FUNCTION 2 rb64_hash_extended(roaringbitmap64, bigint);

-- C-3. gin operator class
CREATE FUNCTION rb64_ginextract_value(roaringbitmap64, internal, internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb64_ginextract_value'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION rb64_ginextract_query(roaringbitmap64, internal, int2,
                                      internal, internal, internal, internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'rb64_ginextract_query'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION rb64_ginconsistent(internal, int2, roaringbitmap64, int4,
                                   internal, internal, internal, internal)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'rb64_ginconsistent'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION rb64_gintriconsistent(internal, int2, roaringbitmap64, int4,
                                      internal, internal, internal)
  RETURNS "char"
  AS 'MODULE_PATHNAME', 'rb64_gintriconsistent'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR CLASS roaringbitmap64_ops
  DEFAULT FOR TYPE roaringbitmap64 USING gin AS
    OPERATOR 1 && (roaringbitmap64, roaringbitmap64),
    OPERATOR 2 @> (roaringbitmap64, roaringbitmap64),
    OPERATOR 3 <@ (roaringbitmap64, roaringbitmap64),
    OPERATOR 4 = (roaringbitmap64, roaringbitmap64),
    OPERATOR 5 @> (roaringbitmap64, bigint),
    FUNCTION 1 btint8cmp(int8, int8),
    FUNCTION 2 rb64_ginextract_value(roaringbitmap64, internal, internal),
    FUNCTION 3 rb64_ginextract_query(roaringbitmap64, internal, int2,
                                     internal, internal, internal, internal),
    FUNCTION 4 rb64_ginconsistent(internal, int2, roaringbitmap64, int4,
                                  internal, internal, internal, internal),
    FUNCTION 6 rb64_gintriconsistent(internal, int2, roaringbitmap64, int4,
                                     internal, internal, internal),
    STORAGE int8;

-- ---------------------------------------------------------------------------
-- D. restriction estimators (RESTRICT only, JOIN stays contjoinsel)
-- ---------------------------------------------------------------------------
--
CREATE FUNCTION rb64_contain_sel(internal, oid, internal, integer)
  RETURNS float8
  AS 'MODULE_PATHNAME', 'rb64_contain_sel'
  LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE FUNCTION rb64_contained_sel(internal, oid, internal, integer)
  RETURNS float8
  AS 'MODULE_PATHNAME', 'rb64_contained_sel'
  LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE FUNCTION rb64_overlap_sel(internal, oid, internal, integer)
  RETURNS float8
  AS 'MODULE_PATHNAME', 'rb64_overlap_sel'
  LANGUAGE C STABLE STRICT PARALLEL SAFE;

ALTER OPERATOR @> (roaringbitmap64, roaringbitmap64) SET (RESTRICT = rb64_contain_sel);
ALTER OPERATOR @> (roaringbitmap64, bigint)          SET (RESTRICT = rb64_contain_sel);
ALTER OPERATOR <@ (roaringbitmap64, roaringbitmap64) SET (RESTRICT = rb64_contained_sel);
ALTER OPERATOR <@ (bigint, roaringbitmap64)          SET (RESTRICT = rb64_contained_sel);
ALTER OPERATOR && (roaringbitmap64, roaringbitmap64) SET (RESTRICT = rb64_overlap_sel);

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
-- rb64_typanalyze() keeps only what 1.2 and earlier collected, so the cost goes
-- back to one pass over the sample. 
--
CREATE FUNCTION rb64_typanalyze(internal)
  RETURNS boolean
  AS 'MODULE_PATHNAME','rb64_typanalyze'
  LANGUAGE C STABLE PARALLEL SAFE;

ALTER TYPE roaringbitmap64 SET (ANALYZE = rb64_typanalyze);
