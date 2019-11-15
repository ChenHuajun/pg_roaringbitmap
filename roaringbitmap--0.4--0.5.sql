/* roaringbitmap--0.4--0.5 */

-- Redefine rb_or_cardinality_agg/rb_and_cardinality_agg/rb_xor_cardinality_agg to support parallel aggregate

DROP AGGREGATE rb_or_cardinality_agg(roaringbitmap);
CREATE AGGREGATE rb_or_cardinality_agg(roaringbitmap)(
  SFUNC = rb_or_trans,
  STYPE = internal,
  FINALFUNC = rb_cardinality_final,
  COMBINEFUNC = rb_or_combine,
  SERIALFUNC = rb_serialize,
  DESERIALFUNC = rb_deserialize,
  PARALLEL = SAFE
);

DROP AGGREGATE rb_and_cardinality_agg(roaringbitmap);
CREATE AGGREGATE rb_and_cardinality_agg(roaringbitmap)(
  SFUNC = rb_and_trans,
  STYPE = internal,
  FINALFUNC = rb_cardinality_final,
  COMBINEFUNC = rb_and_combine,
  SERIALFUNC = rb_serialize,
  DESERIALFUNC = rb_deserialize,
  PARALLEL = SAFE
);

DROP AGGREGATE rb_xor_cardinality_agg(roaringbitmap);
CREATE AGGREGATE rb_xor_cardinality_agg(roaringbitmap)(
  SFUNC = rb_xor_trans,
  STYPE = internal,
  FINALFUNC = rb_cardinality_final,
  COMBINEFUNC = rb_xor_combine,
  SERIALFUNC = rb_serialize,
  DESERIALFUNC = rb_deserialize,
  PARALLEL = SAFE
);

