# Changelog

**English** · [Español](CHANGELOG_ES.md)

What each version of SNES DS brought. Versions follow [semantic versioning](https://semver.org); before 1.0 anything
may still change (see [what 1.0 promises](ROADMAP.md#the-road-to-10)). Downloads are on the
[releases page](https://github.com/lunasvalle02-design/SNES-DS/releases).

## 1.1.0

### Added
- **A picture behind the HUD**: when the second screen has only the HUD, or nothing (a game with no map, such as Super
  Castlevania IV, a game with no profile, or before the first map capture), the game's cover or icon fills the rest,
  darkened so the HUD reads. A picture of the player's own goes in `snes-ds/backgrounds/<id or ROM name>.png` (or
  `.jpg`). On by default; Settings → Picture behind the HUD turns it off.

## 1.0.0

The first stable version: 0.6.0, used on the console for some days without problems. From here on, what
[1.0 promises](ROADMAP.md#the-road-to-10) holds (the `snes-ds.cfg` format, battery saves and save states staying
loadable, the command-line options and the profile format); breaking any of it would mean 2.0. Fixes come as 1.0.x.

- The README's screenshots show the current interface: the games' icons and the controls page.

## 0.6.0 — the release candidate

### Added
- **Controls in the settings** (Settings → Controls): each SNES button (B, A, Y, X, L, R, Start, Select) and one more
  menu button can be moved to any of the console's buttons, a trigger or a key, by pressing it. A button another one
  had swaps with it; Reset puts them all back. On the RG DS Plus this gives Select a button that reaches the game.
- **An icon for each game** in the game list, an original drawing per game (a cover EmulationStation downloaded still
  comes first). `tools/icon.py` draws them from a short description; new games bring theirs (AGENTS.md).
- **CI on GitHub Actions**: every pull request builds for PC (no warnings allowed) and for ROCKNIX; a `v*` tag puts the
  ROCKNIX package on its release.
- The device instructions in English (`README.txt` in the zip), with the Spanish ones beside them (`README_ES.txt`).
- This changelog.

### Changed
- **A Link to the Past: the HUD stays on the game screen**; the bottom screen shows the live map alone, which reads
  better over the dungeon maps.

### Checked on the console
- Sleep and resume during a game.
- The new controls page and the icons.

## 0.5.0

### Added
- **Super Metroid** (Japan and USA, v1.0): the pause map of the area Samus is in, refreshed as she explores, with the
  HUD (energy, ammo, minimap) over it on the bottom screen.
- **Super Castlevania IV** (USA, v1.0): the HUD on the bottom screen, in play and paused.
- Profiles can crop the game screen while their HUD is on the second screen (`gameShowY0/Y1`), so Super Metroid's room
  fills the top screen.

## 0.4.0-dev
- Checked on the console: 60 fps with no audio dropouts; the menu opens with L3 or the R (home) button.
- Battery saves, states and settings written safely: a power cut can't corrupt them.
- The speed log no longer counts the time spent in the menu.

## 0.3.0-dev
- The touch interface, in English and Spanish: the game list, the pause menu, four save-state slots with pictures,
  the settings. The menu also opens with a tap on the bottom screen or any spare button.
- Fixed: a burst of noise when a game started.

## 0.2.0-dev
- The core about 3.7× faster (bit-identical); a cheaper map capture; L1 + Start + Select to quit.

## 0.1.0-dev
- The first build for ROCKNIX, with A Link to the Past. Measured on the console: 18–39 ms per emulated frame
  (25–50 fps), audio dropouts.
