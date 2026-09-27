--
--  Test roaringbitmap extension
--

set client_min_messages = 'warning';
CREATE EXTENSION if not exists roaringbitmap;

-- Test input and output

set roaringbitmap.output_format='array';
set extra_float_digits = 0;

select  '{}'::roaringbitmap;
select  '  { 	 }  '::roaringbitmap;
select  '{	1 }'::roaringbitmap;
select  '{-1,2,555555,-4}'::roaringbitmap;
select  '{ -1 ,  2  , 555555 ,  -4  }'::roaringbitmap;
select  '{ 1 ,  -2  , 555555 ,  -4  }'::roaringbitmap;
select  '{ 1 ,  -2  , 555555 ,  -4  ,2147483647,-2147483648}'::roaringbitmap;
select  roaringbitmap('{ 1 ,  -2  , 555555 ,  -4  }');

set roaringbitmap.output_format='bytea';
select  '{}'::roaringbitmap;
select  '{ -1 ,  2  , 555555 ,  -4  }'::roaringbitmap;

set roaringbitmap.output_format='array';
select  '{}'::roaringbitmap;
select '\x3a30000000000000'::roaringbitmap;
select '\x3a300000030000000000000008000000ffff01002000000022000000240000000200237afcffffff'::roaringbitmap;

-- Exception
select  ''::roaringbitmap;
select  '{'::roaringbitmap;
select  '{1'::roaringbitmap;
select  '{1} x'::roaringbitmap;
select  '{1x}'::roaringbitmap;
select  '{-x}'::roaringbitmap;
select  '{,}'::roaringbitmap;
select  '{1,}'::roaringbitmap;
select  '{1,xxx}'::roaringbitmap;
select  '{1,3'::roaringbitmap;
select  '{1,1'::roaringbitmap;
select  '{1,-2147483649}'::roaringbitmap;
select  '{2147483648}'::roaringbitmap;

-- Test Type cast
select '{}'::roaringbitmap::bytea;
select '{1}'::roaringbitmap::bytea;
select '{1,9999}'::roaringbitmap::bytea;
select '{}'::roaringbitmap::bytea::roaringbitmap;
select '{1}'::roaringbitmap::bytea::roaringbitmap;
select '{1,9999,-88888}'::roaringbitmap::bytea::roaringbitmap;
select roaringbitmap('{1,9999,-88888}'::roaringbitmap::bytea);

-- Exception
select roaringbitmap('\x11'::bytea);
select '\x11'::bytea::roaringbitmap;

-- Test Operator

select roaringbitmap('{}') & roaringbitmap('{}');
select roaringbitmap('{}') & roaringbitmap('{3,4,5}');
select roaringbitmap('{1,2,3}') & roaringbitmap('{}');
select roaringbitmap('{1,2,3}') & roaringbitmap('{3,4,5}');
select roaringbitmap('{1,-2,-3}') & roaringbitmap('{-3,-4,5}');

select roaringbitmap('{}') | roaringbitmap('{}');
select roaringbitmap('{}') | roaringbitmap('{3,4,5}');
select roaringbitmap('{1,2,3}') | roaringbitmap('{}');
select roaringbitmap('{1,2,3}') | roaringbitmap('{3,4,5}');
select roaringbitmap('{1,-2,-3}') | roaringbitmap('{-3,-4,5}');

select roaringbitmap('{}') | 6;
select roaringbitmap('{1,2,3}') | 6;
select roaringbitmap('{1,2,3}') | 1;
select roaringbitmap('{1,2,3}') | -1;
select roaringbitmap('{-1,-2,3}') | -1;

select 6 | roaringbitmap('{}');
select 6 | roaringbitmap('{1,2,3}');
select 1 | roaringbitmap('{1,2,3}');
select -1 | roaringbitmap('{1,2,3}');
select -1 | roaringbitmap('{-1,-2,3}');

select roaringbitmap('{}') # roaringbitmap('{}');
select roaringbitmap('{}') # roaringbitmap('{3,4,5}');
select roaringbitmap('{1,2,3}') # roaringbitmap('{}');
select roaringbitmap('{1,2,3}') # roaringbitmap('{3,4,5}');
select roaringbitmap('{1,-2,-3}') # roaringbitmap('{-3,-4,5}');

select roaringbitmap('{}') - roaringbitmap('{}');
select roaringbitmap('{}') - roaringbitmap('{3,4,5}');
select roaringbitmap('{1,2,3}') - roaringbitmap('{}');
select roaringbitmap('{1,2,3}') - roaringbitmap('{3,4,5}');
select roaringbitmap('{1,-2,-3}') - roaringbitmap('{-3,-4,5}');

select roaringbitmap('{}') - 3;
select roaringbitmap('{1,2,3}') - 3;
select roaringbitmap('{1,2,3}') - 1;
select roaringbitmap('{1,2,3}') - -1;
select roaringbitmap('{-1,-2,3}') - -1;

select roaringbitmap('{}') << 2;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') << 2;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') << 1;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') << 0;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') << -1;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') << -2;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') << 4294967295;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') << 4294967296;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') << -4294967295;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') << -4294967296;

select roaringbitmap('{}') >> 2;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') >> 2;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') >> 1;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') >> 0;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') >> -1;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') >> -2;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') >> 4294967295;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') >> 4294967296;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') >> -4294967295;
select roaringbitmap('{-2,-1,0,1,2,3,2147483647,-2147483648}') >> -4294967296;

select roaringbitmap('{}') @> roaringbitmap('{}');
select roaringbitmap('{}') @> roaringbitmap('{3,4,5}');
select roaringbitmap('{1,2,3}') @> roaringbitmap('{}');
select roaringbitmap('{1,2,3}') @> roaringbitmap('{3,4,5}');
select roaringbitmap('{1,2,3}') @> roaringbitmap('{3,2}');
select roaringbitmap('{1,-2,-3}')  @> roaringbitmap('{-3,1}');

select roaringbitmap('{}') @> 2;
select roaringbitmap('{1,2,3}') @> 20;
select roaringbitmap('{1,2,3}') @> 1;
select roaringbitmap('{1,2,3}') @> -1;
select roaringbitmap('{-1,-2,3}') @> -1;

select roaringbitmap('{}') <@ roaringbitmap('{}');
select roaringbitmap('{}') <@ roaringbitmap('{3,4,5}');
select roaringbitmap('{1,2,3}') <@ roaringbitmap('{}');
select roaringbitmap('{1,2,3}') <@ roaringbitmap('{3,4,5}');
select roaringbitmap('{2,3}') <@ roaringbitmap('{1,3,2}');
select roaringbitmap('{1,-3}')  <@ roaringbitmap('{-3,1,1000}');

select 6 <@ roaringbitmap('{}');
select 3 <@ roaringbitmap('{1,2,3}');
select 1 <@ roaringbitmap('{1,2,3}');
select -1 <@ roaringbitmap('{1,2,3}');
select -1 <@ roaringbitmap('{-1,-2,3}');

select roaringbitmap('{}') && roaringbitmap('{}');
select roaringbitmap('{}') && roaringbitmap('{3,4,5}');
select roaringbitmap('{1,2,3}') && roaringbitmap('{}');
select roaringbitmap('{1,2,3}') && roaringbitmap('{3,4,5}');
select roaringbitmap('{1,-2,-3}') && roaringbitmap('{-3,-4,5}');

select roaringbitmap('{}') = roaringbitmap('{}');
select roaringbitmap('{}') = roaringbitmap('{3,4,5}');
select roaringbitmap('{1,2,3}') = roaringbitmap('{}');
select roaringbitmap('{1,2,3}') = roaringbitmap('{3,1,2}');
select roaringbitmap('{1,-2,-3}') = roaringbitmap('{-3,-4,5}');

select roaringbitmap('{}') <> roaringbitmap('{}');
select roaringbitmap('{}') <> roaringbitmap('{3,4,5}');
select roaringbitmap('{1,2,3}') <> roaringbitmap('{}');
select roaringbitmap('{1,2,3}') <> roaringbitmap('{3,1,2}');
select roaringbitmap('{1,-2,-3}') <> roaringbitmap('{-3,-4,5}');

-- Test the functions with one bitmap variable

select rb_build(NULL);
select rb_build('{}'::int[]);
select rb_build('{1}'::int[]);
select rb_build('{-1,2,555555,-4}'::int[]);
select rb_build('{1,-2,555555,-4,2147483647,-2147483648}'::int[]);

select rb_to_array(NULL);
select rb_to_array('{}'::roaringbitmap);
select rb_to_array('{1}'::roaringbitmap);
select rb_to_array('{-1,2,555555,-4}'::roaringbitmap);
select rb_to_array('{1,-2,555555,-4,2147483647,-2147483648}'::roaringbitmap);

select rb_is_empty(NULL);
select rb_is_empty('{}');
select rb_is_empty('{1}');
select rb_is_empty('{1,10,100}');

select rb_cardinality(NULL);
select rb_cardinality('{}');
select rb_cardinality('{1}');
select rb_cardinality('{1,10,100}');

select rb_max(NULL);
select rb_max('{}');
select rb_max('{1}');
select rb_max('{1,10,100}');
select rb_max('{1,10,100,2147483647,-2147483648,-1}');

select rb_min(NULL);
select rb_min('{}');
select rb_min('{1}');
select rb_min('{1,10,100}');
select rb_min('{1,10,100,2147483647,-2147483648,-1}');

select rb_iterate(NULL);
select rb_iterate('{}');
select rb_iterate('{1}');
select rb_iterate('{1,10,100}');
select rb_iterate('{1,10,100,2147483647,-2147483648,-1}');

select rb_runoptimize(NULL);
select rb_runoptimize('{}');
select rb_runoptimize('{1}');
select rb_runoptimize('{1,10,100}');
select rb_runoptimize('{1,10,100,2147483647,-2147483648,-1}');

-- Test the functions with two bitmap variables

select rb_and(NULL,'{1,10,100}');
select rb_and('{1,10,100}',NULL);
select rb_and('{}','{1,10,100}');
select rb_and('{1,10,100}','{}');
select rb_and('{2}','{1,10,100}');
select rb_and('{1,2,10}','{1,10,100}');
select rb_and('{1,10}','{1,10,100}');

select rb_and_cardinality(NULL,'{1,10,100}');
select rb_and_cardinality('{1,10,100}',NULL);
select rb_and_cardinality('{}','{1,10,100}');
select rb_and_cardinality('{1,10,100}','{}');
select rb_and_cardinality('{2}','{1,10,100}');
select rb_and_cardinality('{1,2,10}','{1,10,100}');
select rb_and_cardinality('{1,10}','{1,10,100}');

select rb_or(NULL,'{1,10,100}');
select rb_or('{1,10,100}',NULL);
select rb_or('{}','{1,10,100}');
select rb_or('{1,10,100}','{}');
select rb_or('{2}','{1,10,100}');
select rb_or('{1,2,10}','{1,10,100}');
select rb_or('{1,10}','{1,10,100}');

select rb_or_cardinality(NULL,'{1,10,100}');
select rb_or_cardinality('{1,10,100}',NULL);
select rb_or_cardinality('{}','{1,10,100}');
select rb_or_cardinality('{1,10,100}','{}');
select rb_or_cardinality('{2}','{1,10,100}');
select rb_or_cardinality('{1,2,10}','{1,10,100}');
select rb_or_cardinality('{1,10}','{1,10,100}');

select rb_xor(NULL,'{1,10,100}');
select rb_xor('{1,10,100}',NULL);
select rb_xor('{}','{1,10,100}');
select rb_xor('{1,10,100}','{}');
select rb_xor('{2}','{1,10,100}');
select rb_xor('{1,2,10}','{1,10,100}');
select rb_xor('{1,10}','{1,10,100}');

select rb_xor_cardinality(NULL,'{1,10,100}');
select rb_xor_cardinality('{1,10,100}',NULL);
select rb_xor_cardinality('{}','{1,10,100}');
select rb_xor_cardinality('{1,10,100}','{}');
select rb_xor_cardinality('{2}','{1,10,100}');
select rb_xor_cardinality('{1,2,10}','{1,10,100}');
select rb_xor_cardinality('{1,10}','{1,10,100}');

select rb_equals(NULL,'{1,10,100}');
select rb_equals('{1,10,100}',NULL);
select rb_equals('{}','{1,10,100}');
select rb_equals('{1,10,100}','{}');
select rb_equals('{2}','{1,10,100}');
select rb_equals('{1,2,10}','{1,10,100}');
select rb_equals('{1,10}','{1,10,100}');
select rb_equals('{1,10,100}','{1,10,100}');
select rb_equals('{1,10,100,10}','{1,100,10}');

select rb_intersect(NULL,'{1,10,100}');
select rb_intersect('{1,10,100}',NULL);
select rb_intersect('{}','{1,10,100}');
select rb_intersect('{1,10,100}','{}');
select rb_intersect('{2}','{1,10,100}');
select rb_intersect('{1,2,10}','{1,10,100}');
select rb_intersect('{1,10}','{1,10,100}');
select rb_intersect('{1,10,100}','{1,10,100}');
select rb_intersect('{1,10,100,10}','{1,100,10}');

select rb_andnot(NULL,'{1,10,100}');
select rb_andnot('{1,10,100}',NULL);
select rb_andnot('{}','{1,10,100}');
select rb_andnot('{1,10,100}','{}');
select rb_andnot('{2}','{1,10,100}');
select rb_andnot('{1,2,10}','{1,10,100}');
select rb_andnot('{1,10}','{1,10,100}');
select rb_andnot('{1,10,100}','{1,10,100}');
select rb_andnot('{1,10,100,10}','{1,100,10}');

select rb_andnot_cardinality(NULL,'{1,10,100}');
select rb_andnot_cardinality('{1,10,100}',NULL);
select rb_andnot_cardinality('{}','{1,10,100}');
select rb_andnot_cardinality('{1,10,100}','{}');
select rb_andnot_cardinality('{2}','{1,10,100}');
select rb_andnot_cardinality('{1,2,10}','{1,10,100}');
select rb_andnot_cardinality('{1,10}','{1,10,100}');
select rb_andnot_cardinality('{1,10,100}','{1,10,100}');
select rb_andnot_cardinality('{1,10,100,10}','{1,100,10}');

select rb_jaccard_dist(NULL,'{1,10,100}');
select rb_jaccard_dist('{1,10,100}',NULL);
select rb_jaccard_dist('{}','{1,10,100}');
select rb_jaccard_dist('{1,10,100}','{}');
select rb_jaccard_dist('{2}','{1,10,100}');
select rb_jaccard_dist('{1,2,10}','{1,10,100}');
select rb_jaccard_dist('{1,10,11,12}','{1,10,100}');
select rb_jaccard_dist('{1,10,100}','{1,10,11,12}');
select rb_jaccard_dist('{1,10,100}','{1,10,100}');
select rb_jaccard_dist('{1,10,-100}','{1,10,-100}');
select rb_jaccard_dist('{1,10,100}','{1,10,-100}');

-- Test other functions

select rb_rank(NULL,0);
select rb_rank('{}',0);
select rb_rank('{1,10,100}',0);
select rb_rank('{1,10,100}',1);
select rb_rank('{1,10,100}',99);
select rb_rank('{1,10,100}',100);
select rb_rank('{1,10,100}',101);
select rb_rank('{1,10,100,-3,-1}',-2);

select rb_remove(NULL,0);
select rb_remove('{}',0);
select rb_remove('{1}',1);
select rb_remove('{1,10,100}',0);
select rb_remove('{1,10,100}',1);
select rb_remove('{1,10,100}',99);

select rb_fill(NULL,0,0);
select rb_fill('{}',0,0);
select rb_fill('{}',0,1);
select rb_fill('{}',0,2);
select rb_fill('{1,10,100}',10,10);
select rb_fill('{1,10,100}',10,11);
select rb_fill('{1,10,100}',10,12);
select rb_fill('{1,10,100}',10,13);
select rb_fill('{1,10,100}',10,20);
select rb_fill('{1,10,100}',0,-1);
select rb_cardinality(rb_fill('{1,10,100}',2,1000000000));
select rb_cardinality(rb_fill('{1,10,100}',-1,5000000000));

select rb_index(NULL,3);
select rb_index('{1,2,3}',NULL);
select rb_index('{}',3);
select rb_index('{1}',3);
select rb_index('{1}',1);
select rb_index('{1,10,100}',10);
select rb_index('{1,10,100}',99);
select rb_index('{1,10,-100}',-100);

select rb_clear(NULL,0,10);
select rb_clear('{}',0,10);
select rb_clear('{1,10,100}',0,10);
select rb_clear('{1,10,100}',3,3);
select rb_clear('{1,10,100}',-3,3);
select rb_clear('{1,10,100}',0,-1);
select rb_clear('{1,10,100}',9,9);
select rb_clear('{1,10,100}',2,1000000000);
select rb_clear('{0,1,10,100,-2,-1}',1,4294967295);
select rb_clear('{0,1,10,100,-2,-1}',0,4294967296);

select rb_flip(NULL,0,10);
select rb_flip('{}',0,10);
select rb_flip('{1,10,100}',9,100);
select rb_flip('{1,10,100}',10,101);
select rb_flip('{1,10,100}',-3,3);
select rb_flip('{1,10,100}',0,-1);
select rb_flip('{1,10,100}',9,9);
select rb_cardinality(rb_flip('{1,10,100}',2,1000000000));
select rb_cardinality(rb_flip('{1,10,100}',-1,5000000000));

select rb_range(NULL,0,10);
select rb_range('{}',0,10);
select rb_range('{1,10,100}',0,10);
select rb_range('{1,10,100}',3,3);
select rb_range('{1,10,100}',-3,3);
select rb_range('{1,10,100}',0,-1);
select rb_range('{1,10,100}',9,9);
select rb_range('{1,10,100}',2,1000000000);
select rb_range('{0,1,10,100,-2,-1}',1,4294967295);
select rb_range('{0,1,10,100,-2,-1}',0,4294967296);

select rb_range_cardinality(NULL,0,10);
select rb_range_cardinality('{}',0,10);
select rb_range_cardinality('{1,10,100}',0,10);
select rb_range_cardinality('{1,10,100}',3,3);
select rb_range_cardinality('{1,10,100}',-3,3);
select rb_range_cardinality('{1,10,100}',0,-1);
select rb_range_cardinality('{1,10,100}',9,9);
select rb_range_cardinality('{1,10,100}',2,1000000000);
select rb_range_cardinality('{0,1,10,100,-2,-1}',1,4294967295);
select rb_range_cardinality('{0,1,10,100,-2,-1}',0,4294967296);

select rb_select(NULL,10);
select rb_select('{}',10);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',0);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',1);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',2);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',2,0);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',2,1);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',2,1,true);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',2,1,true,9);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',2,1,true,9,4294967295);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',2,1,false);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',2,1,false,9);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',2,1,false,10);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',2,1,false,10,10);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',2,1,false,-10,100);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',2,1,false,-10,-10);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',2,1,false,10,10001);
select rb_select('{0,1,2,10,100,1000,2147483647,-2147483648,-2,-1}',2,1,true,10,10001);


-- Test aggregate

select rb_and_agg(id) from (values (NULL::roaringbitmap)) t(id);
select rb_and_agg(id) from (values (roaringbitmap('{}'))) t(id);
select rb_and_agg(id) from (values (roaringbitmap('{1}'))) t(id);
select rb_and_agg(id) from (values (roaringbitmap('{1,10,100}'))) t(id);
select rb_and_agg(id) from (values (roaringbitmap('{1,10,100}')),(roaringbitmap('{2,10}'))) t(id);
select rb_and_agg(id) from (values (roaringbitmap('{1,10,100}')),(roaringbitmap('{1,10,100}'))) t(id);
select rb_and_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{}'))) t(id);
select rb_and_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{1}'))) t(id);
select rb_and_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{2}'))) t(id);
select rb_and_agg(id) from (values (roaringbitmap('{1,10,100}')),(NULL),(roaringbitmap('{2,10}'))) t(id);
select rb_and_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(NULL),(roaringbitmap('{1,10,100,101}')),(NULL)) t(id);

select rb_and_cardinality_agg(id) from (values (NULL::roaringbitmap)) t(id);
select rb_and_cardinality_agg(id) from (values (roaringbitmap('{}'))) t(id);
select rb_and_cardinality_agg(id) from (values (roaringbitmap('{1}'))) t(id);
select rb_and_cardinality_agg(id) from (values (roaringbitmap('{1,10,100}'))) t(id);
select rb_and_cardinality_agg(id) from (values (roaringbitmap('{1,10,100}')),(roaringbitmap('{2,10}'))) t(id);
select rb_and_cardinality_agg(id) from (values (roaringbitmap('{1,10,100}')),(roaringbitmap('{1,10,100}'))) t(id);
select rb_and_cardinality_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{}'))) t(id);
select rb_and_cardinality_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{1}'))) t(id);
select rb_and_cardinality_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{2}'))) t(id);
select rb_and_cardinality_agg(id) from (values (roaringbitmap('{1,10,100}')),(NULL),(roaringbitmap('{2,10}'))) t(id);
select rb_and_cardinality_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(NULL),(roaringbitmap('{1,10,100,101}')),(NULL)) t(id);

select rb_or_agg(id) from (values (NULL::roaringbitmap)) t(id);
select rb_or_agg(id) from (values (roaringbitmap('{}'))) t(id);
select rb_or_agg(id) from (values (roaringbitmap('{1}'))) t(id);
select rb_or_agg(id) from (values (roaringbitmap('{1,10,100}'))) t(id);
select rb_or_agg(id) from (values (roaringbitmap('{1,10,100}')),(roaringbitmap('{2,10}'))) t(id);
select rb_or_agg(id) from (values (roaringbitmap('{1,10,100}')),(roaringbitmap('{1,10,100}'))) t(id);
select rb_or_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{}'))) t(id);
select rb_or_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{1}'))) t(id);
select rb_or_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{2}'))) t(id);
select rb_or_agg(id) from (values (roaringbitmap('{1,10,100}')),(NULL),(roaringbitmap('{2,10}'))) t(id);
select rb_or_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(NULL),(roaringbitmap('{1,10,100,101}')),(NULL)) t(id);

select rb_or_cardinality_agg(id) from (values (NULL::roaringbitmap)) t(id);
select rb_or_cardinality_agg(id) from (values (roaringbitmap('{}'))) t(id);
select rb_or_cardinality_agg(id) from (values (roaringbitmap('{1}'))) t(id);
select rb_or_cardinality_agg(id) from (values (roaringbitmap('{1,10,100}'))) t(id);
select rb_or_cardinality_agg(id) from (values (roaringbitmap('{1,10,100}')),(roaringbitmap('{2,10}'))) t(id);
select rb_or_cardinality_agg(id) from (values (roaringbitmap('{1,10,100}')),(roaringbitmap('{1,10,100}'))) t(id);
select rb_or_cardinality_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{}'))) t(id);
select rb_or_cardinality_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{1}'))) t(id);
select rb_or_cardinality_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{2}'))) t(id);
select rb_or_cardinality_agg(id) from (values (roaringbitmap('{1,10,100}')),(NULL),(roaringbitmap('{2,10}'))) t(id);
select rb_or_cardinality_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(NULL),(roaringbitmap('{1,10,100,101}')),(NULL)) t(id);

select rb_xor_agg(id) from (values (NULL::roaringbitmap)) t(id);
select rb_xor_agg(id) from (values (roaringbitmap('{}'))) t(id);
select rb_xor_agg(id) from (values (roaringbitmap('{1}'))) t(id);
select rb_xor_agg(id) from (values (roaringbitmap('{1,10,100}'))) t(id);
select rb_xor_agg(id) from (values (roaringbitmap('{1,10,100}')),(roaringbitmap('{2,10}'))) t(id);
select rb_xor_agg(id) from (values (roaringbitmap('{1,10,100}')),(roaringbitmap('{1,10,100}'))) t(id);
select rb_xor_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{}'))) t(id);
select rb_xor_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{1}'))) t(id);
select rb_xor_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{2}'))) t(id);
select rb_xor_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{2,10}'))) t(id);
select rb_xor_agg(id) from (values (roaringbitmap('{1,10,100}')),(NULL),(roaringbitmap('{1,10,100,101}'))) t(id);
select rb_xor_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(NULL),(roaringbitmap('{1,10,101}')),(roaringbitmap('{1,100,102}')),(NULL)) t(id);

select rb_xor_cardinality_agg(id) from (values (NULL::roaringbitmap)) t(id);
select rb_xor_cardinality_agg(id) from (values (roaringbitmap('{}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (roaringbitmap('{1}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (roaringbitmap('{1,10,100}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (roaringbitmap('{1,10,100}')),(roaringbitmap('{2,10}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (roaringbitmap('{1,10,100}')),(roaringbitmap('{1,10,100}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{1}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{2}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(roaringbitmap('{2,10}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (roaringbitmap('{1,10,100}')),(NULL),(roaringbitmap('{1,10,100,101}'))) t(id);
select rb_xor_cardinality_agg(id) from (values (NULL),(roaringbitmap('{1,10,100}')),(NULL),(roaringbitmap('{1,10,101}')),(roaringbitmap('{1,100,102}')),(NULL)) t(id);

select rb_build_agg(id) from (values (NULL::int)) t(id);
select rb_build_agg(id) from (values (1)) t(id);
select rb_build_agg(id) from (values (1),(10)) t(id);
select rb_build_agg(id) from (values (1),(10),(10),(100),(1)) t(id);


-- Test Windows aggregate

with t(id,bitmap) as(
 values(0,NULL),(1,roaringbitmap('{1,10}')),(2,NULL),(3,roaringbitmap('{2,10}')),(4,roaringbitmap('{10,100}'))
)
select id,bitmap,rb_and_agg(bitmap) over(order by id),rb_and_cardinality_agg(bitmap) over(order by id) from t;

with t(id,bitmap) as(
 values(0,NULL),(1,roaringbitmap('{1,10}')),(2,NULL),(3,roaringbitmap('{2,10}')),(4,roaringbitmap('{10,100}'))
)
select id,bitmap,rb_or_agg(bitmap) over(order by id),rb_or_cardinality_agg(bitmap) over(order by id) from t;

with t(id,bitmap) as(
 values(0,NULL),(1,roaringbitmap('{1,10}')),(2,NULL),(3,roaringbitmap('{2,10}')),(4,roaringbitmap('{10,100}'))
)
select id,bitmap,rb_xor_agg(bitmap) over(order by id),rb_xor_cardinality_agg(bitmap) over(order by id) from t;

with t(id) as(
 values(0),(1),(2),(NULL),(4),(NULL)
)
select id,rb_build_agg(id) over(order by id) from t;


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


create table if not exists bitmap_test_tb1(id int, bitmap roaringbitmap);
truncate bitmap_test_tb1;
insert into bitmap_test_tb1 values (NULL,NULL);
insert into bitmap_test_tb1 select id,rb_build(ARRAY[id]) from generate_series(1,10000)id;
insert into bitmap_test_tb1 values (NULL,NULL);
insert into bitmap_test_tb1 values (10001,rb_build(ARRAY[10,100,1000,10000,10001]));

select position('"Parallel Aware": true' in get_json_plan('
select rb_cardinality(bitmap),rb_min(bitmap),rb_max(bitmap)
  from (select rb_build_agg(id) bitmap from bitmap_test_tb1)a
')::text) > 0 is_parallel_plan;

select rb_cardinality(bitmap),rb_min(bitmap),rb_max(bitmap)
  from (select rb_build_agg(id) bitmap from bitmap_test_tb1)a;

select position('"Parallel Aware": true' in get_json_plan('
select rb_cardinality(bitmap),rb_min(bitmap),rb_max(bitmap)
  from (select rb_and_agg(bitmap) bitmap from bitmap_test_tb1)a
')::text) > 0 is_parallel_plan;

select rb_cardinality(bitmap),rb_min(bitmap),rb_max(bitmap)
  from (select rb_and_agg(bitmap) bitmap from bitmap_test_tb1)a;

select position('"Parallel Aware": true' in get_json_plan('
select rb_cardinality(bitmap),rb_min(bitmap),rb_max(bitmap)
  from (select rb_or_agg(bitmap) bitmap from bitmap_test_tb1)a
')::text) > 0 is_parallel_plan;

select rb_cardinality(bitmap),rb_min(bitmap),rb_max(bitmap)
  from (select rb_or_agg(bitmap) bitmap from bitmap_test_tb1)a;

select position('"Parallel Aware": true' in get_json_plan('
select rb_cardinality(bitmap),rb_min(bitmap),rb_max(bitmap)
  from (select rb_xor_agg(bitmap) bitmap from bitmap_test_tb1)a
')::text) > 0 is_parallel_plan;

select rb_cardinality(bitmap),rb_min(bitmap),rb_max(bitmap)
  from (select rb_xor_agg(bitmap) bitmap from bitmap_test_tb1)a;

select position('"Parallel Aware": true' in get_json_plan('
select rb_and_cardinality_agg(bitmap),rb_or_cardinality_agg(bitmap),rb_xor_cardinality_agg(bitmap) from bitmap_test_tb1
')::text) > 0 is_parallel_plan;

select rb_and_cardinality_agg(bitmap),rb_or_cardinality_agg(bitmap),rb_xor_cardinality_agg(bitmap) from bitmap_test_tb1;

--rb_iterate() not support parallel on PG10 while run on parallel in PG11+
--explain(costs off) 
--select count(*) from (select rb_iterate(bitmap) from bitmap_test_tb1)a;

select count(*) from (select rb_iterate(bitmap) from bitmap_test_tb1)a;

select position('"Parallel Aware": true' in get_json_plan('
select id,bitmap,
  rb_build_agg(id) over(w),
  rb_or_agg(bitmap) over(w),
  rb_and_agg(bitmap) over(w),
  rb_xor_agg(bitmap) over(w) 
from bitmap_test_tb1
window w as (order by id)
order by id limit 10
')::text) > 0 is_parallel_plan;

select id,bitmap,
  rb_build_agg(id) over(w),
  rb_or_agg(bitmap) over(w),
  rb_and_agg(bitmap) over(w),
  rb_xor_agg(bitmap) over(w) 
from bitmap_test_tb1
window w as (order by id)
order by id limit 10;

-- bugfix #22
SELECT '{100373,1829130,1861002,1975442,2353213,2456403}'::roaringbitmap & '{2353213}'::roaringbitmap;
SELECT rb_and_cardinality('{100373,1829130,1861002,1975442,2353213,2456403}','{2353213}');

-- bugfix #45: rb_to_array and rb_min exhibits inconsistent results in a malformed serialize input
select rb_to_array('\x3a300000010000000000000000000000000000000000000000000000000000000000000000000000000000000008'::roaringbitmap),
rb_min('\x3a300000010000000000000000000000000000000000000000000000000000000000000000000000000000000008'::roaringbitmap);


-- ============================================================
-- rb_group_elements_by_source tests
-- ============================================================

-- Basic: 3 bitmaps with partial overlaps
select sources, rb_to_array(members) from rb_group_elements_by_source(ARRAY[
  rb_build(ARRAY[1,3,5]),
  rb_build(ARRAY[3,4]),
  rb_build(ARRAY[10,5])
]) order by sources::text;

-- Grouping: many elements share the same source-set
select sources, rb_to_array(members) from rb_group_elements_by_source(ARRAY[
  rb_build(ARRAY[1,2,3,4,5]),
  rb_build(ARRAY[1,2,3])
]) order by sources::text;

-- 3 bitmaps with overlapping input sets
select sources, rb_to_array(members) from rb_group_elements_by_source(ARRAY[
  rb_build(ARRAY[1,2,3,4,5,6,7,8,9,10]),
  rb_build(ARRAY[1,2,3,4,5,11,12]),
  rb_build(ARRAY[1,2,3,13,14,15])
]) order by sources::text;

-- All elements in all bitmaps -> single group
select sources, rb_to_array(members) from rb_group_elements_by_source(ARRAY[
  rb_build(ARRAY[1,2,3]),
  rb_build(ARRAY[1,2,3]),
  rb_build(ARRAY[1,2,3])
]) order by sources::text;

-- Empty array -> no rows
select sources, rb_to_array(members) from rb_group_elements_by_source(ARRAY[]::roaringbitmap[]);

-- All NULL bitmaps -> no rows
select sources, rb_to_array(members) from rb_group_elements_by_source(ARRAY[NULL,NULL,NULL]::roaringbitmap[]);

-- NULL argument -> no rows
select sources, rb_to_array(members) from rb_group_elements_by_source(NULL::roaringbitmap[]);

-- Some NULL bitmaps (skipped, but preserve source indices)
select sources, rb_to_array(members) from rb_group_elements_by_source(ARRAY[
  rb_build(ARRAY[1,3]),
  NULL,
  rb_build(ARRAY[3,5])
]) order by sources::text;

-- Empty bitmaps (no bits set)
select sources, rb_to_array(members) from rb_group_elements_by_source(ARRAY[
  rb_build(ARRAY[]::integer[]),
  rb_build(ARRAY[1,2]),
  rb_build(ARRAY[]::integer[])
]) order by sources::text;

-- Single bitmap
select sources, rb_to_array(members) from rb_group_elements_by_source(ARRAY[
  rb_build(ARRAY[1,2,3])
]) order by sources::text;

-- N > 64: exercise variable-width bitmask (65 inputs, requires 2 uint64 words)
-- Build 65 bitmaps where element 1 is in all, element 2 only in bitmap 1, element 3 only in bitmap 65
select sources, rb_to_array(members) from rb_group_elements_by_source(
  (select array_agg(
    case
      when i = 1 then rb_build(ARRAY[1,2])
      when i = 65 then rb_build(ARRAY[1,3])
      else rb_build(ARRAY[1])
    end order by i
  ) from generate_series(1, 65) i)
) order by sources::text;

-- N = 100: larger than 64, element 99 only in bitmaps 1 and 100
select sources, rb_to_array(members) from rb_group_elements_by_source(
  (select array_agg(
    case
      when i = 1 then rb_build(ARRAY[42, 99])
      when i = 100 then rb_build(ARRAY[42, 99])
      else rb_build(ARRAY[42])
    end order by i
  ) from generate_series(1, 100) i)
) order by sources::text;

-- N = 128: exactly 2 words boundary, all bitmaps contain element 7
select count(*), (select count(distinct sources::text) from rb_group_elements_by_source(
  (select array_agg(rb_build(ARRAY[7]) order by i) from generate_series(1, 128) i)
)) as n_groups from generate_series(1,1);

-- N = 65, disjoint: each bitmap has a unique element -> 65 groups
select count(*) from rb_group_elements_by_source(
  (select array_agg(rb_build(ARRAY[i]) order by i) from generate_series(1, 65) i)
);


-- ============================================================
-- opclass support: btree / hash / gin + selectivity
-- ============================================================

-- rb_cmp: lexicographic total order over the ascending element sequences.
-- Elements are compared as unsigned uint32.
select rb_cmp(NULL, '{}');
select rb_cmp('{}', NULL);
select rb_cmp('{}', '{}');
select rb_cmp('{}', '{1}');
select rb_cmp('{1}', '{}');
select rb_cmp('{1,2}', '{1,3}');
select rb_cmp('{1,3}', '{2}');
select rb_cmp('{1,3,9}', '{2}');
select rb_cmp('{1,3,9}', '{1,3,9,10}');
select rb_cmp('{1}', '{1}');
select rb_cmp('{1,2}', '{1,2}');
select rb_cmp('{1,2}', '{1}');
select rb_cmp('{1,2}', '{0}');
select rb_cmp('{-1}', '{1}');
select rb_cmp('{-2147483648}', '{2147483647}');
select rb_cmp('{0,-2147483648}', '{0,2}');
select rb_cmp('{0,-2147483648}', '{0,-1}');

-- rb_cmp == 0 iff rb_equals, independent of run optimization
select rb_cmp(rb_build('{1,2,3}'), rb_runoptimize(rb_build('{1,2,3}')));

-- comparison operators '<' '<=' '=' '>=' '>' (backed by rb_lt / rb_le /
-- rb_equals / rb_ge / rb_gt, all defined through rb_cmp.
select l, r,
       l <  r as lt, l <= r as le, l =  r as eq, l >= r as ge, l >  r as gt
  from (values
    ('{}'::roaringbitmap,            '{}'::roaringbitmap),            -- equal, both empty
    ('{}'::roaringbitmap,            '{1}'::roaringbitmap),           -- empty is smaller
    ('{1}'::roaringbitmap,           '{}'::roaringbitmap),            -- empty is smaller (swapped)
    ('{1,2}'::roaringbitmap,         '{1,2}'::roaringbitmap),         -- equal, non-empty
    ('{1}'::roaringbitmap,           '{1,2}'::roaringbitmap),         -- strict prefix -> smaller
    ('{1,2,3}'::roaringbitmap,       '{1,2}'::roaringbitmap),         -- superset -> greater
    ('{1,3}'::roaringbitmap,         '{2}'::roaringbitmap),           -- same length, first differs
    ('{1,3,9}'::roaringbitmap,       '{2}'::roaringbitmap),           -- 3 vs 1 element, first differs
    ('{-5,-1}'::roaringbitmap,       '{-2}'::roaringbitmap),          -- negatives keep their order
    ('{-5,-3,-1}'::roaringbitmap,    '{-5,-1}'::roaringbitmap),       -- 3 vs 2 elements, -3 < -1 -> smaller
    ('{-2}'::roaringbitmap,          '{-2}'::roaringbitmap),          -- equal negatives
    ('{2147483647}'::roaringbitmap,  '{-2147483648}'::roaringbitmap), -- unsigned: -2147483648 is largest
    ('{-2147483648}'::roaringbitmap, '{-1}'::roaringbitmap),          -- unsigned: -2147483648 < -1
    ('{0,-2147483648}'::roaringbitmap, '{0,2}'::roaringbitmap),       -- unsigned: first difference decides
    ('{0,-2147483648}'::roaringbitmap, '{0,-1}'::roaringbitmap)       -- unsigned: first difference decides
  ) t(l, r);

-- rb_hash / rb_hash_extended: equal sets hash equal, even after runoptimize
select rb_hash('{}') = rb_hash('{}');
select rb_hash('{1,2,3}') = rb_hash('{1,2,3}');
select rb_hash('{1,2,3}') = rb_hash(rb_runoptimize('{1,2,3}'));
select rb_hash('{1,2,3,-1}') = rb_hash('{1,2,3,-1}');
select rb_hash('{1,2,3,-1}') = rb_hash(rb_runoptimize('{1,2,3,-1}'));
select rb_hash_extended('{1,2,3}', 42) = rb_hash_extended('{1,2,3}', 42);
select rb_hash_extended('{1,2,3}', 42) = rb_hash_extended(rb_runoptimize('{1,2,3}'), 42);
select rb_hash_extended('{1,2,3,-1}', 42) = rb_hash_extended('{1,2,3,-1}', 42);
select rb_hash_extended('{1,2,3,-1}', 42) = rb_hash_extended(rb_runoptimize('{1,2,3,-1}'), 42);

-- btree opclass: total order through index
drop table if exists rb_test_opclass;
create table rb_test_opclass (id int, rb roaringbitmap);
insert into rb_test_opclass values
  (1, rb_build('{2}')),
  (2, rb_build('{}')),
  (3, rb_build('{1,2}')),
  (4, rb_build('{1,2,-1}')),
  (5, rb_build('{1}')),
  (6, rb_build('{1,2}')),
  (7, NULL);
create index rb_test_btree_idx on rb_test_opclass using btree (rb);
analyze rb_test_opclass;

set enable_seqscan = off;
set max_parallel_workers_per_gather = 0;

select id,rb from rb_test_opclass order by rb;
select id,rb from rb_test_opclass where rb < rb_build('{1,2}') order by id;
select id,rb from rb_test_opclass where rb >= rb_build('{1}') order by id;
select rb, count(*) from rb_test_opclass group by rb order by rb;
select count(distinct rb) from rb_test_opclass;

-- hash opclass: hash join path returns correct rows
drop table if exists rb_test_hash;
create table rb_test_hash (id int, rb roaringbitmap);
insert into rb_test_hash values
  (1, rb_build('{2}')),
  (2, rb_build('{}')),
  (3, rb_build('{1,2}')),
  (4, rb_build('{1,2,-1}')),
  (5, rb_build('{1}')),
  (6, rb_build('{1,2}')),
  (7, NULL);
create index rb_test_hash_idx on rb_test_hash using hash (rb);
analyze rb_test_hash;

select a.id,a.rb from rb_test_hash a join rb_test_hash b on a.rb = b.rb order by a.id;

-- gin opclass: all five strategies + empty-set boundaries
drop table if exists rb_test_gin;
create table rb_test_gin (id int, rb roaringbitmap);
insert into rb_test_gin values
  (1, rb_build('{}')),
  (2, rb_build('{1}')),
  (3, rb_build('{1,2}')),
  (4, rb_build('{2,3,4}')),
  (5, rb_build('{1,2,3,4,5}')),
  (6, rb_build('{1,2,3,4,5,-1}')),
  (7, rb_build('{1,2,3,4,-1}')),
  (8, NULL);
create index rb_test_gin_idx on rb_test_gin using gin (rb);
analyze rb_test_gin;

select id,rb from rb_test_gin where rb && rb_build('{2}') order by id;
select id,rb from rb_test_gin where rb @> rb_build('{1,2}') order by id;
select id,rb from rb_test_gin where rb <@ rb_build('{1,2}') order by id;
select id,rb from rb_test_gin where rb = rb_build('{1,2}') order by id;
select id,rb from rb_test_gin where rb @> 2 order by id;
select id,rb from rb_test_gin where rb && rb_build('{-1}') order by id;
select id,rb from rb_test_gin where rb @> rb_build('{1,-1}') order by id;
select id,rb from rb_test_gin where rb <@ rb_build('{1,-1}') order by id;
select id,rb from rb_test_gin where rb = rb_build('{1,-1}') order by id;
select id,rb from rb_test_gin where rb @> -1 order by id;
-- empty-set boundaries
select id,rb from rb_test_gin where rb && rb_build('{}') order by id;  -- none
select id,rb from rb_test_gin where rb @> rb_build('{}') order by id;  -- all non-NULL
select id,rb from rb_test_gin where rb <@ rb_build('{}') order by id;  -- only the empty row
select id,rb from rb_test_gin where rb = rb_build('{}') order by id;   -- only the empty row

-- btree: ORDER BY becomes an ordered index scan
select position('Index Scan' in get_json_plan(
  'select id from rb_test_opclass order by rb'
)::text) > 0 as btree_order_by;

-- btree: all five comparison operators are indexable
select position('rb_test_btree_idx' in p) > 0 and position('Index Cond": "(rb < ' in p) > 0 as btree_lt
  from (select get_json_plan('select id from rb_test_opclass where rb <  rb_build(''{1,2}'')')::text as p) s;
select position('rb_test_btree_idx' in p) > 0 and position('Index Cond": "(rb <= ' in p) > 0 as btree_le
  from (select get_json_plan('select id from rb_test_opclass where rb <= rb_build(''{1,2}'')')::text as p) s;
select position('rb_test_btree_idx' in p) > 0 and position('Index Cond": "(rb = ' in p) > 0 as btree_eq
  from (select get_json_plan('select id from rb_test_opclass where rb =  rb_build(''{1,2}'')')::text as p) s;
select position('rb_test_btree_idx' in p) > 0 and position('Index Cond": "(rb >= ' in p) > 0 as btree_ge
  from (select get_json_plan('select id from rb_test_opclass where rb >= rb_build(''{1,2}'')')::text as p) s;
select position('rb_test_btree_idx' in p) > 0 and position('Index Cond": "(rb > ' in p) > 0 as btree_gt
  from (select get_json_plan('select id from rb_test_opclass where rb >  rb_build(''{1,2}'')')::text as p) s;

-- hash: '=' is indexable
select position('rb_test_hash_idx' in p) > 0 and position('Index Cond": "(rb = ' in p) > 0 as hash_eq
  from (select get_json_plan('select id from rb_test_hash where rb = rb_build(''{1,2,3}'')')::text as p) s;

-- gin: all five strategies are indexable
select position('rb_test_gin_idx' in p) > 0 and position('Index Cond": "(rb && ' in p) > 0 as gin_overlap
  from (select get_json_plan('select id from rb_test_gin where rb && rb_build(''{2}'')')::text as p) s;
select position('rb_test_gin_idx' in p) > 0 and position('Index Cond": "(rb @> ' in p) > 0 as gin_contains
  from (select get_json_plan('select id from rb_test_gin where rb @> rb_build(''{1,2}'')')::text as p) s;
select position('rb_test_gin_idx' in p) > 0 and position('Index Cond": "(rb <@ ' in p) > 0 as gin_contained_by
  from (select get_json_plan('select id from rb_test_gin where rb <@ rb_build(''{1,2}'')')::text as p) s;
select position('rb_test_gin_idx' in p) > 0 and position('Index Cond": "(rb = ' in p) > 0 as gin_eq
  from (select get_json_plan('select id from rb_test_gin where rb = rb_build(''{1,2}'')')::text as p) s;
select position('rb_test_gin_idx' in p) > 0 and position('Index Cond": "(rb @> 2' in p) > 0 as gin_contains_int
  from (select get_json_plan('select id from rb_test_gin where rb @> 2')::text as p) s;

reset enable_seqscan;
reset max_parallel_workers_per_gather;

-- test comparison operators in self-join elimination
-- only = operator can set MERGES = true
select oprname, oprcanmerge, oprcanhash from pg_operator
  where oprleft = 'roaringbitmap'::regtype
    and oprright = 'roaringbitmap'::regtype
    and oprname in ('=', '<', '<=', '>=', '>')
  order by oprname;

drop table if exists rb_test_sje;
create table rb_test_sje (a int primary key, rb roaringbitmap);
insert into rb_test_sje values
  (1, rb_build('{}')),
  (2, rb_build('{1}')),
  (3, rb_build('{1,2}')),
  (4, rb_build('{2,3,4}')),
  (5, rb_build('{1,2,3,4,5}')),
  (6, rb_build('{1,2,3,4,5,-1}')),
  (7, rb_build('{1,2,3,4,-1}')),
  (8, NULL);
analyze rb_test_sje;

-- x.a = y.a forces the same row on both sides, so "<" and ">" must match nothing
select count(*) from rb_test_sje x, rb_test_sje y where x.a = y.a and x.rb <  y.rb;
select count(*) from rb_test_sje x, rb_test_sje y where x.a = y.a and x.rb >  y.rb;
-- while "<=", ">=" and "=" must still match every row
select count(*) from rb_test_sje x, rb_test_sje y where x.a = y.a and x.rb <= y.rb;
select count(*) from rb_test_sje x, rb_test_sje y where x.a = y.a and x.rb >= y.rb;
select count(*) from rb_test_sje x, rb_test_sje y where x.a = y.a and x.rb =  y.rb;

-- the comparison qual must survive self-join elimination; from 18 on the
-- elimination collapses both sides to "rb < rb", older servers keep the two
-- aliases ("x.rb < y.rb"), so only "<" can be matched on every version
select position('rb < ' in get_json_plan(
  'select x.a from rb_test_sje x, rb_test_sje y where x.a = y.a and x.rb < y.rb'
)::text) > 0 as sje_keeps_lt_qual;


-- ============================================================
-- statistics: rb_typanalyze collects no MCV / histogram
-- ============================================================

-- The type names a custom typanalyze function.  
select typname, typanalyze::regproc
  from pg_type
 where typname in ('roaringbitmap')
 order by typname;

drop table if exists rb_test_stats;
create table rb_test_stats (id int, small_rb roaringbitmap, big_rb roaringbitmap);
-- small_rb stays inline, big_rb is stored in the TOAST table
insert into rb_test_stats
  select g,
         rb_build(array(select x from generate_series(g * 10, g * 10 + 99) x)),
         rb_build(array(select x from generate_series(g * 100000, g * 100000 + 4999) x))
    from generate_series(1, 10) g;
insert into rb_test_stats
  select g,
         rb_build(array(select x from generate_series(1, 99) x)),
         rb_build(array(select x from generate_series(1, 4999) x))
    from generate_series(1000, 1010) g;
analyze rb_test_stats;

-- statistic for roaringbitmap column: no MCV, no histogram, and n_distinct = 0.
-- n_distinct = 0 means "unknown", which sends the planner back to 
-- DEFAULT_NUM_DISTINCT(200) -- the same estimate it used before 1.3 gave the 
-- type a btree operator class.
select attname, null_frac, n_distinct, avg_width,
       most_common_vals is null as no_mcv,
       histogram_bounds is null as no_histogram
  from pg_stats
 where tablename = 'rb_test_stats'
 order by attname;

-- dropping the typanalyze function brings the MCV and histogram back, except for
-- values above WIDTH_THRESHOLD (1024 byte).
alter type roaringbitmap set (analyze = none);
analyze rb_test_stats;
select attname, null_frac, n_distinct, avg_width,
       most_common_vals is null as no_mcv,
       histogram_bounds is null as no_histogram
  from pg_stats
 where tablename = 'rb_test_stats'
 order by attname;

alter type roaringbitmap set (analyze = rb_typanalyze);


