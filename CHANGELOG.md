
# Change Log

### v0.5.4(2022-06-29)
- Fix incorrect result of rb_and which introduced by v0.5.2 (#22)
  Similar problems exist in rb_and,rb_and_cardinality,rb_andnot,rb_andnot_cardinality,rb_contains,rb_containedby and rb_intersect

### v0.5.3(2021-11-03)
- Adjust test cases to adapt to PG13, PG14

### v0.5.2(2020-07-13)
- Optimize performance of some functions through deferred serialization
  Optimized functions include rb_or_cardinality,rb_and,rb_and_cardinality,rb_andnot,rb_andnot_cardinality,rb_xor_cardinality,rb_cardinality,rb_is_empty,rb_exsit,rb_equals,rb_not_equals,rb_intersect,rb_contains,rb_containedby,rb_jaccard_dist,rb_min,rb_max,rb_rank,rb_index
- Upgrade CRoaring to v0.2.66
- add benchmark script
- add travis CI support

### v0.5.1 (2020-04-30)
- Remove `-march=native` from Makefile and add new Makefile_native to compile using native instructions (#8)
- Fixes memory leak introduced by v0.4.1 which caused by call `PG_GETARG_BYTEA_P()` in aggctx (#9)

### v0.5.0 (2019-11-17)
- Upgrade CRoaring to 0.2.65
- Add support of PostgreSQL 12
- Add support of Greenplum-db 6
- Redefine rb_or_cardinality_agg/rb_and_cardinality_agg/rb_xor_cardinality_agg to support parallel aggregate
- Fixes memory leak of v0.4.1 which caused by aligned malloc

### v0.4.1 (2019-11-04)
- Use PostgreSQL MemoryContext instead of direct use of malloc
- Fixes a bug that could cause crash when run windows aggregate (#5)
- Fixes a bug that parallel aggregate may product wrong result (#6)

### v0.4.0 (2019-05-27)
- Add type cast between roaringbitmp and bytea
- Add support of PostgreSQL 11

### v0.3.0 (2018-08-23)
- Add roaringbitmap.output_format parameter to control 'bytea' or 'array' output format
- Change roaringbitmap default output format to 'bytea' in order to better support large cardinality bitmaps
- Add `rb_iterate()` function and fix memory leak
- Add `roaringbitmap.output_format` parameter

### v0.2.1 (2018-06-19)
- Upgrade CRoaring to 0.2.49

### v0.2.0 (2018-06-09)
- Adds support of input/output syntax similar to int array
- Change range type from integer to bigint
- Add boundary check of range
- Adds `rb_index()`,`rb_fill()`,`rb_clear()`,`rb_range()`,`rb_range_cardinality()`,`rb_jaccard_dist()`,`rb_select()` functions
- Adds Operators
- Rename `rb_minimum()` to `rb_min()`
- Rename `rb_maximum()` to `rb_max()`
- Upgrade CRoaring to 0.2.42

### v0.1.0 (2018-04-07)
- Adds initial regresion test set
- Refactor roaringbitmap.c's code to clean compile warnnings
- Adds `rb_to_array()` function
- Removes `rb_iterate()` function to avoid memory leak
- Fixes a bug that could cause memory leak
- Adds support for parallel aggragation

### v0.0.3 (2018-03-31)

- fork from https://github.com/zeromax007/gpdb-roaringbitmap and make roaringbitmap to be a PostgreSQL extension
- update the CRoaring to v0.2.39.