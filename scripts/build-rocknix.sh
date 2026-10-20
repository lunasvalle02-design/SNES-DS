#!/bin/bash
# Cross-builds snes-ds for ROCKNIX (aarch64) on an x86_64 Linux host and
# packs dist/snes-ds-v<VERSION>-rocknix-aarch64.zip, which unzips into
# /storage/roms/ports.
#
# Needs: aarch64-linux-gnu-gcc and -g++ (Debian/Ubuntu: gcc-aarch64-linux-gnu,
# g++-aarch64-linux-gnu; g++ only for SDL2),
# cmake, ninja, curl, zip. On Windows, run scripts/build-rocknix-docker.sh.
# SDL2 is built here only to link against: on the device, ROCKNIX's own
# libSDL2 runs (with its Wayland/sway support).
set -euo pipefail
cd "$(dirname "$0")/.."
ROOT="$PWD"
DEPS="$ROOT/deps/aarch64"
SDL_VERSION=2.30.9
mkdir -p "$DEPS"

# --- SDL2 (shared, to link against) -----------------------------------------
if [ ! -f "$DEPS/sdl/lib/libSDL2.so" ]; then
	if [ ! -d "$DEPS/SDL2-$SDL_VERSION" ]; then
		curl -fsSL -o "$DEPS/sdl.tar.gz" \
			"https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VERSION/SDL2-$SDL_VERSION.tar.gz"
		tar -xzf "$DEPS/sdl.tar.gz" -C "$DEPS"
	fi
	cmake -S "$DEPS/SDL2-$SDL_VERSION" -B "$DEPS/sdl-build" -G Ninja \
		-DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/aarch64-linux.cmake" -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX="$DEPS/sdl" -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF
	cmake --build "$DEPS/sdl-build"
	cmake --install "$DEPS/sdl-build"
fi

# --- snes-ds ----------------------------------------------------------------
cmake -S "$ROOT" -B "$ROOT/build/aarch64" -G Ninja \
	-DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/aarch64-linux.cmake" -DCMAKE_BUILD_TYPE=Release \
	-DSDL2_DIR="$DEPS/sdl/lib/cmake/SDL2"
cmake --build "$ROOT/build/aarch64"

# --- package ----------------------------------------------------------------
VERSION=$(cat VERSION)
PKG="$ROOT/build/pkg"
rm -rf "$PKG"
mkdir -p "$PKG/snes-ds/licenses"
aarch64-linux-gnu-strip -o "$PKG/snes-ds/snes-ds" build/aarch64/snes-ds
aarch64-linux-gnu-strip -o "$PKG/snes-ds/harness" build/aarch64/harness
cp dist/launch.sh dist/screens.sh dist/bench.sh dist/README.txt VERSION LICENSE "$PKG/snes-ds/"
cp "dist/SNES DS.sh" "$PKG/"
cp third_party/lakesnes/LICENSE.txt "$PKG/snes-ds/licenses/LakeSnes-LICENSE.txt"
cp third_party/zip/README.md "$PKG/snes-ds/licenses/zip-miniz.txt"
cp -r assets "$PKG/snes-ds/"
cp assets/fonts/OFL.txt "$PKG/snes-ds/licenses/Nunito-OFL.txt"
chmod +x "$PKG/snes-ds/"*.sh "$PKG/snes-ds/snes-ds" "$PKG/snes-ds/harness" "$PKG/SNES DS.sh"
mkdir -p dist
ZIP="snes-ds-v$VERSION-rocknix-aarch64.zip"
rm -f "build/$ZIP"
(cd "$PKG" && zip -qr "$ROOT/build/$ZIP" snes-ds "SNES DS.sh")
# into dist/ (a zip of the same name there may be held open, on Windows)
if rm -f "dist/$ZIP" 2>/dev/null && cp "build/$ZIP" "dist/$ZIP"; then
	echo "built dist/$ZIP"
else
	echo "built build/$ZIP (dist/$ZIP is in use: copy it by hand)"
fi
# what the binary needs from the device: glibc symbol versions and libraries
aarch64-linux-gnu-objdump -T build/aarch64/snes-ds | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -1 | sed 's/^/needs /'
aarch64-linux-gnu-readelf -d build/aarch64/snes-ds | grep NEEDED
