SET search_path = public;


DROP AGGREGATE rb_build_agg(integer);
DROP AGGREGATE rb_and_agg(roaringbitmap);
DROP AGGREGATE rb_or_agg(roaringbitmap);

DROP FUNCTION rb_build_trans(internal, integer);
DROP FUNCTION rb_and_trans(internal, roaringbitmap);
DROP FUNCTION rb_or_trans(internal, roaringbitmap);
DROP FUNCTION rb_iterate(roaringbitmap);
DROP FUNCTION rb_cardinality(roaringbitmap);
DROP FUNCTION rb_andnot(roaringbitmap, roaringbitmap);
DROP FUNCTION rb_xor(roaringbitmap, roaringbitmap);
DROP FUNCTION rb_and(roaringbitmap, roaringbitmap);
DROP FUNCTION rb_or(roaringbitmap, roaringbitmap);
DROP FUNCTION rb_build(integer[]);

DROP TYPE roaringbitmap CASCADE;

DROP FUNCTION roaringbitmap(bytea);



