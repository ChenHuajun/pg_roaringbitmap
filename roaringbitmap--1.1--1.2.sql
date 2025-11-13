CREATE OR REPLACE FUNCTION rb_runoptimize(roaringbitmap)
  RETURNS roaringbitmap
  AS 'MODULE_PATHNAME', 'rb_runoptimize'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;
