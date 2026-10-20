#!/bin/bash
# scripts/build-rocknix.sh inside a Debian 12 container, for hosts without
# an aarch64 cross compiler (Windows with Docker Desktop, macOS). Debian 12's
# glibc (2.36) is older than ROCKNIX's, so the binary runs there.
#   scripts/build-rocknix-docker.sh
set -euo pipefail
cd "$(dirname "$0")/.."
MSYS_NO_PATHCONV=1 docker run --rm -v "$PWD:/src" -w /src debian:12 bash -c '
	set -e
	apt-get update -qq
	DEBIAN_FRONTEND=noninteractive apt-get install -y -qq --no-install-recommends \
		gcc-aarch64-linux-gnu g++-aarch64-linux-gnu libc6-dev-arm64-cross binutils-aarch64-linux-gnu \
		cmake ninja-build curl ca-certificates zip >/dev/null
	scripts/build-rocknix.sh
	chown -R '"$(id -u 2>/dev/null || echo 0):$(id -g 2>/dev/null || echo 0)"' build dist deps 2>/dev/null || true
'
