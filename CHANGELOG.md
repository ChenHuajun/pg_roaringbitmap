
# Release Notes

### v0.1.0 (2018-04-07)
- Adds initial regresion test set
- Refactor roaringbitmap.c's code to clean compile warnnings
- Adds `rb_to_array()` function
- Removes `rb_iterate()` function to avoid memory leak
- Fixes a bug that could cause memory leak
- Adds support for parallel aggragation

### v0.0.3 (2018-03-31)

- update the CRoaring to v0.2.39.
- fork from https://github.com/zeromax007/gpdb-roaringbitmap and make roaringbitmap to be a PostgreSQL extension
