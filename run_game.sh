#!/usr/bin/env bash

set -e

cmake .
make
time ./halite --replay-directory replays/ -vv --no-compression --no-logs --width 64 --height 64 "./MyBot" "./Ramp40" "./Ramp24" "./Ramp45"
