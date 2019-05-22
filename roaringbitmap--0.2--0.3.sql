/* roaringbitmap--0.2--0.3 */


CREATE OR REPLACE FUNCTION rb_iterate(roaringbitmap)
   RETURNS SETOF integer 
   AS 'MODULE_PATHNAME', 'rb_iterate'
   LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;
