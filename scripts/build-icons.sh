#!/usr/bin/env bash
set -exo pipefail

make puzzle_icons

# Copy "official" save files to remarkable
ssh remarkable mkdir -p /opt/etc/puzzle-icons/
scp vendor/puzzles/icons/*.sav remarkable:/opt/etc/puzzle-icons/

# Generate icons
scp build/icons/puzzles remarkable:/opt/bin/puzzle-icons
ssh remarkable -t /opt/bin/rm2fb-client /opt/bin/puzzle-icons 300

# Copy icons from remarkable, center them, and save to icons/
scp 'remarkable:/opt/etc/puzzle-icons/*.png' build/icons/
mkdir -p icons
find build/icons/ -name '*.png' -exec sh -c \
  'convert "$0" -gravity center -background white -extent 300x300 icons/$(basename "$0")' {} \;
