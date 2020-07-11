/* roaringbitmap--0.5--0.6 */

--
-- Functions definition for cache accelerated bitmap operation
--

CREATE OR REPLACE FUNCTION rb_cache_and(bitmap roaringbitmap, cached_bitmap roaringbitmap, cache_key text, cache_capacity integer = 1024)
  RETURNS roaringbitmap 
  AS 'MODULE_PATHNAME', 'rb_cache_and'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_cache_and_cardinality(bitmap roaringbitmap, cached_bitmap roaringbitmap, cache_key text, cache_capacity integer = 1024)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb_cache_and_cardinality'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_cache_andnot(bitmap roaringbitmap, cached_bitmap roaringbitmap, cache_key text, cache_capacity integer = 1024)
  RETURNS roaringbitmap 
  AS 'MODULE_PATHNAME', 'rb_cache_andnot'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_cache_andnot_cardinality(bitmap roaringbitmap, cached_bitmap roaringbitmap, cache_key text, cache_capacity integer = 1024)
  RETURNS bigint
  AS 'MODULE_PATHNAME', 'rb_cache_andnot_cardinality'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_cache_contains(cached_bitmap roaringbitmap, value integer, cache_key text, cache_capacity integer = 1024)
  RETURNS bool 
  AS 'MODULE_PATHNAME', 'rb_cache_exsit'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_cache_contains(cached_bitmap roaringbitmap, bitmap roaringbitmap, cache_key text, cache_capacity integer = 1024)
  RETURNS bool 
  AS 'MODULE_PATHNAME', 'rb_cache_contains'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb_cache_intersect(bitmap roaringbitmap, cached_bitmap roaringbitmap, cache_key text, cache_capacity integer = 1024)
  RETURNS bool 
  AS 'MODULE_PATHNAME', 'rb_cache_intersect'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;