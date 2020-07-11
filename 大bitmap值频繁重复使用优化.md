# 大bitmap值频繁重复使用优化

## 问题

表中roaringbitmap类型的字段大小超过2K时会以toast形式存储，SQL中使用这些roaringbitmap字段时需要进行detoast。
如果一个SQL中detoast roaringbitmap数据的次数非常多，可能非常影响SQL性能。
下面是一个例子。

### 表定义

```
create table label as
  select grp1,rb_build_agg((random()*100000000)::int) member_ids
    from generate_series(1,1000000),generate_series(1,10)grp1
    group by grp1;

create table fact as
  select id,(random()*10)::int grp1, random() value1, random() value2
    from generate_series(1,10000) id;
```

### SQL

```
select a.grp1,rb_cardinality(rb_build_agg(id)), sum(value1) sum_value1,sum(value2) sum_value2
  from fact a join label b on(a.grp1 = b.grp1)
  where rb_contains(b.member_ids,id)
  group by a.grp1;
```

这个SQL执行时间是9秒
```
postgres=# explain analyze
select a.grp1,rb_cardinality(rb_build_agg(id)), sum(value1) sum_value1,sum(value2) sum_value2
  from fact a join label b on(a.grp1 = b.grp1)
  where rb_contains(b.member_ids,id)
  group by a.grp1;
                                                         QUERY PLAN                                                          
-----------------------------------------------------------------------------------------------------------------------------
 GroupAggregate  (cost=920.57..2259.19 rows=200 width=28) (actual time=1029.180..9151.787 rows=10 loops=1)
   Group Key: a.grp1
   ->  Merge Join  (cost=920.57..2043.51 rows=21268 width=24) (actual time=327.159..9151.051 rows=101 loops=1)
         Merge Cond: (b.grp1 = a.grp1)
         Join Filter: rb_contains(b.member_ids, a.id)
         Rows Removed by Join Filter: 9412
         ->  Sort  (cost=88.17..91.35 rows=1270 width=36) (actual time=0.016..0.020 rows=10 loops=1)
               Sort Key: b.grp1
               Sort Method: quicksort  Memory: 25kB
               ->  Seq Scan on label b  (cost=0.00..22.70 rows=1270 width=36) (actual time=0.007..0.009 rows=10 loops=1)
         ->  Sort  (cost=832.40..857.52 rows=10048 width=24) (actual time=7.433..15.522 rows=10000 loops=1)
               Sort Key: a.grp1
               Sort Method: quicksort  Memory: 1166kB
               ->  Seq Scan on fact a  (cost=0.00..164.48 rows=10048 width=24) (actual time=0.016..4.017 rows=10000 loops=1)
 Planning Time: 0.224 ms
 Execution Time: 9151.872 ms
(16 rows)

Time: 9153.537 ms (00:09.154)
```

## 原因

通过perf分析，可以看出绝大部分时间都耗在detoast上

```
-   99.05%     0.00%  postgres  roaringbitmap.so  [.] rb_exsit
     rb_exsit
   - pg_detoast_datum
      - detoast_attr
         - toast_fetch_datum
            - 98.67% table_relation_fetch_toast_slice
               + 61.83% heap_fetch_toast_slice
                 36.83% __memcpy_ssse3_back
```

## 优化方式1

SQL中反复对同一个roaringbitmap值进行解序列化，可以在roaringbitmap函数内部将解序列化的结果缓存起来。
`pg_roaringbitmap`的`rb_cache2`分支中支持了这个功能。使用示例如下：

```
select a.grp1,rb_cardinality(rb_build_agg(id)), sum(value1) sum_value1,sum(value2) sum_value2
  from fact a join label b on(a.grp1 = b.grp1)
  where rb_cache_contains(b.member_ids,id,b.grp1::text)
  group by a.grp1;
```

`rb_cache_contains`中需要传入缓存的key，下次以相同的key调用这个函数时，直接使用第一次缓存的值。

这个SQL的执行时间是87毫秒


## 优化方式2

可以先detoast roaringbitmap数据再通过CTE物化。

```
WITH b as MATERIALIZED(
  select grp1, member_ids::bytea::roaringbitmap from label
)
select a.grp1,rb_cardinality(rb_build_agg(id)), sum(value1) sum_value1,sum(value2) sum_value2
  from fact a join b on(a.grp1 = b.grp1)
  where rb_contains(b.member_ids,id)
  group by a.grp1;
```

这个SQL的执行时间是200毫秒。虽然比方式1慢，但这里包含了CTE物化的初始成本，重复的次数从1w变成10w以上时，2者的差距比较小。

## 小结

优化方式1需要考虑缓存溢出的问题，而且需要对`pg_roaringbitmap`有比较多的修改。
相对而言，优化方式2副作用更小。所以暂不把`rb_cache2`分支合并到master。
