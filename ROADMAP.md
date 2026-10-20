# SNES DS roadmap

**English** · [Español](ROADMAP_ES.md)

The goal is a SNES emulator for dual-screen handhelds (Anbernic RG DS Plus, RG DS) that uses the second screen for
each game's map, menus or HUD, after the model of [gba-ds](https://github.com/kyannon/gba-ds). The feasibility study
that started it is in [RESEARCH.md](RESEARCH.md).

Legend: ✅ done · 🔶 in progress · ⬜ to do

## Status at a glance

| Phase | | Status |
|---|---|---|
| 0 | Research | ✅ |
| 1 | Core and harness on PC | ✅ |
| 2 | Desktop app with two screens, game list and pause menu | ✅ |
| 3 | Second screen for A Link to the Past | 🔶 A fallback screen is missing |
| 4 | The device (ROCKNIX) | ✅ Full speed on the RG DS Plus; input and touch confirmed |
| 5 | Touch and comfort | ✅ Closed for now |
| 6 | More games | 🔶 Super Metroid and Super Castlevania IV added; each game has an icon |
| 7 | Integration (EmulationStation, RetroAchievements) | ⬜ |

## Versions

| Version | What it brought |
|---|---|
| 0.1.0-dev | The first build for ROCKNIX. Measured on the console: 18-39 ms per emulated frame (25-50 fps), audio dropouts |
| 0.2.0-dev | The core about 3.7× faster (bit-identical); a cheaper map capture; L1 + Start + Select to quit |
| 0.3.0-dev | The touch interface, in English and Spanish: game list, pause menu, four save-state slots with screenshots, settings. The menu also opens with a tap on the bottom screen or any spare button. Fixed: a burst of noise when a game started |
| 0.4.0-dev | Checked on the console: 60 fps with no audio dropouts; the menu opens with L3 or the R (home) button. Saves, states and settings written safely (a power cut can't corrupt them); the speed log no longer counts time in the menu |
| 0.5.0 | Two more games, each on its own branch: Super Metroid (the pause map, with the HUD over it) and Super Castlevania IV (the HUD). A profile can crop the game screen while its HUD is below, so Super Metroid's room fills the top screen. Both checked on the console |
| 0.6.0 | The release candidate. Controls remapped in the settings; an icon for each game in the list; A Link to the Past keeps its HUD on the game screen. CI on GitHub Actions, `CHANGELOG.md`, the device instructions in English. Sleep and resume checked on the console |
| 1.0.0 | The first stable version: the release candidate, used on the console for some days without problems |

## The road to 1.0

| Version | Goal | Status |
|---|---|---|
| 0.4 | Checked on the console: speed, input, touch; saves that a power cut can't corrupt | ✅ |
| 0.5 | A second and third game, on `game/…` branches (Super Metroid, Super Castlevania IV), with whatever engine hooks they need; the profile format settles | ✅ |
| 0.6 (release candidate) | Sleep and resume, long sessions, CI on GitHub Actions, `CHANGELOG.md`, GitHub releases, the device instructions in English | ✅ Long sessions are what the days before 1.0 check |
| 1.0.0 | The release candidate used on the console for some days without problems | ✅ |

What 1.0 promises, once out (semantic versioning: breaking any of it means 2.0): the `snes-ds.cfg` format, battery
saves and save states staying loadable, the command-line options, and the profile format game modules are written
in.

## Decisions

| Topic | Decision | Why |
|---|---|---|
| Core | **LakeSnes** (MIT, C, vendored in `third_party/lakesnes`) | Its license allows reusing gba-ds code (GPL-3). It's small and readable (~7,700 lines), so it can be patched to split layers, and zelda3 uses it to verify A Link to the Past |
| Isolating the core | All code goes through [`src/core.h`](src/core.h) | Should LakeSnes fall short on the device, another core (bsnes-jg, Snes9x…) can replace it without touching anything else |
| Layer split | A picture-unit patch: `hideMask` (hide layers per line) and `splitMask` (draw layers into a second picture per line) | The counterpart of gba-ds's `emu_hud_split`, but exact to the scanline and almost free (~3 %) |
| Telling games apart | The CRC32 of the ROM without a copier header | SNES headers carry no reliable game code |
| Per-game code | Only in `games/<id>/`; the engine holds no game data | Games never mix; aspect, layout and the interface are generic app features |
| Build | CMake + Ninja; MSYS2 GCC (UCRT64) on PC, an aarch64 toolchain in Docker for the device | |
| License | GPL-3.0-or-later | Allows reusing gba-ds code |

## Phase 0: Research ✅

- ✅ Analysis of gba-ds: architecture, profiles, map capture, HUD split, ROCKNIX deployment.
- ✅ The reference ROM checked: A Link to the Past USA v1.0, CRC32 `777AAC2F`, SHA-1 `6D4F10A8…`.
- ✅ A Link to the Past's RAM map, from zelda3.

## Phase 1: Core and harness on PC ✅

- ✅ LakeSnes vendored and patched ([PATCHES.md](third_party/lakesnes/PATCHES.md)).
- ✅ `src/core.h/.c`: ROM, frames, pad, WRAM/VRAM/CGRAM/OAM/ARAM/DSP access, peek/poke, in-memory states, state copy
  between cores, SRAM, hiding and splitting layers.
- ✅ `tools/harness.c`: `wait/hold/tap/until`, `shot/layershot/splitshot/splitbox/sheet`, `peek/poke/watch/expect`,
  `dumpram`, `save/load`, `savesram/loadsram`, `hide/split`, `map`, `bench`, `hash`, `audio`.
- ✅ Scripts that reach the game from power-on, in [`games/zelda_alttp/tests/`](games/zelda_alttp/tests/).
- ✅ **Checked live against the ROM:**
  - `$7E0010/11`: `07/00` is free movement indoors, `09/00` outdoors, `0E/02` dialogue, `0E/01` the item menu,
    `0E/07` the world map, and `09/20-22` the map closing.
  - World map: X opens the close-up, X again the whole world, and X once more closes it. Link's marker blinks red
    and white.
  - Layers: BG1 is the rain or the top layer, BG2 the terrain and the room, and **BG3 holds the HUD, the dialogue
    text and the whole item menu**.
  - The HUD is BG3, lines 15-59 (lines 0-64 are split off); splitting it costs ~3 % of the speed.
  - Map capture on the hidden core: the whole world with 8 animation frames, 86 game frames and about 220 ms on PC.
    In Link's house it correctly answers "no map here".
- ⬜ `tools/ramdiff.py`: compare RAM dumps, as in gba-ds.
- ✅ Regression tests: `new_game.txt` (power-on to the map and the menu) and `sanctuary.txt` (a dungeon: its map and
  the text box at the top and the bottom). They use `expect`, `map … ok` and `untiltap`, exit with 1 if anything
  changes and need no saves.

## Phase 2: Desktop app with two screens ✅

- ✅ `src/main.c`: an SDL2 window with both screens, stacked (the game above, the second screen below) or side by side
  (`--layout=side --screen=1024x768`, as on the device), at whole-number scales.
- ✅ Keyboard and controller input (SDL GameController, by button position). ✅ Configurable mapping (Settings → Controls).
- ✅ 48 kHz audio through an SDL queue, paced by the audio (by the clock without audio).
- ✅ Battery saves (`.srm` beside the ROM or in `--save-dir`; written when they change and on exit).
- ✅ Quick state (F5/F7), fast forward (Tab) and four save-state slots with screenshots (from the menu).
- ✅ Aspect: the original (256×224 at whole-number scales, the default) or 4:3, which fills the 1024×768 panels.
  For 4:3 the picture is first scaled up by whole numbers, sharp, and only the last step is smoothed, so every pixel
  is the same size.
- ✅ **Game list** at startup (touch and buttons), with a CRC cache (`library-cache.txt`) for fast startup with many
  ROMs, and EmulationStation's covers (`gamelist.xml`, `images/`).
- ✅ **Pause menu** on the touch screen: continue, save or load a state, settings, leave the game or the app.
- ✅ **Settings** (aspect, screen swap, HUD split, map, volume, FPS, language) kept in `snes-ds.cfg`.
- ✅ The interface in English (the default) and Spanish ([`src/i18n.h`](src/i18n.h)), switched from the settings.
- ✅ The interface ([`src/ui/`](src/ui)): resolution-independent layout, anti-aliased shapes, Nunito through
  stb_truetype, an orange theme with the SNES face-button colours for the button hints.

## Phase 3: Second screen for A Link to the Past 🔶

- ✅ Data-driven per-game profiles (memory probes with a mask and a value, layer-split rules, how to open the map).
  One module per game in `games/<id>/` (profile, `README.md` with its memory map, tests), registered in
  `games/games.c`; the engine (`src/profiles.c`) holds no game data.
- ✅ **HUD below:** in modules `07`/`09` (including the scroll between rooms), BG3 lines 0-64 go to the bottom
  screen; also during dialogue (`0E/02`) and through doors (`0F`/`10`). The text box starts at BG3 row 9 (line 72)
  when at the top and row 18 when at the bottom (`$7E1CD2`), so it never reaches the HUD. **Undone for 0.6.0:** over
  the dungeon map the HUD looked wrong, so it stays on the game screen.
- ✅ **Item menu below:** with `0E/01`, all of BG3 goes down and the world stays visible above.
- ⬜ **Dialogue below** (optional): move the text box to the second screen too. A profile option.
- ✅ **Live map on a hidden core** (`src/mapcap.c`, on a thread): copy the state, press X (and X again for the whole
  world), wait for it to settle (`$10/$11 = 0E/07` or `0E/03`), record 8 frames for the marker's animation, check
  and publish. Every ~1.5 s and whenever `$A0`/`$8A` change.
- ✅ The dungeon map (`0E/03`). The map item isn't needed (it shows the rooms visited; the item reveals the rest).
  The map comes up after some black frames, which the capture no longer accepts.
- ✅ Composition: the HUD over the map, on its own lines; both screens the same size. The map is dimmed where there
  is none. ⬜ Configurable layout (close-up or whole world…).
- ✅ The start-up hiss (a sound effect the game asks for before anything is shown) muted from the profile.
- ⬜ A fallback screen when there is nothing to show: the game's logo or title.

## Phase 4: The device (RG DS Plus / RG DS, ROCKNIX) 🔶

- ✅ aarch64 cross-build: `scripts/build-rocknix.sh` (on any x86-64 Linux) or `scripts/build-rocknix-docker.sh`
  (Debian 12 in Docker, from Windows). The binary only needs ROCKNIX's `libSDL2` and glibc ≥ 2.34.
- ✅ Tested on aarch64 under QEMU (Docker `--platform linux/arm64`): both Zelda tests pass, and the app runs with
  SDL2, side by side on 2048×768, with map captures and the interface. Speed under QEMU means nothing.
- ✅ The app ready for the device:
  - Fullscreen, and `swaymsg '[pid=…] fullscreen enable global'` to cover both panels.
  - The layout follows the real output size (side by side when wide).
  - Clean exit on SIGTERM (the battery save is written).
  - The log records the input devices, the controller mapping, the first presses and touches, and the speed.
- ✅ `dist/launch.sh`, `dist/screens.sh` (turn the second panel on and place it to the right; put it back on exit),
  `dist/SNES DS.sh` (the Ports entry) and `dist/bench.sh`. Written for this project, not copied from gba-ds.
- ✅ A zip for `/storage/roms/ports/`: `dist/snes-ds-v<VERSION>-rocknix-aarch64.zip`.
- ✅ **First measurement on the console (v0.1.0):** 18-39 ms per emulated frame, 25-50 fps, with audio dropouts.
  Not enough.
- ✅ **LakeSnes optimised** ([PATCHES.md](third_party/lakesnes/PATCHES.md)) without changing what it emulates: the
  picture and WRAM hashes of `games/zelda_alttp/tests/core_check.txt` are identical before and after, with and
  without the render thread, on x86-64 and aarch64. Emulated frames per second on PC, outdoors in the rain:

  | Step | fps | × |
  |---|---|---|
  | Original | 245 | 1.0 |
  | Per-line picture unit: layers decoded once, windows and brightness worked out ahead | 293 | 1.2 |
  | Timing in stretches between events | 321 | 1.3 |
  | Composition by layer instead of by pixel | 439 | 1.8 |
  | A full step only after the IRQ setup changes (`irqDirty`) | 460 | 1.9 |
  | `-O3` + LTO | 486 | 2.0 |
  | Skipping layer and priority pairs with no pixels on the line | 504 | 2.1 |
  | Backgrounds decoded tile by tile | 589 | 2.4 |
  | **Drawing on a thread of its own** (another CPU core) | **~900** | **~3.7** |

- ✅ A cheaper hidden map core: it doesn't draw while opening the map, runs at low priority on Linux, and the wait
  between captures counts from the end of the last one. A world capture went from 271 to 108 ms on PC.
- ✅ **Second measurement on the console (v0.3.0, A Link to the Past, from Link's house to the castle):** 60 fps
  throughout, **9.3-10.2 ms per emulated frame** (the estimate was ~10), peaks of 15-19 ms while the map is captured,
  which the audio queue absorbs: **no audio dropouts**. Drawing takes ~1.7 ms. Map captures take ~290 ms outdoors and
  up to ~1.1 s in the castle, in the background. No further optimisation is needed for this game; heavier games
  (enhancement chips, Mode 7 throughout) will be measured as they come.
- ✅ Touch on the device: one Goodix panel, tied by sway to the whole layout; since SNES DS's window spans both
  panels, taps land where they should (the bottom panel is the right half).
- ✅ The menu button. The console's log showed guide (button 10) and L3 (11) reaching the app, but the menu didn't
  open: the main loop dropped the controller's menu action (only Esc and taps got through). Fixed in 0.4, with a test
  that drives the app through a virtual controller (`tools/app_tests/menu.txt`, run by `scripts/check.sh`).
  Confirmed on the console: **L3 and the R button** (the home button, SDL's guide) open the menu.
- ✅ Battery saves, save states, their pictures, the settings and the game list's cache are written to a temporary
  file, flushed to the disk and renamed over the old one ([`src/fileutil.c`](src/fileutil.c)): a power cut leaves the
  old file or the new one.
- ✅ The speed log and the FPS counter leave out the time spent in the game list and the menu (a pause read as
  28 fps before).
- ⬜ BananaOS (KMSDRM, one window per panel).

## Phase 5: Touch and comfort ✅

Closed for now. Touch drives SNES DS's own screens; games themselves don't use it, by choice (convenience and
stability), so the touch inventory for A Link to the Past is dropped.

- ✅ The game list and the menu by touch; a tap on the bottom screen pauses the game into the menu.
- ✅ Swap the screens (settings).
- ✅ Covers in the game list.
- ✅ The interface in English and Spanish.
- ❌ Touch inventory in A Link to the Past (dropped).
- Later: hide the second screen, per-game settings, list the games without a profile (playable on one screen).

## Phase 6: More games 🔶

**Step 0, done ✅: the groundwork for adding games on separate branches**, by agents or people alike:

- ✅ [AGENTS.md](AGENTS.md): the ground rules, the branch scheme (`game/<id>`, `engine/<topic>`, `fix/<topic>`,
  `docs/<topic>`), the research → profile → tests → documentation workflow, and how game-specific code and
  optimisations reach the engine (generic, opt-in hooks in `engine/` branches).
- ✅ [`scripts/check.sh`](scripts/check.sh): every game's tests in one command, each ROM found by its profile
  (`harness --find`), with the core's reference hashes kept in the repository (`core_check.expected`).
- ✅ [`docs/game-template/`](docs/game-template): a profile, READMEs and a first test to start from.

Then the same path for each game: harness → `layershot` for the layers → `watch` for the indicators → profile →
tests → test on the device. One branch each:

1. ✅ **Super Metroid** (`game/super_metroid`): the pause map and the HUD's minimap.
2. ⬜ Super Mario World (`game/super_mario_world`): the world map and the HUD.
3. ⬜ Secret of Mana, Chrono Trigger and Final Fantasy VI (`game/secret_of_mana`, `game/chrono_trigger`,
   `game/final_fantasy_vi`): menus and the Mode 7 world map.
4. 🔶 Super Castlevania IV ✅ and Dracula X ⬜ (`game/super_castlevania_iv`, `game/castlevania_dracula_x`): the HUD.

## Phase 7: Integration ⬜

- ⬜ EmulationStation and BananaOS, like gba-ds's `esinteg.c`.
- ⬜ RetroAchievements through rcheevos (casual mode).
- ⬜ Android (GammaOS), later on.

## Risks

| Risk | Impact | Mitigation |
|---|---|---|
| LakeSnes doesn't reach 60 fps on the Cortex-A55 | Was high, now low | Optimised about 3.7× (phase 4): A Link to the Past runs at ~9.5 ms/frame on the RG DS Plus. Heavier games still to be measured; `core.h` allows swapping the core |
| LakeSnes accuracy bugs (its `bugs.md`: hangs on SPC uploads in some games) | Medium | A Link to the Past is verified by zelda3; test every new game |
| A game changes BG3's registers mid-frame | Low | The split happens in the picture unit itself, scanline by scanline, so it's already covered |
| Licensing (Snes9x isn't GPL-compatible) | Medium | Stay with LakeSnes or bsnes |
| The handheld's menu button or touch panels reach the app differently than expected | Medium | Many ways to open the menu (a tap, any spare button, Select + Start); the log records the input devices |
