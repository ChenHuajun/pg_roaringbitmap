/* roaringbitmap--0.3--0.4 */

-- type cast for bytea
CREATE OR REPLACE FUNCTION roaringbitmap(bytea)
  RETURNS roaringbitmap
  AS 'MODULE_PATHNAME','rb_from_bytea'
  LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE CAST (roaringbitmap AS bytea) WITHOUT FUNCTION;

CREATE CAST (bytea AS roaringbitmap) WITH FUNCTION roaringbitmap(bytea);

