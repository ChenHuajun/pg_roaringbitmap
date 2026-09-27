-- ============================================================
-- 1.2 -> 1.3 upgrade path.
-- ============================================================
-- It must run last (see Makefile), because it drops and reinstalls the extension.
--
-- The assertions come in four parts: roaringbitmap (32-bit) before the upgrade,
-- roaringbitmap64 (64-bit) before the upgrade, and the same two after it.
--

SET client_min_messages = warning;
DROP EXTENSION IF EXISTS roaringbitmap CASCADE;
CREATE EXTENSION roaringbitmap VERSION '1.2';

-- ============================================================================
-- Part 1 of 4: roaringbitmap (32-bit) -- before the upgrade
-- ============================================================================

-- Before the upgrade there is only the = operator, with neither flag set, and no
-- operator class for the type.
SELECT oprname, oprleft::regtype, oprright::regtype, oprcanhash, oprcanmerge, oprrest::regproc, oprjoin::regproc
  FROM pg_operator
 WHERE oprleft = 'roaringbitmap'::regtype
   OR oprright = 'roaringbitmap'::regtype
 ORDER BY oprname,oprleft,oprright;

SELECT count(*) AS opclasses_before
  FROM pg_opclass
 WHERE opcintype = 'roaringbitmap'::regtype;

-- 1.2 collected statistics through the backend's compute_trivial_stats(); 1.3
-- replaces that with rb_typanalyze.
SELECT typname, typanalyze::regproc
  FROM pg_type
 WHERE typname = 'roaringbitmap';

SELECT prosrc AS containedby_before
  FROM pg_proc
 WHERE oid = 'rb_containedby(integer, roaringbitmap)'::regprocedure;

-- create test table before upgrade
CREATE TABLE rb_upgrade_t (rb roaringbitmap);
INSERT INTO rb_upgrade_t VALUES (rb_build(ARRAY[1, 2, 3, 4, 5]));
INSERT INTO rb_upgrade_t 
  SELECT rb_build_agg(a*111+b*3+7) bitmap
    FROM generate_series(1,100)a,generate_series(1,1000)b;

-- ============================================================================
-- Part 2 of 4: roaringbitmap64 (64-bit) -- before the upgrade
-- ============================================================================

-- Same as part 1 for roaringbitmap64: only the = operator, with neither flag
-- set, no operator class, and no typanalyze function.
SELECT oprname, oprleft::regtype, oprright::regtype, oprcanhash, oprcanmerge, oprrest::regproc, oprjoin::regproc
  FROM pg_operator
 WHERE oprleft = 'roaringbitmap64'::regtype
   OR oprright = 'roaringbitmap64'::regtype
 ORDER BY oprname,oprleft,oprright;

SELECT count(*) AS opclasses64_before
  FROM pg_opclass
 WHERE opcintype = 'roaringbitmap64'::regtype;

SELECT typname, typanalyze::regproc
  FROM pg_type
 WHERE typname = 'roaringbitmap64';

SELECT prosrc AS containedby64_before
  FROM pg_proc
 WHERE oid = 'rb64_containedby(bigint, roaringbitmap64)'::regprocedure;

CREATE TABLE rb64_upgrade_t (rb roaringbitmap64);
INSERT INTO rb64_upgrade_t VALUES (rb64_build(ARRAY[1, 2, 3, 4, 5]));
INSERT INTO rb64_upgrade_t 
  SELECT rb64_build_agg(a*111+b*3+7) bitmap
    FROM generate_series(1,100)a,generate_series(1,1000)b;

ALTER EXTENSION roaringbitmap UPDATE TO '1.3';

-- ============================================================================
-- Part 3 of 4: roaringbitmap (32-bit) -- after the upgrade
-- ============================================================================

-- After the upgrade: = must be hashable and mergejoinable, < <= >= > must NOT be
-- mergejoinable.  ALTER OPERATOR can only turn HASHES/MERGES on from PostgreSQL
-- 17 on, before that the upgraded = keeps its 1.2 flags, so accept either value
-- there and keep a single expected file for PostgreSQL 13 - 19.
SELECT oprname, oprleft::regtype, oprright::regtype,
       CASE WHEN oprname = '='
                 AND current_setting('server_version_num')::int < 170000
            THEN true ELSE oprcanhash END AS oprcanhash,
       CASE WHEN oprname = '='
                 AND current_setting('server_version_num')::int < 170000
            THEN true ELSE oprcanmerge END AS oprcanmerge,
       oprrest::regproc, oprjoin::regproc
  FROM pg_operator
 WHERE oprleft = 'roaringbitmap'::regtype
   OR oprright = 'roaringbitmap'::regtype
 ORDER BY oprname,oprleft,oprright;

-- The rb_containedby wrapper must use the @> form, otherwise "42 <@ rb" is
-- inlined into a FUNCEXPR and cannot use the GIN index.
SELECT prosrc AS containedby_after
  FROM pg_proc
 WHERE oid = 'rb_containedby(integer, roaringbitmap)'::regprocedure;

-- The operator functions behind &&, @>, <@ and = 
SELECT p.proname, oidvectortypes(p.proargtypes) AS args, p.procost
  FROM pg_proc p
 WHERE p.oid IN ('rb_contains(roaringbitmap, roaringbitmap)'::regprocedure,
                 'rb_contains(roaringbitmap, integer)'::regprocedure,
                 'rb_containedby(roaringbitmap, roaringbitmap)'::regprocedure,
                 'rb_containedby(integer, roaringbitmap)'::regprocedure,
                 'rb_intersect(roaringbitmap, roaringbitmap)'::regprocedure,
                 'rb_equals(roaringbitmap, roaringbitmap)'::regprocedure)
 ORDER BY p.proname, args;

-- All three operator classes, and their members, must have been created.
SELECT opcname, am.amname, opcdefault
  FROM pg_opclass c
  JOIN pg_am am ON am.oid = c.opcmethod
 WHERE c.opcintype = 'roaringbitmap'::regtype
 ORDER BY am.amname;

SELECT am.amname, amopstrategy, amopopr::regoperator
  FROM pg_amop ao
  JOIN pg_am am ON am.oid = ao.amopmethod
 WHERE ao.amoplefttype = 'roaringbitmap'::regtype
 ORDER BY am.amname, amopstrategy;

-- Functional sanity check on the upgraded objects.
SELECT count(*) AS lt_matches FROM rb_upgrade_t WHERE rb < rb_build(ARRAY[9]);
SELECT count(*) AS eq_matches FROM rb_upgrade_t WHERE rb = rb_build(ARRAY[1, 2, 3, 4, 5]);
SELECT count(*) AS containedby_int
  FROM rb_upgrade_t WHERE 3 <@ rb;
SELECT count(DISTINCT rb) AS distinct_rb FROM rb_upgrade_t;

-- The upgraded type must have picked up rb_typanalyze, so analysing a
-- roaringbitmap column writes a statistics row with no MCV and no histogram.
SELECT typname, typanalyze::regproc
  FROM pg_type
 WHERE typname = 'roaringbitmap';

INSERT INTO rb_upgrade_t
  SELECT rb_build(ARRAY[1, 2, 3, 4, 5]) FROM generate_series(1, 30);
ANALYZE rb_upgrade_t;
select attname, null_frac, n_distinct, avg_width,
       most_common_vals is null as no_mcv,
       histogram_bounds is null as no_histogram
  from pg_stats
 where tablename = 'rb_upgrade_t'
 order by attname;

-- ============================================================================
-- Part 4 of 4: roaringbitmap64 (64-bit) -- after the upgrade
-- ============================================================================

-- Same as part 3 for roaringbitmap64: = is hashable and mergejoinable, < <= >= >
-- are not mergejoinable, and the three operator classes exist with the same
-- members as a fresh 1.3 install.
SELECT oprname, oprleft::regtype, oprright::regtype,
       CASE WHEN oprname = '='
                 AND current_setting('server_version_num')::int < 170000
            THEN true ELSE oprcanhash END AS oprcanhash,
       CASE WHEN oprname = '='
                 AND current_setting('server_version_num')::int < 170000
            THEN true ELSE oprcanmerge END AS oprcanmerge,
       oprrest::regproc, oprjoin::regproc
  FROM pg_operator
 WHERE oprleft = 'roaringbitmap64'::regtype
   OR oprright = 'roaringbitmap64'::regtype
 ORDER BY oprname,oprleft,oprright;

-- The rb64_containedby wrapper must use the @> form, otherwise "42 <@ rb64" is
-- inlined into a FUNCEXPR and cannot use the GIN index.
SELECT prosrc AS containedby64_after
  FROM pg_proc
 WHERE oid = 'rb64_containedby(bigint, roaringbitmap64)'::regprocedure;

-- The operator functions behind &&, @>, <@ and =
SELECT p.proname, oidvectortypes(p.proargtypes) AS args, p.procost
  FROM pg_proc p
 WHERE p.oid IN ('rb64_contains(roaringbitmap64, roaringbitmap64)'::regprocedure,
                 'rb64_contains(roaringbitmap64, bigint)'::regprocedure,
                 'rb64_containedby(roaringbitmap64, roaringbitmap64)'::regprocedure,
                 'rb64_containedby(bigint, roaringbitmap64)'::regprocedure,
                 'rb64_intersect(roaringbitmap64, roaringbitmap64)'::regprocedure,
                 'rb64_equals(roaringbitmap64, roaringbitmap64)'::regprocedure)
 ORDER BY p.proname, args;

-- All three operator classes, and their members, must have been created.
SELECT opcname, am.amname, opcdefault
  FROM pg_opclass c
  JOIN pg_am am ON am.oid = c.opcmethod
 WHERE c.opcintype = 'roaringbitmap64'::regtype
 ORDER BY am.amname;

SELECT am.amname, amopstrategy, amopopr::regoperator
  FROM pg_amop ao
  JOIN pg_am am ON am.oid = ao.amopmethod
 WHERE ao.amoplefttype = 'roaringbitmap64'::regtype
 ORDER BY am.amname, amopstrategy;

-- Functional sanity check on the upgraded objects.
SELECT count(*) AS lt_matches FROM rb64_upgrade_t WHERE rb < rb64_build(ARRAY[9]);
SELECT count(*) AS eq_matches FROM rb64_upgrade_t WHERE rb = rb64_build(ARRAY[1, 2, 3, 4, 5]);
SELECT count(*) AS containedby_int
  FROM rb64_upgrade_t WHERE 3 <@ rb;
SELECT count(DISTINCT rb) AS distinct_rb FROM rb64_upgrade_t;

-- The upgraded type must have picked up rb64_typanalyze, so analysing a
-- roaringbitmap64 column writes a statistics row with no MCV and no histogram.
SELECT typname, typanalyze::regproc
  FROM pg_type
 WHERE typname = 'roaringbitmap64';

INSERT INTO rb64_upgrade_t
  SELECT rb64_build(ARRAY[1, 2, 3, 4, 5]) FROM generate_series(1, 30);
ANALYZE rb64_upgrade_t;
select attname, null_frac, n_distinct, avg_width,
       most_common_vals is null as no_mcv,
       histogram_bounds is null as no_histogram
  from pg_stats
 where tablename = 'rb64_upgrade_t'
 order by attname;
