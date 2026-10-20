#!/bin/bash
# snes-ds on ROCKNIX (RG DS Plus / RG DS).
#   launch.sh                  the first game in the SNES folder with a profile
#   launch.sh /path/Game.sfc   that game (.sfc, .smc or .zip)
#   any --option of snes-ds can follow (see README.txt)
# Everything snes-ds prints goes to snes-ds.log next to this script.

. /etc/profile >/dev/null 2>&1

DIR="$(cd "$(dirname "$0")" && pwd)"
LOG="$DIR/snes-ds.log"
ROMS="${SNESDS_ROMS:-/storage/roms/snes}"
cd "$DIR"
mkdir -p "$DIR/states"

# the controller mappings ROCKNIX ships
for db in /storage/.config/SDL-GameControllerDB/gamecontrollerdb.txt \
          /usr/config/SDL-GameControllerDB/gamecontrollerdb.txt; do
	if [ -f "$db" ]; then
		export SDL_GAMECONTROLLERCONFIG_FILE="$db"
		break
	fi
done

# sway's socket: snes-ds asks sway to span its window over both panels
if [ -z "$SWAYSOCK" ]; then
	for s in "$XDG_RUNTIME_DIR"/sway-ipc.*.sock /var/run/0-runtime-dir/sway-ipc.*.sock /run/user/*/sway-ipc.*.sock; do
		[ -S "$s" ] && export SWAYSOCK="$s" && break
	done
fi

{
	echo "snes-ds $(cat "$DIR/VERSION" 2>/dev/null) on ${QUIRK_DEVICE:-unknown device}, $(date)"
	echo "SWAYSOCK=${SWAYSOCK:-none}"
} > "$LOG"

# both panels on and side by side for the run, put back afterwards (also
# when snes-ds is killed)
"$DIR/screens.sh" on >> "$LOG" 2>&1
trap '"$DIR/screens.sh" off >> "$LOG" 2>&1' EXIT
trap 'exit 143' TERM INT HUP

if [ $# -gt 0 ] && [ "${1#--}" = "$1" ]; then
	./snes-ds --state-dir="$DIR/states" "$@" >> "$LOG" 2>&1
else
	./snes-ds --state-dir="$DIR/states" --find-in="$ROMS" "$@" >> "$LOG" 2>&1
fi
echo "exit $?" >> "$LOG"
