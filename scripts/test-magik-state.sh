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
  "$ROOT/support/mister_magik/launcher_reply.cpp" \
  "$ROOT/tests/launcher_reply_test.cpp" \
  -o "$OUT-reply"
"$OUT-reply"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/launcher_state.cpp" \
  "$ROOT/tests/launcher_state_test.cpp" \
  -o "$OUT-state"
"$OUT-state"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/launcher_ready.cpp" \
  "$ROOT/tests/launcher_ready_test.cpp" \
  -o "$OUT-ready"
"$OUT-ready"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/launcher_command.cpp" \
  "$ROOT/tests/launcher_command_test.cpp" \
  -o "$OUT-command"
"$OUT-command"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/tests/launcher_display_timing_test.cpp" \
  -o "$OUT-display-timing"
"$OUT-display-timing"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/launcher_diag.cpp" \
  "$ROOT/tests/launcher_diag_test.cpp" \
  -o "$OUT-diag"
"$OUT-diag"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/fpga_ownership.cpp" \
  "$ROOT/tests/fpga_ownership_test.cpp" \
  -o "$OUT-fpga-ownership"
"$OUT-fpga-ownership"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/bootstrap_sequence.cpp" \
  "$ROOT/tests/bootstrap_sequence_test.cpp" \
  -o "$OUT-bootstrap-sequence"
"$OUT-bootstrap-sequence"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/launcher_return.cpp" \
  "$ROOT/tests/launcher_return_test.cpp" \
  -o "$OUT-return"
"$OUT-return"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/layout.cpp" \
  "$ROOT/support/mister_magik/menu_path.cpp" \
  "$ROOT/tests/menu_path_test.cpp" \
  -o "$OUT-menu-path"
"$OUT-menu-path"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/layout.cpp" \
  "$ROOT/tests/layout_test.cpp" \
  -o "$OUT-layout"
"$OUT-layout"
DEV_LAYOUT_EXE="$(dirname "$OUT-layout")/MiSTer_MagiKDev"
cp "$OUT-layout" "$DEV_LAYOUT_EXE"
"$DEV_LAYOUT_EXE"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/button_overrides.cpp" \
  "$ROOT/tests/button_overrides_test.cpp" \
  -o "$OUT-button-overrides"
"$OUT-button-overrides"

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/support/mister_magik/input_proxy.cpp" \
  "$ROOT/tests/input_proxy_test.cpp" \
  -o "$OUT-input-proxy"
"$OUT-input-proxy"

if [ "$(grep -c 'magik_input_proxy_allows_fpga_output(launcher_mode)' "$ROOT/input.cpp")" -lt 2 ]; then
  echo "ERROR: launcher input polling must isolate both disk-LED and post-poll FPGA output" >&2
  exit 1
fi

${CXX:-c++} -std=c++14 -Wall -Wextra -I"$ROOT" \
  "$ROOT/tests/user_io_config_map_test.cpp" \
  -o "$OUT-user-io-config-map"
"$OUT-user-io-config-map"

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

awk '
  /static void complete_handoff_to_game\(/ { in_handoff=1; state_gate=0; accepted=0; rejected=0 }
  in_handoff && /magik_launcher_accepts_handoff\(s_state\)/ { state_gate=1 }
  in_handoff && /eventf\("external_load_core_rejected"/ { rejected=state_gate }
  in_handoff && /eventf\("external_load_core_handoff"/ { accepted=state_gate }
  in_handoff && /transition\(MagikLauncherEvent::HandoffComplete\)/ {
    if (!accepted || !rejected) exit 1
    checked=1
    in_handoff=0
  }
  END { if (!checked) exit 1 }
' "$ROOT/support/mister_magik/launcher.cpp" || {
  echo "ERROR: external load_core diagnostics must follow the supervised handoff state gate" >&2
  exit 1
}

grep -q 'complete_handoff_to_game(cmd.path, true)' "$ROOT/support/mister_magik/launcher.cpp" || {
  echo "ERROR: external load_core must identify itself to the supervised game handoff" >&2
  exit 1
}

awk '
  /static bool write_launcher_script\(/ { in_script=1; sourced=0; token=0; fifo=0; settings=0; display=0; confirm=0 }
  in_script && /"  \. / { sourced=1 }
  in_script && /MISTER_MAGIK_STARTUP_TOKEN/ { token=sourced }
  in_script && /MISTER_MAGIK_READY_FIFO/ { fifo=sourced }
  in_script && /MISTER_MAGIK_RUNTIME_SETTINGS_V1/ { settings=sourced }
  in_script && /MISTER_MAGIK_RUNTIME_DISPLAY_V1/ { display=sourced }
  in_script && /MISTER_MAGIK_DISPLAY_CONFIRM_UI/ { confirm=sourced }
  in_script && /fclose\(f\)/ {
    if (!token || !fifo || !settings || !display || !confirm) exit 1
    checked=1
    in_script=0
  }
  END { if (!checked) exit 1 }
' "$ROOT/support/mister_magik/launcher.cpp" || {
  echo "ERROR: launcher.env must not override Main-owned readiness or runtime display contracts" >&2
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
grep -q 'magik_app_path' "$ROOT/support/mister_magik/launcher.cpp"
! grep -q '/media/fat/mister-magik/' "$ROOT/support/mister_magik/launcher.cpp"
! grep -q '/media/fat/mister-magik/experiments' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'latch_artifact_verification_failed' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'main_sha256' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'platform-v3.manifest' "$ROOT/support/mister_magik/launcher.cpp"
! grep -q 'platform-v[12].manifest' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'latch_capability_mask.*0x01ff' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'platform_contract_sha256' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'scanout_slots_module_loaded' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'scanout_slots_device_ready' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'latch-readiness-report' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'run_launcher_readiness_preflight' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'latch_startup_tsv valid=1 action=launch-latch-ui reason=preflight-passed' "$ROOT/support/mister_magik/launcher.cpp"
! grep -q 'compatibility-screen' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'video_magik_enter_bootstrap_black' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'mister_magik_supervised_restart_launcher' "$ROOT/support/mister_magik/launcher_command.cpp"
grep -q 'mister_magik_reload_main' "$ROOT/support/mister_magik/launcher_command.cpp"
grep -q 'local_main_reload_supported' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'app_restart(magik_latch_menu_path(), NULL, s_local_dev_main_path)' "$ROOT/support/mister_magik/launcher.cpp"
grep -Fq 'make clean && make' "$ROOT/build-container.sh"
grep -q 'launcher-restart-token' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'launcher-restart-used' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'module_loaded != scanout_slots_module_loaded' "$ROOT/support/mister_magik/launcher.cpp"
! grep -q 'mister-magik-scanout"' "$ROOT/support/mister_magik/launcher.cpp"
! grep -q 'execl(path, path, "early-black"' "$ROOT/support/mister_magik/launcher.cpp"

awk '
  /int video_magik_enter_bootstrap_black\(\)/ { in_black=1; command=0; disable=0; forbidden=0 }
  in_black && /spi_uio_cmd_cont\(UIO_SET_FBUF\)/ { command++ }
  in_black && /spi_w\(0\)/ { disable++ }
  in_black && /(MiSTer_fb\/parameters\/mode|\/dev\/fb0|FB_EN|FB_FMT_565|set_vga_fb)/ { forbidden=1 }
  in_black && /^}/ {
    if (command != 1 || disable != 1 || forbidden) exit 1
    checked=1
    in_black=0
  }
  END { if (!checked) exit 1 }
' "$ROOT/video.cpp" || {
  echo "ERROR: bootstrap black must use only the canonical UIO_SET_FBUF disable command" >&2
  exit 1
}

awk '
  /static bool enter_bootstrap_black\(const char \*source\)/ { in_black=1; disabled=0; mux=0 }
  in_black && /s_bootstrap_sequence.black_completed\(acknowledged\)/ { disabled=1 }
  in_black && /if \(cfg.direct_video\) set_vga_fb\(1\);/ {
    if (!disabled) exit 1
    mux=1
  }
  in_black && /^}/ {
    if (!disabled || !mux) exit 1
    checked=1
    in_black=0
  }
  END { if (!checked) exit 1 }
' "$ROOT/support/mister_magik/launcher.cpp" || {
  echo "ERROR: Direct Video framebuffer mux must follow successful bootstrap black" >&2
  exit 1
}

grep -q 's_bootstrap_sequence.black_completed' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 's_bootstrap_sequence.preflight_completed' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 's_bootstrap_sequence.ownership_transferred' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 's_bootstrap_sequence.child_spawned' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 's_bootstrap_sequence.ready' "$ROOT/support/mister_magik/launcher.cpp"
grep -q 'MISTER_MAGIK_READY_FIFO' "$ROOT/support/mister_magik/launcher.cpp"
! grep -q 'ACK_FIFO\|visibility_ack\|VISIBILITY_FIFO' "$ROOT/support/mister_magik/launcher.cpp"

awk '
  /void mister_magik_launcher_route_early_black/ { in_early=1; black=0 }
  in_early && /enter_bootstrap_black\("post-video-init"\)/ { black=1 }
  in_early && /^}/ {
    if (!black) exit 1
    checked=1
    in_early=0
  }
  END { if (!checked) exit 1 }
' "$ROOT/support/mister_magik/launcher.cpp" || {
  echo "ERROR: post-video_init bootstrap must enter the common black transition" >&2
  exit 1
}

awk '
  /video_init\(\)/ { video_init=NR }
  /mister_magik_launcher_route_early_black\(\)/ { early_black=NR }
  END {
    if (!video_init || !early_black || early_black <= video_init) exit 1
  }
' "$ROOT/user_io.cpp" || {
  echo "ERROR: initial bootstrap black must run immediately after video_init" >&2
  exit 1
}

awk '
  /static void restore_stock_menu_after_failed_spawn/ { in_cleanup=1; child_guard=0; black=0; osd=0; input=0; legacy=0 }
  in_cleanup && /if \(s_pid\)/ { child_guard=1 }
  in_cleanup && /video_magik_enter_bootstrap_black\(\)/ { black=child_guard }
  in_cleanup && /user_io_osd_key_enable\(1\)/ { osd=child_guard }
  in_cleanup && /input_switch\(1\)/ { input=child_guard }
  in_cleanup && /(video_fb_enable|video_menu_bg)\(/ { legacy=1 }
  in_cleanup && /launcher_spawn_restored_stock_menu/ {
    if (!child_guard || !black || !osd || !input || legacy) exit 1
    checked=1
    in_cleanup=0
  }
  END { if (!checked) exit 1 }
' "$ROOT/support/mister_magik/launcher.cpp" || {
  echo "ERROR: failed-spawn cleanup must restore stock OSD/input over native black without a legacy framebuffer" >&2
  exit 1
}

awk '
  /static MagikLauncherSpawnResult spawn_launcher\(void\)/ && $0 !~ /;/ { spawn_signature=1; next }
  spawn_signature && /^\{/ {
    in_spawn=1
    spawn_signature=0
    black=0
    preflight=0
    owner=0
  }
  in_spawn && /enter_bootstrap_black\("supervised-spawn"\)/ { black=1 }
  in_spawn && /run_launcher_readiness_preflight\(path\)/ {
    if (!black) exit 1
    preflight=1
  }
  in_spawn && /transfer_fpga_owner_to_launcher/ {
    if (!black || !preflight) exit 1
    owner=1
  }
  in_spawn && /s_pid = fork\(\)/ {
    if (!black || !preflight || !owner) exit 1
    checked=1
    in_spawn=0
  }
  END { if (!checked) exit 1 }
' "$ROOT/support/mister_magik/launcher.cpp" || {
  echo "ERROR: Main must establish black, preflight, transfer ownership, then spawn" >&2
  exit 1
}

awk '
  /static MagikLauncherSpawnResult spawn_launcher\(void\)/ && $0 !~ /;/ { signature=1; next }
  signature && /^\{/ { in_spawn=1; signature=0 }
  in_spawn && /if \(!enter_bootstrap_black\("supervised-spawn"\)\)/ { black_failure=1 }
  black_failure && /restore_stock_menu_after_failed_spawn\(\)/ { black_recovered=1; black_failure=0 }
  in_spawn && /s_bootstrap_sequence\.preflight_completed\(preflight_passed\)/ { preflight_failure=1 }
  preflight_failure && /restore_stock_menu_after_failed_spawn\(\)/ { preflight_recovered=1; preflight_failure=0 }
  in_spawn && /if \(s_pid < 0\)/ { fork_failure=1 }
  fork_failure && /restore_fpga_owner_to_main\("fork-failed"\)/ { fork_owner=1 }
  fork_failure && /restore_stock_menu_after_failed_spawn\(\)/ { fork_recovered=fork_owner; fork_failure=0 }
  in_spawn && /return MagikLauncherSpawnResult::Spawned/ { in_spawn=0 }
  END {
    if (!black_recovered || !preflight_recovered || !fork_recovered) exit 1
  }
' "$ROOT/support/mister_magik/launcher.cpp" || {
  echo "ERROR: bootstrap-command, preflight, and fork failures must fail closed with no child" >&2
  exit 1
}

for event in \
  bootstrap_black_entered \
  bootstrap_preflight_completed \
  bootstrap_ownership_transferred \
  bootstrap_spawned \
  launcher_ready \
  launcher_ready_failed; do
  grep -q "\"$event\"" "$ROOT/support/mister_magik/launcher.cpp" || {
    echo "ERROR: missing bootstrap event $event" >&2
    exit 1
  }
done

for field in \
  bootstrap_phase \
  bootstrap_source \
  bootstrap_phase_ms \
  bootstrap_black_count \
  launcher_ready_phase \
  launcher_ready_attempt \
  launcher_ready_remaining_ms \
  launcher_ready_last_failure; do
  grep -q "$field" "$ROOT/support/mister_magik/launcher.cpp" || {
    echo "ERROR: missing bootstrap status field $field" >&2
    exit 1
  }
done

if [[ "$(grep -Ec '^[[:space:]]*s_pid = fork[(][)];' "$ROOT/support/mister_magik/launcher.cpp")" -ne 1 ]]; then
  echo "ERROR: all initial, resume, restart, and crash-respawn child starts must converge on spawn_launcher" >&2
  exit 1
fi

verify_manifest_selection_fixture() {
  local app="$1"
  local manifest="$app/platform-v3.manifest"
  local expected_format="mister-magik-platform-v3"
  [[ -r "$manifest" ]] || return 1
  [[ "$(sed -n 's/^format=//p' "$manifest")" == "$expected_format" ]]
}

MANIFEST_FIXTURE="$(mktemp -d "${TMPDIR:-/tmp}/mister-magik-manifest-test.XXXXXX")"
trap 'rm -rf "$MANIFEST_FIXTURE"' EXIT

printf 'format=mister-magik-platform-v3\n' >"$MANIFEST_FIXTURE/platform-v3.manifest"
verify_manifest_selection_fixture "$MANIFEST_FIXTURE"

printf 'format=mister-magik-platform-v1\n' >"$MANIFEST_FIXTURE/platform-v1.manifest"
rm "$MANIFEST_FIXTURE/platform-v3.manifest"
if verify_manifest_selection_fixture "$MANIFEST_FIXTURE"; then
  echo "ERROR: a legacy manifest must not be accepted" >&2
  exit 1
fi

printf 'format=invalid-v3\n' >"$MANIFEST_FIXTURE/platform-v3.manifest"
if verify_manifest_selection_fixture "$MANIFEST_FIXTURE"; then
  echo "ERROR: an invalid v3 manifest must fail closed" >&2
  exit 1
fi
