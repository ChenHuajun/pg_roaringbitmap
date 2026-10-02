-- One round for the build loaded in this session. Prints: case|shape|n|calls|us_per_call
-- psql variables: filter, shapes (regex on case and shape), minn (smallest n),
-- budget_ms and max_calls (time and call limit per case).
\set ON_ERROR_STOP 1
\pset format unaligned
\pset tuples_only on
create or replace function pg_temp.bt(q text, a32 int[], a64 bigint[], r32 roaringbitmap, r64 roaringbitmap64,
                                      lo32 bigint, hi32 bigint, slo32 bigint, shi32 bigint,
                                      lo64 bigint, hi64 bigint, slo64 bigint, shi64 bigint, n int, r3264 roaringbitmap64,
                                      budget interval, max_calls int)
returns table(calls int, us float8) language plpgsql as $$
declare t0 timestamptz; k int := 0; dummy bigint;
  -- local copies: assignment detoasts once, so calls do not pay for it
  v32 int[] := a32; v64 bigint[] := a64; b32 roaringbitmap := r32; b64 roaringbitmap64 := r64;
  b3264 roaringbitmap64 := r3264;
begin
  -- warm up once, then repeat until the time budget is used
  execute q into dummy using v32, v64, b32, b64, lo32, hi32, slo32, shi32, lo64, hi64, slo64, shi64, n, b3264;
  t0 := clock_timestamp();
  loop
    execute q into dummy using v32, v64, b32, b64, lo32, hi32, slo32, shi32, lo64, hi64, slo64, shi64, n, b3264;
    k := k + 1;
    exit when clock_timestamp() - t0 > budget or k >= max_calls;
  end loop;
  calls := k;
  us := extract(epoch from clock_timestamp() - t0) * 1e6 / k;
  return next;
end $$;

-- $1 arr32 $2 arr64 $3 r32 $4 r64 $5/$6 half range 32 $7/$8 small range 32 $9/$10 half range 64 $11/$12 small range 64 $13 n
-- $14 the 32-bit bitmap as roaringbitmap64 (members fit in int4)
create temp table cases(c text, q text);
insert into cases values
 ('rb_to_array',           'select cardinality(rb_to_array($3))'),
 ('rb_build',              'select octet_length(rb_build($1)::bytea)'),
 ('rb_range_half',         'select octet_length(rb_range($3,$5,$6)::bytea)'),
 ('rb_range_small',        'select octet_length(rb_range($3,$7,$8)::bytea)'),
 ('rb_range_cardinality',  'select rb_range_cardinality($3,$5,$6)'),
 ('rb_select_fwd',         'select octet_length(rb_select($3,$13/2,$13/4,false)::bytea)'),
 ('rb_select_rev',         'select octet_length(rb_select($3,$13/2,$13/4,true)::bytea)'),
 ('rb_select_rev_small',   'select octet_length(rb_select($3,10,0,true)::bytea)'),
 ('rb_shiftright',         'select octet_length(rb_shiftright($3,12345)::bytea)'),
 ('rb_to_rb64_cast',       'select octet_length(($3::roaringbitmap64)::bytea)'),
 ('rb64_to_array',         'select cardinality(rb64_to_array($4))'),
 ('rb64_build',            'select octet_length(rb64_build($2)::bytea)'),
 ('rb64_range_half',       'select octet_length(rb64_range($4,$9,$10)::bytea)'),
 ('rb64_range_small',      'select octet_length(rb64_range($4,$11,$12)::bytea)'),
 ('rb64_range_cardinality','select rb64_range_cardinality($4,$9,$10)'),
 ('rb64_select_fwd',       'select octet_length(rb64_select($4,$13/2,$13/4,false)::bytea)'),
 ('rb64_select_rev',       'select octet_length(rb64_select($4,$13/2,$13/4,true)::bytea)'),
 ('rb64_select_rev_small', 'select octet_length(rb64_select($4,10,0,true)::bytea)'),
 ('rb64_shiftright',       'select octet_length(rb64_shiftright($4,12345)::bytea)'),
 ('rb64_to_rb_cast',       'select octet_length(($14::roaringbitmap)::bytea)');

-- range bounds: "half" keeps the members between the 25th and 75th percentile,
-- "small" keeps about 100 members around the median (unsigned order)
create temp table inp as
select shape, n, arr32, arr64, b32::roaringbitmap as r32, b64::roaringbitmap64 as r64, b3264::roaringbitmap64 as r3264,
       (u32[1 + n/4])::bigint as lo32, (u32[1 + (3*n)/4])::bigint as hi32,
       (u32[1 + greatest(n/2 - 50, 0)])::bigint as slo32, (u32[1 + least(n/2 + 50, n - 1)])::bigint + 1 as shi32,
       s64[1 + n/4] as lo64, s64[1 + (3*n)/4] as hi64,
       s64[1 + greatest(n/2 - 50, 0)] as slo64, s64[1 + least(n/2 + 50, n - 1)] + 1 as shi64
  from (select shape, n, arr32, arr64, b32, b64, b3264,
               array(select (x::bigint + 4294967296) % 4294967296 from unnest(rb_to_array(b32::roaringbitmap)) x) as u32,
               rb64_to_array(b64::roaringbitmap64) as s64
          from bin) s;

select c, shape, n, b.calls, b.us
  from cases, inp, lateral pg_temp.bt(q, arr32, arr64, r32, r64, lo32, hi32, slo32, shi32, lo64, hi64, slo64, shi64, n, r3264,
                         make_interval(secs => :budget_ms / 1000.0), :max_calls) b
 where c ~ :'filter' and shape ~ :'shapes' and n >= :minn
 order by c, shape, n;
