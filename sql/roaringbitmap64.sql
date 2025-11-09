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

-- Exception
select roaringbitmap64('\x11'::bytea);
select '\x11'::bytea::roaringbitmap64;

-- Test Opperator

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

select rb64_fill(NULL,0,0);
select rb64_fill('{}',0,0);
select rb64_fill('{}',0,1);
select rb64_fill('{}',0,2);
select rb64_fill('{1,10,100}',10,10);
select rb64_fill('{1,10,100}',10,11);
select rb64_fill('{1,10,100}',10,12);
select rb64_fill('{1,10,100}',10,13);
select rb64_fill('{1,10,100}',10,20);
select rb64_fill('{1,10,100}',0,0);
select rb64_fill('{1,10,100,9223372036854775807,-9223372036854775808,-1}',9223372036854775800,9223372036854775807);
select rb64_cardinality(rb64_fill('{1,10,100}',2,1000000000));
select rb64_cardinality(rb64_fill('{1,10,100}',0,5000000000));
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
select rb64_flip('{1,10,100}',0,0);
select rb64_flip('{1,10,100}',9,9);
select rb64_flip('{1,10,100,9223372036854775807,-9223372036854775808,-1}',9223372036854775800,9223372036854775807);
select rb64_cardinality(rb64_flip('{1,10,100}',2,1000000000));
select rb64_cardinality(rb64_flip('{1,10,100}',-1,5000000000));
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



