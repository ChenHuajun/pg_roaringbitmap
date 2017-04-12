# gpdb-roaringbitmap
RoaringBitmap extension for greenplum-db


# Introduction
Roaring bitmaps are compressed bitmaps which tend to outperform conventional compressed bitmaps such as WAH, EWAH or Concise. In some instances, roaring bitmaps can be hundreds of times faster and they often offer significantly better compression. They can even be faster than uncompressed bitmaps. More information https://github.com/RoaringBitmap/CRoaring .

# Build

If $GPHOME is /usr/local/gpdb .

```
gcc -march=native -O3 -std=c11 -Wall -Wpointer-arith  -Wendif-labels -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -fno-aggressive-loop-optimizations -Wno-unused-but-set-variable -Wno-address -fpic -D_GNU_SOURCE -I/usr/local/gpdb/include/postgresql/server -I/usr/local/gpdb/include/postgresql/internal -c -o roaringbitmap.o roaringbitmap.c

gcc -O3 -std=gnu99 -Wall -Wpointer-arith  -Wendif-labels -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -fno-aggressive-loop-optimizations -Wno-unused-but-set-variable -Wno-address  -fpic -shared --enable-new-dtags -o roaringbitmap.so roaringbitmap.o

cp ./roaringbitmap.so /usr/local/gpdb/lib/postgresql/

psql -f ./roaringbitmap.sql
```

# Usage

## Create table
```
CREATE TABLE t1 (id integer, bitmap roaringbitmap);
```

## Build bitmap
```
INSERT INTO t1 SELECT 1,RB_BUILD(ARRAY[1,2,3,4,5,6,7,8,9,200]);

INSERT INTO t1 SELECT 2,RB_BUILD_AGG(e) FROM GENERATE_SERIES(1,100) e;
```

## Bitmap Calculation (OR, AND, XOR, ANDNOT)
```
SELECT RB_OR(a.bitmap,b.bitmap) FORM (SELECT bitmap FROM t1 WHERE id = 1) AS a,(SELECT bitmap FROM t1 WHERE id = 2) AS b;
```

## Bitmap Aggregate (OR, AND, XOR, BUILD)
```
SELECT RB_OR_AGG(bitmap) FROM t1;
SELECT RB_AND_AGG(bitmap) FORM t1;
SELECT RB_XOR_AGG(bitmap) FROM t1;
SELECT RB_BUILD_AGG(e) FROM GENERATE_SERIES(1,100) e;
```

## Cardinality
```
SELECT RB_CARDINALITY(bitmap) FROM t1;
```

## Bitmap to SETOF integer
```
SELECT RB_ITERATE(bitmap) FROM t1 WHERE id = 1;
```




