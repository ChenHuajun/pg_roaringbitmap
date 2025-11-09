set max_parallel_workers_per_gather=0;

\timing
\echo create test table tb_test_bitmaps
create temp table tb_test_bitmaps as
  select id,rb64_build_agg((random()*10000000)::int) bitmap
    from generate_series(1,100)id, generate_series(1,100000)b
    group by id;
\timing

select now() time_start \gset

\echo rb64_and_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb64_and(ra1,ra2) from t,generate_series(1,100) id;

\echo rb64_and_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb64_fill('{}'::roaringbitmap64,5000001,15000000) ra2
)
select rb64_and(ra1,ra2) from t,generate_series(1,1000) id;

\echo rb64_or_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb64_or(ra1,ra2) from t,generate_series(1,100) id;

\echo rb64_or_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb64_fill('{}'::roaringbitmap64,5000001,15000000) ra2
)
select rb64_or(ra1,ra2) from t,generate_series(1,1000) id;

\echo rb64_xor_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb64_xor(ra1,ra2) from t,generate_series(1,100) id;

\echo rb64_xor_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb64_fill('{}'::roaringbitmap64,5000001,15000000) ra2
)
select rb64_xor(ra1,ra2) from t,generate_series(1,1000) id;

\echo rb64_andnot_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb64_andnot(ra1,ra2) from t,generate_series(1,100) id;

\echo rb64_andnot_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb64_fill('{}'::roaringbitmap64,5000001,15000000) ra2
)
select rb64_andnot(ra1,ra2) from t,generate_series(1,1000) id;

\echo rb64_add
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb64_add(ra1,id) from t,generate_series(1,1000)id;

\echo rb64_contains_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb64_contains(ra1,ra2) from t,generate_series(1,100);

\echo rb64_contains_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb64_fill('{}'::roaringbitmap64,5000001,15000000) ra2
)
select rb64_contains(ra1,ra2) from t,generate_series(1,1000);

\echo rb64_contains_3
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb64_contains(ra1,id) from t,generate_series(1,1000)id;

\echo rb64_intersect_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb64_intersect(ra1,ra2) from t,generate_series(1,100) id;

\echo rb64_intersect_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb64_fill('{}'::roaringbitmap64,5000001,15000000) ra2
)
select rb64_intersect(ra1,ra2) from t,generate_series(1,1000) id;

\echo rb64_equals_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb64_equals(ra1,ra2) from t,generate_series(1,100) id;

\echo rb64_equals_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb64_fill('{}'::roaringbitmap64,5000001,15000000) ra2
)
select rb64_equals(ra1,ra2) from t,generate_series(1,1000) id;

\echo rb64_andnot_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb64_andnot(ra1,ra2) from t,generate_series(1,100) id;

\echo rb64_andnot_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb64_fill('{}'::roaringbitmap64,5000001,15000000) ra2
)
select rb64_andnot(ra1,ra2) from t,generate_series(1,1000) id;

\echo rb64_index_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb64_index(ra1,id) from t,generate_series(1,1000)id;

\echo rb64_cardinality_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb64_cardinality(ra1) from t,generate_series(1,1000);

\echo rb64_and_cardinality_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb64_and_cardinality(ra1,ra2) from t,generate_series(1,100);

\echo rb64_and_cardinality_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb64_fill('{}'::roaringbitmap64,5000001,15000000) ra2
)
select rb64_and_cardinality(ra1,ra2) from t,generate_series(1,1000);

\echo rb64_or_cardinality_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb64_or_cardinality(ra1,ra2) from t,generate_series(1,100);

\echo rb64_or_cardinality_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb64_fill('{}'::roaringbitmap64,5000001,15000000) ra2
)
select rb64_or_cardinality(ra1,ra2) from t,generate_series(1,1000);

\echo rb64_xor_cardinality_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb64_xor_cardinality(ra1,ra2) from t,generate_series(1,100);

\echo rb64_xor_cardinality_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb64_fill('{}'::roaringbitmap64,5000001,15000000) ra2
)
select rb64_xor_cardinality(ra1,ra2) from t,generate_series(1,1000);

\echo rb64_andnot_cardinality_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb64_andnot_cardinality(ra1,ra2) from t,generate_series(1,100);

\echo rb64_andnot_cardinality_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb64_fill('{}'::roaringbitmap64,5000001,15000000) ra2
)
select rb64_andnot_cardinality(ra1,ra2) from t,generate_series(1,1000);

\echo rb64_is_empty_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb64_is_empty(ra1) from t,generate_series(1,1000);

\echo rb64_range_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb64_range(ra1,id,2000000) from t,generate_series(1,100)id;

\echo rb64_range_cardinality_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb64_range_cardinality(ra1,id,2000000) from t,generate_series(1,1000)id;

\echo rb64_min_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb64_min(ra1) from t,generate_series(1,1000);

\echo rb64_max_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb64_max(ra1) from t,generate_series(1,1000);

\echo rb64_rank_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb64_rank(ra1,100000+id) from t,generate_series(1,1000)id;

\echo rb64_jaccard_dist_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,(select bitmap from tb_test_bitmaps where id=2 limit 1) ra2
)
select rb64_jaccard_dist(ra1,ra2) from t,generate_series(1,100);

\echo rb64_jaccard_dist_2
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1,rb64_fill('{}'::roaringbitmap64,5000001,15000000) ra2
)
select rb64_jaccard_dist(ra1,ra2) from t,generate_series(1,1000);

\echo rb64_to_array_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb64_to_array(ra1) from t,generate_series(1,100);

\echo rb64_iterate_1
explain analyze
with t as(
  select (select bitmap from tb_test_bitmaps where id=1 limit 1) ra1
)
select rb64_iterate(ra1) from t,generate_series(1,10);

\echo rb64_build_agg_1
explain analyze
select rb64_build_agg(id) from generate_series(1,1000000) id;

\echo rb64_or_agg_1
explain analyze
select rb64_or_agg(bitmap) from tb_test_bitmaps;

\echo rb64_and_agg_1
explain analyze
select rb64_and_agg(bitmap) from tb_test_bitmaps;

\echo rb64_xor_agg_1
explain analyze
select rb64_xor_agg(bitmap) from tb_test_bitmaps;

\echo rb64_or_cardinality_agg_1
explain analyze
select rb64_or_cardinality_agg(bitmap) from tb_test_bitmaps;

\echo rb64_and_cardinality_agg_1
explain analyze
select rb64_and_cardinality_agg(bitmap) from tb_test_bitmaps;

\echo rb64_xor_cardinality_agg_1
explain analyze
select rb64_xor_cardinality_agg(bitmap) from tb_test_bitmaps;

select now() - :'time_start'::timestamp as time_escape \gset

\echo  Total time: :time_escape

