#!/bin/bash
# Compares two builds of the extension, for example master and a branch.
# Runs inside a postgres:<ver>-bookworm container with postgresql-server-dev-<ver>,
# make and gcc installed; see README.md for the docker command.
#
#   BASE_SRC, TEST_SRC  source trees of the two builds (default /src/base, /src/test)
#   ROUNDS              interleaved rounds (default 6); the order alternates each round
#   BUDGET_MS, MAX_CALLS  time and call limit per case (default 150, 2000)
#   FILTER, SHAPES, MINN  limit the cases (regex on case and shape, smallest n)
#   OUT                 log directory (default ./logs)
#   SKIP_SETUP          set to reuse the inputs of an earlier run
#
# Each run is a new psql session, so it loads the roaringbitmap.so copied just before.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
BASE_SRC=${BASE_SRC:-/src/base}
TEST_SRC=${TEST_SRC:-/src/test}
ROUNDS=${ROUNDS:-6}
OUT=${OUT:-$HERE/logs}
P="su postgres -c"
LIB=$(pg_config --pkglibdir)
for t in base test; do
  src=$BASE_SRC; [ $t = test ] && src=$TEST_SRC
  rm -rf /tmp/build-$t && cp -r "$src" /tmp/build-$t && chown -R postgres /tmp/build-$t
  $P "cd /tmp/build-$t && make clean >/dev/null 2>&1; make >/dev/null"
  mkdir -p /tmp/so/$t && cp /tmp/build-$t/roaringbitmap.so /tmp/so/$t/
done
# The inputs are built with the base build.
(cd /tmp/build-base && make install >/dev/null)
if [ ! -f /tmp/pgdata/postmaster.pid ]; then
  rm -rf /tmp/pgdata
  $P "initdb -D /tmp/pgdata >/dev/null && pg_ctl -D /tmp/pgdata -l /tmp/pg.log -o '-c work_mem=256MB -c jit=off' -w start >/dev/null"
fi
$P "createdb bench" 2>/dev/null || true
$P "psql -qX -d bench -c 'create extension if not exists roaringbitmap'"
[ -n "$SKIP_SETUP" ] || $P "psql -qX -d bench -f $HERE/setup.sql"
mkdir -p "$OUT"
for r in $(seq 1 "$ROUNDS"); do
  if [ $((r % 2)) -eq 1 ]; then order="base test"; else order="test base"; fi
  for t in $order; do
    rm -f "$LIB/roaringbitmap.so" && cp /tmp/so/$t/roaringbitmap.so "$LIB/roaringbitmap.so"
    $P "psql -qX -d bench -v filter='${FILTER:-.}' -v shapes='${SHAPES:-.}' -v minn=${MINN:-0} \
        -v budget_ms=${BUDGET_MS:-150} -v max_calls=${MAX_CALLS:-2000} -f $HERE/bench.sql" > "$OUT/round$r-$t.txt"
    echo "round $r $t done $(date +%T)"
  done
done
python3 "$HERE/summarize.py" "$OUT"
