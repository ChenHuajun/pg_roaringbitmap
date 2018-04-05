--
--  Test roaringbitmap extension
--

CREATE EXTENSION roaringbitmap;

-- Test the functions with one bitmap variable

select rb_build(NULL);

select rb_to_array(NULL);
select rb_to_array(rb_build('{}'));
select rb_to_array(rb_build('{1}'));
select rb_to_array(rb_build('{1,10,100}'));

select rb_is_empty(NULL);
select rb_is_empty(rb_build('{}'));
select rb_is_empty(rb_build('{1}'));
select rb_is_empty(rb_build('{1,10,100}'));

select rb_cardinality(NULL);
select rb_cardinality(rb_build('{}'));
select rb_cardinality(rb_build('{1}'));
select rb_cardinality(rb_build('{1,10,100}'));

select rb_maximum(NULL);
select rb_maximum(rb_build('{}'));
select rb_maximum(rb_build('{1}'));
select rb_maximum(rb_build('{1,10,100}'));

select rb_minimum(NULL);
select rb_minimum(rb_build('{}'));
select rb_minimum(rb_build('{1}'));
select rb_minimum(rb_build('{1,10,100}'));

-- Test the functions with two bitmap variables

select rb_and(NULL,rb_build('{1,10,100}'));
select rb_and(rb_build('{1,10,100}'),NULL);
select rb_to_array(rb_and(rb_build('{}'),rb_build('{1,10,100}')));
select rb_to_array(rb_and(rb_build('{1,10,100}'),rb_build('{}')));
select rb_to_array(rb_and(rb_build('{2}'),rb_build('{1,10,100}')));
select rb_to_array(rb_and(rb_build('{1,2,10}'),rb_build('{1,10,100}')));
select rb_to_array(rb_and(rb_build('{1,10}'),rb_build('{1,10,100}')));

select rb_and_cardinality(NULL,rb_build('{1,10,100}'));
select rb_and_cardinality(rb_build('{1,10,100}'),NULL);
select rb_and_cardinality(rb_build('{}'),rb_build('{1,10,100}'));
select rb_and_cardinality(rb_build('{1,10,100}'),rb_build('{}'));
select rb_and_cardinality(rb_build('{2}'),rb_build('{1,10,100}'));
select rb_and_cardinality(rb_build('{1,2,10}'),rb_build('{1,10,100}'));
select rb_and_cardinality(rb_build('{1,10}'),rb_build('{1,10,100}'));

select rb_or(NULL,rb_build('{1,10,100}'));
select rb_or(rb_build('{1,10,100}'),NULL);
select rb_to_array(rb_or(rb_build('{}'),rb_build('{1,10,100}')));
select rb_to_array(rb_or(rb_build('{1,10,100}'),rb_build('{}')));
select rb_to_array(rb_or(rb_build('{2}'),rb_build('{1,10,100}')));
select rb_to_array(rb_or(rb_build('{1,2,10}'),rb_build('{1,10,100}')));
select rb_to_array(rb_or(rb_build('{1,10}'),rb_build('{1,10,100}')));

select rb_or_cardinality(NULL,rb_build('{1,10,100}'));
select rb_or_cardinality(rb_build('{1,10,100}'),NULL);
select rb_or_cardinality(rb_build('{}'),rb_build('{1,10,100}'));
select rb_or_cardinality(rb_build('{1,10,100}'),rb_build('{}'));
select rb_or_cardinality(rb_build('{2}'),rb_build('{1,10,100}'));
select rb_or_cardinality(rb_build('{1,2,10}'),rb_build('{1,10,100}'));
select rb_or_cardinality(rb_build('{1,10}'),rb_build('{1,10,100}'));

select rb_xor(NULL,rb_build('{1,10,100}'));
select rb_xor(rb_build('{1,10,100}'),NULL);
select rb_to_array(rb_xor(rb_build('{}'),rb_build('{1,10,100}')));
select rb_to_array(rb_xor(rb_build('{1,10,100}'),rb_build('{}')));
select rb_to_array(rb_xor(rb_build('{2}'),rb_build('{1,10,100}')));
select rb_to_array(rb_xor(rb_build('{1,2,10}'),rb_build('{1,10,100}')));
select rb_to_array(rb_xor(rb_build('{1,10}'),rb_build('{1,10,100}')));

select rb_xor_cardinality(NULL,rb_build('{1,10,100}'));
select rb_xor_cardinality(rb_build('{1,10,100}'),NULL);
select rb_xor_cardinality(rb_build('{}'),rb_build('{1,10,100}'));
select rb_xor_cardinality(rb_build('{1,10,100}'),rb_build('{}'));
select rb_xor_cardinality(rb_build('{2}'),rb_build('{1,10,100}'));
select rb_xor_cardinality(rb_build('{1,2,10}'),rb_build('{1,10,100}'));
select rb_xor_cardinality(rb_build('{1,10}'),rb_build('{1,10,100}'));

select rb_equals(NULL,rb_build('{1,10,100}'));
select rb_equals(rb_build('{1,10,100}'),NULL);
select rb_equals(rb_build('{}'),rb_build('{1,10,100}'));
select rb_equals(rb_build('{1,10,100}'),rb_build('{}'));
select rb_equals(rb_build('{2}'),rb_build('{1,10,100}'));
select rb_equals(rb_build('{1,2,10}'),rb_build('{1,10,100}'));
select rb_equals(rb_build('{1,10}'),rb_build('{1,10,100}'));
select rb_equals(rb_build('{1,10,100}'),rb_build('{1,10,100}'));
select rb_equals(rb_build('{1,10,100,10}'),rb_build('{1,100,10}'));

select rb_intersect(NULL,rb_build('{1,10,100}'));
select rb_intersect(rb_build('{1,10,100}'),NULL);
select rb_intersect(rb_build('{}'),rb_build('{1,10,100}'));
select rb_intersect(rb_build('{1,10,100}'),rb_build('{}'));
select rb_intersect(rb_build('{2}'),rb_build('{1,10,100}'));
select rb_intersect(rb_build('{1,2,10}'),rb_build('{1,10,100}'));
select rb_intersect(rb_build('{1,10}'),rb_build('{1,10,100}'));

select rb_andnot(NULL,rb_build('{1,10,100}'));
select rb_andnot(rb_build('{1,10,100}'),NULL);
select rb_to_array(rb_andnot(rb_build('{}'),rb_build('{1,10,100}')));
select rb_to_array(rb_andnot(rb_build('{1,10,100}'),rb_build('{}')));
select rb_to_array(rb_andnot(rb_build('{2}'),rb_build('{1,10,100}')));
select rb_to_array(rb_andnot(rb_build('{1,2,10}'),rb_build('{1,10,100}')));
select rb_to_array(rb_andnot(rb_build('{1,10}'),rb_build('{1,10,100}')));
select rb_to_array(rb_andnot(rb_build('{1,10,100}'),rb_build('{1,10}')));

-- Test other functions

select rb_rank(NULL,0);
select rb_rank(rb_build('{}'),0);
select rb_rank(rb_build('{1,10,100}'),0);
select rb_rank(rb_build('{1,10,100}'),1);
select rb_rank(rb_build('{1,10,100}'),99);
select rb_rank(rb_build('{1,10,100}'),100);
select rb_rank(rb_build('{1,10,100}'),101);

select rb_remove(NULL,0);
select rb_to_array(rb_remove(rb_build('{}'),0));
select rb_to_array(rb_remove(rb_build('{1}'),1));
select rb_to_array(rb_remove(rb_build('{1,10,100}'),0));
select rb_to_array(rb_remove(rb_build('{1,10,100}'),1));
select rb_to_array(rb_remove(rb_build('{1,10,100}'),99));

select rb_flip(NULL,0,0);
select rb_to_array(rb_flip(rb_build('{}'),0,0));
select rb_to_array(rb_flip(rb_build('{}'),0,1));
select rb_to_array(rb_flip(rb_build('{}'),0,2));
select rb_to_array(rb_flip(rb_build('{1,10,100}'),10,10));
select rb_to_array(rb_flip(rb_build('{1,10,100}'),10,11));
select rb_to_array(rb_flip(rb_build('{1,10,100}'),10,12));
select rb_to_array(rb_flip(rb_build('{1,10,100}'),10,13));

-- Test aggregate

select rb_and_agg(id) from (values (NULL::roaringbitmap)) t(id);
select rb_to_array(rb_and_agg(id)) from (values (rb_build('{}'))) t(id);
select rb_to_array(rb_and_agg(id)) from (values (rb_build('{1}'))) t(id);
select rb_to_array(rb_and_agg(id)) from (values (rb_build('{1,10,100}'))) t(id);
select rb_to_array(rb_and_agg(id)) from (values (rb_build('{1,10,100}')),(rb_build('{2,10}'))) t(id);
select rb_to_array(rb_and_agg(id)) from (values (rb_build('{1,10,100}')),(rb_build('{1,10,100}'))) t(id);
select rb_to_array(rb_and_agg(id)) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{}'))) t(id);
select rb_to_array(rb_and_agg(id)) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{1}'))) t(id);
select rb_to_array(rb_and_agg(id)) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{2}'))) t(id);
select rb_to_array(rb_and_agg(id)) from (values (rb_build('{1,10,100}')),(NULL),(rb_build('{2,10}'))) t(id);
select rb_to_array(rb_and_agg(id)) from (values (NULL),(rb_build('{1,10,100}')),(NULL),(rb_build('{1,10,100,101}')),(NULL)) t(id);

select rb_and_cardinality_agg(id) from (values (NULL::roaringbitmap)) t(id);
select rb_and_cardinality_agg(id) from (values (rb_build('{}'))) t(id);
select rb_and_cardinality_agg(id) from (values (rb_build('{1}'))) t(id);
select rb_and_cardinality_agg(id) from (values (rb_build('{1,10,100}'))) t(id);
select rb_and_cardinality_agg(id) from (values (rb_build('{1,10,100}')),(rb_build('{2,10}'))) t(id);
select rb_and_cardinality_agg(id) from (values (rb_build('{1,10,100}')),(rb_build('{1,10,100}'))) t(id);
select rb_and_cardinality_agg(id) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{}'))) t(id);
select rb_and_cardinality_agg(id) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{1}'))) t(id);
select rb_and_cardinality_agg(id) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{2}'))) t(id);
select rb_and_cardinality_agg(id) from (values (rb_build('{1,10,100}')),(NULL),(rb_build('{2,10}'))) t(id);
select rb_and_cardinality_agg(id) from (values (NULL),(rb_build('{1,10,100}')),(NULL),(rb_build('{1,10,100,101}')),(NULL)) t(id);

select rb_or_agg(id) from (values (NULL::roaringbitmap)) t(id);
select rb_to_array(rb_or_agg(id)) from (values (rb_build('{}'))) t(id);
select rb_to_array(rb_or_agg(id)) from (values (rb_build('{1}'))) t(id);
select rb_to_array(rb_or_agg(id)) from (values (rb_build('{1,10,100}'))) t(id);
select rb_to_array(rb_or_agg(id)) from (values (rb_build('{1,10,100}')),(rb_build('{2,10}'))) t(id);
select rb_to_array(rb_or_agg(id)) from (values (rb_build('{1,10,100}')),(rb_build('{1,10,100}'))) t(id);
select rb_to_array(rb_or_agg(id)) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{}'))) t(id);
select rb_to_array(rb_or_agg(id)) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{1}'))) t(id);
select rb_to_array(rb_or_agg(id)) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{2}'))) t(id);
select rb_to_array(rb_or_agg(id)) from (values (rb_build('{1,10,100}')),(NULL),(rb_build('{2,10}'))) t(id);
select rb_to_array(rb_or_agg(id)) from (values (NULL),(rb_build('{1,10,100}')),(NULL),(rb_build('{1,10,100,101}')),(NULL)) t(id);

select rb_or_cardinality_agg(id) from (values (NULL::roaringbitmap)) t(id);
select rb_or_cardinality_agg(id) from (values (rb_build('{}'))) t(id);
select rb_or_cardinality_agg(id) from (values (rb_build('{1}'))) t(id);
select rb_or_cardinality_agg(id) from (values (rb_build('{1,10,100}'))) t(id);
select rb_or_cardinality_agg(id) from (values (rb_build('{1,10,100}')),(rb_build('{2,10}'))) t(id);
select rb_or_cardinality_agg(id) from (values (rb_build('{1,10,100}')),(rb_build('{1,10,100}'))) t(id);
select rb_or_cardinality_agg(id) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{}'))) t(id);
select rb_or_cardinality_agg(id) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{1}'))) t(id);
select rb_or_cardinality_agg(id) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{2}'))) t(id);
select rb_or_cardinality_agg(id) from (values (rb_build('{1,10,100}')),(NULL),(rb_build('{2,10}'))) t(id);
select rb_or_cardinality_agg(id) from (values (NULL),(rb_build('{1,10,100}')),(NULL),(rb_build('{1,10,100,101}')),(NULL)) t(id);

select rb_xor_agg(id) from (values (NULL::roaringbitmap)) t(id);
select rb_to_array(rb_xor_agg(id)) from (values (rb_build('{}'))) t(id);
select rb_to_array(rb_xor_agg(id)) from (values (rb_build('{1}'))) t(id);
select rb_to_array(rb_xor_agg(id)) from (values (rb_build('{1,10,100}'))) t(id);
select rb_to_array(rb_xor_agg(id)) from (values (rb_build('{1,10,100}')),(rb_build('{2,10}'))) t(id);
select rb_to_array(rb_xor_agg(id)) from (values (rb_build('{1,10,100}')),(rb_build('{1,10,100}'))) t(id);
select rb_to_array(rb_xor_agg(id)) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{}'))) t(id);
select rb_to_array(rb_xor_agg(id)) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{1}'))) t(id);
select rb_to_array(rb_xor_agg(id)) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{2}'))) t(id);
select rb_to_array(rb_xor_agg(id)) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{2,10}'))) t(id);
select rb_to_array(rb_xor_agg(id)) from (values (rb_build('{1,10,100}')),(NULL),(rb_build('{1,10,100,101}'))) t(id);
select rb_to_array(rb_xor_agg(id)) from (values (NULL),(rb_build('{1,10,100}')),(NULL),(rb_build('{1,10,101}')),(rb_build('{1,100,102}')),(NULL)) t(id);

select rb_xor_cardinality_agg(id) from (values (NULL::roaringbitmap)) t(id);
select rb_xor_cardinality_agg(id) from (values (rb_build('{}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (rb_build('{1}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (rb_build('{1,10,100}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (rb_build('{1,10,100}')),(rb_build('{2,10}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (rb_build('{1,10,100}')),(rb_build('{1,10,100}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{1}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{2}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (NULL),(rb_build('{1,10,100}')),(rb_build('{2,10}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (rb_build('{1,10,100}')),(NULL),(rb_build('{1,10,100,101}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (NULL),(rb_build('{1,10,100}')),(NULL),(rb_build('{1,10,101}')),(rb_build('{1,100,102}')),(NULL)) t(id);

select rb_build_agg(id) from (values (NULL::int)) t(id);
select rb_to_array(rb_build_agg(id)) from (values (1)) t(id);
select rb_to_array(rb_build_agg(id)) from (values (1),(10)) t(id);
select rb_to_array(rb_build_agg(id)) from (values (1),(10),(10),(100),(1)) t(id);

-- Test parallel aggregate

set max_parallel_workers=8;
set max_parallel_workers_per_gather=2;
set parallel_setup_cost=0;
set parallel_tuple_cost=0;
set min_parallel_table_scan_size=0;
create table test_tb1(id int, bitmap roaringbitmap);
insert into test_tb1 values (NULL,NULL);
insert into test_tb1 select id,rb_build(ARRAY[id]) from generate_series(1,10000)id;
insert into test_tb1 values (NULL,NULL);
insert into test_tb1 values (10001,rb_build(ARRAY[10,100,1000,10000,10001]));

explain(costs off) 
with a as
(
select rb_build_agg(id) bitmap from test_tb1
)
select rb_cardinality(bitmap),rb_minimum(bitmap),rb_maximum(bitmap) from a;

with a as
(
select rb_build_agg(id) bitmap from test_tb1
)
select rb_cardinality(bitmap),rb_minimum(bitmap),rb_maximum(bitmap) from a;

explain(costs off) 
with a as
(
select rb_and_agg(bitmap) bitmap from test_tb1
)
select rb_cardinality(bitmap),rb_minimum(bitmap),rb_maximum(bitmap) from a;

with a as
(
select rb_and_agg(bitmap) bitmap from test_tb1
)
select rb_cardinality(bitmap),rb_minimum(bitmap),rb_maximum(bitmap) from a;

explain(costs off) 
with a as
(
select rb_or_agg(bitmap) bitmap from test_tb1
)
select rb_cardinality(bitmap),rb_minimum(bitmap),rb_maximum(bitmap) from a;

with a as
(
select rb_or_agg(bitmap) bitmap from test_tb1
)
select rb_cardinality(bitmap),rb_minimum(bitmap),rb_maximum(bitmap) from a;

explain(costs off) 
with a as
(
select rb_xor_agg(bitmap) bitmap from test_tb1
)
select rb_cardinality(bitmap),rb_minimum(bitmap),rb_maximum(bitmap) from a;

with a as
(
select rb_xor_agg(bitmap) bitmap from test_tb1
)
select rb_cardinality(bitmap),rb_minimum(bitmap),rb_maximum(bitmap) from a;

explain(costs off)
select rb_and_cardinality_agg(bitmap),rb_or_cardinality_agg(bitmap),rb_xor_cardinality_agg(bitmap) from test_tb1;

select rb_and_cardinality_agg(bitmap),rb_or_cardinality_agg(bitmap),rb_xor_cardinality_agg(bitmap) from test_tb1;
