CREATE OR REPLACE FUNCTION rb64_select(roaringbitmap64, bitset_limit bigint,bitset_offset bigint=0,reverse boolean=false,range_start bigint=0,range_end bigint=0)
  RETURNS roaringbitmap64
  AS 'MODULE_PATHNAME', 'rb64_select'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;
  