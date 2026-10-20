#!/bin/bash
# Both panels ready for snes-ds under sway (ROCKNIX), and back as they were:
#   screens.sh on    before snes-ds starts
#   screens.sh off   after it ends
# Stock ROCKNIX keeps the second panel off and unplaced. "on" turns it on and
# puts it to the right of the first, so one window can span both (snes-ds
# then lays its two screens side by side). "off" restores each panel's power
# and position. Does nothing without sway, swaymsg, jq or a second panel.

STATE=/tmp/snes-ds-screens.state
say() { echo "screens: $*"; }

command -v swaymsg >/dev/null 2>&1 && command -v jq >/dev/null 2>&1 || { say "no swaymsg/jq"; exit 0; }
if [ -z "$SWAYSOCK" ]; then
	for s in "$XDG_RUNTIME_DIR"/sway-ipc.*.sock /var/run/0-runtime-dir/sway-ipc.*.sock /run/user/*/sway-ipc.*.sock; do
		[ -S "$s" ] && export SWAYSOCK="$s" && break
	done
fi
[ -n "$SWAYSOCK" ] || { say "no sway"; exit 0; }

run() { swaymsg "$1" >/dev/null 2>&1 || say "swaymsg '$1' failed"; }
# name, active, powered, x, y, width (mode), focused: one panel per line
outputs() {
	swaymsg -r -t get_outputs | jq -r '.[] | [.name, .active, (if .power == null then .dpms else .power end),
		.rect.x, .rect.y, (.current_mode.width // .modes[0].width // 0), .focused] | @tsv'
}

case "$1" in
on)
	rm -f "$STATE"
	OUTS=$(outputs)
	[ -n "$OUTS" ] || exit 0
	say "before: $(echo "$OUTS" | tr '\t\n' ' ;')"
	# the first panel: ROCKNIX's main output, else the focused one
	FIRST="$WLR_CON"
	echo "$OUTS" | cut -f1 | grep -qx "$FIRST" 2>/dev/null || FIRST=$(echo "$OUTS" | awk -F'\t' '$7 == "true" { print $1; exit }')
	[ -n "$FIRST" ] || FIRST=$(echo "$OUTS" | head -n1 | cut -f1)
	# the second: another built-in panel (DSI/eDP, not HDMI)
	SECOND=$(echo "$OUTS" | awk -F'\t' -v f="$FIRST" '$1 != f && $1 ~ /^(DSI|eDP)/ { print $1; exit }')
	if [ -z "$SECOND" ]; then
		say "one panel ($FIRST): nothing to do"
		exit 0
	fi
	W=$(echo "$OUTS" | awk -F'\t' -v f="$FIRST" '$1 == f { print $6 }')
	{
		echo "FIRST=$FIRST"
		echo "SECOND=$SECOND"
		echo "$OUTS" | awk -F'\t' -v a="$FIRST" -v b="$SECOND" '$1 == a || $1 == b { print "PANEL\t" $1 "\t" $2 "\t" $3 "\t" $4 "\t" $5 }'
	} > "$STATE"
	run "output $SECOND enable"
	run "output $SECOND power on"
	run "output $FIRST pos 0 0"
	run "output $SECOND pos ${W:-0} 0"
	say "after: $(outputs | tr '\t\n' ' ;')"
	;;
off)
	[ -f "$STATE" ] || exit 0
	grep '^PANEL' "$STATE" | while IFS=$'\t' read -r _ name active power x y; do
		[ "$active" = "true" ] && run "output $name pos $x $y"
	done
	grep '^PANEL' "$STATE" | while IFS=$'\t' read -r _ name active power x y; do
		if [ "$active" != "true" ]; then
			run "output $name disable"
		elif [ "$power" = "false" ]; then
			run "output $name power off"
		fi
	done
	rm -f "$STATE"
	say "panels put back"
	;;
*)
	echo "usage: $0 on|off" >&2
	exit 1
	;;
esac
exit 0
