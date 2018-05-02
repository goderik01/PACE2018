#!/bin/bash

for f in "$@"; do
  perl -pe 's{\t}{  }g; s{^((?:  )*)}{"\t" x (length($1)/2)}e; s{^#((?:  )*)}{"#" . ("\t" x (length($1)/2))}e' \
    < "$f" | sed -re 's/\s*$//' > .fix_ident.tmp
  mv .fix_ident.tmp "$f"
done

