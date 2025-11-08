#!/bin/bash

psql -f benchmark_rb64.sql $*>benchmark_rb64.log
rc=$?

if [[ $rc -ne 0 ]]; then
  exit $rc
fi

dbinfo=`psql $* -c "select now() now, extversion roaringbitmap,(select setting pg_version from pg_settings where name='server_version') from pg_extension where extname='roaringbitmap'"`

echo "$dbinfo"

echo '| No | test | time(ms) |'
echo '| -- | ---- | -------- |'

count=0
cat benchmark_rb64.log | while read line
do
  if [[ $line =~ ^rb64_ ]]; then
    let count++
    echo -n "| $count | $line "
  elif [[ $line =~ 'Execution '[Tt]'ime: ' ]]; then
    array=($line)
    echo "| ${array[2]} |"
  fi
done

grep "Total time" benchmark_rb64.log

