-- Inputs for the bulk-ops benchmark, built once with the master build and stored as
-- bytea, so both builds read byte-identical inputs.
\set ON_ERROR_STOP 1
drop table if exists bin;
create table bin(w int, shape text, n int, arr32 int[], arr64 bigint[], b32 bytea, b64 bytea, b3264 bytea);
select setseed(0.17);
insert into bin(w, shape, n, arr32, arr64)
select 0, shape, n,
       case when shape = 'dense' then array(select g::int from generate_series(1, n) g)
            else array(select (random()*4294967295 - 2147483648)::bigint::int from generate_series(1, n)) end,
       case when shape = 'dense' then array(select g::bigint from generate_series(1, n) g)
            else array(select ((random()*2 - 1) * 9223372036854775807)::numeric::bigint from generate_series(1, n)) end
  from unnest('{1,100,10000,500000,5000000}'::int[]) n, unnest('{dense,sparse}'::text[]) shape;
update bin set b32 = rb_build(arr32)::bytea, b64 = rb64_build(arr64)::bytea;
update bin set b3264 = (b32::roaringbitmap::roaringbitmap64)::bytea;
alter table bin alter arr32 set storage external, alter arr64 set storage external,
                alter b32 set storage external, alter b64 set storage external, alter b3264 set storage external;
vacuum full analyze bin;
