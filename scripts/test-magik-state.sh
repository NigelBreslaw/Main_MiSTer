#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="${TMPDIR:-/tmp}/mister-magik-launcher-state-test"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/launcher_wait.cpp" \
  "$ROOT/tests/launcher_wait_test.cpp" \
  -o "$OUT-wait"
"$OUT-wait"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/launcher_state.cpp" \
  "$ROOT/tests/launcher_state_test.cpp" \
  -o "$OUT-state"
"$OUT-state"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/launcher_command.cpp" \
  "$ROOT/tests/launcher_command_test.cpp" \
  -o "$OUT-command"
"$OUT-command"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/launcher_diag.cpp" \
  "$ROOT/tests/launcher_diag_test.cpp" \
  -o "$OUT-diag"
"$OUT-diag"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/launcher_return.cpp" \
  "$ROOT/tests/launcher_return_test.cpp" \
  -o "$OUT-return"
"$OUT-return"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/button_overrides.cpp" \
  "$ROOT/tests/button_overrides_test.cpp" \
  -o "$OUT-button-overrides"
"$OUT-button-overrides"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/sdram_probe.cpp" \
  "$ROOT/tests/sdram_probe_test.cpp" \
  -o "$OUT-sdram-probe"
"$OUT-sdram-probe"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/neogeo_memory.cpp" \
  "$ROOT/tests/neogeo_memory_test.cpp" \
  -o "$OUT-neogeo-memory"
"$OUT-neogeo-memory"

awk '
  /static void complete_handoff_to_game\(/ { in_real=1; seen=0 }
  in_real && /user_io_ensure_sdram_config\(\)/ { seen=1 }
  in_real && /fpga_load_rbf\(path\)/ && !seen { exit 1 }
  in_real && /transition\(MagikLauncherEvent::HandoffComplete\)/ { in_real=0 }
  END { exit 0 }
' "$ROOT/support/mister_magik/launcher.cpp" || {
  echo "ERROR: real MagiK handoff must ensure SDRAM config before fpga_load_rbf(path)" >&2
  exit 1
}

awk '
  /static void complete_handoff_to_game_plan\(/ { in_plan=1; seen=0 }
  in_plan && /user_io_ensure_sdram_config\(\)/ { seen=1 }
  in_plan && /fpga_load_rbf\(rbf, NULL, plan->arg\)/ && !seen { exit 1 }
  in_plan && /transition\(MagikLauncherEvent::HandoffComplete\)/ { in_plan=0 }
  END { exit 0 }
' "$ROOT/support/mister_magik/launcher.cpp" || {
  echo "ERROR: structured MagiK handoff must ensure SDRAM config before fpga_load_rbf(rbf, ...)" >&2
  exit 1
}

if grep -q 'method=main-reset' "$ROOT/support/mister_magik/launcher.cpp"; then
  echo "ERROR: supervised reboot must not use the direct Main reset path" >&2
  exit 1
fi

if grep 'reboot(1)' "$ROOT/support/mister_magik/launcher.cpp" | grep -v 'unsafe direct reset experiment path only' >/dev/null; then
  echo "ERROR: supervised reboot must not call Main's direct reboot(1)" >&2
  exit 1
fi

if grep -q 'library-refresh' "$ROOT/support/mister_magik/launcher.cpp"; then
  echo "ERROR: Main must start the visible launcher before a missing catalog is rebuilt" >&2
  exit 1
fi

grep -q 'mister_magik_scanout_slots.ko' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'latch_artifact_verification_failed' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'main_binary_sha256' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'platform_contract_sha256=8481c082c327c2892bf0fd64a68195472d4336723f6ab467a611655f949b1faf' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'scanout_slots_module_loaded' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'scanout_slots_device_ready' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'module_loaded != scanout_slots_module_loaded' "$ROOT/support/mister_magik/launcher.cpp"
! grep -q 'mister-magik-scanout"' "$ROOT/support/mister_magik/launcher.cpp"
