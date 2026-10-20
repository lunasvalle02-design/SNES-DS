#!/bin/bash
# The Ports entry for snes-ds (ROCKNIX): plays the first game in the SNES
# folder that has a second-screen profile.
exec "$(dirname "$0")/snes-ds/launch.sh" "$@"
