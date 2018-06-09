
# Change Log

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