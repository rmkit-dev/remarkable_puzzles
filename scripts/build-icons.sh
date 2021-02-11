#!/usr/bin/env bash
set -exo pipefail

make puzzle_icons

# Copy "official" save files to remarkable
ssh remarkable mkdir -p /opt/etc/puzzle-icons/
scp vendor/puzzles/icons/*.sav remarkable:/opt/etc/puzzle-icons/

# Generate icons
scp build/icons/puzzles remarkable:/opt/bin/puzzle-icons
ssh remarkable -t /opt/bin/rm2fb-client /opt/bin/puzzle-icons 300

scp 'remarkable:/opt/etc/puzzle-icons/*.png' build/icons/
mkdir -p icons
# Trim the white border and convert to grayscale
find build/icons/ -name '*.png' -exec sh -c \
  'convert "$0" -trim -colorspace Gray icons/$(basename "$0")' {} \;
