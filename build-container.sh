#!/usr/bin/env bash
# Build Main_MiSTer inside an arm64 Linux container using Apple's container
# runtime on Apple Silicon.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE="${MISTER_MAIN_CONTAINER_IMAGE:-mister-magik-main-builder:ubuntu20-arm64}"
default_container_cpus() {
  local cpus
  cpus="$(getconf _NPROCESSORS_ONLN 2>/dev/null || true)"
  case "$cpus" in
    ''|*[!0-9]*) ;;
    *) printf '%s\n' "$cpus"; return ;;
  esac

  cpus="$(sysctl -n hw.logicalcpu 2>/dev/null || true)"
  case "$cpus" in
    ''|*[!0-9]*) ;;
    *) printf '%s\n' "$cpus"; return ;;
  esac

  echo "ERROR: could not detect online CPU count for Apple container build." >&2
  exit 1
}

CONTAINER_CPUS="$(default_container_cpus)"
CONTAINER_MEMORY="8g"

usage() {
  cat <<'EOF'
usage: ./build-container.sh [make-args...]

Builds the Main_MiSTer fork in an arm64 Linux container through Apple's
Virtualization Framework container runtime. Pass make arguments such as
`clean`, `DEBUG=1`, or `V=1` after the script name.

Requires:
  container system start
  container builder start --cpus "$(getconf _NPROCESSORS_ONLN)" --memory 8g
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

if [[ "$(uname -s)" != "Darwin" || "$(uname -m)" != "arm64" ]]; then
  echo "ERROR: build-container.sh requires arm64 macOS; got $(uname -s)/$(uname -m)." >&2
  exit 1
fi

if ! command -v container >/dev/null 2>&1; then
  echo "ERROR: Apple container is not installed or not on PATH." >&2
  exit 1
fi

echo "==> container tool: $(container --version 2>&1 | head -n 1)"
echo "==> build backend: apple-container"
echo "==> build CPUs: $CONTAINER_CPUS"
echo "==> build memory: $CONTAINER_MEMORY"
echo "==> building linux/arm64 Main toolchain image: $IMAGE"
container build --arch arm64 --file "$HERE/.devcontainer/Dockerfile.apple-container" --tag "$IMAGE" "$HERE/.devcontainer"

echo "==> image arch probe"
container run --arch arm64 --rm "$IMAGE" uname -m

echo "==> Building Main_MiSTer in Apple container"
container run --arch arm64 --rm \
  --cpus "$CONTAINER_CPUS" \
  --memory "$CONTAINER_MEMORY" \
  --volume "$HERE:/src" \
  --workdir /src \
  "$IMAGE" \
  sh -lc 'export PATH=/usr/local/bin/gcc-arm-10.2-2020.11-aarch64-arm-none-linux-gnueabihf/bin:$PATH; make "$@"' \
  sh "$@"
