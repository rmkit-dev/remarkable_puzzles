#!/usr/bin/env bash

# Very basic html -> markdown converter for puzzle help files
PERL_SCRIPT='
    s#\n# #g;

    s#<strong>(.*?)</strong>#*$1*#g;
    s#<em>(.*?)</em>#_$1_#g;
    s#<i>(.*?)</i>#_$1_#g;
    s#<code>(.*?)</code>#`$1`#g;

    s#<ul>\s*(.*?)\s*</ul>#\n$1\n#g;
    s#<li>\s*(.*?)\s*</li>#\n- $1#g;
    s#<li>\s*#\n- #g;

    s#\s*<p>\s*(.*?)\s*</p>\s*#$1\n\n#g;
    s#\s*<p>\s*#\n\n#g;
'

build_1() {
  outf="help/$(basename "${1%%.html}").txt"
  if [[ -f "$outf" ]]; then
      echo "$outf already exists; skipping"
  else
      echo "converting $1 -> $outf"
      perl -0777 -pe "$PERL_SCRIPT" "$1" > "$outf"
  fi
}

if [[ -n "$1" ]]; then
  for f in "$@"; do
    build_1 "vendor/puzzles/html/$f.html"
  done
else
  for f in vendor/puzzles/html/*.html; do
    build_1 "$f"
  done
fi
