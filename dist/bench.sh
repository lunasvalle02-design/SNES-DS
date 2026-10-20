#!/bin/bash
# How fast the emulator runs on this device, without a window:
#   bench.sh [ROM]   (default: the first game in the SNES folder with a profile)
# Runs the game's attract mode with the harness and writes bench.log next to
# this script. 60 fps is full speed; the map screen's hidden core needs more
# room on top of that (about a second of game every 1.5 s, in bursts).

DIR="$(cd "$(dirname "$0")" && pwd)"
ROMS="${SNESDS_ROMS:-/storage/roms/snes}"
LOG="$DIR/bench.log"
cd "$DIR"

ROM="$1"
if [ -z "$ROM" ]; then
	ROM=$(./snes-ds --find-in="$ROMS" --print-rom 2>/dev/null)
fi
[ -n "$ROM" ] || { echo "no ROM (give one: bench.sh /path/Game.sfc)" | tee "$LOG"; exit 1; }

OUT=$(mktemp -d)
cat > "$OUT/bench.txt" <<'EOF'
echo one thread, the core alone (title and attract mode):
wait 600
bench 1800
echo one thread, with a HUD split (BG3, lines 0-64):
split 4 0 64
bench 1800
split 0
echo drawing on its own thread (as the app does), the core alone:
threads on
bench 1800
echo drawing on its own thread, with a HUD split:
split 4 0 64
bench 1800
EOF
{
	echo "snes-ds $(cat "$DIR/VERSION" 2>/dev/null) bench on ${QUIRK_DEVICE:-unknown device}, $(date)"
	grep -m1 "model name\|Hardware" /proc/cpuinfo
	echo "cores: $(nproc)"
	for c in /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor; do
		[ -f "$c" ] && echo "$(basename "$c"): $(cat "$c")"
	done
	echo "ROM: $ROM"
	./harness "$ROM" "$OUT" "$OUT/bench.txt" 2>&1 | grep -v "^Loaded\|banks\|^\""
} | tee "$LOG"
rm -rf "$OUT"
