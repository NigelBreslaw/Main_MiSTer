#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

UPSTREAM_COMMIT="dfb4791bea126afea66025be806651a995f9cfd6"
BASE_REF="${MAGIK_BASELINE_REF:-$UPSTREAM_COMMIT}"

if ! git rev-parse --verify "$BASE_REF^{commit}" >/dev/null 2>&1; then
  echo "ERROR: baseline ref is not a commit: $BASE_REF" >&2
  exit 2
fi

allowed_path() {
  case "$1" in
    FORK.md|MAGIK_PATCHSET.md|build-container.sh|build-docker.sh) return 0 ;;
    .devcontainer/Dockerfile.apple-container) return 0 ;;
    .github/workflows/magik-main.yml) return 0 ;;
    scripts/check-magik-patch-surface.sh|scripts/test-magik-state.sh) return 0 ;;
    support/mister_magik/*) return 0 ;;
    support/arcade/mra_loader.cpp|support/arcade/mra_loader.h) return 0 ;;
    tests/*) return 0 ;;
    fpga_io.cpp|fpga_io.h|input.cpp|input.h|joymapping.cpp|main.cpp|menu.cpp|scheduler.cpp|user_io.cpp|user_io.h|osd.cpp|video.cpp|video.h) return 0 ;;
  esac
  return 1
}

mapfile -t paths < <(
  {
    git diff --name-only "$BASE_REF..HEAD"
    git diff --name-only
    git diff --name-only --cached
  } | LC_ALL=C sort -u
)

violations=()
for path in "${paths[@]}"; do
  [[ -z "$path" ]] && continue
  if ! allowed_path "$path"; then
    violations+=("$path")
  fi
done

echo "MagiK patch-surface check"
echo "  upstream baseline: $UPSTREAM_COMMIT"
echo "  local baseline ref: $BASE_REF"
echo "  changed paths: ${#paths[@]}"

if ((${#violations[@]})); then
  echo ""
  echo "ERROR: changed paths outside the approved patch surface:" >&2
  printf '  %s\n' "${violations[@]}" >&2
  exit 1
fi

echo "  result: approved patch surface only"
