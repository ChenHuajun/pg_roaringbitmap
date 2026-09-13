CREATE OR REPLACE FUNCTION rb_group_elements_by_source(bitmaps roaringbitmap[])
  RETURNS TABLE (sources int[], members roaringbitmap)
  AS 'MODULE_PATHNAME', 'rb_group_elements_by_source'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION rb64_group_elements_by_source(bitmaps roaringbitmap64[])
  RETURNS TABLE (sources int[], members roaringbitmap64)
  AS 'MODULE_PATHNAME', 'rb64_group_elements_by_source'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;
