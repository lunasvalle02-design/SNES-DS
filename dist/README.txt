SNES DS for ROCKNIX (Anbernic RG DS Plus / RG DS)
=================================================
(En español: README_ES.txt)

Installing
----------
Unzip the zip into /storage/roms/ports/. You'll get:
  /storage/roms/ports/SNES DS.sh       the entry in Ports
  /storage/roms/ports/snes-ds/         the program

ROMs go where they always do: /storage/roms/snes/ (.sfc, .smc or .zip,
subfolders too). "SNES DS" in Ports opens the list of the games in that
folder that have a dual-screen profile (for now The Legend of Zelda: A Link
to the Past, USA; Super Metroid, Japan/USA; Super Castlevania IV, USA). If
EmulationStation has downloaded covers (gamelist.xml, images/), the list
uses them; otherwise each game shows its own icon.
If it doesn't show up in Ports, restart EmulationStation.

From a terminal (SSH) a game can be started directly too:
  /storage/roms/ports/snes-ds/launch.sh "/storage/roms/snes/Game.sfc" [options]

The game list
-------------
On the bottom (touch) screen: tap a game to play it. With the buttons: the
d-pad chooses, A plays, X opens the settings; the settings and the quit
button are at the top.

The menu (in a game)
--------------------
Clicking the left stick (L3) during a game pauses it and opens the menu on
the bottom screen: resume, save and load a state (4 slots, each with its
picture), settings, quit the game (back to the list) or quit SNES DS. The
R button (home), a tap on the bottom screen, R3 and Select + Start open it
too. B, L3 or R go back to the game.

If no menu button works, snes-ds.log lists the input devices and the first
presses ("input: ..."): open an issue on GitHub with it (or try
--menu-button=NAME in launch.sh yourself).

Settings
--------
Aspect ratio (original / 4:3), swap screens, HUD and menus below, map on
the bottom screen, picture behind the HUD, volume, show FPS, language and
controls. They're kept in snes-ds/snes-ds.cfg and apply to every game.

Picture behind the HUD: when the second screen has only the HUD (a game
with no map, such as Super Castlevania IV, or one with no profile), the
game's cover or icon fills the rest, darkened. For a picture of your own, put a PNG or JPG in
snes-ds/backgrounds/, named after the game's id or the ROM's file name:
  snes-ds/backgrounds/super_castlevania_iv.png
  snes-ds/backgrounds/Super Castlevania IV (USA).jpg
(ids: zelda_alttp, super_metroid, super_castlevania_iv)

Controls: choose a SNES button (B, A, Y, X, L, R, Start, Select or the
menu), press A, then the console's button you want for it (a trigger
works too). If another button had it, they swap. If a button doesn't do
what it should (Select, say), give it one here. "Reset" puts everything
back as it came.

The interface starts in English. For Spanish: Settings, the Language row,
Español.

In-game controls
----------------
The SNES buttons, by position: bottom = B, right = A, top = X, left = Y
(unless moved in Settings > Controls).
Shortcuts (Select held + another button):
  Select + Start      the menu        Select + X   swap the screens
  L1 + Start + Select quit            Select + Y   aspect: original / 4:3
  Select + R          save a state    Select + A   HUD on the 2nd screen on/off
  Select + L          load a state    Select + Right  fast forward on/off
The shortcuts' save and load use slot 1, and happen when R or L is let go.
Select on its own still reaches the game when it's let go (or when held
for half a second).

Useful options (in launch.sh, after the ROM if there is one)
------------------------------------------------------------
  --no-map           no hidden map core, whatever the setting says
  --no-threads       draw on the emulation's thread (by default drawing
                     runs on another CPU core, which is faster)
  --layout=stack     if the game and the 2nd screen are placed wrong
  --menu-button=X    the menu's button (an SDL name, or joy:N)

Saves
-----
The game's battery save (.srm) is kept beside the ROM, in RetroArch's
format. Save states go in snes-ds/states/.

Speed: what to look at
----------------------
1. snes-ds/snes-ds.log: every 10 seconds a line like
     stats: 60.0 fps, emulation 9.80 ms/frame (max 14.20), drawing 1.10 ms,
            audio underruns 0, map captures 12 (850 ms avg, 1200 max)
   - fps: 60 is full speed.
   - emulation: has to stay under 16.6 ms.
   - audio underruns: sound dropouts (should be 0).
   - map captures: how long the hidden core takes to get the map.
2. snes-ds/bench.sh: measures the emulator's speed without a window, into
   snes-ds/bench.log. Run it over SSH:
     /storage/roms/ports/snes-ds/bench.sh
For a speed problem, attach both files (snes-ds.log and bench.log) to an
issue on GitHub.

If it doesn't start, snes-ds.log says why (and what screens.sh did with the
screens).

License: GPL-3.0 or later (LICENSE). Third-party licenses in licenses/.
https://github.com/lunasvalle02-design/SNES-DS
