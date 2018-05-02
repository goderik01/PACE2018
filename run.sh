#!/bin/bash

: ${TIMEOUT:=1800} 

trap_int() {
  echo "  INTERRUPTED" >&2
  kill %1 &>/dev/null
  trap "kill -9 %1 &>/dev/null; echo '' >&2" INT
}

bin/star_contractions_test "$@" <&0 &
trap "kill -9 %1 &>/dev/null" EXIT
trap trap_int INT

for ((i = 0; i < TIMEOUT; i++)); do
  jobs %1 &>/dev/null
  jobs %1 &>/dev/null || exit 1
  sleep 1
done

kill %1 &>/dev/null
wait %1 &>/dev/null

