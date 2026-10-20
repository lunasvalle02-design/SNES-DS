#!/bin/bash
# Every game's tests, in one go: what a change has to pass before it's merged.
#   scripts/check.sh ROMDIR [ID...]     (default: every game in games/)
#
# For each game with a tests/order file, the ROM is found in ROMDIR by its
# profile (harness --find), and the scripts listed there run in that order
# with one output folder (out/check/ID), so later ones can use the states
# earlier ones leave. A script NAME.txt with a NAME.expected beside it must
# print exactly those "hash" lines; any other must exit 0. A game whose ROM
# isn't in ROMDIR is skipped, and said so. Then the app itself runs without a
# window through tools/app_tests/*.txt (the menu opened and closed with a
# virtual controller), when it was built.
#
# The harness is build/harness (or build/harness.exe); HARNESS=... to use
# another one. Exits 1 if anything failed.
set -u
cd "$(dirname "$0")/.."

ROMS="${1:?usage: scripts/check.sh ROMDIR [ID...]}"
shift
HARNESS="${HARNESS:-}"
if [ -z "$HARNESS" ]; then
	for h in build/harness build/harness.exe; do
		[ -x "$h" ] && HARNESS="$h" && break
	done
fi
[ -n "$HARNESS" ] || { echo "no harness: build first (cmake --build build)"; exit 1; }

if [ $# -gt 0 ]; then
	IDS="$*"
else
	IDS=$(for d in games/*/; do basename "$d"; done)
fi

failed=0 ran=0 skipped=0
for id in $IDS; do
	order="games/$id/tests/order"
	[ -f "$order" ] || continue
	rom=$("$HARNESS" --find "$id" "$ROMS" 2>/dev/null) || {
		echo "skip  $id: no ROM for it in $ROMS"
		skipped=$((skipped + 1))
		continue
	}
	out="out/check/$id"
	rm -rf "$out"
	mkdir -p "$out"
	while read -r script; do
		case "$script" in "" | "#"*) continue ;; esac
		path="games/$id/tests/$script"
		expected="${path%.txt}.expected"
		log="$out/${script%.txt}.log"
		"$HARNESS" "$rom" "$out" "$path" > "$log" 2>&1
		status=$?
		if [ $status -eq 0 ] && [ -f "$expected" ]; then
			if ! diff <(grep -v '^#' "$expected") <(grep '^hash' "$log") > "$out/${script%.txt}.diff"; then
				status=1
				echo "      hashes differ: $out/${script%.txt}.diff"
			fi
		fi
		ran=$((ran + 1))
		if [ $status -eq 0 ]; then
			echo "ok    $id/$script"
		else
			echo "FAIL  $id/$script (log: $log)"
			failed=$((failed + 1))
		fi
	done < "$order"
done

# the app itself, without a window: tools/app_tests/NAME.txt drives it (the
# first game in ROMDIR's list) and its "menu:" lines must be NAME.expected
APP=""
for b in build/snes-ds build/snes-ds.exe; do
	[ -x "$b" ] && APP="$b" && break
done
if [ -z "$APP" ]; then
	echo "skip  app tests: no build/snes-ds (built only with SDL2)"
else
	mkdir -p out/check/app
	for script in tools/app_tests/*.txt; do
		name=$(basename "$script" .txt)
		log="out/check/app/$name.log"
		rm -f out/check/app/test.cfg
		SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy "$APP" --no-audio --windowed --config=out/check/app/test.cfg \
			--rom-dir="$ROMS" --save-dir=out/check/app --state-dir=out/check/app --script="$script" > "$log" 2>&1
		ran=$((ran + 1))
		if grep '^menu:' "$log" | diff "tools/app_tests/$name.expected" - > "out/check/app/$name.diff"; then
			echo "ok    app/$name.txt"
		else
			echo "FAIL  app/$name.txt (log: $log, diff: out/check/app/$name.diff)"
			failed=$((failed + 1))
		fi
	done
fi

echo "$ran run, $failed failed, $skipped games skipped"
[ $failed -eq 0 ]
