CREATE OR REPLACE FUNCTION rb_group_elements_by_source(bitmaps roaringbitmap[])
  RETURNS TABLE (sources int[], members roaringbitmap)
  AS 'MODULE_PATHNAME', 'rb_group_elements_by_source'
  LANGUAGE C IMMUTABLE PARALLEL SAFE;
