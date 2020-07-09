#!/bin/bash

psql -f benchmark.sql $*>benchmark.log
rc=$?

if [[ $rc -ne 0 ]]; then
  exit $rc
fi

dbinfo=`psql $* -c "select now() now, extversion roaringbitmap,(select setting pg_version from pg_settings where name='server_version') from pg_extension where extname='roaringbitmap'"`

echo "$dbinfo"

echo '| No | test | time(ms) |'
echo '| -- | ---- | -------- |'

count=0
cat benchmark.log | while read line
do
  if [[ $line =~ ^rb_ ]]; then
    let count++
    echo -n "| $count | $line "
  elif [[ $line =~ 'Execution '[Tt]'ime: ' ]]; then
    array=($line)
    echo "| ${array[2]} |"
  fi
done
