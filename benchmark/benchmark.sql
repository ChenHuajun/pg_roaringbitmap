set max_parallel_workers_per_gather=0;

\timing
\echo create test table tb_test_bitmaps
create temp table tb_test_bitmaps as
  select id,rb_build_agg((random()*10000000)::int) bitmap
    from generate_series(1,100)id, generate_series(1,100000)b
    group by id;
\timing

\echo rb_and_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb_and(ra1,ra2) from t,generate_series(1,100) id;

\echo rb_and_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb_fill('{}'::roaringbitmap,5000001,15000000) ra2
)
select rb_and(ra1,ra2) from t,generate_series(1,1000) id;

\echo rb_or_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb_or(ra1,ra2) from t,generate_series(1,100) id;

\echo rb_or_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb_fill('{}'::roaringbitmap,5000001,15000000) ra2
)
select rb_or(ra1,ra2) from t,generate_series(1,1000) id;

\echo rb_xor_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb_xor(ra1,ra2) from t,generate_series(1,100) id;

\echo rb_xor_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb_fill('{}'::roaringbitmap,5000001,15000000) ra2
)
select rb_xor(ra1,ra2) from t,generate_series(1,1000) id;

\echo rb_andnot_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb_andnot(ra1,ra2) from t,generate_series(1,100) id;

\echo rb_andnot_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb_fill('{}'::roaringbitmap,5000001,15000000) ra2
)
select rb_andnot(ra1,ra2) from t,generate_series(1,1000) id;

\echo rb_add
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb_add(ra1,id) from t,generate_series(1,1000)id;

\echo rb_contains_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb_contains(ra1,ra2) from t,generate_series(1,100);

\echo rb_contains_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb_fill('{}'::roaringbitmap,5000001,15000000) ra2
)
select rb_contains(ra1,ra2) from t,generate_series(1,1000);

\echo rb_contains_3
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb_contains(ra1,id) from t,generate_series(1,1000)id;

\echo rb_intersect_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb_intersect(ra1,ra2) from t,generate_series(1,100) id;

\echo rb_intersect_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb_fill('{}'::roaringbitmap,5000001,15000000) ra2
)
select rb_intersect(ra1,ra2) from t,generate_series(1,1000) id;

\echo rb_equals_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb_equals(ra1,ra2) from t,generate_series(1,100) id;

\echo rb_equals_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb_fill('{}'::roaringbitmap,5000001,15000000) ra2
)
select rb_equals(ra1,ra2) from t,generate_series(1,1000) id;

\echo rb_andnot_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb_andnot(ra1,ra2) from t,generate_series(1,100) id;

\echo rb_andnot_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb_fill('{}'::roaringbitmap,5000001,15000000) ra2
)
select rb_andnot(ra1,ra2) from t,generate_series(1,1000) id;

\echo rb_index_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb_index(ra1,id) from t,generate_series(1,1000)id;

\echo rb_cardinality_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb_cardinality(ra1) from t,generate_series(1,1000);

\echo rb_and_cardinality_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb_and_cardinality(ra1,ra2) from t,generate_series(1,100);

\echo rb_and_cardinality_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb_fill('{}'::roaringbitmap,5000001,15000000) ra2
)
select rb_and_cardinality(ra1,ra2) from t,generate_series(1,1000);

\echo rb_or_cardinality_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb_or_cardinality(ra1,ra2) from t,generate_series(1,100);

\echo rb_or_cardinality_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb_fill('{}'::roaringbitmap,5000001,15000000) ra2
)
select rb_or_cardinality(ra1,ra2) from t,generate_series(1,1000);

\echo rb_xor_cardinality_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb_xor_cardinality(ra1,ra2) from t,generate_series(1,100);

\echo rb_xor_cardinality_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb_fill('{}'::roaringbitmap,5000001,15000000) ra2
)
select rb_xor_cardinality(ra1,ra2) from t,generate_series(1,1000);

\echo rb_andnot_cardinality_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb_andnot_cardinality(ra1,ra2) from t,generate_series(1,100);

\echo rb_andnot_cardinality_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb_fill('{}'::roaringbitmap,5000001,15000000) ra2
)
select rb_andnot_cardinality(ra1,ra2) from t,generate_series(1,1000);

\echo rb_is_empty_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb_is_empty(ra1) from t,generate_series(1,1000);

\echo rb_range_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb_range(ra1,id,2000000) from t,generate_series(1,100)id;

\echo rb_range_cardinality_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb_range_cardinality(ra1,id,2000000) from t,generate_series(1,1000)id;

\echo rb_min_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb_min(ra1) from t,generate_series(1,1000);

\echo rb_max_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb_max(ra1) from t,generate_series(1,1000);

\echo rb_rank_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb_rank(ra1,100000+id) from t,generate_series(1,1000)id;

\echo rb_jaccard_dist_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb_jaccard_dist(ra1,ra2) from t,generate_series(1,100);

\echo rb_jaccard_dist_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb_fill('{}'::roaringbitmap,5000001,15000000) ra2
)
select rb_jaccard_dist(ra1,ra2) from t,generate_series(1,1000);

\echo rb_to_array_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb_to_array(ra1) from t,generate_series(1,100);

\echo rb_iterate_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb_iterate(ra1) from t,generate_series(1,10);

\echo rb_build_agg_1
explain analyze
select rb_build_agg(id) from generate_series(1,1000000) id;

\echo rb_or_agg_1
explain analyze
select rb_or_agg(bitmap) from tb_test_bitmaps;

\echo rb_and_agg_1
explain analyze
select rb_and_agg(bitmap) from tb_test_bitmaps;

\echo rb_xor_agg_1
explain analyze
select rb_xor_agg(bitmap) from tb_test_bitmaps;

\echo rb_or_cardinality_agg_1
explain analyze
select rb_or_cardinality_agg(bitmap) from tb_test_bitmaps;

\echo rb_and_cardinality_agg_1
explain analyze
select rb_and_cardinality_agg(bitmap) from tb_test_bitmaps;

\echo rb_xor_cardinality_agg_1
explain analyze
select rb_xor_cardinality_agg(bitmap) from tb_test_bitmaps;
