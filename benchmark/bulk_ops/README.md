# Bulk-call benchmark

Compares two builds of the extension (for example `master` and a branch) on the functions that use CRoaring bulk calls: `build`, `to_array`, `range`, `range_cardinality`, `select`, `shiftright` and the 32/64-bit casts.

- `setup.sql` builds the inputs once with the base build and stores them as `bytea`, so both builds read the same bytes.
  For n in 1, 100, 10,000, 500,000 and 5,000,000 it makes a dense (`1..n`) and a sparse (random over the full int4/int8 range) input.
- `bench.sql` times each case in a PL/pgSQL loop for the build loaded in the session: one warm-up call, then repeated calls until the time budget or the call limit is reached.
  It prints `case|shape|n|calls|us_per_call`.
- `run_bench.sh` builds both trees, then runs interleaved rounds (the order alternates each round), each in a new `psql` session after copying that build's `roaringbitmap.so` into place.
- `summarize.py` prints the median microseconds per call of each build and the speedup (base / test).

## Run

From a directory that holds two checkouts, `base` (for example `master`) and `test` (the branch):

```sh
docker run --rm -v "$PWD":/src postgres:18-bookworm bash -c '
  apt-get update -qq && apt-get install -y -qq make gcc postgresql-server-dev-18 python3 >/dev/null &&
  ROUNDS=6 /src/test/benchmark/bulk_ops/run_bench.sh'
```

Useful settings: `ROUNDS=12 BUDGET_MS=200 MAX_CALLS=50000` for a more stable run, `FILTER=range SHAPES=sparse MINN=10000` to limit the cases, and `SKIP_SETUP=1` to reuse the inputs.
Run on an idle machine; differences of a few percent are within the round-to-round spread.
