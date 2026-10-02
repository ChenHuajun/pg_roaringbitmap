--
--  Test roaringbitmap64 data type
--

set client_min_messages = 'warning';
CREATE EXTENSION if not exists roaringbitmap;

-- Test input and output

set roaringbitmap.output_format='array';
set extra_float_digits = 0;

select  '{}'::roaringbitmap64;
select  '  { 	 }  '::roaringbitmap64;
select  '{	1 }'::roaringbitmap64;
select  '{-1,2,555555,-4}'::roaringbitmap64;
select  '{ -1 ,  2  , 555555 ,  -4  }'::roaringbitmap64;
select  '{ 1 ,  -2  , 555555 ,  -4  }'::roaringbitmap64;
select  '{ 1 ,  -2  , 555555 ,  -4  ,9223372036854775807,-9223372036854775808}'::roaringbitmap64;
select  roaringbitmap64('{ 1 ,  -2  , 555555 ,  -4  }');

set roaringbitmap.output_format='bytea';
select  '{}'::roaringbitmap64;
select  '{ -1 ,  2  , 555555 ,  -4  }'::roaringbitmap64;

set roaringbitmap.output_format='array';
select  '{}'::roaringbitmap64;
select '\x0000000000000000'::roaringbitmap64;
select '\x0200000000000000000000003a300000020000000000000008000000180000001a0000000200237affffffff3a30000001000000ffff010010000000fcffffff'::roaringbitmap64;

-- Exception
select  ''::roaringbitmap64;
select  '{'::roaringbitmap64;
select  '{1'::roaringbitmap64;
select  '{1} x'::roaringbitmap64;
select  '{1x}'::roaringbitmap64;
select  '{-x}'::roaringbitmap64;
select  '{,}'::roaringbitmap64;
select  '{1,}'::roaringbitmap64;
select  '{1,xxx}'::roaringbitmap64;
select  '{1,3'::roaringbitmap64;
select  '{1,1'::roaringbitmap64;
select  '{1,-9223372036854775809}'::roaringbitmap64;
select  '{9223372036854775808}'::roaringbitmap64;

-- Test Type cast
select '{}'::roaringbitmap64::bytea;
select '{1}'::roaringbitmap64::bytea;
select '{1,9999}'::roaringbitmap64::bytea;
select '{}'::roaringbitmap64::bytea::roaringbitmap64;
select '{1}'::roaringbitmap64::bytea::roaringbitmap64;
select '{1,9999,-88888}'::roaringbitmap64::bytea::roaringbitmap64;
select roaringbitmap64('{1,9999,-88888}'::roaringbitmap64::bytea);

select roaringbitmap('{}')::roaringbitmap64;
select roaringbitmap('{0,1,9999,2147483647,-2147483648,-2,-1}')::roaringbitmap64;
select roaringbitmap64('{}')::roaringbitmap;
select roaringbitmap64('{0,1,9999,2147483647,-2147483648,-2,-1}')::roaringbitmap;

-- Exception
select roaringbitmap64('\x11'::bytea);
select '\x11'::bytea::roaringbitmap64;
select roaringbitmap64('{0,1,9999,2147483648,-2147483648,-2,-1}')::roaringbitmap;
select roaringbitmap64('{0,1,9999,2147483647,-2147483649,-2,-1}')::roaringbitmap;

-- Test Operator

select roaringbitmap64('{}') & roaringbitmap64('{}');
select roaringbitmap64('{}') & roaringbitmap64('{3,4,5}');
select roaringbitmap64('{1,2,3}') & roaringbitmap64('{}');
select roaringbitmap64('{1,2,3}') & roaringbitmap64('{3,4,5}');
select roaringbitmap64('{1,-2,-3}') & roaringbitmap64('{-3,-4,5}');

select roaringbitmap64('{}') | roaringbitmap64('{}');
select roaringbitmap64('{}') | roaringbitmap64('{3,4,5}');
select roaringbitmap64('{1,2,3}') | roaringbitmap64('{}');
select roaringbitmap64('{1,2,3}') | roaringbitmap64('{3,4,5}');
select roaringbitmap64('{1,-2,-3}') | roaringbitmap64('{-3,-4,5}');

select roaringbitmap64('{}') | 6;
select roaringbitmap64('{1,2,3}') | 6;
select roaringbitmap64('{1,2,3}') | 1;
select roaringbitmap64('{1,2,3}') | -1;
select roaringbitmap64('{-1,-2,3}') | -1;

select 6 | roaringbitmap64('{}');
select 6 | roaringbitmap64('{1,2,3}');
select 1 | roaringbitmap64('{1,2,3}');
select -1 | roaringbitmap64('{1,2,3}');
select -1 | roaringbitmap64('{-1,-2,3}');

select roaringbitmap64('{}') # roaringbitmap64('{}');
select roaringbitmap64('{}') # roaringbitmap64('{3,4,5}');
select roaringbitmap64('{1,2,3}') # roaringbitmap64('{}');
select roaringbitmap64('{1,2,3}') # roaringbitmap64('{3,4,5}');
select roaringbitmap64('{1,-2,-3}') # roaringbitmap64('{-3,-4,5}');

select roaringbitmap64('{}') - roaringbitmap64('{}');
select roaringbitmap64('{}') - roaringbitmap64('{3,4,5}');
select roaringbitmap64('{1,2,3}') - roaringbitmap64('{}');
select roaringbitmap64('{1,2,3}') - roaringbitmap64('{3,4,5}');
select roaringbitmap64('{1,-2,-3}') - roaringbitmap64('{-3,-4,5}');

select roaringbitmap64('{}') - 3;
select roaringbitmap64('{1,2,3}') - 3;
select roaringbitmap64('{1,2,3}') - 1;
select roaringbitmap64('{1,2,3}') - -1;
select roaringbitmap64('{-1,-2,3}') - -1;

select roaringbitmap64('{}') << 2;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') << 2;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') << 1;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') << 0;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') << -1;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') << -2;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') << 4294967295;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') << 9223372036854775807;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') << -4294967295;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') << -9223372036854775807;

select roaringbitmap64('{}') >> 2;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') >> 2;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') >> 1;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') >> 0;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') >> -1;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') >> -2;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') >> 4294967295;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') >> 9223372036854775807;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') >> -4294967295;
select roaringbitmap64('{-2,-1,0,1,2,3,9223372036854775807,-9223372036854775808}') >> -9223372036854775807;
-- negating a negative shift distance must not be signed overflow either:
-- distance = INT64_MIN has no representable negation, and {1} shifted down by
-- 2^63 has no non-negative result, so the result is empty
select roaringbitmap64('{1}') >> -9223372036854775808;
select roaringbitmap64('{1,2,3}') >> -1;
select roaringbitmap64('{9223372036854775807}') >> 1;

select roaringbitmap64('{}') @> roaringbitmap64('{}');
select roaringbitmap64('{}') @> roaringbitmap64('{3,4,5}');
select roaringbitmap64('{1,2,3}') @> roaringbitmap64('{}');
select roaringbitmap64('{1,2,3}') @> roaringbitmap64('{3,4,5}');
select roaringbitmap64('{1,2,3}') @> roaringbitmap64('{3,2}');
select roaringbitmap64('{1,-2,-3}')  @> roaringbitmap64('{-3,1}');

select roaringbitmap64('{}') @> 2;
select roaringbitmap64('{1,2,3}') @> 20;
select roaringbitmap64('{1,2,3}') @> 1;
select roaringbitmap64('{1,2,3}') @> -1;
select roaringbitmap64('{-1,-2,3}') @> -1;

select roaringbitmap64('{}') <@ roaringbitmap64('{}');
select roaringbitmap64('{}') <@ roaringbitmap64('{3,4,5}');
select roaringbitmap64('{1,2,3}') <@ roaringbitmap64('{}');
select roaringbitmap64('{1,2,3}') <@ roaringbitmap64('{3,4,5}');
select roaringbitmap64('{2,3}') <@ roaringbitmap64('{1,3,2}');
select roaringbitmap64('{1,-3}')  <@ roaringbitmap64('{-3,1,1000}');

select 6 <@ roaringbitmap64('{}');
select 3 <@ roaringbitmap64('{1,2,3}');
select 1 <@ roaringbitmap64('{1,2,3}');
select -1 <@ roaringbitmap64('{1,2,3}');
select -1 <@ roaringbitmap64('{-1,-2,3}');

select roaringbitmap64('{}') && roaringbitmap64('{}');
select roaringbitmap64('{}') && roaringbitmap64('{3,4,5}');
select roaringbitmap64('{1,2,3}') && roaringbitmap64('{}');
select roaringbitmap64('{1,2,3}') && roaringbitmap64('{3,4,5}');
select roaringbitmap64('{1,-2,-3}') && roaringbitmap64('{-3,-4,5}');

select roaringbitmap64('{}') = roaringbitmap64('{}');
select roaringbitmap64('{}') = roaringbitmap64('{3,4,5}');
select roaringbitmap64('{1,2,3}') = roaringbitmap64('{}');
select roaringbitmap64('{1,2,3}') = roaringbitmap64('{3,1,2}');
select roaringbitmap64('{1,-2,-3}') = roaringbitmap64('{-3,-4,5}');

select roaringbitmap64('{}') <> roaringbitmap64('{}');
select roaringbitmap64('{}') <> roaringbitmap64('{3,4,5}');
select roaringbitmap64('{1,2,3}') <> roaringbitmap64('{}');
select roaringbitmap64('{1,2,3}') <> roaringbitmap64('{3,1,2}');
select roaringbitmap64('{1,-2,-3}') <> roaringbitmap64('{-3,-4,5}');

-- Test the functions with one bitmap variable

select rb64_build(NULL);
select rb64_build('{}'::int[]);
select rb64_build('{1}'::int[]);
select rb64_build('{-1,2,555555,-4}'::int[]);
select rb64_build('{1,-2,555555,-4,9223372036854775807,-9223372036854775808}'::bigint[]);

select rb64_to_array(NULL);
select rb64_to_array('{}'::roaringbitmap64);
select rb64_to_array('{1}'::roaringbitmap64);
select rb64_to_array('{-1,2,555555,-4}'::roaringbitmap64);
select rb64_to_array('{1,-2,555555,-4,9223372036854775807,-9223372036854775808}'::roaringbitmap64);

select rb64_is_empty(NULL);
select rb64_is_empty('{}');
select rb64_is_empty('{1}');
select rb64_is_empty('{1,10,100}');
select rb64_is_empty('{1,10,100,-4,9223372036854775807,-9223372036854775808}');

select rb64_cardinality(NULL);
select rb64_cardinality('{}');
select rb64_cardinality('{1}');
select rb64_cardinality('{1,10,100}');
select rb64_cardinality('{1,10,100,-4,9223372036854775807,-9223372036854775808}');

select rb64_max(NULL);
select rb64_max('{}');
select rb64_max('{1}');
select rb64_max('{1,10,100}');
select rb64_max('{1,10,100,9223372036854775807,-9223372036854775808,-1}');

select rb64_min(NULL);
select rb64_min('{}');
select rb64_min('{1}');
select rb64_min('{1,10,100}');
select rb64_min('{1,10,100,9223372036854775807,-9223372036854775808,-1}');

select rb64_iterate(NULL);
select rb64_iterate('{}');
select rb64_iterate('{1}');
select rb64_iterate('{1,10,100}');
select rb64_iterate('{1,10,100,9223372036854775807,-9223372036854775808,-1}');

select rb64_runoptimize(NULL);
select rb64_runoptimize('{}');
select rb64_runoptimize('{1}');
select rb64_runoptimize('{1,10,100}');
select rb64_runoptimize('{1,10,100,9223372036854775807,-9223372036854775808,-1}');

-- Test the functions with two bitmap variables

select rb64_and(NULL,'{1,10,100}');
select rb64_and('{1,10,100}',NULL);
select rb64_and('{}','{1,10,100}');
select rb64_and('{1,10,100}','{}');
select rb64_and('{2}','{1,10,100}');
select rb64_and('{1,2,10}','{1,10,100}');
select rb64_and('{1,10}','{1,10,100}');
select rb64_and('{1,10,9223372036854775807,-9223372036854775808,-1}','{1,10,100,-9223372036854775808,-1}');

select rb64_and_cardinality(NULL,'{1,10,100}');
select rb64_and_cardinality('{1,10,100}',NULL);
select rb64_and_cardinality('{}','{1,10,100}');
select rb64_and_cardinality('{1,10,100}','{}');
select rb64_and_cardinality('{2}','{1,10,100}');
select rb64_and_cardinality('{1,2,10}','{1,10,100}');
select rb64_and_cardinality('{1,10}','{1,10,100}');
select rb64_and_cardinality('{1,10,9223372036854775807,-9223372036854775808,-1}','{1,10,100,-9223372036854775808,-1}');

select rb64_or(NULL,'{1,10,100}');
select rb64_or('{1,10,100}',NULL);
select rb64_or('{}','{1,10,100}');
select rb64_or('{1,10,100}','{}');
select rb64_or('{2}','{1,10,100}');
select rb64_or('{1,2,10}','{1,10,100}');
select rb64_or('{1,10}','{1,10,100}');
select rb64_or('{1,10,9223372036854775807,-9223372036854775808,-1}','{1,10,100,-9223372036854775808,-1}');

select rb64_or_cardinality(NULL,'{1,10,100}');
select rb64_or_cardinality('{1,10,100}',NULL);
select rb64_or_cardinality('{}','{1,10,100}');
select rb64_or_cardinality('{1,10,100}','{}');
select rb64_or_cardinality('{2}','{1,10,100}');
select rb64_or_cardinality('{1,2,10}','{1,10,100}');
select rb64_or_cardinality('{1,10}','{1,10,100}');
select rb64_or_cardinality('{1,10,9223372036854775807,-9223372036854775808,-1}','{1,10,100,-9223372036854775808,-1}');

select rb64_xor(NULL,'{1,10,100}');
select rb64_xor('{1,10,100}',NULL);
select rb64_xor('{}','{1,10,100}');
select rb64_xor('{1,10,100}','{}');
select rb64_xor('{2}','{1,10,100}');
select rb64_xor('{1,2,10}','{1,10,100}');
select rb64_xor('{1,10}','{1,10,100}');
select rb64_xor('{1,10,9223372036854775807,-9223372036854775808,-1}','{1,10,100,-9223372036854775808,-1}');

select rb64_xor_cardinality(NULL,'{1,10,100}');
select rb64_xor_cardinality('{1,10,100}',NULL);
select rb64_xor_cardinality('{}','{1,10,100}');
select rb64_xor_cardinality('{1,10,100}','{}');
select rb64_xor_cardinality('{2}','{1,10,100}');
select rb64_xor_cardinality('{1,2,10}','{1,10,100}');
select rb64_xor_cardinality('{1,10}','{1,10,100}');
select rb64_xor_cardinality('{1,10,9223372036854775807,-9223372036854775808,-1}','{1,10,100,-9223372036854775808,-1}');

select rb64_equals(NULL,'{1,10,100}');
select rb64_equals('{1,10,100}',NULL);
select rb64_equals('{}','{1,10,100}');
select rb64_equals('{1,10,100}','{}');
select rb64_equals('{2}','{1,10,100}');
select rb64_equals('{1,2,10}','{1,10,100}');
select rb64_equals('{1,10}','{1,10,100}');
select rb64_equals('{1,10,100}','{1,10,100}');
select rb64_equals('{1,10,100,10}','{1,100,10}');
select rb64_equals('{1,10,9223372036854775807,-9223372036854775808,-1}','{1,10,100,-9223372036854775808,-1}');
select rb64_equals('{1,10,9223372036854775807,-9223372036854775808,-1}','{1,10,9223372036854775807,-9223372036854775808,-1}');

select rb64_intersect(NULL,'{1,10,100}');
select rb64_intersect('{1,10,100}',NULL);
select rb64_intersect('{}','{1,10,100}');
select rb64_intersect('{1,10,100}','{}');
select rb64_intersect('{2}','{1,10,100}');
select rb64_intersect('{1,2,10}','{1,10,100}');
select rb64_intersect('{1,10}','{1,10,100}');
select rb64_intersect('{1,10,100}','{1,10,100}');
select rb64_intersect('{1,10,100,10}','{1,100,10}');
select rb64_intersect('{1,10,9223372036854775807,-9223372036854775808,-1}','{1,10,100,-9223372036854775808,-1}');

select rb64_andnot(NULL,'{1,10,100}');
select rb64_andnot('{1,10,100}',NULL);
select rb64_andnot('{}','{1,10,100}');
select rb64_andnot('{1,10,100}','{}');
select rb64_andnot('{2}','{1,10,100}');
select rb64_andnot('{1,2,10}','{1,10,100}');
select rb64_andnot('{1,10}','{1,10,100}');
select rb64_andnot('{1,10,100}','{1,10,100}');
select rb64_andnot('{1,10,100,10}','{1,100,10}');
select rb64_andnot('{1,10,9223372036854775807,-9223372036854775808,-1}','{1,10,100,-9223372036854775808,-1}');

select rb64_andnot_cardinality(NULL,'{1,10,100}');
select rb64_andnot_cardinality('{1,10,100}',NULL);
select rb64_andnot_cardinality('{}','{1,10,100}');
select rb64_andnot_cardinality('{1,10,100}','{}');
select rb64_andnot_cardinality('{2}','{1,10,100}');
select rb64_andnot_cardinality('{1,2,10}','{1,10,100}');
select rb64_andnot_cardinality('{1,10}','{1,10,100}');
select rb64_andnot_cardinality('{1,10,100}','{1,10,100}');
select rb64_andnot_cardinality('{1,10,100,10}','{1,100,10}');
select rb64_andnot_cardinality('{1,10,9223372036854775807,-9223372036854775808,-1}','{1,10,100,-9223372036854775808,-1}');

select rb64_jaccard_dist(NULL,'{1,10,100}');
select rb64_jaccard_dist('{1,10,100}',NULL);
-- two empty bitmaps are identical, the similarity is 1, not NaN
select rb64_jaccard_dist('{}','{}');
select rb64_jaccard_dist('{}','{1,10,100}');
select rb64_jaccard_dist('{1,10,100}','{}');
select rb64_jaccard_dist('{2}','{1,10,100}');
select rb64_jaccard_dist('{1,2,10}','{1,10,100}');
select rb64_jaccard_dist('{1,10,11,12}','{1,10,100}');
select rb64_jaccard_dist('{1,10,100}','{1,10,11,12}');
select rb64_jaccard_dist('{1,10,100}','{1,10,100}');
select rb64_jaccard_dist('{1,10,-100}','{1,10,-100}');
select rb64_jaccard_dist('{1,10,100}','{1,10,-100}');
select rb64_jaccard_dist('{1,10,9223372036854775807,-9223372036854775808,-1}','{1,10,100,-9223372036854775808,-1}');

-- rb64_jaccard_index() is the correctly named version of rb64_jaccard_dist()
select rb64_jaccard_index(NULL,'{1,10,100}');
select rb64_jaccard_index('{}','{}');
select rb64_jaccard_index('{}','{1,10,100}');
select rb64_jaccard_index('{1,10,100}','{}');
select rb64_jaccard_index('{2}','{1,10,100}');
select rb64_jaccard_index('{1,2,10}','{1,10,100}');
select rb64_jaccard_index('{1,10,100}','{1,10,100}');
select rb64_jaccard_index('{1,10,-100}','{1,10,100}');
select rb64_jaccard_index('{1,10,9223372036854775807,-9223372036854775808,-1}','{1,10,100,-9223372036854775808,-1}');
select rb64_jaccard_dist('{1,2,10}','{1,10,100}') = rb64_jaccard_index('{1,2,10}','{1,10,100}');

-- Test other functions

select rb64_rank(NULL,0);
select rb64_rank('{}',0);
select rb64_rank('{1,10,100}',0);
select rb64_rank('{1,10,100}',1);
select rb64_rank('{1,10,100}',99);
select rb64_rank('{1,10,100}',100);
select rb64_rank('{1,10,100}',101);
select rb64_rank('{1,10,100,-3,-1}',-2);

select rb64_remove(NULL,0);
select rb64_remove('{}',0);
select rb64_remove('{1}',1);
select rb64_remove('{1,10,100}',0);
select rb64_remove('{1,10,100}',1);
select rb64_remove('{1,10,100}',99);
select rb64_remove('{1,10,100,9223372036854775807,-9223372036854775808,-1}',-9223372036854775808);

select rb64_fill(NULL,-1,-1);
select rb64_fill('{}',-1,-1);
select rb64_fill('{}',0,1);
select rb64_fill('{}',0,2);
select rb64_fill('{1,10,100}',10,10);
select rb64_fill('{1,10,100}',10,11);
select rb64_fill('{1,10,100}',10,12);
select rb64_fill('{1,10,100}',10,13);
select rb64_fill('{1,10,100}',10,20);
select rb64_fill('{1,10,100}',-1,-1);
select rb64_fill('{1,10,100,9223372036854775807,-9223372036854775808,-1}',9223372036854775800,9223372036854775807);
select rb64_cardinality(rb64_fill('{1,10,100}',2,1000000000));
-- rb64_fill()/rb64_flip() cover [range_start, range_end), at most 2^32 values
select rb64_cardinality(rb64_fill('{1,10,100}',0,4294967296));
select rb64_fill('{1,10,100}',0,4294967297); -- error to fill an excessively large range
select rb64_fill('{1}',9223372036854775807,-1); -- error, hint reports the bigint arguments
select rb64_cardinality(rb64_fill('{1,10,100}',0,0)); -- range_end = 0 is an empty range, not "unlimited"
select rb64_cardinality(rb64_fill('{1}',-1,0));
select rb64_cardinality(rb64_fill('{1,10,100,9223372036854775807,-9223372036854775808,-1}',9223372036854775800,9223372036854775807));

select rb64_index(NULL,3);
select rb64_index('{1,2,3}',NULL);
select rb64_index('{}',3);
select rb64_index('{1}',3);
select rb64_index('{1}',1);
select rb64_index('{1,10,100}',10);
select rb64_index('{1,10,100}',99);
select rb64_index('{1,10,-100}',-100);

select rb64_clear(NULL,0,10);
select rb64_clear('{}',0,10);
select rb64_clear('{1,10,100}',0,10);
select rb64_clear('{1,10,100}',3,3);
select rb64_clear('{1,10,100}',-3,3);
select rb64_clear('{1,10,100}',0,-1);
select rb64_clear('{1,10,100}',9,9);
select rb64_clear('{1,10,100}',2,1000000000);
select rb64_clear('{0,1,10,100,-2,-1}',1,4294967295);
select rb64_clear('{0,1,10,100,-2,-1}',0,4294967296);
select rb64_clear('{0,1,10,100,-2,-1}',1,-1);
select rb64_clear('{0,1,10,100,-2,-1}',0,9223372036854775800);

select rb64_flip(NULL,0,10);
select rb64_flip('{}',0,10);
select rb64_flip('{1,10,100}',9,100);
select rb64_flip('{1,10,100}',10,101);
select rb64_flip('{1,10,100}',-3,3);
select rb64_flip('{1,10,100}',-1,-1);
select rb64_flip('{1,10,100}',9,9);
select rb64_flip('{1,10,100,9223372036854775807,-9223372036854775808,-1}',9223372036854775800,9223372036854775807);
select rb64_cardinality(rb64_flip('{1,10,100}',2,1000000000));
select rb64_cardinality(rb64_flip('{1,10,100}',-1,5000000000));
select rb64_cardinality(rb64_flip('{1,10,100}',0,0)); -- range_end = 0 is an empty range, not "unlimited"
select rb64_cardinality(rb64_flip('{1,10,100}',0,4294967296));
select rb64_flip('{1,10,100}',0,4294967297); -- error to fill an excessively large range
select rb64_cardinality(rb64_flip('{1,10,100,9223372036854775807,-9223372036854775808,-1}',9223372036854775800,9223372036854775807));


select rb64_range(NULL,0,10);
select rb64_range('{}',0,10);
select rb64_range('{1,10,100}',0,10);
select rb64_range('{1,10,100}',3,3);
select rb64_range('{1,10,100}',-3,3);
select rb64_range('{1,10,100}',0,-1);
select rb64_range('{1,10,100}',9,9);
select rb64_range('{1,10,100}',2,1000000000);
select rb64_range('{0,1,10,100,-2,-1}',1,4294967295);
select rb64_range('{0,1,10,100,-2,-1}',0,4294967296);
select rb64_range('{0,1,10,100,-2,-1}',9223372036854775800,9223372036854775807);
select rb64_range('{0,1,10,100,-2,-1}',1,9223372036854775807);
select rb64_range('{0,1,10,100,-2,-1}',1,0);

select rb64_range_cardinality(NULL,0,10);
select rb64_range_cardinality('{}',0,10);
select rb64_range_cardinality('{1,10,100}',0,10);
select rb64_range_cardinality('{1,10,100}',3,3);
select rb64_range_cardinality('{1,10,100}',-3,3);
select rb64_range_cardinality('{1,10,100}',0,-1);
select rb64_range_cardinality('{1,10,100}',9,9);
select rb64_range_cardinality('{1,10,100}',2,1000000000);
select rb64_range_cardinality('{0,1,10,100,-2,-1}',1,4294967295);
select rb64_range_cardinality('{0,1,10,100,-2,-1}',0,4294967296);
select rb64_range_cardinality('{0,1,10,100,-2,-1}',9223372036854775800,9223372036854775807);
select rb64_range_cardinality('{0,1,10,100,-2,-1}',1,9223372036854775807);
select rb64_range_cardinality('{0,1,10,100,-2,-1}',1,0);

select rb64_select(NULL,10);
select rb64_select('{}',10);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',0);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',1);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',2);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',2,0);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',2,1);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',2,1,true);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',2,1,true,9);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',2,1,true,9,4294967295);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',2,1,false);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',2,1,false,9);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',2,1,false,10);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',2,1,false,10,10);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',2,1,false,-10,100);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',2,1,false,-10,-10);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',2,1,false,10,10001);
select rb64_select('{0,1,2,10,100,1000,9223372036854775807,-9223372036854775808,-2,-1}',2,1,true,10,10001);
-- a negative offset is clamped to 0, avoiding a count - offset signed overflow
select rb64_select('{1,2,3}',100,-9223372036854775808);
select rb64_select('{1,2,3}',100,-1);

-- input/output must stay exact for values at or above 2^31 on LLP64 platforms
select '{1,4294967296,9223372036854775807}'::roaringbitmap64;
select rb64_iterate('{1,4294967296,9223372036854775807}'::roaringbitmap64);
select rb64_to_roaringbitmap('{4294967296}'::roaringbitmap64);


-- Test aggregate

select rb64_and_agg(id) from (values (NULL::roaringbitmap64)) t(id);
select rb64_and_agg(id) from (values (roaringbitmap64('{}'))) t(id);
select rb64_and_agg(id) from (values (roaringbitmap64('{1}'))) t(id);
select rb64_and_agg(id) from (values (roaringbitmap64('{1,10,100}'))) t(id);
select rb64_and_agg(id) from (values (roaringbitmap64('{1,10,100}')),(roaringbitmap64('{2,10}'))) t(id);
select rb64_and_agg(id) from (values (roaringbitmap64('{1,10,100}')),(roaringbitmap64('{1,10,100}'))) t(id);
select rb64_and_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{}'))) t(id);
select rb64_and_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{1}'))) t(id);
select rb64_and_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{2}'))) t(id);
select rb64_and_agg(id) from (values (roaringbitmap64('{1,10,100}')),(NULL),(roaringbitmap64('{2,10}'))) t(id);
select rb64_and_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(NULL),(roaringbitmap64('{1,10,100,101}')),(NULL)) t(id);
select rb64_and_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100,9223372036854775807}')),(NULL),(roaringbitmap64('{1,10,100,101,9223372036854775807,-9223372036854775808,-2}')),(NULL)) t(id);

select rb64_and_cardinality_agg(id) from (values (NULL::roaringbitmap64)) t(id);
select rb64_and_cardinality_agg(id) from (values (roaringbitmap64('{}'))) t(id);
select rb64_and_cardinality_agg(id) from (values (roaringbitmap64('{1}'))) t(id);
select rb64_and_cardinality_agg(id) from (values (roaringbitmap64('{1,10,100}'))) t(id);
select rb64_and_cardinality_agg(id) from (values (roaringbitmap64('{1,10,100}')),(roaringbitmap64('{2,10}'))) t(id);
select rb64_and_cardinality_agg(id) from (values (roaringbitmap64('{1,10,100}')),(roaringbitmap64('{1,10,100}'))) t(id);
select rb64_and_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{}'))) t(id);
select rb64_and_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{1}'))) t(id);
select rb64_and_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{2}'))) t(id);
select rb64_and_cardinality_agg(id) from (values (roaringbitmap64('{1,10,100}')),(NULL),(roaringbitmap64('{2,10}'))) t(id);
select rb64_and_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(NULL),(roaringbitmap64('{1,10,100,101}')),(NULL)) t(id);
select rb64_and_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100,9223372036854775807}')),(NULL),(roaringbitmap64('{1,10,100,101,9223372036854775807,-9223372036854775808,-2}')),(NULL)) t(id);


select rb64_or_agg(id) from (values (NULL::roaringbitmap64)) t(id);
select rb64_or_agg(id) from (values (roaringbitmap64('{}'))) t(id);
select rb64_or_agg(id) from (values (roaringbitmap64('{1}'))) t(id);
select rb64_or_agg(id) from (values (roaringbitmap64('{1,10,100}'))) t(id);
select rb64_or_agg(id) from (values (roaringbitmap64('{1,10,100}')),(roaringbitmap64('{2,10}'))) t(id);
select rb64_or_agg(id) from (values (roaringbitmap64('{1,10,100}')),(roaringbitmap64('{1,10,100}'))) t(id);
select rb64_or_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{}'))) t(id);
select rb64_or_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{1}'))) t(id);
select rb64_or_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{2}'))) t(id);
select rb64_or_agg(id) from (values (roaringbitmap64('{1,10,100}')),(NULL),(roaringbitmap64('{2,10}'))) t(id);
select rb64_or_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(NULL),(roaringbitmap64('{1,10,100,101}')),(NULL)) t(id);
select rb64_or_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100,9223372036854775807}')),(NULL),(roaringbitmap64('{1,10,100,101,9223372036854775807,-9223372036854775808,-2}')),(NULL)) t(id);

select rb64_or_cardinality_agg(id) from (values (NULL::roaringbitmap64)) t(id);
select rb64_or_cardinality_agg(id) from (values (roaringbitmap64('{}'))) t(id);
select rb64_or_cardinality_agg(id) from (values (roaringbitmap64('{1}'))) t(id);
select rb64_or_cardinality_agg(id) from (values (roaringbitmap64('{1,10,100}'))) t(id);
select rb64_or_cardinality_agg(id) from (values (roaringbitmap64('{1,10,100}')),(roaringbitmap64('{2,10}'))) t(id);
select rb64_or_cardinality_agg(id) from (values (roaringbitmap64('{1,10,100}')),(roaringbitmap64('{1,10,100}'))) t(id);
select rb64_or_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{}'))) t(id);
select rb64_or_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{1}'))) t(id);
select rb64_or_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{2}'))) t(id);
select rb64_or_cardinality_agg(id) from (values (roaringbitmap64('{1,10,100}')),(NULL),(roaringbitmap64('{2,10}'))) t(id);
select rb64_or_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(NULL),(roaringbitmap64('{1,10,100,101}')),(NULL)) t(id);
select rb64_or_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100,9223372036854775807}')),(NULL),(roaringbitmap64('{1,10,100,101,9223372036854775807,-9223372036854775808,-2}')),(NULL)) t(id);

select rb64_xor_agg(id) from (values (NULL::roaringbitmap64)) t(id);
select rb64_xor_agg(id) from (values (roaringbitmap64('{}'))) t(id);
select rb64_xor_agg(id) from (values (roaringbitmap64('{1}'))) t(id);
select rb64_xor_agg(id) from (values (roaringbitmap64('{1,10,100}'))) t(id);
select rb64_xor_agg(id) from (values (roaringbitmap64('{1,10,100}')),(roaringbitmap64('{2,10}'))) t(id);
select rb64_xor_agg(id) from (values (roaringbitmap64('{1,10,100}')),(roaringbitmap64('{1,10,100}'))) t(id);
select rb64_xor_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{}'))) t(id);
select rb64_xor_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{1}'))) t(id);
select rb64_xor_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{2}'))) t(id);
select rb64_xor_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{2,10}'))) t(id);
select rb64_xor_agg(id) from (values (roaringbitmap64('{1,10,100}')),(NULL),(roaringbitmap64('{1,10,100,101}'))) t(id);
select rb64_xor_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(NULL),(roaringbitmap64('{1,10,101}')),(roaringbitmap64('{1,100,102}')),(NULL)) t(id);
select rb64_xor_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100,9223372036854775807}')),(NULL),(roaringbitmap64('{1,10,100,101,9223372036854775807,-9223372036854775808,-2}')),(NULL)) t(id);

select rb64_xor_cardinality_agg(id) from (values (NULL::roaringbitmap64)) t(id);
select rb64_xor_cardinality_agg(id) from (values (roaringbitmap64('{}'))) t(id);
select rb64_xor_cardinality_agg(id) from (values (roaringbitmap64('{1}'))) t(id);
select rb64_xor_cardinality_agg(id) from (values (roaringbitmap64('{1,10,100}'))) t(id);
select rb64_xor_cardinality_agg(id) from (values (roaringbitmap64('{1,10,100}')),(roaringbitmap64('{2,10}'))) t(id);
select rb64_xor_cardinality_agg(id) from (values (roaringbitmap64('{1,10,100}')),(roaringbitmap64('{1,10,100}'))) t(id);
select rb64_xor_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{}'))) t(id);
select rb64_xor_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{1}'))) t(id);
select rb64_xor_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{2}'))) t(id);
select rb64_xor_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(roaringbitmap64('{2,10}'))) t(id);
select rb64_xor_cardinality_agg(id) from (values (roaringbitmap64('{1,10,100}')),(NULL),(roaringbitmap64('{1,10,100,101}'))) t(id);
select rb64_xor_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100}')),(NULL),(roaringbitmap64('{1,10,101}')),(roaringbitmap64('{1,100,102}')),(NULL)) t(id);
select rb64_xor_cardinality_agg(id) from (values (NULL),(roaringbitmap64('{1,10,100,9223372036854775807}')),(NULL),(roaringbitmap64('{1,10,100,101,9223372036854775807,-9223372036854775808,-2}')),(NULL)) t(id);

select rb64_build_agg(id) from (values (NULL::int)) t(id);
select rb64_build_agg(id) from (values (1)) t(id);
select rb64_build_agg(id) from (values (1),(10)) t(id);
select rb64_build_agg(id) from (values (1),(10),(10),(100),(1)) t(id);
select rb64_build_agg(id) from (values (1),(10),(10),(100),(1),(-2),(9223372036854775807),(-9223372036854775808)) t(id);

-- Test Windows aggregate

with t(id,bitmap) as(
 values(0,NULL),(1,roaringbitmap64('{1,10}')),(2,NULL),(3,roaringbitmap64('{2,10}')),(4,roaringbitmap64('{10,100}'))
)
select id,bitmap,rb64_and_agg(bitmap) over(order by id),rb64_and_cardinality_agg(bitmap) over(order by id) from t;

with t(id,bitmap) as(
 values(0,NULL),(1,roaringbitmap64('{1,10}')),(2,NULL),(3,roaringbitmap64('{2,10}')),(4,roaringbitmap64('{10,100}'))
)
select id,bitmap,rb64_or_agg(bitmap) over(order by id),rb64_or_cardinality_agg(bitmap) over(order by id) from t;

with t(id,bitmap) as(
 values(0,NULL),(1,roaringbitmap64('{1,10}')),(2,NULL),(3,roaringbitmap64('{2,10}')),(4,roaringbitmap64('{10,100}'))
)
select id,bitmap,rb64_xor_agg(bitmap) over(order by id),rb64_xor_cardinality_agg(bitmap) over(order by id) from t;

with t(id) as(
 values(0),(1),(2),(NULL),(4),(NULL)
)
select id,rb64_build_agg(id) over(order by id) from t;

with t(id,bitmap) as(
 values(0,NULL),(1,roaringbitmap64('{1,10}')),(2,NULL),(3,roaringbitmap64('{2,10}')),(4,roaringbitmap64('{10,100}')),(5,roaringbitmap64('{10,100,9223372036854775807,-9223372036854775808,-2}'))
)
select id,bitmap,rb64_xor_agg(bitmap) over(order by id),rb64_xor_cardinality_agg(bitmap) over(order by id) from t;



-- Test parallel aggregate

set max_parallel_workers=8;
set max_parallel_workers_per_gather=2;
set parallel_setup_cost=0;
set parallel_tuple_cost=0;
set min_parallel_table_scan_size=0;

CREATE OR REPLACE FUNCTION get_json_plan(sql text) RETURNS SETOF json AS
$BODY$
BEGIN
    RETURN QUERY EXECUTE 'EXPLAIN (COSTS OFF,FORMAT JSON) ' || sql;

    RETURN;
 END
$BODY$
LANGUAGE plpgsql;

create table if not exists bitmap64_test_tb1(id int, bitmap roaringbitmap64);
truncate bitmap64_test_tb1;
insert into bitmap64_test_tb1 values (NULL,NULL);
insert into bitmap64_test_tb1 select id,rb64_build(ARRAY[id]) from generate_series(1,10000)id;
insert into bitmap64_test_tb1 values (NULL,NULL);
insert into bitmap64_test_tb1 values (10001,rb64_build(ARRAY[10,100,1000,10000,10001,9223372036854775807,-9223372036854775808,-2]));

select position('"Parallel Aware": true' in get_json_plan('
select rb64_cardinality(bitmap),rb64_min(bitmap),rb64_max(bitmap)
  from (select rb64_build_agg(id) bitmap from bitmap64_test_tb1)a
')::text) > 0 is_parallel_plan;

select rb64_cardinality(bitmap),rb64_min(bitmap),rb64_max(bitmap)
  from (select rb64_build_agg(id) bitmap from bitmap64_test_tb1)a;

select position('"Parallel Aware": true' in get_json_plan('
select rb64_cardinality(bitmap),rb64_min(bitmap),rb64_max(bitmap)
  from (select rb64_and_agg(bitmap) bitmap from bitmap64_test_tb1)a
')::text) > 0 is_parallel_plan;

select rb64_cardinality(bitmap),rb64_min(bitmap),rb64_max(bitmap)
  from (select rb64_and_agg(bitmap) bitmap from bitmap64_test_tb1)a;

select position('"Parallel Aware": true' in get_json_plan('
select rb64_cardinality(bitmap),rb64_min(bitmap),rb64_max(bitmap)
  from (select rb64_or_agg(bitmap) bitmap from bitmap64_test_tb1)a
')::text) > 0 is_parallel_plan;

select rb64_cardinality(bitmap),rb64_min(bitmap),rb64_max(bitmap)
  from (select rb64_or_agg(bitmap) bitmap from bitmap64_test_tb1)a;

select position('"Parallel Aware": true' in get_json_plan('
select rb64_cardinality(bitmap),rb64_min(bitmap),rb64_max(bitmap)
  from (select rb64_xor_agg(bitmap) bitmap from bitmap64_test_tb1)a
')::text) > 0 is_parallel_plan;

select rb64_cardinality(bitmap),rb64_min(bitmap),rb64_max(bitmap)
  from (select rb64_xor_agg(bitmap) bitmap from bitmap64_test_tb1)a;

select position('"Parallel Aware": true' in get_json_plan('
select rb64_and_cardinality_agg(bitmap),rb64_or_cardinality_agg(bitmap),rb64_xor_cardinality_agg(bitmap) from bitmap64_test_tb1
')::text) > 0 is_parallel_plan;

select rb64_and_cardinality_agg(bitmap),rb64_or_cardinality_agg(bitmap),rb64_xor_cardinality_agg(bitmap) from bitmap64_test_tb1;

--rb64_iterate() not support parallel on PG10 while run on parallel in PG11+
--explain(costs off) 
--select count(*) from (select rb64_iterate(bitmap) from bitmap64_test_tb1)a;

select count(*) from (select rb64_iterate(bitmap) from bitmap64_test_tb1)a;

select position('"Parallel Aware": true' in get_json_plan('
select id,bitmap,
  rb64_build_agg(id) over(w),
  rb64_or_agg(bitmap) over(w),
  rb64_and_agg(bitmap) over(w),
  rb64_xor_agg(bitmap) over(w) 
from bitmap64_test_tb1
window w as (order by id)
order by id limit 10
')::text) > 0 is_parallel_plan;

select id,bitmap,
  rb64_build_agg(id) over(w),
  rb64_or_agg(bitmap) over(w),
  rb64_and_agg(bitmap) over(w),
  rb64_xor_agg(bitmap) over(w) 
from bitmap64_test_tb1
window w as (order by id)
order by id limit 10;

-- bugfix #22
SELECT '{100373,1829130,1861002,1975442,2353213,2456403}'::roaringbitmap64 & '{2353213}'::roaringbitmap64;
SELECT rb64_and_cardinality('{100373,1829130,1861002,1975442,2353213,2456403}','{2353213}');

-- bugfix #45: rb64_to_array and rb64_min exhibits inconsistent results in a malformed serialize input
select rb64_to_array('\x0100000000000000000000003a300000010000000000000000000000000000000000000000000000000000000000000000000000000000000008'::roaringbitmap64),
rb64_min('\x0100000000000000000000003a300000010000000000000000000000000000000000000000000000000000000000000000000000000000000008'::roaringbitmap64);


-- ============================================================
-- rb64_group_elements_by_source tests
-- ============================================================

-- Three bitmaps with overlapping input sets (canonical example)
select sources, rb64_to_array(members) from rb64_group_elements_by_source(ARRAY[
  rb64_build(ARRAY[1,2,3,4,5,6,7,8,9,10]::bigint[]),
  rb64_build(ARRAY[1,2,3,4,5,11,12]::bigint[]),
  rb64_build(ARRAY[1,2,3,13,14,15]::bigint[])
]) order by sources;

-- NULL argument -> no rows
select sources, rb64_to_array(members) from rb64_group_elements_by_source(NULL::roaringbitmap64[]);

-- Empty array -> no rows
select sources, rb64_to_array(members) from rb64_group_elements_by_source(ARRAY[]::roaringbitmap64[]);

-- Some NULL bitmaps (skipped, but preserve source indices)
select sources, rb64_to_array(members) from rb64_group_elements_by_source(ARRAY[
  rb64_build(ARRAY[1,3]::bigint[]),
  NULL,
  rb64_build(ARRAY[3,5]::bigint[])
]) order by sources;

-- Single bitmap -> all elements grouped under sources={1}
select sources, rb64_to_array(members) from rb64_group_elements_by_source(ARRAY[
  rb64_build(ARRAY[1,2,3]::bigint[])
]) order by sources;

-- Disjoint inputs: each bitmap contributes its own group
select sources, rb64_to_array(members) from rb64_group_elements_by_source(ARRAY[
  rb64_build(ARRAY[1,2]::bigint[]),
  rb64_build(ARRAY[3,4]::bigint[]),
  rb64_build(ARRAY[5,6]::bigint[])
]) order by sources;

-- 64-bit specific: element values that overflow uint32
-- 4294967296 = 2^32, 9999999999 > UINT32_MAX. Mix with small values.
select sources, rb64_to_array(members) from rb64_group_elements_by_source(ARRAY[
  rb64_build(ARRAY[1, 4294967296, 9999999999]::bigint[]),
  rb64_build(ARRAY[1, 4294967296]::bigint[]),
  rb64_build(ARRAY[9999999999]::bigint[])
]) order by sources;

-- N > 64: exercise variable-width bitmask (65 inputs, requires 2 uint64 words)
-- Build 65 bitmaps where element 1 is in all, element 2 only in bitmap 1, element 3 only in bitmap 65
select sources, rb64_to_array(members) from rb64_group_elements_by_source(
  (select array_agg(
    case
      when i = 1 then rb64_build(ARRAY[1,2]::bigint[])
      when i = 65 then rb64_build(ARRAY[1,3]::bigint[])
      else rb64_build(ARRAY[1]::bigint[])
    end order by i
  ) from generate_series(1, 65) i)
) order by sources;


-- ============================================================
-- opclass support: btree / hash / gin + selectivity
-- ============================================================

-- rb64_cmp: lexicographic total order over the ascending element sequences.
-- Elements are compared as unsigned uint64.
select rb64_cmp(NULL, '{}');
select rb64_cmp('{}', NULL);
select rb64_cmp('{}', '{}');
select rb64_cmp('{}', '{1}');
select rb64_cmp('{1}', '{}');
select rb64_cmp('{1,2}', '{1,3}');
select rb64_cmp('{1,3}', '{2}');
select rb64_cmp('{1,3,9}', '{2}');
select rb64_cmp('{1,3,9}', '{1,3,9,10}');
select rb64_cmp('{1}', '{1}');
select rb64_cmp('{1,2}', '{1,2}');
select rb64_cmp('{1,2}', '{1}');
select rb64_cmp('{1,2}', '{0}');
select rb64_cmp('{-1}', '{1}');
select rb64_cmp('{-9223372036854775808}', '{9223372036854775807}');
select rb64_cmp('{0,-9223372036854775808}', '{0,2}');
select rb64_cmp('{0,-9223372036854775808}', '{0,-1}');

-- rb64_cmp == 0 iff rb64_equals, independent of run optimization
select rb64_cmp(rb64_build('{1,2,3}'), rb64_runoptimize(rb64_build('{1,2,3}')));

-- comparison operators '<' '<=' '=' '>=' '>' (backed by rb64_lt / rb64_le /
-- rb64_equals / rb64_ge / rb64_gt, all defined through rb64_cmp.
select l, r,
       l <  r as lt, l <= r as le, l =  r as eq, l >= r as ge, l >  r as gt
  from (values
    ('{}'::roaringbitmap64,             '{}'::roaringbitmap64),             -- equal, both empty
    ('{}'::roaringbitmap64,             '{1}'::roaringbitmap64),            -- empty is smaller
    ('{1}'::roaringbitmap64,            '{}'::roaringbitmap64),            -- empty is smaller (swapped)
    ('{1,2}'::roaringbitmap64,          '{1,2}'::roaringbitmap64),         -- equal, non-empty
    ('{1}'::roaringbitmap64,            '{1,2}'::roaringbitmap64),         -- strict prefix -> smaller
    ('{1,2,3}'::roaringbitmap64,        '{1,2}'::roaringbitmap64),         -- superset -> greater
    ('{1,3}'::roaringbitmap64,          '{2}'::roaringbitmap64),           -- same length, first differs
    ('{1,3,9}'::roaringbitmap64,        '{2}'::roaringbitmap64),           -- 3 vs 1 element, first differs
    ('{-5,-1}'::roaringbitmap64,        '{-2}'::roaringbitmap64),          -- negatives keep their order
    ('{-5,-3,-1}'::roaringbitmap64,     '{-5,-1}'::roaringbitmap64),       -- 3 vs 2 elements, -3 < -1 -> smaller
    ('{-2}'::roaringbitmap64,           '{-2}'::roaringbitmap64),          -- equal negatives
    ('{9223372036854775807}'::roaringbitmap64, '{-9223372036854775808}'::roaringbitmap64), -- unsigned: -9223372036854775808 is largest
    ('{-9223372036854775808}'::roaringbitmap64, '{-1}'::roaringbitmap64),          -- unsigned: -9223372036854775808 < -1
    ('{0,-9223372036854775808}'::roaringbitmap64, '{0,2}'::roaringbitmap64),       -- unsigned: first difference decides
    ('{0,-9223372036854775808}'::roaringbitmap64, '{0,-1}'::roaringbitmap64)       -- unsigned: first difference decides
  ) t(l, r);

-- rb64_hash / rb64_hash_extended: equal sets hash equal, even after runoptimize
select rb64_hash('{}') = rb64_hash('{}');
select rb64_hash('{1,2,3}') = rb64_hash('{1,2,3}');
select rb64_hash('{1,2,3}') = rb64_hash(rb64_runoptimize('{1,2,3}'));
select rb64_hash('{1,2,3,-1}') = rb64_hash('{1,2,3,-1}');
select rb64_hash('{1,2,3,-1}') = rb64_hash(rb64_runoptimize('{1,2,3,-1}'));
select rb64_hash_extended('{1,2,3}', 42) = rb64_hash_extended('{1,2,3}', 42);
select rb64_hash_extended('{1,2,3}', 42) = rb64_hash_extended(rb64_runoptimize('{1,2,3}'), 42);
select rb64_hash_extended('{1,2,3,-1}', 42) = rb64_hash_extended('{1,2,3,-1}', 42);
select rb64_hash_extended('{1,2,3,-1}', 42) = rb64_hash_extended(rb64_runoptimize('{1,2,3,-1}'), 42);

-- btree opclass: total order through index
create table if not exists rb64_test_opclass (id int, rb roaringbitmap64);
truncate rb64_test_opclass;
insert into rb64_test_opclass values
  (1, rb64_build('{2}')),
  (2, rb64_build('{}')),
  (3, rb64_build('{1,2}')),
  (4, rb64_build('{1,2,-1}')),
  (5, rb64_build('{1}')),
  (6, rb64_build('{1,2}')),
  (7, NULL);
create index if not exists rb64_test_btree_idx on rb64_test_opclass using btree (rb);
analyze rb64_test_opclass;

set enable_seqscan = off;
set max_parallel_workers_per_gather = 0;

select id,rb from rb64_test_opclass order by rb;
select id,rb from rb64_test_opclass where rb < rb64_build('{1,2}') order by id;
select id,rb from rb64_test_opclass where rb >= rb64_build('{1}') order by id;
select rb, count(*) from rb64_test_opclass group by rb order by rb;
select count(distinct rb) from rb64_test_opclass;

-- hash opclass: hash join path returns correct rows
create table if not exists rb64_test_hash (id int, rb roaringbitmap64);
truncate rb64_test_hash;
insert into rb64_test_hash values
  (1, rb64_build('{2}')),
  (2, rb64_build('{}')),
  (3, rb64_build('{1,2}')),
  (4, rb64_build('{1,2,-1}')),
  (5, rb64_build('{1}')),
  (6, rb64_build('{1,2}')),
  (7, NULL);
create index if not exists rb64_test_hash_idx on rb64_test_hash using hash (rb);
analyze rb64_test_hash;

select a.id,a.rb from rb64_test_hash a join rb64_test_hash b on a.rb = b.rb order by a.id;

-- gin opclass: all five strategies + empty-set boundaries
create table if not exists rb64_test_gin (id int, rb roaringbitmap64);
truncate rb64_test_gin;
insert into rb64_test_gin values
  (1, rb64_build('{}')),
  (2, rb64_build('{1}')),
  (3, rb64_build('{1,2}')),
  (4, rb64_build('{2,3,4}')),
  (5, rb64_build('{1,2,3,4,5}')),
  (6, rb64_build('{1,2,3,4,5,-1}')),
  (7, rb64_build('{1,2,3,4,-1}')),
  (8, NULL);
create index if not exists rb64_test_gin_idx on rb64_test_gin using gin (rb);
analyze rb64_test_gin;

select id,rb from rb64_test_gin where rb && rb64_build('{2}') order by id;
select id,rb from rb64_test_gin where rb @> rb64_build('{1,2}') order by id;
select id,rb from rb64_test_gin where rb <@ rb64_build('{1,2}') order by id;
select id,rb from rb64_test_gin where rb = rb64_build('{1,2}') order by id;
select id,rb from rb64_test_gin where rb @> 2 order by id;
select id,rb from rb64_test_gin where rb && rb64_build('{-1}') order by id;
select id,rb from rb64_test_gin where rb @> rb64_build('{1,-1}') order by id;
select id,rb from rb64_test_gin where rb <@ rb64_build('{1,-1}') order by id;
select id,rb from rb64_test_gin where rb = rb64_build('{1,-1}') order by id;
select id,rb from rb64_test_gin where rb @> -1 order by id;
-- empty-set boundaries
select id,rb from rb64_test_gin where rb && rb64_build('{}') order by id;  -- none
select id,rb from rb64_test_gin where rb @> rb64_build('{}') order by id;  -- all non-NULL
select id,rb from rb64_test_gin where rb <@ rb64_build('{}') order by id;  -- only the empty row
select id,rb from rb64_test_gin where rb = rb64_build('{}') order by id;   -- only the empty row

-- btree: ORDER BY becomes an ordered index scan
select position('Index Scan' in get_json_plan(
  'select id from rb64_test_opclass order by rb'
)::text) > 0 as btree_order_by;

-- btree: all five comparison operators are indexable
select position('rb64_test_btree_idx' in p) > 0 and position('Index Cond": "(rb < ' in p) > 0 as btree_lt
  from (select get_json_plan('select id from rb64_test_opclass where rb <  rb64_build(''{1,2}'')')::text as p) s;
select position('rb64_test_btree_idx' in p) > 0 and position('Index Cond": "(rb <= ' in p) > 0 as btree_le
  from (select get_json_plan('select id from rb64_test_opclass where rb <= rb64_build(''{1,2}'')')::text as p) s;
select position('rb64_test_btree_idx' in p) > 0 and position('Index Cond": "(rb = ' in p) > 0 as btree_eq
  from (select get_json_plan('select id from rb64_test_opclass where rb =  rb64_build(''{1,2}'')')::text as p) s;
select position('rb64_test_btree_idx' in p) > 0 and position('Index Cond": "(rb >= ' in p) > 0 as btree_ge
  from (select get_json_plan('select id from rb64_test_opclass where rb >= rb64_build(''{1,2}'')')::text as p) s;
select position('rb64_test_btree_idx' in p) > 0 and position('Index Cond": "(rb > ' in p) > 0 as btree_gt
  from (select get_json_plan('select id from rb64_test_opclass where rb >  rb64_build(''{1,2}'')')::text as p) s;

-- hash: '=' is indexable
select position('rb64_test_hash_idx' in p) > 0 and position('Index Cond": "(rb = ' in p) > 0 as hash_eq
  from (select get_json_plan('select id from rb64_test_hash where rb = rb64_build(''{1,2,3}'')')::text as p) s;

-- gin: all five strategies are indexable
select position('rb64_test_gin_idx' in p) > 0 and position('Index Cond": "(rb && ' in p) > 0 as gin_overlap
  from (select get_json_plan('select id from rb64_test_gin where rb && rb64_build(''{2}'')')::text as p) s;
select position('rb64_test_gin_idx' in p) > 0 and position('Index Cond": "(rb @> ' in p) > 0 as gin_contains
  from (select get_json_plan('select id from rb64_test_gin where rb @> rb64_build(''{1,2}'')')::text as p) s;
select position('rb64_test_gin_idx' in p) > 0 and position('Index Cond": "(rb <@ ' in p) > 0 as gin_contained_by
  from (select get_json_plan('select id from rb64_test_gin where rb <@ rb64_build(''{1,2}'')')::text as p) s;
select position('rb64_test_gin_idx' in p) > 0 and position('Index Cond": "(rb = ' in p) > 0 as gin_eq
  from (select get_json_plan('select id from rb64_test_gin where rb = rb64_build(''{1,2}'')')::text as p) s;
select position('rb64_test_gin_idx' in p) > 0 and position('Index Cond": "(rb @> ''2''::bigint' in p) > 0 as gin_contains_int
  from (select get_json_plan('select id from rb64_test_gin where rb @> 2')::text as p) s;

reset enable_seqscan;
reset max_parallel_workers_per_gather;

-- test comparison operators in self-join elimination
-- only = operator can set MERGES = true
select oprname, oprcanmerge, oprcanhash from pg_operator
  where oprleft = 'roaringbitmap64'::regtype
    and oprright = 'roaringbitmap64'::regtype
    and oprname in ('=', '<', '<=', '>=', '>')
  order by oprname;

create table if not exists rb64_test_sje (a int primary key, rb roaringbitmap64);
truncate rb64_test_sje;
insert into rb64_test_sje values
  (1, rb64_build('{}')),
  (2, rb64_build('{1}')),
  (3, rb64_build('{1,2}')),
  (4, rb64_build('{2,3,4}')),
  (5, rb64_build('{1,2,3,4,5}')),
  (6, rb64_build('{1,2,3,4,5,-1}')),
  (7, rb64_build('{1,2,3,4,-1}')),
  (8, NULL);
analyze rb64_test_sje;

-- x.a = y.a forces the same row on both sides, so "<" and ">" must match nothing
select count(*) from rb64_test_sje x, rb64_test_sje y where x.a = y.a and x.rb <  y.rb;
select count(*) from rb64_test_sje x, rb64_test_sje y where x.a = y.a and x.rb >  y.rb;
-- while "<=", ">=" and "=" must still match every row
select count(*) from rb64_test_sje x, rb64_test_sje y where x.a = y.a and x.rb <= y.rb;
select count(*) from rb64_test_sje x, rb64_test_sje y where x.a = y.a and x.rb >= y.rb;
select count(*) from rb64_test_sje x, rb64_test_sje y where x.a = y.a and x.rb =  y.rb;

-- the comparison qual must survive self-join elimination; from 18 on the
-- elimination collapses both sides to "rb < rb", older servers keep the two
-- aliases ("x.rb < y.rb"), so only "<" can be matched on every version
select position('rb < ' in get_json_plan(
  'select x.a from rb64_test_sje x, rb64_test_sje y where x.a = y.a and x.rb < y.rb'
)::text) > 0 as sje_keeps_lt_qual;


-- ============================================================
-- 64-bit specific: elements beyond uint32 order and index correctly
-- ============================================================

create table if not exists rb64_test_wide (id int, rb roaringbitmap64);
truncate rb64_test_wide;
insert into rb64_test_wide values
  (1, rb64_build('{}')),
  (2, rb64_build('{4294967296}')),
  (3, rb64_build('{4294967296,9999999999}')),
  (4, rb64_build('{9999999999}')),
  (5, rb64_build('{-9223372036854775808}')),
  (6, rb64_build('{9223372036854775807}')),
  (7, rb64_build('{-1}')),
  (8, NULL);
create index if not exists rb64_test_wide_btree_idx on rb64_test_wide using btree (rb);
create index if not exists rb64_test_wide_hash_idx on rb64_test_wide using hash (rb);
create index if not exists rb64_test_wide_gin_idx on rb64_test_wide using gin (rb);
analyze rb64_test_wide;

set enable_seqscan = off;
set max_parallel_workers_per_gather = 0;

-- btree: uint64 order, so -9223372036854775808 sorts last
select id,rb from rb64_test_wide order by rb;
select id,rb from rb64_test_wide where rb < rb64_build('{9999999999}') order by id;
select id,rb from rb64_test_wide where rb >= rb64_build('{9223372036854775807}') order by id;

-- hash: equal sets hash equal
select a.id,a.rb from rb64_test_wide a join rb64_test_wide b on a.rb = b.rb order by a.id;

-- gin: elements wider than uint32 become int8 keys
select id,rb from rb64_test_wide where rb @> 4294967296 order by id;
select id,rb from rb64_test_wide where rb @> 9223372036854775807 order by id;
select id,rb from rb64_test_wide where rb @> -1 order by id;
select id,rb from rb64_test_wide where rb && rb64_build('{9999999999}') order by id;
select id,rb from rb64_test_wide where rb @> rb64_build('{4294967296}') order by id;
select id,rb from rb64_test_wide where rb <@ rb64_build('{4294967296,9999999999}') order by id;
select id,rb from rb64_test_wide where rb = rb64_build('{4294967296}') order by id;
select id,rb from rb64_test_wide where rb @> rb64_build('{4294967296,-9223372036854775808}') order by id;

-- btree and gin are chosen for the wide values as well
select position('rb64_test_wide_btree_idx' in p) > 0 and position('Index Cond": "(rb < ' in p) > 0 as wide_btree_lt
  from (select get_json_plan('select id from rb64_test_wide where rb < rb64_build(''{9999999999}'')')::text as p) s;
select position('rb64_test_wide_gin_idx' in p) > 0 and position('Index Cond": "(rb && ' in p) > 0 as wide_gin_overlap
  from (select get_json_plan('select id from rb64_test_wide where rb && rb64_build(''{9999999999}'')')::text as p) s;
select position('rb64_test_wide_gin_idx' in p) > 0 and position('Index Cond": "(rb @> ''4294967296''::bigint' in p) > 0 as wide_gin_contains_int
  from (select get_json_plan('select id from rb64_test_wide where rb @> 4294967296')::text as p) s;

reset enable_seqscan;
reset max_parallel_workers_per_gather;


-- ============================================================
-- statistics: rb64_typanalyze collects no MCV / histogram
-- ============================================================

-- The type names a custom typanalyze function.  
select typname, typanalyze::regproc
  from pg_type
 where typname in ('roaringbitmap64')
 order by typname;

create table if not exists rb64_test_stats (id int, small_rb roaringbitmap64, big_rb roaringbitmap64);
truncate rb64_test_stats;
-- small_rb stays inline, big_rb is stored in the TOAST table
insert into rb64_test_stats
  select g,
         rb64_build(array(select x from generate_series(g * 10, g * 10 + 99) x)),
         rb64_build(array(select x from generate_series(g * 100000, g * 100000 + 4999) x))
    from generate_series(1, 10) g;
insert into rb64_test_stats
  select g,
         rb64_build(array(select x from generate_series(1, 99) x)),
         rb64_build(array(select x from generate_series(1, 4999) x))
    from generate_series(1000, 1010) g;
analyze rb64_test_stats;

-- statistic for roaringbitmap64 column: no MCV, no histogram, and n_distinct = 0.
-- n_distinct = 0 means "unknown", which sends the planner back to 
-- DEFAULT_NUM_DISTINCT(200) -- the same estimate it used before 1.3 gave the 
-- type a btree operator class.
select attname, null_frac, n_distinct, avg_width,
       most_common_vals is null as no_mcv,
       histogram_bounds is null as no_histogram
  from pg_stats
 where tablename = 'rb64_test_stats'
 order by attname;

-- dropping the typanalyze function brings the MCV and histogram back, except for
-- values above WIDTH_THRESHOLD (1024 byte).
alter type roaringbitmap64 set (analyze = none);
analyze rb64_test_stats;
select attname, null_frac, n_distinct, avg_width,
       most_common_vals is null as no_mcv,
       histogram_bounds is null as no_histogram
  from pg_stats
 where tablename = 'rb64_test_stats'
 order by attname;

alter type roaringbitmap64 set (analyze = rb64_typanalyze);

-- ============================================================
-- bulk CRoaring paths: rb64_build, rb64_to_array, rb64_range,
-- rb64_range_cardinality, rb64_select, rb64_shiftright, the casts to and
-- from roaringbitmap and GIN key extraction
-- ============================================================

-- rb64_build edge cases: duplicates, unsorted, negatives, int8 limits, nulls
select rb64_build('{}'::bigint[]);
select rb64_build(NULL::bigint[]);
select rb64_build('{3,1,2,3,1,-1,-9223372036854775808,9223372036854775807,0,0,4294967296}');
select rb64_build('{{4,3},{2,1}}'::bigint[]);
select rb64_build('{1,NULL,3}');
select rb64_build('{-1,-1,-1}');
select rb64_to_array('{}');
select rb64_to_array(NULL);
select rb64_to_array('{9223372036854775807,-9223372036854775808,-1,0,1,4294967296}');

-- fixtures that cover array, bitset and run containers, including members
-- above 2^63 (negative int8) and on both sides of 2^32
create temp table rb64_bulk_fx(name text, bm roaringbitmap64);
insert into rb64_bulk_fx values
  ('empty', '{}'),
  ('small', '{5,1,-1,0,-9223372036854775808,9223372036854775807,65535,65536,4294967295,4294967296}'),
  ('sparse', rb64_build(array(select g * 1844674407370955 - 9223372036854775807
                              from generate_series(1, 5000) g))),
  ('dense', rb64_build(array(select generate_series(1, 70000)::bigint))),
  ('runs', rb64_runoptimize(rb64_fill(rb64_fill(rb64_fill('{-5,-1,170000}', 100, 100000),
                                                4294900000, 4295000000), -30000, -3000))),
  ('edges', rb64_build('{0,65535,65536,4294967295,4294967296,-65537,-65536,-1,9223372036854775807,-9223372036854775808}'));
insert into rb64_bulk_fx select 'mixed', rb64_or_agg(bm) from rb64_bulk_fx;

-- members in iteration (unsigned) order: position i and unsigned value u
create temp table rb64_bulk_m as
  select f.name, t.x, case when t.x < 0 then t.x::numeric + 18446744073709551616 else t.x::numeric end as u, t.o - 1 as i
    from rb64_bulk_fx f, rb64_iterate(f.bm) with ordinality t(x, o);
create index on rb64_bulk_m (name, u);
create index on rb64_bulk_m (name, i);
analyze rb64_bulk_m;

-- the bitmap built one member at a time from an int8 array (text input path)
create function rb64_bulk_ref(bigint[]) returns bytea language sql immutable
  as $$ select (coalesce($1, '{}')::text)::roaringbitmap64::bytea $$;

-- ranges as signed arguments plus the unsigned bounds they mean; range_end 0
-- means no upper bound
create temp table rb64_bulk_rng as
  select s, e, case when s < 0 then s::numeric + 18446744073709551616 else s::numeric end as su,
         case when e = 0 then 18446744073709551616
              when e < 0 then e::numeric + 18446744073709551616 else e::numeric end as eu
    from (values (0::bigint, 0::bigint), (0, 1), (1, 0), (100, 70000), (65536, 131072),
                 (4294967295, 4294967297), (-1, 0), (-5, -1), (70000, 100), (5, 5), (-1, -1),
                 (9223372036854775807, -9223372036854775808), (-9223372036854775808, 0),
                 (0, 4097), (0, 4098)) v(s, e);

-- rb64_to_array matches rb64_iterate; rb64_build gives the same bytes from the
-- array, from the array followed by a reversed copy, and from the text form
select f.name, rb64_cardinality(f.bm),
       rb64_to_array(f.bm) = coalesce((select array_agg(x order by i) from rb64_bulk_m m where m.name = f.name), '{}') as to_array_ok,
       rb64_build(rb64_to_array(f.bm))::bytea = rb64_bulk_ref(rb64_to_array(f.bm)) as build_ok,
       rb64_build(rb64_to_array(f.bm) || coalesce((select array_agg(x order by i desc) from rb64_bulk_m m where m.name = f.name), '{}'))::bytea
         = rb64_bulk_ref(rb64_to_array(f.bm)) as build_unsorted_ok
  from rb64_bulk_fx f order by f.name;

-- rb64_range gives the same bytes as building the in-range members one by one;
-- rb64_range_cardinality counts them
select f.name,
       bool_and(rb64_range(f.bm, r.s, r.e)::bytea =
                rb64_bulk_ref((select array_agg(x order by i) from rb64_bulk_m m
                                where m.name = f.name and m.u >= r.su and m.u < r.eu))) as range_ok,
       bool_and(rb64_range_cardinality(f.bm, r.s, r.e) =
                (select count(*) from rb64_bulk_m m where m.name = f.name and m.u >= r.su and m.u < r.eu)) as range_card_ok
  from rb64_bulk_fx f, rb64_bulk_rng r group by f.name order by f.name;

-- rb64_select, forward and reverse, against the members picked by position:
-- lo is the position of the first in-range member and n the in-range count;
-- reverse keeps the existing offset arithmetic
select f.name,
       bool_and(rb64_select(f.bm, p.lim, p.off, p.rev, r.s, r.e)::bytea =
                rb64_bulk_ref((select array_agg(x order by i) from rb64_bulk_m m
                                where m.name = f.name and m.u >= r.su and m.u < r.eu and p.lim > 0
                                  and m.i - c.lo >= c.a and m.i - c.lo < c.a + p.lim
                                  and (not p.rev or c.n > greatest(p.off, 0))))) as select_ok
  from rb64_bulk_fx f, rb64_bulk_rng r,
       (values (0::bigint, 0::bigint, false), (1, 0, false), (10, 3, false), (5000, 100, false),
               (9223372036854775807, 0, false), (10, -5, false), (1, 0, true), (10, 3, true),
               (5000, 100, true), (9223372036854775807, 9223372036854775807, true),
               (100000, 20, true)) p(lim, off, rev),
       lateral (select lo, n,
                       case when not p.rev then greatest(p.off, 0)::numeric
                            else greatest(n - greatest(p.off, 0)::numeric - p.lim, 0) end as a
                  from (select coalesce(min(i), 0) as lo, count(*) as n from rb64_bulk_m m
                         where m.name = f.name and m.u >= r.su and m.u < r.eu) s) c
 group by f.name order by f.name;

-- rb64_shiftright drops members shifted out of [0, 2^64); distance 0 returns
-- the input unchanged
select f.name,
       bool_and(rb64_shiftright(f.bm, d)::bytea = case when d = 0 then f.bm::bytea else
                rb64_bulk_ref((select array_agg(((m.u + d) - case when m.u + d >= 9223372036854775808 then 18446744073709551616 else 0 end)::bigint order by i)
                                from rb64_bulk_m m where m.name = f.name and m.u + d >= 0 and m.u + d < 18446744073709551616)) end) as shift_ok
  from rb64_bulk_fx f,
       unnest('{0,1,-1,65536,-65536,100000,-100000,4294967296,-4294967296,9223372036854775807,-9223372036854775807,-9223372036854775808}'::bigint[]) d
 group by f.name order by f.name;

-- casts between roaringbitmap and roaringbitmap64
select f.name,
       (f.bm::roaringbitmap64)::bytea = rb64_bulk_ref(rb_to_array(f.bm)::bigint[]) as to_rb64_ok,
       (f.bm::roaringbitmap64)::roaringbitmap::bytea = (rb_to_array(f.bm)::text)::roaringbitmap::bytea as round_trip_ok
  from (values ('empty', '{}'::roaringbitmap),
               ('small', '{5,1,-1,0,-2147483648,2147483647,65535,65536}'),
               ('dense', rb_build(array(select generate_series(-70000, 70000)))),
               ('runs', rb_runoptimize(rb_fill('{-5,-1,170000}', 100, 100000)))) f(name, bm)
 order by f.name;
select '{0,2147483647,-2147483648,-1}'::roaringbitmap64::roaringbitmap;
select '{0,2147483648}'::roaringbitmap64::roaringbitmap;
select '{0,-2147483649}'::roaringbitmap64::roaringbitmap;
select '{-2147483649,2147483648}'::roaringbitmap64::roaringbitmap;

-- GIN key extraction from bitmaps with many members
create temp table rb64_bulk_gin as select name, bm from rb64_bulk_fx;
create index on rb64_bulk_gin using gin (bm);
set enable_seqscan = off;
select name from rb64_bulk_gin where bm @> 69999::bigint order by name;
select name from rb64_bulk_gin where bm @> (-1)::bigint order by name;
select name from rb64_bulk_gin where bm @> rb64_build('{100,4294967296}') order by name;
reset enable_seqscan;
select rb64_select('{1,2,3}',-1,0);
select rb64_select('{1,2,3}',-1,0,true);
-- rb64_range copies without the total count when in_card <= input bytes / 1024:
-- k members in [0, k), m more in the same container and c containers above,
-- sized to put k * 1024 just above (1), at (0) and just below (-1) the input
-- bytes, plus cases on the remove path
select sign(k * 1024 - octet_length(bm::bytea)) as vs_bytes, count(*),
       bool_and(rb64_range(bm, 0, k)::bytea = rb64_bulk_ref(array(select generate_series(0, k - 1)::bigint))) as range_ok
  from (select k, rb64_build(array(select generate_series(0, k - 1)::bigint)
                             || array(select 60000 + j::bigint from generate_series(1, m) j)
                             || array(select g::bigint * 65536 from generate_series(1, c) g)) as bm
          from (select k, (t - 28 - 2 * k) / 10 as c, ((t - 28 - 2 * k) % 10) / 2 as m
                  from generate_series(1, 8) k, unnest(array[1024 * k - 2, 1024 * k, 1024 * k + 2]) t
                union all values (100, 2000, 0), (1000, 1, 0), (1000, 20, 0)) p) f
 group by 1 order by 1;
-- a small range, and an empty one, inside a bitmap with many 2^32 buckets
select rb64_range(rb64_build(array(select g * 4294967296 from generate_series(0, 99) g) || 42949672962), 42949672960, 42949672963)::bytea = rb64_bulk_ref('{42949672960,42949672962}');
select rb64_range(rb64_build(array(select g * 4294967296 from generate_series(0, 99) g)), 42949672961, 42949672962)::bytea = rb64_bulk_ref('{}');
drop function rb64_bulk_ref(bigint[]);
