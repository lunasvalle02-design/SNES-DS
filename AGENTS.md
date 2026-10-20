# AGENTS.md

**English** · [Español](AGENTS_ES.md)

Instructions for coding agents (and people) working on SNES DS. Read this before you change anything; read
[README.md](README.md) for what the project is. This file is the authoritative version; `AGENTS_ES.md` is its
translation.

The main job these instructions are for: **adding dual-screen support for a new game, one game per branch.**

## Ground rules

These are not negotiable. A change that breaks one is not merged, however good the rest is.

1. **No game data in the repository, ever.** No ROMs, battery saves, save states, memory dumps, screenshots or
   recordings of a game, nor anything extracted from a ROM (graphics, music, text). Keep all of it in `out/`, which
   git ignores. Don't upload game screenshots to pull requests or issues either: describe what you see in words. CRC32
   and SHA-1 hashes, memory addresses and the harness's `hash` lines are fine.
2. **One game, one branch, one directory.** A game branch changes only `games/<id>/`, its line in
   [`games/games.c`](games/games.c), and that game's row in the documentation (see
   [Finishing a game](#6-finish-it)). Nothing in `src/`, `third_party/`, `tools/` or another game's directory.
3. **The engine knows no game.** Code in `src/` never tests for a particular game. If a game needs something the
   engine can't do yet, add it to the engine in a separate `engine/` branch, generically and switched off by default
   (see [Game-specific code](#game-specific-code-and-optimisations)).
4. **The emulation core stays bit-identical.** `scripts/check.sh` compares the core's picture, split-picture and RAM
   hashes against [`games/zelda_alttp/tests/core_check.expected`](games/zelda_alttp/tests/core_check.expected). A
   change that alters them is a change to what the emulator does, and needs the maintainer's explicit approval
   (an accuracy fix, say), together with the new reference.
5. **Every test passes before a merge:** `scripts/check.sh ROMDIR` with every ROM you have, at least
   A Link to the Past and the game you're adding.
6. **Two languages.** Interface text is written with its translation beside it, `T("Settings", "Ajustes")`
   ([`src/i18n.h`](src/i18n.h)); a profile's `about` comes in English and Spanish. Documentation is English in
   `NAME.md` and Spanish in `NAME_ES.md`, kept in step.
7. **Never push to `main`**, never force-push a branch that isn't yours, and never rewrite published history.
   Everything reaches `main` through a reviewed pull request.

## Branches

| Branch | For | May change |
|---|---|---|
| `main` | What's released; always builds and passes `scripts/check.sh` | Only through pull requests |
| `game/<id>` | Adding or improving one game | `games/<id>/`, its line in `games/games.c`, its documentation rows |
| `engine/<topic>` | Engine or core features a game needs: new profile fields, hooks, core patches | `src/`, `tools/`, `third_party/` (with [PATCHES.md](third_party/lakesnes/PATCHES.md)), docs |
| `fix/<topic>` | Bug fixes outside any one game | What the fix needs |
| `docs/<topic>` | Documentation only | `*.md`, `docs/` |

**`<id>` is the game's module name**, the same string everywhere: the directory `games/<id>/`, the profile's `.id`,
the symbol `game_<id>`, and the branch `game/<id>`. It is:

- lowercase ASCII letters, digits and underscores (it has to be a valid C identifier);
- the game's name, not its region: other regions are more CRCs in the same profile when their addresses match
  (a profile per region, in the same directory, when they don't);
- short but unambiguous, with a series prefix when it helps: `super_metroid`, `super_mario_world`,
  `super_castlevania_iv`, `castlevania_dracula_x`, `chrono_trigger`, `final_fantasy_vi`, `secret_of_mana`.

`<topic>` is free-form, lowercase, words joined by hyphens: `engine/split-hooks`, `fix/menu-button-release`.

Why `game/` and not `agent/`: the prefix says what the branch *contains*, not who wrote it. Agents and people follow
the same path, and what wrote a commit is recorded in the commit itself (see [Commits](#commits-and-pull-requests)).

**A branch's life:**

1. Check nobody is on that game already: `git ls-remote --heads origin 'game/*'` and the open pull requests.
2. Branch from the latest `main`: `git switch -c game/<id> origin/main`.
3. Push early and open a **draft** pull request, so the game is claimed.
4. Keep it current with `git rebase origin/main` (your branch only). If it needs an engine change, that change goes
   in its own `engine/` branch and pull request first; rebase onto `main` once it's merged.
5. When `scripts/check.sh` passes and the documentation is done, mark the pull request ready for review.
6. After the merge (squash), the branch is deleted.

## Setup

```bash
# Windows: MSYS2 UCRT64 (gcc, cmake, ninja, SDL2); Linux: build-essential cmake ninja-build libsdl2-dev
cmake -S . -B build -G Ninja
cmake --build build
scripts/check.sh /path/to/your/roms        # every game's tests
```

- `build/harness` (`.exe` on Windows) is the headless harness; `build/snes-ds` the app. Without SDL2 only the harness
  is built, which is all game research needs.
- ROMs come from the maintainer or the person running the agent, at a path they give you; never search the internet
  for them, and never copy them into the repository. If you don't have the ROM of the game you're asked to add, stop
  and say so.
- The app can be driven without a window, for checking the interface: `SDL_VIDEODRIVER=offscreen` with `--script`
  (see the top of [src/main.c](src/main.c)).
- The device itself (an Anbernic RG DS Plus on ROCKNIX) is only in the maintainer's hands. Anything that can only be
  checked there goes in the pull request's description.

## Adding a game

### 1. Know the ROM

Run any harness script on it: the first line prints the header's title, the CRC32 the profiles go by (without a
copier header) and whether a profile already matches. Note the region and version; prefer the most common clean
dump.

### 2. Research it with the harness

The harness ([tools/harness.c](tools/harness.c), commands at the top) runs the game frame by frame from a script.
Write your scripts in `out/` while exploring; the ones worth keeping become the tests (step 4).

- **Get to the screens that matter** (normal play, each menu, the map) with `wait`, `hold`, `tap`, `until` and
  `untiltap`, and `save` a state at each so you can `load` straight back.
- **Find which layer holds what** with `layershot NAME`: one picture per layer (`_bg1` … `_bg4`, `_obj`), so you can
  see whether the HUD or a menu has a layer to itself. Then `split MASK Y0 Y1` with `splitbox` and `splitshot` to
  find the exact lines.
- **Find the memory that says which screen is up:** `watch ADDR LEN` prints a range whenever it changes; `peek`,
  `dumpram` and comparing dumps between two states do the rest. Look for the game's main mode/state byte first;
  disassemblies and RAM maps (Data Crystal and the like) are good starting points, but **check every address
  yourself** with the harness, and say in the README how.
- **The map, if the game has one:** find the presses that open it from play, the memory that says it's open, and
  the values that change when the player changes room or area. `map NAME` runs the profile's capture on a hidden core,
  as the app does, and prints the result.
- **Sound:** `audio N NAME` records the sound to a WAV and prints the APU ports and the DSP setup when a sound
  starts, should the game make a noise the profile has to mute (`bootMuteFrames`).

Things that have mattered before: a layer can hold both the HUD and part of the world (split by lines, not by layer);
the HUD can move during transitions; text boxes may share the HUD's layer; maps can come up after black frames.

### 3. Write the profile

Copy [`docs/game-template/`](docs/game-template) to `games/<id>/` and fill in `profile.c`. The fields are described in
[`src/profiles.h`](src/profiles.h):

| Field | What it is |
|---|---|
| `id`, `title`, `shortTitle`, `about[LANG_EN]`, `about[LANG_ES]` | Names, and one sentence on what the second screen shows |
| `crc32[]` | The ROMs it's for (up to 4) |
| `split[]` | Up to 4 rules, first match wins: memory probes (`PROBE`, `PROBE_EITHER`) → which layers and lines go to the second screen, as a HUD strip (`SPLIT_HUD`, over the map) or a whole screen (`SPLIT_FULL`, a menu) |
| `hudShowY0/Y1` | The lines of a HUD strip worth showing |
| `bootMuteFrames` | Frames of sound to mute after power-on |
| `map` | How the hidden core opens the map, when a capture may start (`live`), how to know it's up (`shown`), what changes trigger a new capture (`room`), and how to tell a settled, non-empty picture |

Comment every value with what it is and how it was found, as `games/zelda_alttp/profile.c` does. Then register it in
[`games/games.c`](games/games.c): `extern const Profile game_<id>;` and `&game_<id>,` in the list. CMake picks up
`games/*/*.c` by itself.

### 4. Write its tests

In `games/<id>/tests/`:

- Scripts that go from power-on to each screen the profile handles, checking on the way, with `expect`, the memory
  values the profile relies on, and with `map NAME ok` (or `no_map` where there is none) the map capture. They exit
  with 1 when anything is off. Use `untiltap` rather than fixed waits where the timing may vary.
- They must not need any saved game: create the progress they need (pokes into the save area are fine, as
  `games/zelda_alttp/tests/sanctuary.txt` does).
- `tests/order` lists them in the order `scripts/check.sh` runs them; later scripts may `load` the states earlier ones
  `save`.

### 5. Try it in the app

`build/snes-ds --rom-dir=ROMDIR` (or with `--windowed --layout=side --screen=1024x768` for the device's layout) and
play through what the profile covers: the HUD or menu on the second screen, nothing missing from either screen, the
map refreshing. Check the start-up sound. Keep any screenshots in `out/`.

### 6. Finish it

- `games/<id>/README.md` and `README_ES.md` (from the template): ROM, what the second screen shows, the memory map
  with how each value was checked, findings, tests, what's left.
- Add the game's row to "Supported games" in [README.md](README.md) and [README_ES.md](README_ES.md), and tick it in
  phase 6 of [ROADMAP.md](ROADMAP.md) and [ROADMAP_ES.md](ROADMAP_ES.md). These rows are the only shared lines a game
  branch touches, so merges stay trivial.
- `scripts/check.sh ROMDIR` passes, with Zelda's ROM there too.

## Game-specific code and optimisations

**Data first.** Most games need nothing but a profile, and a profile is easy to review: prefer it whenever it can
express what you need, even if that means a few more rules.

When it can't (a second screen drawn in a way of its own, a decision memory probes can't express, a speed shortcut
that only works for one game):

1. **The game's code goes in its own directory,** `games/<id>/*.c` (CMake compiles it), and is reached only through its
   profile.
2. **The engine gets a generic hook for it, in an `engine/<topic>` branch first:** an optional field in `Profile`
   (a function pointer or a flag) that the engine calls or honours when it's set. With the field unset, nothing
   changes for any other game: `scripts/check.sh` proves it, hashes included. Document the hook in
   `src/profiles.h`, and keep it as small as the first game that needs it requires; don't design for games nobody is
   working on.
3. **Optimisations that change what the emulator does** (skipping work a game never needs, say) are opt-in profile
   flags, measured with `bench` in the harness, and never on by default. Optimisations that keep it bit-identical
   belong to everybody: they go in an `engine/` branch.
4. **Core changes** (`third_party/lakesnes`) are listed in [PATCHES.md](third_party/lakesnes/PATCHES.md), marked
   `snes-ds` in the code, and keep the hashes identical (ground rule 4). A change to what a save state holds also
   raises `stateVersion` in `snes_other.c`, so older states are refused rather than loaded wrongly.
5. **Files the player keeps** (battery saves, states, settings) are written with
   [`src/fileutil.h`](src/fileutil.h), never with a plain `fopen`: a power cut must not leave half a file.

## Code style

- C11. Tabs in the project's own code (LakeSnes keeps its own two-space style). `-Wall -Wextra` must stay quiet.
- Follow the code around you: short functions, names that say what things are, comments that say why. A file starts
  with a comment on what it's for.
- No new dependencies, and nothing downloaded at build time, without the maintainer's approval. Third-party code goes
  in `third_party/` with its license.
- Paths are UTF-8 everywhere (Windows included: the executables carry a manifest for it).

## Commits and pull requests

- One logical change per commit. The summary line is in English, at most 72 characters, prefixed with the game id or
  the area: `super_metroid: split the HUD's minimap`, `engine: add a split hook to Profile`.
- If you're an agent, say so in a trailer, e.g. `Co-Authored-By: <agent name> <address>`.
- Pull request title: `game: <Full title> (<region>)` for a game, otherwise like a commit summary. The description says:
  - what the second screen shows, and where;
  - the ROM's region, version and CRC32;
  - the memory addresses used and how each was checked;
  - the output of `scripts/check.sh ROMDIR`;
  - what only the device can confirm;
  - in words, not pictures, anything that looks wrong.

## Candidate games

From the [roadmap](ROADMAP.md), phase 6. Branch names follow the rules above.

| Game | Branch | Second screen |
|---|---|---|
| Super Metroid | `game/super_metroid` | The pause map; the HUD's minimap |
| Super Mario World | `game/super_mario_world` | The world map; the HUD |
| Secret of Mana | `game/secret_of_mana` | Ring menus; the Mode 7 world map |
| Chrono Trigger | `game/chrono_trigger` | Menus; the world map |
| Final Fantasy VI | `game/final_fantasy_vi` | Menus; the Mode 7 world map |
| Super Castlevania IV | `game/super_castlevania_iv` | The HUD |
| Castlevania: Dracula X | `game/castlevania_dracula_x` | The HUD |

## Where things are

| Path | |
|---|---|
| [`src/profiles.h`](src/profiles.h) | Profile fields: the reference for step 3 |
| [`src/screen2.c`](src/screen2.c), [`src/mapcap.c`](src/mapcap.c) | How profiles are used: the second screen, the map capture |
| [`src/core.h`](src/core.h) | Everything the rest of the code may ask of the emulator |
| [`tools/harness.c`](tools/harness.c) | The harness and its commands |
| [`scripts/check.sh`](scripts/check.sh) | The tests of every game, and of the app (`tools/app_tests/`) |
| [`games/zelda_alttp/`](games/zelda_alttp) | A complete example: profile, README, tests |
| [`docs/game-template/`](docs/game-template) | The starting point for a new game |
