# pg_roaringbitmap
RoaringBitmap extension for PostgreSQL.

It's initial based on https://github.com/zeromax007/gpdb-roaringbitmap.


# Introduction
Roaring bitmaps are compressed bitmaps which tend to outperform conventional compressed bitmaps such as WAH, EWAH or Concise. In some instances, roaring bitmaps can be hundreds of times faster and they often offer significantly better compression. They can even be faster than uncompressed bitmaps. More information https://github.com/RoaringBitmap/CRoaring.


# Build

## Requirements
- PostgreSQL 10+
- Other Requirements from https://github.com/RoaringBitmap/CRoaring

Note: The regression testing before the version release only covers PostgreSQL 13 and above.

## Build

	su - postgres
	make
	sudo make install
	psql -c "create extension roaringbitmap"

Note:You can use `make -f Makefile_native` instead of `make` to let the compiler use the SIMD instructions if it's supported by your CPU. In some scenarios, it may double the performance. But if you copy the `pg_roaringbitmap` binary which builded on machine with SIMD support to other machine without SIMD and run, you could get a SIGILL crash.

## Test

	make installcheck

# Usage

## roaringbitmap
### about roaringbitmap data type

Logically, you could think of roaringbitmap data type as `bit(4294967296)`, and it should be noted that
the integers added to bitmaps is considered to be unsigned. Within bitmaps, numbers are ordered according to uint32. We order the numbers like 0, 1, ..., 2147483647, -2147483648, -2147483647,..., -1. But we use bigint to
reference the range of these integers, that is [0 4294967296).

### input and ouput

Support two kind of input/output syntax 'array' and 'bytea',
The default output format is 'bytea'.

    postgres=# select roaringbitmap('{1,100,10}');
                     roaringbitmap
    ------------------------------------------------
     \x3a30000001000000000002001000000001000a006400
    (1 row)

or

    postgres=# select '\x3a30000001000000000002001000000001000a006400'::roaringbitmap;
                     roaringbitmap
    ------------------------------------------------
     \x3a30000001000000000002001000000001000a006400
    (1 row)


output format can changed by `roaringbitmap.output_format`

	postgres=# set roaringbitmap.output_format='bytea';
	SET
	postgres=# select '{1}'::roaringbitmap;
	             roaringbitmap
	----------------------------------------
	 \x3a3000000100000000000000100000000100
	(1 row)
	
	postgres=# set roaringbitmap.output_format='array';
	SET
	postgres=# select '{1}'::roaringbitmap;
	 roaringbitmap
	---------------
	 {1}
	(1 row)

### sample of usage
#### Use bitmap as type of column

	CREATE TABLE t1 (id integer, bitmap roaringbitmap);


#### Build bitmap from integers

	INSERT INTO t1 SELECT 1,rb_build(ARRAY[1,2,3,4,5,6,7,8,9,200]);
	
	INSERT INTO t1 SELECT 2,rb_build_agg(e) FROM generate_series(1,100) e;

#### Bitmap Calculation (OR, AND, XOR, ANDNOT)

	SELECT roaringbitmap('{1,2,3}') | roaringbitmap('{3,4,5}');
	SELECT roaringbitmap('{1,2,3}') & roaringbitmap('{3,4,5}');
	SELECT roaringbitmap('{1,2,3}') # roaringbitmap('{3,4,5}');
	SELECT roaringbitmap('{1,2,3}') - roaringbitmap('{3,4,5}');

#### Bitmap Aggregate (OR, AND, XOR, BUILD)

	SELECT rb_or_agg(bitmap) FROM t1;
	SELECT rb_and_agg(bitmap) FROM t1;
	SELECT rb_xor_agg(bitmap) FROM t1;
	SELECT rb_build_agg(e) FROM generate_series(1,100) e;

#### Calculate cardinality

	SELECT rb_cardinality('{1,2,3}');

#### Convert bitmap to integer array

	SELECT rb_to_array(bitmap) FROM t1 WHERE id = 1;

#### Convert bitmap to SET of integers

	SELECT unnest(rb_to_array('{1,2,3}'::roaringbitmap));

or

	SELECT rb_iterate('{1,2,3}'::roaringbitmap);

### Opperator List
<table>
    <thead>
           <th>Opperator</th>
           <th>Input</th>
           <th>Output</th>
           <th>Desc</th>
           <th>Example</th>
           <th>Result</th>
    </thead>
    <tr>
        <td><code>&</code></td>
        <td><code>roaringbitmap,roaringbitmap</code></td>
        <td><code>roaringbitmap</code></td>
        <td>bitwise AND</td>
        <td><code>roaringbitmap('{1,2,3}') & roaringbitmap('{3,4,5}')</code></td>
        <td><code>{3}</code></td>
    </tr>
    <tr>
        <td><code>|</code></td>
        <td><code>roaringbitmap,roaringbitmap</code></td>
        <td><code>roaringbitmap</code></td>
        <td>bitwise OR</td>
        <td><code>roaringbitmap('{1,2,3}') | roaringbitmap('{3,4,5}')</code></td>
        <td><code>{1,2,3,4,5}</code></td>
    </tr>
    <tr>
        <td><code>|</code></td>
        <td><code>roaringbitmap,integer</code></td>
        <td><code>roaringbitmap</code></td>
        <td>add element to roaringbitmap</td>
        <td><code>roaringbitmap('{1,2,3}') | 6</code></td>
        <td><code>{1,2,3,6}</code></td>
    </tr>
    <tr>
        <td><code>|</code></td>
        <td><code>integer,roaringbitmap</code></td>
        <td><code>roaringbitmap</code></td>
        <td>add element to roaringbitmap</td>
        <td><code>6 | roaringbitmap('{1,2,3}')</code></td>
        <td><code>{1,2,3,6}</code></td>
    </tr>
    <tr>
        <td><code>#</code></td>
        <td><code>roaringbitmap,roaringbitmap</code></td>
        <td><code>roaringbitmap</code></td>
        <td>bitwise XOR</td>
        <td><code>roaringbitmap('{1,2,3}') # roaringbitmap('{3,4,5}')</code></td>
        <td><code>{1,2,4,5}</code></td>
    </tr>
    <tr>
        <td><code><<</code></td>
        <td><code>roaringbitmap,bigint</code></td>
        <td><code>roaringbitmap</code></td>
        <td>bitwise shift left</td>
        <td><code>roaringbitmap('{1,2,3}') << 2</code></td>
        <td><code>{0,1}</code></td>
    </tr>
    <tr>
        <td><code>>></code></td>
        <td><code>roaringbitmap,bigint</code></td>
        <td><code>roaringbitmap</code></td>
        <td>bitwise shift right</td>
        <td><code>roaringbitmap('{1,2,3}') >> 3</code></td>
        <td><code>{4,5,6}</code></td>
    </tr>
    <tr>
        <td><code>-</code></td>
        <td><code>roaringbitmap,roaringbitmap</code></td>
        <td><code>roaringbitmap</code></td>
        <td>difference(bitwise ANDNOT)</td>
        <td><code>roaringbitmap('{1,2,3}') - roaringbitmap('{3,4,5}')</code></td>
        <td><code>{1,2}</code></td>
    </tr>
    <tr>
        <td><code>-</code></td>
        <td><code>roaringbitmap,integer</code></td>
        <td><code>roaringbitmap</code></td>
        <td>remove element from roaringbitmap</td>
        <td><code>roaringbitmap('{1,2,3}') - 3</code></td>
        <td><code>{1,2}</code></td>
    </tr>
    <tr>
        <td><code>@></code></td>
        <td><code>roaringbitmap,roaringbitmap</code></td>
        <td><code>bool</code></td>
        <td>contains</td>
        <td><code>roaringbitmap('{1,2,3}') @> roaringbitmap('{3,4,5}')</code></td>
        <td><code>f</code></td>
    </tr>
    <tr>
        <td><code>@></code></td>
        <td><code>roaringbitmap,integer</code></td>
        <td><code>bool</code></td>
        <td>contains</td>
        <td><code>roaringbitmap('{1,2,3,4,5}') @> 3</code></td>
        <td><code>t</code></td>
    </tr>
    <tr>
        <td><code><@</code></td>
        <td><code>roaringbitmap,roaringbitmap</code></td>
        <td><code>bool</code></td>
        <td>is contained by</td>
        <td><code>roaringbitmap('{1,2,3}') <@ roaringbitmap('{3,4,5}')</code></td>
        <td><code>f</code></td>
    </tr>
    <tr>
        <td><code><@</code></td>
        <td><code>integer,roaringbitmap</code></td>
        <td><code>bool</code></td>
        <td>is contained by</td>
        <td><code>3 <@ roaringbitmap('{3,4,5}')</code></td>
        <td><code>t</code></td>
    </tr>
    <tr>
        <td><code>&&</code></td>
        <td><code>roaringbitmap,roaringbitmap</code></td>
        <td><code>bool</code></td>
        <td>overlap (have elements in common)</td>
        <td><code>roaringbitmap('{1,2,3}') && roaringbitmap('{3,4,5}')</code></td>
        <td><code>t</code></td>
    </tr>
    <tr>
        <td><code>=</code></td>
        <td><code>roaringbitmap,roaringbitmap</code></td>
        <td><code>bool</code></td>
        <td>equal</td>
        <td><code>roaringbitmap('{1,2,3}') = roaringbitmap('{3,4,5}')</code></td>
        <td><code>f</code></td>
    </tr>
    <tr>
        <td><code><></code></td>
        <td><code>roaringbitmap,roaringbitmap</code></td>
        <td><code>bool</code></td>
        <td>not equal</td>
        <td><code>roaringbitmap('{1,2,3}') <> roaringbitmap('{3,4,5}')</code></td>
        <td><code>t</code></td>
    </tr>
</table>

### Function List
<table>
    <thead>
           <th>Function</th>
           <th>Input</th>
           <th>Output</th>
           <th>Desc</th>
           <th>Example</th>
           <th>Result</th>
    </thead>
    <tr>
        <td><code>rb_build</code></td>
        <td><code>integer[]</code></td>
        <td><code>roaringbitmap</code></td>
        <td>Create roaringbitmap from integer array</td>
        <td><code>rb_build('{1,2,3,4,5}')</code></td>
        <td><code>{1,2,3,4,5}</code></td>
    </tr>
    <tr>
        <td><code>rb_index</code></td>
        <td><code>roaringbitmap,integer</code></td>
        <td><code>bigint</code></td>
        <td>Return the 0-based index of element in this roaringbitmap, or -1 if do not exist</td>
        <td><code>rb_index('{1,2,3}',3)</code></td>
        <td><code>2</code></td>
    </tr>
    <tr>
        <td><code>rb_cardinality</code></td>
        <td><code>roaringbitmap</code></td>
        <td><code>bigint</code></td>
        <td>Return cardinality of the roaringbitmap</td>
        <td><code>rb_cardinality('{1,2,3,4,5}')</code></td>
        <td><code>5</code></td>
    </tr>
    <tr>
        <td><code>rb_and_cardinality</code></td>
        <td><code>roaringbitmap,roaringbitmap</code></td>
        <td><code>bigint</code></td>
        <td>Return cardinality of the AND of two roaringbitmaps</td>
        <td><code>rb_and_cardinality('{1,2,3}','{3,4,5}')</code></td>
        <td><code>1</code></td>
    </tr>
    <tr>
        <td><code>rb_or_cardinality</code></td>
        <td><code>roaringbitmap,roaringbitmap</code></td>
        <td><code>bigint</code></td>
        <td>Return cardinality of the OR of two roaringbitmaps</td>
        <td><code>rb_or_cardinality('{1,2,3}','{3,4,5}')</code></td>
        <td><code>5</code></td>
    </tr>
    <tr>
        <td><code>rb_xor_cardinality</code></td>
        <td><code>roaringbitmap,roaringbitmap</code></td>
        <td><code>bigint</code></td>
        <td>Return cardinality of the XOR of two roaringbitmaps</td>
        <td><code>rb_xor_cardinality('{1,2,3}','{3,4,5}')</code></td>
        <td><code>4</code></td>
    </tr>
    <tr>
        <td><code>rb_andnot_cardinality</code></td>
        <td><code>roaringbitmap,roaringbitmap</code></td>
        <td><code>bigint</code></td>
        <td>Return cardinality of the ANDNOT of two roaringbitmaps</td>
        <td><code>rb_andnot_cardinality('{1,2,3}','{3,4,5}')</code></td>
        <td><code>2</code></td>
    </tr>
    <tr>
        <td><code>rb_is_empty</code></td>
        <td><code>roaringbitmap</code></td>
        <td><code>boolean</code></td>
        <td>Check if roaringbitmap is empty.</td>
        <td><code>rb_is_empty('{1,2,3,4,5}')</code></td>
        <td><code>f</code></td>
    </tr>
    <tr>
        <td><code>rb_fill</code></td>
        <td><code>roaringbitmap,range_start bigint,range_end bigint</code></td>
        <td><code>roaringbitmap</code></td>
        <td>Fill the specified range (not include the range_end)</td>
        <td><code>rb_fill('{1,2,3}',5,7)</code></td>
        <td><code>{1,2,3,5,6}</code></td>
    </tr>
    <tr>
        <td><code>rb_clear</code></td>
        <td><code>roaringbitmap,range_start bigint,range_end bigint</code></td>
        <td><code>roaringbitmap</code></td>
        <td>Clear the specified range (not include the range_end)</td>
        <td><code>rb_clear('{1,2,3}',2,3)</code></td>
        <td><code>{1,3}</code></td>
    </tr>
    <tr>
        <td><code>rb_flip</code></td>
        <td><code>roaringbitmap,range_start bigint,range_end bigint</code></td>
        <td><code>roaringbitmap</code></td>
        <td>Negative the specified range (not include the range_end)</td>
        <td><code>rb_flip('{1,2,3}',2,10)</code></td>
        <td><code>{1,4,5,6,7,8,9}</code></td>
    </tr>
    <tr>
        <td><code>rb_range</code></td>
        <td><code>roaringbitmap,range_start bigint,range_end bigint</code></td>
        <td><code>roaringbitmap</code></td>
        <td>Return new set with specified range (not include the range_end)</td>
        <td><code>rb_range('{1,2,3}',2,3)</code></td>
        <td><code>{2}</code></td>
    </tr>
    <tr>
        <td><code>rb_range_cardinality</code></td>
        <td><code>roaringbitmap,range_start bigint,range_end bigint</code></td>
        <td><code>bigint</code></td>
        <td>Return the cardinality of specified range (not include the range_end)</td>
        <td><code>rb_range_cardinality('{1,2,3}',2,3)</code></td>
        <td><code>1</code></td>
    </tr>
    <tr>
        <td><code>rb_min</code></td>
        <td><code>roaringbitmap</code></td>
        <td><code>integer</code></td>
        <td>Return the smallest offset in roaringbitmap. Return NULL if the bitmap is empty</td>
        <td><code>rb_min('{1,2,3}')</code></td>
        <td><code>1</code></td>
    </tr>
    <tr>
        <td><code>rb_max</code></td>
        <td><code>roaringbitmap</code></td>
        <td><code>integer</code></td>
        <td>Return the greatest offset in roaringbitmap. Return NULL if the bitmap is empty</td>
        <td><code>rb_max('{1,2,3}')</code></td>
        <td><code>3</code></td>
   </tr>
    <tr>
        <td><code>rb_rank</code></td>
        <td><code>roaringbitmap,integer</code></td>
        <td><code>bigint</code></td>
        <td>Return the number of elements that are smaller or equal to the specified offset</td>
        <td><code>rb_rank('{1,2,3}',3)</code></td>
        <td><code>3</code></td>
    </tr>
    <tr>
        <td><code>rb_jaccard_dist</code></td>
        <td><code>roaringbitmap,roaringbitmap</code></td>
        <td><code>double precision</code></td>
        <td>Return the jaccard distance(or the Jaccard similarity coefficient) of two bitmaps</td>
        <td><code>rb_jaccard_dist('{1,2,3}','{3,4}')</code></td>
        <td><code>0.25</code></td>
    </tr>
    <tr>
        <td><code>rb_select</code></td>
        <td><code>roaringbitmap,bitset_limit bigint,bitset_offset bigint=0,reverse boolean=false,range_start bigint=0,range_end bigint=4294967296</code></td>
        <td><code>roaringbitmap</code></td>
        <td>Return subset [bitset_offset,bitset_offset+bitset_limit) of bitmap between range [range_start,range_end)</td>
        <td><code>rb_select('{1,2,3,4,5,6,7,8,9}',5,2)</code></td>
        <td><code>{3,4,5,6,7}</code></td>
    </tr>
    <tr>
        <td><code>rb_to_array</code></td>
        <td><code>roaringbitmap</code></td>
        <td><code>integer[]</code></td>
        <td>Convert roaringbitmap to integer array</td>
        <td><code>rb_to_array(roaringbitmap('{1,2,3}'))</code></td>
        <td><code>{1,2,3}</code></td>
    </tr>
    <tr>
        <td><code>rb_iterate</code></td>
        <td><code>roaringbitmap</code></td>
        <td><code>SET of integer</code></td>
        <td>Return set of integer from a roaringbitmap data.</td>
        <td><pre>rb_iterate(roaringbitmap('{1,2,3}'))</pre></td>
        <td><pre>1
2
3</pre></td>
    </tr>
</table>

### Aggregation List
<table>
    <thead>
           <th>Function</th>
           <th>Input</th>
           <th>Output</th>
           <th>Desc</th>
           <th>Example</th>
           <th>Result</th>
    </thead>
    <tr>
        <td><code>rb_build_agg</code></td>
        <td><code>integer</code></td>
        <td><code>roaringbitmap</code></td>
        <td>Build a roaringbitmap from a integer set</td>
        <td><pre>select rb_build_agg(id)
    from (values (1),(2),(3)) t(id)</pre></td>
        <td><code>{1,2,3}</code></td>
    </tr>
    <tr>
        <td><code>rb_or_agg</code></td>
        <td><code>roaringbitmap</code></td>
        <td><code>roaringbitmap</code></td>
        <td>AND Aggregate calculations from a roaringbitmap set</td>
        <td><pre>select rb_or_agg(bitmap)
    from (values (roaringbitmap('{1,2,3}')),
                 (roaringbitmap('{2,3,4}'))
          ) t(bitmap)</pre></td>
        <td><code>{1,2,3,4}</code></td>
    </tr>
    <tr>
        <td><code>rb_and_agg</code></td>
        <td><code>roaringbitmap</code></td>
        <td><code>roaringbitmap</code></td>
        <td>AND Aggregate calculations from a roaringbitmap set</td>
        <td><pre>select rb_and_agg(bitmap)
    from (values (roaringbitmap('{1,2,3}')),
                 (roaringbitmap('{2,3,4}'))
          ) t(bitmap)</pre></td>
        <td><code>{2,3}</code></td>
    </tr>
    <tr>
        <td><code>rb_xor_agg</code></td>
        <td><code>roaringbitmap</code></td>
        <td><code>roaringbitmap</code></td>
        <td>XOR Aggregate calculations from a roaringbitmap set</td>
        <td><pre>select rb_xor_agg(bitmap)
    from (values (roaringbitmap('{1,2,3}')),
                 (roaringbitmap('{2,3,4}'))
          ) t(bitmap)</pre></td>
        <td><code>{1,4}</code></td>
    </tr>
    <tr>
        <td><code>rb_or_cardinality_agg</code></td>
        <td><code>roaringbitmap</code></td>
        <td><code>bigint</code></td>
        <td>OR Aggregate calculations from a roaringbitmap set, return cardinality.</td>
        <td><pre>select rb_or_cardinality_agg(bitmap)
    from (values (roaringbitmap('{1,2,3}')),
                 (roaringbitmap('{2,3,4}'))
          ) t(bitmap)</pre></td>
        <td><code>4</code></td>
    </tr>
    <tr>
        <td><code>rb_and_cardinality_agg</code></td>
        <td><code>roaringbitmap</code></td>
        <td><code>bigint</code></td>
        <td>AND Aggregate calculations from a roaringbitmap set, return cardinality</td>
        <td><pre>select rb_and_cardinality_agg(bitmap)
    from (values (roaringbitmap('{1,2,3}')),
                 (roaringbitmap('{2,3,4}'))
          ) t(bitmap)</pre></td>
        <td><code>2</code></td>
    </tr>
    <tr>
        <td><code>rb_xor_cardinality_agg</code></td>
        <td><code>roaringbitmap</code></td>
        <td><code>bigint</code></td>
        <td>XOR Aggregate calculations from a roaringbitmap set, return cardinality</td>
        <td><pre>select rb_xor_cardinality_agg(bitmap)
    from (values (roaringbitmap('{1,2,3}')),
                 (roaringbitmap('{2,3,4}'))
          ) t(bitmap)</pre></td>
        <td><code>2</code></td>
    </tr>
</table>

## roaringbitmap64
### about roaringbitmap64 data type

​​roaringbitmap64​​ is a 64-bit Roaring bitmap implementation, and its format definition can be found in https://github.com/RoaringBitmap/RoaringFormatSpec.
Logically, you could think of roaringbitmap64 data type as `bit(18446744073709551615)` just like roaringbitmap, and it should be noted that
the bigint data added to bitmaps is considered to be unsigned. Within 64 bit bitmaps, numbers are ordered according to uint64. 
We order the numbers like 0, 1, ..., 9223372036854775807, -9223372036854775808, -9223372036854775807,..., -1. 

### input and ouput

Support two kind of input/output syntax 'array' and 'bytea',
The default output format is 'bytea'.

    postgres=# select roaringbitmap64('{1,100,10}');
                                roaringbitmap64
    ------------------------------------------------------------------------
     \x0100000000000000000000003a30000001000000000002001000000001000a006400
    (1 row)


or

    postgres=# select '\x0100000000000000000000003a30000001000000000002001000000001000a006400'::roaringbitmap64;
                                roaringbitmap64
    ------------------------------------------------------------------------
     \x0100000000000000000000003a30000001000000000002001000000001000a006400
    (1 row)


output format can changed by `roaringbitmap.output_format`

    postgres=# set roaringbitmap.output_format='bytea';
    SET
    postgres=# select '{1}'::roaringbitmap64;
                            roaringbitmap64
    ----------------------------------------------------------------
     \x0100000000000000000000003a3000000100000000000000100000000100
    (1 row)

    postgres=# set roaringbitmap.output_format='array';
    SET
    postgres=# select '{1}'::roaringbitmap64;
     roaringbitmap64
    -----------------
     {1}
    (1 row)

### sample of usage
#### Use bitmap as type of column

    CREATE TABLE t1 (id integer, bitmap roaringbitmap64);


#### Build bitmap from set of bigint

    INSERT INTO t1 SELECT 1,rb64_build(ARRAY[1,2,3,4,5,6,7,8,9,200]);
    
    INSERT INTO t1 SELECT 2,rb64_build_agg(e) FROM generate_series(1,100) e;

#### Bitmap Calculation (OR, AND, XOR, ANDNOT)

    SELECT roaringbitmap64('{1,2,3}') | roaringbitmap64('{3,4,5}');
    SELECT roaringbitmap64('{1,2,3}') & roaringbitmap64('{3,4,5}');
    SELECT roaringbitmap64('{1,2,3}') # roaringbitmap64('{3,4,5}');
    SELECT roaringbitmap64('{1,2,3}') - roaringbitmap64('{3,4,5}');

#### Bitmap Aggregate (OR, AND, XOR, BUILD)

    SELECT rb64_or_agg(bitmap) FROM t1;
    SELECT rb64_and_agg(bitmap) FROM t1;
    SELECT rb64_xor_agg(bitmap) FROM t1;
    SELECT rb64_build_agg(e) FROM generate_series(1,100) e;

#### Calculate cardinality

    SELECT rb64_cardinality('{1,2,3}');

#### Convert bitmap to bigint array

    SELECT rb64_to_array(bitmap) FROM t1 WHERE id = 1;

#### Convert bitmap to SET of bigint

    SELECT unnest(rb64_to_array('{1,2,3}'::roaringbitmap64));

or

    SELECT rb64_iterate('{1,2,3}'::roaringbitmap64);

### Opperator List
<table>
    <thead>
           <th>Opperator</th>
           <th>Input</th>
           <th>Output</th>
           <th>Desc</th>
           <th>Example</th>
           <th>Result</th>
    </thead>
    <tr>
        <td><code>&</code></td>
        <td><code>roaringbitmap64,roaringbitmap64</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>bitwise AND</td>
        <td><code>roaringbitmap64('{1,2,3}') & roaringbitmap64('{3,4,5}')</code></td>
        <td><code>{3}</code></td>
    </tr>
    <tr>
        <td><code>|</code></td>
        <td><code>roaringbitmap64,roaringbitmap64</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>bitwise OR</td>
        <td><code>roaringbitmap64('{1,2,3}') | roaringbitmap64('{3,4,5}')</code></td>
        <td><code>{1,2,3,4,5}</code></td>
    </tr>
    <tr>
        <td><code>|</code></td>
        <td><code>roaringbitmap64,bigint</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>add element to roaringbitmap64</td>
        <td><code>roaringbitmap64('{1,2,3}') | 6</code></td>
        <td><code>{1,2,3,6}</code></td>
    </tr>
    <tr>
        <td><code>|</code></td>
        <td><code>bigint,roaringbitmap64</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>add element to roaringbitmap64</td>
        <td><code>6 | roaringbitmap64('{1,2,3}')</code></td>
        <td><code>{1,2,3,6}</code></td>
    </tr>
    <tr>
        <td><code>#</code></td>
        <td><code>roaringbitmap64,roaringbitmap64</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>bitwise XOR</td>
        <td><code>roaringbitmap64('{1,2,3}') # roaringbitmap64('{3,4,5}')</code></td>
        <td><code>{1,2,4,5}</code></td>
    </tr>
    <tr>
        <td><code><<</code></td>
        <td><code>roaringbitmap64,bigint</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>bitwise shift left</td>
        <td><code>roaringbitmap64('{1,2,3}') << 2</code></td>
        <td><code>{0,1}</code></td>
    </tr>
    <tr>
        <td><code>>></code></td>
        <td><code>roaringbitmap64,bigint</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>bitwise shift right</td>
        <td><code>roaringbitmap64('{1,2,3}') >> 3</code></td>
        <td><code>{4,5,6}</code></td>
    </tr>
    <tr>
        <td><code>-</code></td>
        <td><code>roaringbitmap64,roaringbitmap64</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>difference(bitwise ANDNOT)</td>
        <td><code>roaringbitmap64('{1,2,3}') - roaringbitmap64('{3,4,5}')</code></td>
        <td><code>{1,2}</code></td>
    </tr>
    <tr>
        <td><code>-</code></td>
        <td><code>roaringbitmap64,bigint</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>remove element from roaringbitmap64</td>
        <td><code>roaringbitmap64('{1,2,3}') - 3</code></td>
        <td><code>{1,2}</code></td>
    </tr>
    <tr>
        <td><code>@></code></td>
        <td><code>roaringbitmap64,roaringbitmap64</code></td>
        <td><code>bool</code></td>
        <td>contains</td>
        <td><code>roaringbitmap64('{1,2,3}') @> roaringbitmap64('{3,4,5}')</code></td>
        <td><code>f</code></td>
    </tr>
    <tr>
        <td><code>@></code></td>
        <td><code>roaringbitmap64,bigint</code></td>
        <td><code>bool</code></td>
        <td>contains</td>
        <td><code>roaringbitmap64('{1,2,3,4,5}') @> 3</code></td>
        <td><code>t</code></td>
    </tr>
    <tr>
        <td><code><@</code></td>
        <td><code>roaringbitmap64,roaringbitmap64</code></td>
        <td><code>bool</code></td>
        <td>is contained by</td>
        <td><code>roaringbitmap64('{1,2,3}') <@ roaringbitmap64('{3,4,5}')</code></td>
        <td><code>f</code></td>
    </tr>
    <tr>
        <td><code><@</code></td>
        <td><code>bigint,roaringbitmap64</code></td>
        <td><code>bool</code></td>
        <td>is contained by</td>
        <td><code>3 <@ roaringbitmap64('{3,4,5}')</code></td>
        <td><code>t</code></td>
    </tr>
    <tr>
        <td><code>&&</code></td>
        <td><code>roaringbitmap64,roaringbitmap64</code></td>
        <td><code>bool</code></td>
        <td>overlap (have elements in common)</td>
        <td><code>roaringbitmap64('{1,2,3}') && roaringbitmap64('{3,4,5}')</code></td>
        <td><code>t</code></td>
    </tr>
    <tr>
        <td><code>=</code></td>
        <td><code>roaringbitmap64,roaringbitmap64</code></td>
        <td><code>bool</code></td>
        <td>equal</td>
        <td><code>roaringbitmap64('{1,2,3}') = roaringbitmap64('{3,4,5}')</code></td>
        <td><code>f</code></td>
    </tr>
    <tr>
        <td><code><></code></td>
        <td><code>roaringbitmap64,roaringbitmap64</code></td>
        <td><code>bool</code></td>
        <td>not equal</td>
        <td><code>roaringbitmap64('{1,2,3}') <> roaringbitmap64('{3,4,5}')</code></td>
        <td><code>t</code></td>
    </tr>
</table>

### Function List
<table>
    <thead>
           <th>Function</th>
           <th>Input</th>
           <th>Output</th>
           <th>Desc</th>
           <th>Example</th>
           <th>Result</th>
    </thead>
    <tr>
        <td><code>rb64_build</code></td>
        <td><code>bigint[]</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>Create roaringbitmap64 from bigint array</td>
        <td><code>rb64_build('{1,2,3,4,5}')</code></td>
        <td><code>{1,2,3,4,5}</code></td>
    </tr>
    <tr>
        <td><code>rb64_index</code></td>
        <td><code>roaringbitmap64,bigint</code></td>
        <td><code>bigint</code></td>
        <td>Return the 0-based index of element in this roaringbitmap64, or -1 if do not exist</td>
        <td><code>rb64_index('{1,2,3}',3)</code></td>
        <td><code>2</code></td>
    </tr>
    <tr>
        <td><code>rb64_cardinality</code></td>
        <td><code>roaringbitmap64</code></td>
        <td><code>bigint</code></td>
        <td>Return cardinality of the roaringbitmap64</td>
        <td><code>rb64_cardinality('{1,2,3,4,5}')</code></td>
        <td><code>5</code></td>
    </tr>
    <tr>
        <td><code>rb64_and_cardinality</code></td>
        <td><code>roaringbitmap64,roaringbitmap64</code></td>
        <td><code>bigint</code></td>
        <td>Return cardinality of the AND of two roaringbitmaps</td>
        <td><code>rb64_and_cardinality('{1,2,3}','{3,4,5}')</code></td>
        <td><code>1</code></td>
    </tr>
    <tr>
        <td><code>rb64_or_cardinality</code></td>
        <td><code>roaringbitmap64,roaringbitmap64</code></td>
        <td><code>bigint</code></td>
        <td>Return cardinality of the OR of two roaringbitmaps</td>
        <td><code>rb64_or_cardinality('{1,2,3}','{3,4,5}')</code></td>
        <td><code>5</code></td>
    </tr>
    <tr>
        <td><code>rb64_xor_cardinality</code></td>
        <td><code>roaringbitmap64,roaringbitmap64</code></td>
        <td><code>bigint</code></td>
        <td>Return cardinality of the XOR of two roaringbitmaps</td>
        <td><code>rb64_xor_cardinality('{1,2,3}','{3,4,5}')</code></td>
        <td><code>4</code></td>
    </tr>
    <tr>
        <td><code>rb64_andnot_cardinality</code></td>
        <td><code>roaringbitmap64,roaringbitmap64</code></td>
        <td><code>bigint</code></td>
        <td>Return cardinality of the ANDNOT of two roaringbitmaps</td>
        <td><code>rb64_andnot_cardinality('{1,2,3}','{3,4,5}')</code></td>
        <td><code>2</code></td>
    </tr>
    <tr>
        <td><code>rb64_is_empty</code></td>
        <td><code>roaringbitmap64</code></td>
        <td><code>boolean</code></td>
        <td>Check if roaringbitmap64 is empty.</td>
        <td><code>rb64_is_empty('{1,2,3,4,5}')</code></td>
        <td><code>f</code></td>
    </tr>
    <tr>
        <td><code>rb64_fill</code></td>
        <td><code>roaringbitmap64,range_start bigint,range_end bigint</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>Fill the specified range (not include the range_end),negative range value will be internally converted to unsigned int64</td>
        <td><code>rb64_fill('{1,2,3}',5,7)</code></td>
        <td><code>{1,2,3,5,6}</code></td>
    </tr>
    <tr>
        <td><code>rb64_clear</code></td>
        <td><code>roaringbitmap64,range_start bigint,range_end bigint</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>Clear the specified range (not include the range_end),negative range value will be internally converted to unsigned int64</td>
        <td><code>rb64_clear('{1,2,3}',2,3)</code></td>
        <td><code>{1,3}</code></td>
    </tr>
    <tr>
        <td><code>rb64_flip</code></td>
        <td><code>roaringbitmap64,range_start bigint,range_end bigint</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>Negative the specified range (not include the range_end),negative range value will be internally converted to unsigned int64</td>
        <td><code>rb64_flip('{1,2,3}',2,10)</code></td>
        <td><code>{1,4,5,6,7,8,9}</code></td>
    </tr>
    <tr>
        <td><code>rb64_range</code></td>
        <td><code>roaringbitmap64,range_start bigint,range_end bigint</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>Return new set with specified range (not include the range_end),negative range value will be internally converted to unsigned int64</td>
        <td><code>rb64_range('{1,2,3}',2,3)</code></td>
        <td><code>{2}</code></td>
    </tr>
    <tr>
        <td><code>rb64_range_cardinality</code></td>
        <td><code>roaringbitmap64,range_start bigint,range_end bigint</code></td>
        <td><code>bigint</code></td>
        <td>Return the cardinality of specified range (not include the range_end),negative range value will be internally converted to unsigned int64</td>
        <td><code>rb64_range_cardinality('{1,2,3}',2,3)</code></td>
        <td><code>1</code></td>
    </tr>
    <tr>
        <td><code>rb64_min</code></td>
        <td><code>roaringbitmap64</code></td>
        <td><code>bigint</code></td>
        <td>Return the smallest offset in roaringbitmap64. Return NULL if the bitmap is empty</td>
        <td><code>rb64_min('{1,2,3}')</code></td>
        <td><code>1</code></td>
    </tr>
    <tr>
        <td><code>rb64_max</code></td>
        <td><code>roaringbitmap64</code></td>
        <td><code>bigint</code></td>
        <td>Return the greatest offset in roaringbitmap64. Return NULL if the bitmap is empty</td>
        <td><code>rb64_max('{1,2,3}')</code></td>
        <td><code>3</code></td>
   </tr>
    <tr>
        <td><code>rb64_rank</code></td>
        <td><code>roaringbitmap64,bigint</code></td>
        <td><code>bigint</code></td>
        <td>Return the number of elements that are smaller or equal to the specified offset</td>
        <td><code>rb64_rank('{1,2,3}',3)</code></td>
        <td><code>3</code></td>
    </tr>
    <tr>
        <td><code>rb64_jaccard_dist</code></td>
        <td><code>roaringbitmap64,roaringbitmap64</code></td>
        <td><code>double precision</code></td>
        <td>Return the jaccard distance(or the Jaccard similarity coefficient) of two bitmaps</td>
        <td><code>rb64_jaccard_dist('{1,2,3}','{3,4}')</code></td>
        <td><code>0.25</code></td>
    </tr>
    <tr>
        <td><code>rb64_select</code></td>
        <td><code>roaringbitmap64,bitset_limit bigint,bitset_offset bigint=0,reverse boolean=false,range_start bigint=0,range_end bigint=-1</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>Return subset [bitset_offset,bitset_offset+bitset_limit) of bitmap between range [range_start,range_end),negative range value will be internally converted to unsigned int64</td>
        <td><code>rb64_select('{1,2,3,4,5,6,7,8,9}',5,2)</code></td>
        <td><code>{3,4,5,6,7}</code></td>
    </tr>
    <tr>
        <td><code>rb64_to_array</code></td>
        <td><code>roaringbitmap64</code></td>
        <td><code>bigint[]</code></td>
        <td>Convert roaringbitmap64 to bigint array</td>
        <td><code>rb64_to_array(roaringbitmap64('{1,2,3}'))</code></td>
        <td><code>{1,2,3}</code></td>
    </tr>
    <tr>
        <td><code>rb64_iterate</code></td>
        <td><code>roaringbitmap64</code></td>
        <td><code>SET of bigint</code></td>
        <td>Return set of bigint from a roaringbitmap64 data.</td>
        <td><pre>rb64_iterate(roaringbitmap64('{1,2,3}'))</pre></td>
        <td><pre>1
2
3</pre></td>
    </tr>
</table>

### Aggregation List
<table>
    <thead>
           <th>Function</th>
           <th>Input</th>
           <th>Output</th>
           <th>Desc</th>
           <th>Example</th>
           <th>Result</th>
    </thead>
    <tr>
        <td><code>rb64_build_agg</code></td>
        <td><code>bigint</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>Build a roaringbitmap64 from a bigint set</td>
        <td><pre>select rb64_build_agg(id)
    from (values (1),(2),(3)) t(id)</pre></td>
        <td><code>{1,2,3}</code></td>
    </tr>
    <tr>
        <td><code>rb64_or_agg</code></td>
        <td><code>roaringbitmap64</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>AND Aggregate calculations from a roaringbitmap64 set</td>
        <td><pre>select rb64_or_agg(bitmap)
    from (values (roaringbitmap64('{1,2,3}')),
                 (roaringbitmap64('{2,3,4}'))
          ) t(bitmap)</pre></td>
        <td><code>{1,2,3,4}</code></td>
    </tr>
    <tr>
        <td><code>rb64_and_agg</code></td>
        <td><code>roaringbitmap64</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>AND Aggregate calculations from a roaringbitmap64 set</td>
        <td><pre>select rb64_and_agg(bitmap)
    from (values (roaringbitmap64('{1,2,3}')),
                 (roaringbitmap64('{2,3,4}'))
          ) t(bitmap)</pre></td>
        <td><code>{2,3}</code></td>
    </tr>
    <tr>
        <td><code>rb64_xor_agg</code></td>
        <td><code>roaringbitmap64</code></td>
        <td><code>roaringbitmap64</code></td>
        <td>XOR Aggregate calculations from a roaringbitmap64 set</td>
        <td><pre>select rb64_xor_agg(bitmap)
    from (values (roaringbitmap64('{1,2,3}')),
                 (roaringbitmap64('{2,3,4}'))
          ) t(bitmap)</pre></td>
        <td><code>{1,4}</code></td>
    </tr>
    <tr>
        <td><code>rb64_or_cardinality_agg</code></td>
        <td><code>roaringbitmap64</code></td>
        <td><code>bigint</code></td>
        <td>OR Aggregate calculations from a roaringbitmap64 set, return cardinality.</td>
        <td><pre>select rb64_or_cardinality_agg(bitmap)
    from (values (roaringbitmap64('{1,2,3}')),
                 (roaringbitmap64('{2,3,4}'))
          ) t(bitmap)</pre></td>
        <td><code>4</code></td>
    </tr>
    <tr>
        <td><code>rb64_and_cardinality_agg</code></td>
        <td><code>roaringbitmap64</code></td>
        <td><code>bigint</code></td>
        <td>AND Aggregate calculations from a roaringbitmap64 set, return cardinality</td>
        <td><pre>select rb64_and_cardinality_agg(bitmap)
    from (values (roaringbitmap64('{1,2,3}')),
                 (roaringbitmap64('{2,3,4}'))
          ) t(bitmap)</pre></td>
        <td><code>2</code></td>
    </tr>
    <tr>
        <td><code>rb64_xor_cardinality_agg</code></td>
        <td><code>roaringbitmap64</code></td>
        <td><code>bigint</code></td>
        <td>XOR Aggregate calculations from a roaringbitmap64 set, return cardinality</td>
        <td><pre>select rb64_xor_cardinality_agg(bitmap)
    from (values (roaringbitmap64('{1,2,3}')),
                 (roaringbitmap64('{2,3,4}'))
          ) t(bitmap)</pre></td>
        <td><code>2</code></td>
    </tr>
</table>

# Cloud Vendor Support

`pg_roaringbitmap` is supported by the following cloud vendors 

- Alibaba Cloud RDS PostgreSQL: https://www.alibabacloud.com/help/doc-detail/142340.htm
- Huawei Cloud RDS PostgreSQL: https://support.huaweicloud.com/usermanual-rds-pg/rds_09_0045.html
- Tencent Cloud RDS PostgreSQL: https://cloud.tencent.com/document/product/409/67299
- Google Cloud SQL: https://docs.cloud.google.com/sql/docs/postgres/extensions

To request support for `pg_roaringbitmap` from other cloud vendors, please see the following:

- Amazon RDS: send an email to rds-postgres-extensions-request@amazon.com with the extension name and use case ([docs](https://aws.amazon.com/rds/postgresql/faqs/))
- DigitalOcean Managed Databases: comment on [this idea](https://ideas.digitalocean.com/app-framework-services/p/postgres-extension-request-pgroaringbitmap)
- Azure Database for PostgreSQL: comment on [this post](https://feedback.azure.com/d365community/idea/e6f5ff90-da4b-ec11-a819-0022484bf651)
