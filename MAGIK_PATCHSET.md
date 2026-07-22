# MiSTer MagiK Patch Set

This file is the rebuild ledger for the MiSTer MagiK Main_MiSTer fork. If
upstream Main_MiSTer changes massively, rebuild this fork from the baseline and
reapply only the features and tests listed here.

## Baseline

- Upstream project: `MiSTer-devel/Main_MiSTer`
- Baseline commit: `93d13fb690db4581768389450fb639822ae88333`
- Baseline release: `Release 20260707.`
- Public fork/app: `/media/fat/MiSTer_MagiK` and `/media/fat/mister-magik/`
- Development fork/app: `/media/fat/MiSTer_MagiKDev` and
  `/media/fat/mister-magik-dev/`
- The fork selects the matching application root from its executable name; all
  launcher, manifest, module, RBF, input, environment, crash, and log paths use
  that root.
- Layout-specific latch RBF: `<application-root>/fpga/menu-magik-vblank-latch.rbf`.

The 20260707 refresh was rebuilt as a compact patch stack rather than replaying
the historical development commits. Upstream changes in `input.cpp`,
`user_io.cpp`, and `video.cpp` were retained while the MagiK adapters were
reapplied at their narrow integration seams.

## Intended Features

- Boot through stock `/media/fat/MiSTer` and MiSTer.ini `main=MiSTer_MagiK`.
- Let Main initialize HDMI/video/menu-core prerequisites.
- Start MiSTer MagiK Slint on `tty2` after Main video initialization.
- Keep Main in dormant launcher mode while Slint owns the visible launcher UI.
  Dormant launcher mode blocks on the command FIFO and supervised-child exit,
  with a bounded maintenance timeout, so Main does not burn CPU1 or wake at a
  fixed high rate while waiting for Slint or launcher commands.
- Accept explicit handoff commands:
  - `mister_magik_launch <absolute path>` for real `.mra`, `.mgl`, and `.rbf` paths
  - `mister_magik_launch_plan_v1 <encoded-plan>` for MagiK structured catalog rows
  - `mister_magik_exit_to_menu`
- Preserve MiSTer's public `load_core <absolute path>` contract while the
  supervised launcher owns the command FIFO. External `.mgl`, `.mra`, and
  `.rbf` requests use the same real-path handoff as MagiK-owned launches;
  inactive Main continues to use the stock command reader.
- Accept explicit lifecycle commands:
  - `mister_magik_suspend`
  - `mister_magik_resume`
  - `mister_magik_restart_launcher`
  - `mister_magik_reboot`
- Accept explicit attended display diagnostics for rare HDMI/scaler bad states:
  - `mister_magik_hdmi_power_cycle`
  - `mister_magik_video_adjust`
  - `mister_magik_video_reinit`
- Source `/media/fat/mister-magik/launcher.env` from the generated launcher
  script before starting Slint, so tooling can benchmark the real launcher
  Arcade screen without direct Rust launches.
- Use Main's original loaders after handoff for `.mra`/`.mgl`/`.rbf` launch.
- Carry MagiK structured launch plans through Main's re-exec path as a
  `magik-plan-v1:` argument and seed the existing MGL action state directly,
  avoiding temporary `.mgl` files.
- Honor MagiK's per-launch simple joystick policy marker. When active, Main
  ignores stock/user joystick map files under `/media/fat/config/inputs/` for
  that launched core, uses MagiK-owned controller baselines from
  `/media/fat/mister-magik/input/`, and then falls back to Main's existing
  gamecontrollerdb/default mapping path if MagiK has no managed baseline.
- Publish status and event files under `/tmp/mister-magik/`.
- Write local crash reports under `/media/fat/mister-magik/crashes/` when the
  supervised launcher child exits unexpectedly, and surface the latest report
  path in `main-status.json`.
- Append compact reboot breadcrumbs to
  `/media/fat/mister-magik/bootlogs/main-reboot.log` before the Linux reboot so
  shutdown-side timing survives the reboot.
- Support deploy-lock-aware launcher startup.
- Start the visible Slint launcher without a Main-owned catalog refresh. When
  the catalog is missing or empty, the Rust launcher owns the foreground build
  and its full-screen progress UI.
- Treat an intentional suspend as MagiK-owned dormant time, not a launcher crash.
- Treat a supervised reboot as MagiK-owned visual lockdown until reset: Main
  keeps OSD/menu/framebuffer paths suppressed, does not poll more commands, and
  asks Linux to reboot cleanly from a detached, fd-scrubbed child after syncing.
- Treat an unexpected Slint child exit as a MagiK-owned crash-recovery state:
  keep Main UI/OSD/framebuffer paths suppressed, keep polling lifecycle
  commands, and allow `mister_magik_restart_launcher` to respawn Slint.
- Detect old Main UI/framebuffer work as invariant violations instead of relying
  on normal-path suppression.
- Avoid pegging CPU1 during MagiK-owned dormant states. The stock Main process
  remains pinned to CPU1 for low-latency core/menu operation, but
  `LauncherActive`, `LauncherSuspended`, and `LauncherCrashed` wait on command
  FIFO activity or `SIGCHLD`, with a one-second timeout for launcher
  configuration and scanout-device maintenance checks.
- Own production FPGA latch startup for MagiK menu boots. When the fork is
  entered for the default Menu core, it redirects to the persistent MagiK
  vblank-latch RBF at
  `/media/fat/mister-magik/fpga/menu-magik-vblank-latch.rbf`, re-execs
  itself on that core, and loads the persistent stock-kernel plugin probe
  module before starting Rust so the launcher can use hidden-buffer latch mode
  by default.
- Keep that production latch RBF's physical path for loading and verification,
  but present it to Menu's core browser as logical root-level `menu.rbf` so
  `mister_magik_exit_to_menu` opens the normal top-level MiSTer view instead of
  exposing the private `mister-magik/fpga` deployment directory.

## Approved Patch Surface

Runtime changes should stay in or immediately around:

- `support/mister_magik/`
- `support/arcade/mra_loader.cpp` / `support/arcade/mra_loader.h` only for
  shared RBF-name resolution and direct MGL action seeding used by structured
  handoff
- `user_io.cpp` post-`video_init()` / Menu-core boot hook
- `input.cpp` only for MagiK simple joystick policy gating and MagiK-owned
  baseline-map loading
- `joymapping.cpp` only for applying MagiK-provided simple-mode button override
  tokens by MRA button index
- `scheduler.cpp` dormant-mode polling seam
- `main.cpp` only for the non-scheduler dormant launcher wait hook
- narrow command handoff wiring needed to launch through Main
- `menu.cpp` only for the production latch RBF's logical root-level core-browser
  identity and minimal diagnostic guards
- `video.h` only for exposing existing video diagnostic entrypoints to MagiK
  command handlers
- minimal diagnostic guards in OSD/video entrypoints

Build/docs/test changes may touch:

- `.devcontainer/Dockerfile.apple-container`
- `build-container.sh`
- `FORK.md`
- `MAGIK_PATCHSET.md`
- `scripts/`
- `tests/`

## Implemented Features And Tests

Update this section in every PR that adds behavior.

- PR 1: provenance docs, patch ledger, container build wrapper.
- PR 2: host-testable launcher state machine skeleton and tests.
- PR 3: boot hook, dormant scheduler path, Slint tty2 spawn, and status/events.
- PR 4: explicit `mister_magik_launch` and `mister_magik_exit_to_menu` commands.
- Direct command acknowledgements: pair `/dev/MiSTer_cmd` with the
  human-readable `/dev/MiSTer_cmd_reply`, serialize callers externally, return
  explicit `ok`, `rejected`, or `error` results from supervised launcher
  command paths, and publish a five-second event-loop heartbeat. Main host tests
  cover command parsing, state acceptance, and reply FIFO framing, closure, and
  node-type safety; device-side agent/launcher tests cover response handling.
- External `load_core` compatibility update: while the supervised launcher owns
  Main's command FIFO, absolute `.mgl`, `.mra`, and `.rbf` requests now enter
  the same `complete_handoff_to_game()` path as `mister_magik_launch`. The
  launcher command parser rejects empty, relative, oversized, unsupported, and
  control-character paths, records accepted and lifecycle-rejected external
  handoffs, and leaves the stock command reader authoritative whenever the
  MagiK launcher is inactive.
- PR 5: defensive invariant diagnostics for unexpected OSD, framebuffer route,
  framebuffer mode, and menu-background work while the launcher owns the UI.
- PR 9: patch-surface checker and GitHub Actions workflow for host tests,
  approved-surface verification, and Main builds.
- PR 10: explicit `mister_magik_suspend` / `mister_magik_resume` lifecycle
  commands so deploy/dev stops are expected exits, while unexpected Slint exits
  remain crash recovery.
- PR 11: explicit `mister_magik_restart_launcher` lifecycle command and
  launcher-env sourcing. This lets benchmark tooling restart only the
  supervised Slint child while Main reapplies OSD suppression, VT, tty, and
  input setup, avoiding direct `mister-magik-fb` launches and OSD
  contamination.
- PR 12: explicit `mister_magik_reboot` lifecycle command and
  `LauncherRebooting` state. Reboot now enters a MagiK-active visual lockdown,
  closes the command FIFO, keeps OSD/menu/framebuffer routing blocked, treats
  Slint child exit during shutdown as expected, syncs storage, and performs
  Linux `/sbin/reboot` so the final MagiK shutdown frame stays visible until
  reset while Ethernet gets the normal kernel shutdown path.
- Supervised reboot reliability update: `LauncherRebooting` still suppresses
  OSD/menu/framebuffer paths, but the final reset primitive is Linux
  `/sbin/reboot` instead of Main's reset-manager write. This is the required
  path for unattended reboot loops because the direct reset path reproduced an
  Ethernet receive stall after warm reboot. The reboot child must detach and
  close inherited Main file descriptors before `exec`, so the Linux reboot
  primitive is not launched with Main's hardware-owning process context.
- Reboot observability update: `mister_magik_reboot` writes compact persistent
  breadcrumbs for request, visual lockdown, sync start/done, `/sbin/reboot`
  fork/spawn, and failure stages. Use
  `/media/fat/mister-magik/bootlogs/main-reboot.log` to explain shutdown-side
  delay; `/tmp/mister-magik/events.jsonl` remains boot-local and disappears on
  reboot.
- Fast direct reset path: `mister_magik_direct_reset` and
  `mister_magik_direct_reset_no_sync` enter the same MagiK reboot lockdown but
  call Main's reset-manager `reboot(1)` path instead of Linux `/sbin/reboot`.
  Keep the Linux reboot path as the default for settings changes, release gates,
  and unknown write state. Use `mister_magik_direct_reset` for quiescent dev-loop
  reboots after writes are complete; reserve the no-sync command for explicit
  attended experiments.
- Commit `c6942ae` originally removed the delayed Main-owned catalog refresh.
  The generated launcher script now performs no catalog refresh at all: missing
  or empty catalogs are rebuilt by the visible Rust launcher, while non-empty
  catalogs use the Rust launcher's background validation path.
- Commit `05987b0`: allows `LauncherCrashed` to keep polling lifecycle commands
  so `mister_magik_restart_launcher` can recover the supervised Slint child
  after an unexpected exit.
- Commit `c18cf71`: treats `LauncherCrashed` as MagiK-active ownership, keeping
  the OSD/menu/framebuffer defensive suppression model in force during crash
  recovery instead of exposing stock Main UI while waiting for restart.
- Crash-restart recovery update: `mister_magik_restart_launcher` from
  `LauncherCrashed` now resets the launcher tty/input/OSD state and respawns
  the Slint child immediately from the restart command. `main-status.json`
  includes crash/restart counters plus last crash, restart, and spawn error
  details so device gates can distinguish command rejection, spawn deferral, and
  child crash state. Host tests cover the restart action policy and
  `LauncherCrashed -> EnteringLauncher -> LauncherActive` recovery sequence.
- Expected-exit crash hygiene update: child exits while `LauncherSuspending`
  and clean child `exit_status=0` are treated as expected dormant launcher
  states instead of writing crash reports. Main still writes crash reports for
  nonzero exits and unexpected signals outside suspend/reboot ownership.
- MagiK boot lockdown update: Menu-core boots with MagiK configured now enter
  `BootingMain` visual lockdown before `video_init()`. While that lockdown is
  active, Main suppresses stock HPS framebuffer reset, stock `8888` framebuffer
  mode writes, framebuffer enable calls, and menu-background work until Rust's
  `early-black` frame takes over the display.
- Main-side early-black update: `BootingMain` now routes and clears a `565`
  MagiK black framebuffer directly from Main before spawning Slint. The old
  `mister-magik-fb early-black` fork/exec remains as a fallback if the direct
  route fails.
- Launcher reveal update: Main reasserts the same `565` MagiK black framebuffer
  immediately before every supervised launcher spawn, including resume/restart
  paths used after games and tooling. This keeps HDMI on a known MagiK-sized
  black frame while the Rust launcher privately settles its first Slint frame.
- Return reveal update: launcher scripts spawned from `menu.rbf` set
  `MISTER_MAGIK_RETURN_TO_LAUNCHER=1`, allowing Rust to skip the startup splash
  on game return and reveal only the first real launcher frame. The explicit
  Menu return marker survives the extra re-exec through the MagiK latch Menu
  RBF and is consumed when the launcher script is written, so later core
  launches cannot inherit stale return authorization.
- Structured launch handoff update: adds
  `mister_magik_launch_plan_v1 <encoded-plan>` for virtual `magik-plan:*`
  catalog rows. The parser accepts only `schema=1`, required `core_path`,
  `payload_path`, and `mount_kind`, strict non-negative numeric fields, and the
  supported mount kinds `load-file` and `mount-image`. Main resolves
  `core_path` with the same RBF-name semantics used by MRA `<rbf>`, stops the
  supervised Slint launcher, re-execs the selected core with a
  `magik-plan-v1:` argument, and `user_io_init` seeds a single existing MGL
  load action from the structured fields. Real `.mra`, `.mgl`, and `.rbf`
  launches remain on `mister_magik_launch <absolute path>`. This is deliberately
  not compatible with old Rust launchers that still materialize virtual `.mgl`
  descriptors; deploy `MiSTer_MagiK` and `mister-magik-fb` together.
- Runtime settings v1 exposes Main's resolved launcher output as `hdmi` or
  `crt-240p60`, `crt-288p50`, `crt-480p60`, or `crt-576p50`, including the
  result of `direct_video=2` known-DAC detection and the selected PAL/scandoubler mode.
  The typed `mister_magik_settings_get_v1` and
  `mister_magik_settings_set_v1 output=<auto|hdmi|crt-240p60>` command retains the
  existing acknowledged FIFO but rejects runtime output changes as
  `restart-required`; it never edits `MiSTer.ini`. Output is the first namespace;
  future input settings must use separate typed fields.
- Runtime display transactions v1 add typed get/apply/confirm/cancel commands
  for the qualified HDMI modes, automatic HDMI-DAC detection, and the four
  standard CRT/VGA modes. Apply suspends Slint, forces Main's video mode, and
  restarts only the launcher. Main starts a ten-second deadline when the new
  launcher queries the pending state, retains a bounded no-launcher fallback,
  and restores the in-memory working mode on cancel or timeout. Confirmation
  delegates comment-preserving atomic `MiSTer.ini` persistence to the matching
  `mister-magik-fb`; therefore a crash or power loss before confirmation keeps
  the previously persisted output.
- MagiK pre-handoff SDRAM probe update: MagiK launch-active mode keeps Main's
  normal `user_io_poll()` dormant while Slint owns display/input, but Main still
  has to know the hardware memory configuration before launching a core. Both
  `mister_magik_launch` and `mister_magik_launch_plan_v1` now call the shared
  `user_io_ensure_sdram_config()` helper immediately before `fpga_load_rbf`.
  The helper reuses the stock `UIO_GET_OSDMASK` SDRAM module probe, caches the
  same `sdram_sz()` value used by 8-bit cores, and remains idempotent for the
  stock menu poll path. This fixes the Metal Slug 3 / NeoGeo regression where a
  valid structured launch reached Main, but NeoGeo saw `ram_sz == 0` and showed
  `Not enough memory! Graphics will be corrupted`.
- Simple joystick handling update: `input.cpp` checks the boot-local
  `/tmp/mister-magik/input-policy` marker written by the Rust launcher for a
  MagiK-launched game. With marker value `simple`, Main does not load the
  normal global or core-specific joystick maps from
  `/media/fat/config/inputs/`, does not load advanced maps, and therefore does
  not convert legacy `.jk` maps. Instead it first loads a MagiK-owned v3
  baseline named `/media/fat/mister-magik/input/input_<id>_v3.map`, then lets
  the existing MRA/core button-name mapping path derive the active core map.
  Missing MagiK baselines intentionally fall back to Main's existing
  gamecontrollerdb/default logic. Menu input and stock launches remain on the
  original path because the marker is launch-scoped and is cleared on
  launcher/menu entry.
- Simple button override update: with marker value `simple`, `joymapping.cpp`
  may read `/tmp/mister-magik/button-overrides`, a launch-time file written by
  Rust. Main treats it as a generic index-to-token adapter only: values are
  base virtual names (`A/B/X/Y/L/R/Select/Start`) or `unmap`. Main does not
  parse MRA XML or classify labels such as coin/start/pause; all arcade policy
  remains in MagiK.
- Dormant idle CPU update: `LauncherActive`, `LauncherSuspended`, and
  `LauncherCrashed` now report that the Main loop should wait between poll
  passes. The non-scheduler loop and scheduler poll coroutine both use the same
  policy hook after `mister_magik_launcher_poll()`, preserving normal stock
  Main polling outside MagiK-owned dormant states while preventing the
  supervised launcher from pinning CPU1 at 100%. Host state tests cover the
  idle-wait policy for active, suspended, crashed, handoff, suspending, and
  rebooting states. The wait itself uses `poll()` on `/dev/MiSTer_cmd` plus a
  `SIGCHLD` self-pipe, so commands and supervised-child exits wake immediately;
  the previous signal disposition is restored after every dormant wait so the
  stock core/menu path is unchanged after handoff. A one-second timeout retains
  bounded launcher-path and scanout-module health checks without the previous
  100 Hz filesystem/FIFO polling. Host wait tests cover timeout, FIFO wake, and
  child-exit wake behavior.
- Attended display diagnostics update: Main accepts three explicit
  MagiK FIFO commands for rare HDMI/scaler bad-state experiments without a
  Linux reboot. `mister_magik_hdmi_power_cycle` toggles ADV7513 HDMI power.
  `mister_magik_video_adjust` forces Main's runtime video-mode adjustment pass.
  `mister_magik_video_reinit` stops/suspends the Slint child, temporarily
  releases MagiK video suppression, disables the HPS framebuffer, reruns Main's
  existing `video_reinit()` path, and leaves the launcher suspended for visual
  inspection. These are attended diagnostic levers, not automatic recovery or
  normal launcher lifecycle commands; host parser tests cover their command
  names and type labels.
- Production FPGA latch startup update: MagiK menu boots now treat an empty
  core path or `menu.rbf` as the default Menu core and redirect to
  `/media/fat/mister-magik/fpga/menu-magik-vblank-latch.rbf` when it is
  present. The first process exits after `fpga_load_rbf`; the re-execed
  `MiSTer_MagiK` process then runs with the latch RBF path on its command line.
  The generated launcher script loads
  `/media/fat/mister-magik/mister_magik_scanout_slots.ko` if the module is not
  already live, then starts `mister-magik-fb`. The module exposes only two
  bounded write-combined hidden framebuffer slots. Main status exposes both
  module-loaded and slot-device-ready booleans. If the latch RBF is absent, Main
  logs `latch_menu_rbf_missing` and continues on the stock Menu path so Rust can
  fall back to `/dev/fb0`. The core browser maps only that exact physical latch
  path to logical root-level `menu.rbf`, so exit-to-menu keeps the qualified
  RBF active without exposing its private deployment directory; a host path
  test covers exact, null, stock, and near-matching inputs.

## Verification

- `scripts/test-magik-state.sh` runs host launcher transition, command parser,
  crash-policy, crash-restart, reboot-lockdown, invariant-format, and generic
  simple button override parser tests. It rejects Main-owned `library-refresh`
  startup behavior and also covers the dormant idle-wait policy plus timeout,
  FIFO, and `SIGCHLD` wake behavior for MagiK-owned launcher states. The
  return-marker test proves that an
  explicit `menu.rbf` return survives the latch Menu re-exec exactly once and
  is not inherited by later core launches. The Menu-path test proves that only
  the exact production latch RBF receives the logical root-level `menu.rbf`
  browser identity; normal and near-matching RBF paths remain unchanged.
  The command parser tests cover external `load_core` acceptance and rejection,
  while source invariants prove accepted and lifecycle-rejected diagnostics
  follow the handoff state gate before entering the supervised real-path handoff.
- `scripts/check-magik-patch-surface.sh` compares this fork with the upstream
  release baseline and fails if files outside the approved patch surface
  changed, including the narrow structured-handoff allowance for
  `support/arcade/mra_loader.*`.
- `.github/workflows/magik-main.yml` runs host tests and patch-surface validation
  for fork PRs and pushes to `mister-magik`.
- `support/mister_magik/main_component.py` defines the attested Main component
  artifact used by MiSTer MagiK platform promotion. Its identity binds the
  authoritative repository and branch, exact source revision, and pinned ARM
  toolchain; create/verify tests cover deterministic identity, malformed
  receipts, wrong revisions and toolchains, corrupt binaries, and checksums.
- Real-view Arcade benchmark tooling must write
  `/media/fat/mister-magik/launcher.env`, send
  `mister_magik_restart_launcher`, and verify the child remains
  Main-supervised.
- Catalog-refresh behavior must preserve fast launcher entry on normal boots:
  Main only performs the generated-script refresh when `library.sqlite3` is
  missing or empty and must not schedule a delayed external refresh for a
  non-empty catalog.
- Structured launch behavior must preserve path launches and prove that virtual
  `magik-plan:*` rows reach Main through `mister_magik_launch_plan_v1`, resolve
  the target RBF, and seed MGL state without reading or writing a temporary
  `.mgl` descriptor.
- SDRAM handoff behavior must prove the actual NeoGeo failure chain: a missing
  SDRAM cache marker makes Metal Slug 3-sized graphics trigger the existing
  NeoGeo warning predicate, valid 128MB or dual-SDRAM/digital-I/O cache state
  avoids the false warning, and both MagiK handoff paths ensure SDRAM
  configuration before `fpga_load_rbf`.
- Simple joystick behavior must prove both modes: with no marker, Main attempts
  the same `/media/fat/config/inputs/` map filenames as stock; with marker value
  `simple`, Main skips global/core/advanced/`.jk` input files, loads only the
  MagiK-owned v3 baseline when present, and still falls back safely for
  unmanaged controllers. Air Gallet should be tested with a deliberately bad
  `agallet_advanced_input_*` file present to confirm the bad override is ignored
  only in simple mode. Generic button override behavior must prove that Main
  applies MagiK-provided virtual tokens by MRA button index only when the simple
  marker is active.
- Dormant idle CPU behavior must be verified after deploying and rebooting the
  updated fork: a short `/proc/stat` delta should show CPU1 no longer advancing
  with zero idle jiffies during `LauncherActive`; Main should fall below five
  voluntary wakeups and command-FIFO reads per second while remaining
  responsive to `mister_magik_launch`, `mister_magik_restart_launcher`, and
  crash recovery commands.
- Production latch boot must prove all three live signals after reboot or game
  return: Main command line contains
  `/media/fat/mister-magik/fpga/menu-magik-vblank-latch.rbf`,
  `mister_magik_scanout_slots` is present in `/proc/modules`, and
  `/dev/mister-magik-scanout-slots` exists before accepting Rust latch-mode
  results.

## Device Smoke Results

### Acknowledged command-channel status

State-changing MagiK commands have a paired reply FIFO at
`/dev/MiSTer_cmd_reply`. Main writes one short line for every parsed command:
`ok <state>`, `rejected <state>`, or `error <reason>`. Successful handoff replies
mean that Main accepted a readable launch payload and began the handoff; they
are written before Main terminates the supervised Slint child, not after the
core loader returns. `ok LauncherActive` means Main successfully spawned the
supervised launcher child and entered its `LauncherActive` state. It does not
claim that the child has completed `exec` or rendered its first frame. Callers
must serialize commands and drain any abandoned reply immediately before
writing a new command. A caller must retain serialization ownership until it
reads the reply or Main closes the channel; heartbeat failure starts supervisor
recovery but does not permit another command against the same Main process.
There is no command timeout that can abandon a delayed reply while Main remains
alive. Legacy generic `load_core` commands do not write replies. Main also
refreshes `main-status.json` every five seconds as an event-loop heartbeat so
callers can distinguish a responsive Main from a stopped one without command
deadlines.

Active-core return uses the dedicated acknowledged
`mister_magik_return_to_launcher` command. Main replies `ok HandoffStarted`
before loading `menu.rbf`. The generic command reader opens and retains Main's
reply-channel writer for its full process lifetime, so callers cannot observe
EOF before command processing begins. Legacy generic `load_core` writers remain
fire-and-forget and never enqueue replies.

`/tmp/mister-magik/main-status.json` publishes `main_generation`,
`executable_path`, `command_channel`, `command_ready_ms`, and
`command_fifo_inode`. Readiness covers Main's generic command reader as well as
the launcher-supervisor reader, because active-core return commands are handled
by the generic reader. The status also reports the last launcher operation,
result, and timestamp. Host tooling must use the authenticated MagiK agent and
must not infer readiness from the FIFO pathname.

Device smoke on 2026-07-19 passed with the matched development platform:

- `/dev/MiSTer_cmd_reply` existed and `main-status.json` advanced from
  `ts_boot_ms=71928` to `77081` across a six-second observation.
- Acknowledged suspend, resume, and launcher restart completed in 99 ms, 60 ms,
  and 161 ms respectively.
- Arcade launch returned `ok HandoffStarted` in 72 ms; the dedicated
  active-core return returned it in 65 ms and reached a fresh `LauncherActive`
  generation with zero crashes and zero invariant violations.

2026-07-12, MiSTer at `192.168.1.117`, after deploying app commit `c598dada`
and Main fork commit `2498c03`:

- App host CI gates passed during commit, including Rust/catalog tests, clippy,
  host tools, MagiK agent checks, and script self-tests. Main host tests passed
  with `scripts/test-magik-state.sh`, including the new one-shot latch return
  marker regression.
- A clean `release-device` ARM build passed after clearing stale generated
  Slint target output. Main passed `./build-container.sh`.
- The combined deploy script stopped before device mutation because the local
  CI-built latch RBF artifact was absent. The existing qualified latch RBF and
  was preserved. The checksummed `mister-magik-fb`, catalog builder, art, and
  `MiSTer_MagiK` binaries were replaced through `scripts/mister`. Initial logs
  then exposed an older loaded scanout-module ABI (`ENOTTY`); the locally
  validated current module and provenance were installed without replacing the
  RBF, followed by a normal bounded reboot.
- Air Gallet launched from Arcade system `arcade`, filter `all`, row 17. A
  `load_core menu.rbf` return traversed the latch Menu re-exec and restarted the
  launcher in `return_from_game` mode. The restored context reported the exact
  path `/media/fat/_Arcade/Air Gallet (Europe).mra`, game index 17, visual index
  17.000, and the return-state file was consumed.
- The host smoke runner did not complete autonomously: its repeated
  `test -s` wrapper probe stalled while the active core was running even though
  the return-state file existed. The run was interrupted after its wall-clock
  bound, cleanup removed `launcher.env`, and the Menu return plus exact-context
  assertions were completed with one bounded on-device wait. Treat the product
  return path as device-verified, but the host runner polling as follow-up work.
- Post-reboot verification found `MiSTer_MagiK` and `mister-magik-fb` active,
  `mister_magik_scanout_slots` loaded with its device node ready, launcher
  backend `fpga-vblank-latch-hidden`, framebuffer mode `565 1 960 540 1920`,
  and no `fpga_vblank_latch_hidden_open_failed` event.

2026-07-01, MiSTer at `192.168.1.117`, after replacing Main-side arcade
semantics with the generic MagiK button override adapter and deploying both
`MiSTer_MagiK` and `mister-magik-fb` from
`mister-slint/scripts/deploy-main-mister-experiment.sh --device`:

- MagiK host tests passed with `cargo test --manifest-path magik-gui/Cargo.toml
  --features ui --no-default-features`.
- Main build passed with `./build-container.sh`.
- Host fork tests passed with `scripts/test-magik-state.sh`, and patch-surface
  validation passed with `scripts/check-magik-patch-surface.sh`.
- Deployment completed and a raw recovery reboot was required because the
  supervised and direct-reset wrapper paths did not observe the device going
  down from the current launcher state.
- Post-reboot `scripts/mister doctor` reported no obvious launcher/display
  problems: `MiSTer_MagiK` and `mister-magik-fb` were running, active VT was
  `tty2`, framebuffer mode was `565 1 960 540 1920`, and the launcher held
  roughly 60 fps.
- `/tmp/mister-magik/input-policy` and `/tmp/mister-magik/button-overrides`
  were absent after clean launcher boot, confirming no stale simple-input launch
  policy leaked across reboot.
- Physical button acceptance for Air Gallet still needs attended controller
  verification: Select should insert credit, Start should start, A/B should
  remain Shot/Bomb, and L should pause only when the core exposes pause.

2026-06-14, MiSTer at `192.168.1.117`, after deploy from
`mister-slint/scripts/deploy-main-mister-experiment.sh` using external
`../Main_MiSTer`:

- Main build passed with `./build-container.sh`.
- Boot reached `LauncherActive` with active VT `tty2`, Slint child alive,
  framebuffer mode `565 1 960 540 1920`, and `invariant_count=0`.
- 120-second launcher idle test produced no unexpected invariant events.
- `mister_magik_exit_to_menu` stopped Slint and transitioned
  `LauncherActive -> HandoffToStockMenu -> Unconfigured`; active VT remained
  `tty2`, which is acceptable for this smoke but worth revisiting for polish.
- `mister_magik_launch /media/fat/_Arcade/Missile Command (rev 3).mra` stopped
  Slint and launched Main's normal arcade path:
  `/media/fat/MiSTer_MagiK /media/fat/_Arcade/cores/MissileCommand_20240530.rbf
  /media/fat/_Arcade/Missile Command (rev 3).mra`, `/tmp/CORENAME` became
  `missileA.MISSILE`, and framebuffer mode became `8888 1 640 240 2560`.
  The status file remains at `HandoffToGame` because the normal loader path
  continues into the core before writing a final completion event.
- Crash-policy test killed the Slint launcher child PID; Main transitioned
  `LauncherActive -> LauncherCrashed`, set `launcher_pid=0`, and recorded no
  invariant violations.
- Supervised restart/env behavior should be retested after deployment by
  writing an Arcade benchmark `launcher.env`, sending
  `mister_magik_restart_launcher`, confirming the Slint child returns to
  `scene=launcher screen=arcade` with no OSD overlay, then removing the env file
  and restarting the normal launcher.
- Final reboot left the device back in `LauncherActive` with Slint child alive
  and `invariant_count=0`.
- Supervised reboot soak must pass 15 consecutive `scripts/mister agent
  boot-profile 15 --timeout 60 --fail-on-timeout` samples with agent, SSH, and
  `LauncherActive` recovery on every sample before this reboot path is treated
  as release-ready.

2026-06-18, MiSTer at `192.168.1.117`, after deploying the crash-restart
recovery update:

- Main build passed with `./build-container.sh`.
- Host fork tests passed with `scripts/test-magik-state.sh`, and patch-surface
  validation passed with `scripts/check-magik-patch-surface.sh`.
- Killing the supervised Slint child moved Main to `LauncherCrashed` with
  `crash_count=1`, `launcher_pid=0`, `last_crash_reason` recording `signal=9`,
  and `invariant_count=0`.
- Sending `mister_magik_restart_launcher` returned Main to `LauncherActive`
  without raw reboot, spawned a fresh Slint child, set `restart_count=1`, and
  left `last_restart_error` and `last_spawn_error` empty.

2026-06-20, MiSTer at `192.168.1.117`, after deploying the Linux reboot
primitive under `LauncherRebooting` visual lockdown:

- Deploy passed with `mister-slint/scripts/deploy-main-mister-experiment.sh`.
- Raw post-deploy reboot started the new `MiSTer_MagiK` fork and reached
  `LauncherActive` with Slint child alive.
- Supervised reboot soak passed 15/15 samples with
  `scripts/mister agent boot-profile 15 --timeout 60 --fail-on-timeout`.
  Worst agent-ready time was 31911ms, worst SSH-ready time was 32697ms, with
  `noroute=0`, `refused=0`, and every sample returning Main to
  `LauncherActive`.
- Explicit raw fallback smoke passed with `scripts/mister reboot-wait --raw`;
  the device returned to `LauncherActive` with `MiSTer_MagiK` and
  `mister-magik-fb` running.
- Reboot shutdown breadcrumbs are required for future supervised reboot
  debugging: inspect `/media/fat/mister-magik/bootlogs/main-reboot.log` for
  sync and `/sbin/reboot` spawn timing.
- Follow-up diagnosis found a supervised reboot could still produce an RX-zero
  boot even though Main reached `sync_done` and spawned `/sbin/reboot`. The
  fix is to start `/sbin/reboot` from a detached child with inherited fds
  closed, matching the known-good raw Linux reboot process context more closely
  while keeping MagiK visual lockdown.
- After the detached-child fix, the 15-sample supervised reboot soak passed
  again with worst agent-ready time 31299ms, worst SSH-ready time 32065ms,
  `noroute=0`, `refused=0`, and every sample returning to `LauncherActive`.
- MagiK boot lockdown deployed and raw-reboot smoke passed. The fresh boot event
  order was `launcher_boot_lockdown`, `early_black_spawn`,
  `early_black_route_completed`, then `LauncherActive`; final status reported
  `tty2`, framebuffer mode `565 1 960 540 1920`, `fb0: slint_like`, and
  `invariant_count=0`.
- Main-side early-black deployed and raw-reboot smoke passed. The fresh boot
  event order was `launcher_boot_lockdown`, `early_black_main_route_start`,
  `early_black_route_frame_copied`, `early_black_route_completed`, then
  `LauncherActive`; there was no `early_black_spawn` fallback. Final status
  reported `tty2`, framebuffer mode `565 1 960 540 1920`, Slint child alive,
  and `invariant_count=0`.

2026-06-25, MiSTer at `192.168.1.117`, after deploying structured launch
handoff with the paired `mister-magik-fb` catalog rewrite:

- Real path launches still use `mister_magik_launch <absolute path>`. Virtual
  catalog launches now use `mister_magik_launch_plan_v1 <encoded-plan>` with
  `schema=1`, catalog `launch_ref`, display metadata, `core_path`,
  `payload_path`, `mount_kind`, `mount_index`, and `delay_secs`.
- Main parses the encoded plan, resolves `core_path` via the shared MRA RBF-name
  resolver, passes the encoded plan through core re-exec as `magik-plan-v1:...`,
  and `user_io_init` seeds the existing MGL load queue directly from the plan.
- The old launch-time virtual `.mgl` descriptor path is intentionally gone from
  the paired Rust launcher: no SQLite lookup, launch-cache stamp/repair,
  materialize-on-miss, priority prewarm, or post-ready descriptor prewarm is
  expected for selected `magik-plan:*` rows.
- Device acceptance passed with `scripts/device-catalog-acceptance.sh` and
  `scripts/device-release-acceptance.sh --tiers handoff`. Final handoff samples
  stayed within the existing frame-gap/recovery envelope, and device logs showed
  no `virtual_launch_cache` events after catalog readiness.
- Launch-prep benchmark for virtual NeoGeo rows reported `bad_targets=0`,
  `read_bytes=0`, and `write_bytes=0`; structured-row selection no longer opens
  SQLite or touches generated launch-cache files.

2026-07-09, MiSTer at `192.168.1.117`, after deploying production
FPGA latch startup ownership:

- Main build passed through `mister-slint/scripts/deploy-main-mister-experiment.sh`,
  which deployed `MiSTer_MagiK`, `mister-magik-fb`, the persistent plugin probe
  module, and the CI-built latch RBF together.
- A raw agent reboot was required because the previously running old Main
  process did not observe a down/up transition for the supervised or direct
  reset wrappers after deployment.
- Post-reboot validation showed
  `/media/fat/MiSTer_MagiK /media/fat/mister-magik/fpga/menu-magik-vblank-latch.rbf`
  in Main's command line, `mister_magik_plugin_probe` live in `/proc/modules`,
  `/dev/mister-magik-plugin-probe` present, and the launcher running on the
  Settings screen.
- Main events contained `latch_menu_rbf_load from=(default-menu)` and Rust
  events contained `launcher_present_backend=fpga-vblank-latch-hidden`, with no
  new `open_failed` or `present_failed` latch errors in the final validation.
- `/dev/fb0` agent capture is not valid HDMI proof once latch mode is active;
  HDMI capture still depends on the host USB Video device being visible to
  AVFoundation.

## Rebuild From Scratch

1. Reset to upstream release commit `93d13fb690db4581768389450fb639822ae88333`.
2. Reapply the features listed in this file, keeping to the approved patch
   surface.
3. Run host tests from this repo.
4. Build with `./build-container.sh`.
5. Deploy as `/media/fat/MiSTer_MagiK` alongside the Slint child binary.
6. Run the device acceptance scenarios listed below.

## Required Tests

Host tests:

- Launcher state transition tests.
- Handoff command parser tests, including strict structured-plan parser
  acceptance/rejection for schema, mount kind, required paths, and numeric
  fields.
- External `load_core` parser tests for `.mgl`, `.mra`, and `.rbf` paths,
  including empty, relative, oversized, unsupported-extension, and control-
  character rejection; verify accepted commands use the supervised real-path
  handoff and inactive Main retains its stock command reader.
- Restart command parser and launcher restart lifecycle tests.
- Reboot command parser and `LauncherRebooting` lifecycle tests.
- Crash-policy tests.
- Expected-exit tests: suspend/restart and clean child exits must not write
  crash reports or increment crash counters, while nonzero/signal exits still
  enter crash recovery.
- Crash-restart tests: after `LauncherActive -> LauncherCrashed`, the state must
  remain MagiK-active, keep polling commands, and accept
  `BeginEnterLauncher -> EnteringLauncher` for supervised restart.
- Status/event contract tests.
- Invariant event formatting tests.
- Production latch Menu browser-path identity tests.
- Patch-surface check.
- Main container build.
- Rebuild parity after env/restart changes.

Device tests:

- Boot to Slint launcher with active VT `tty2`.
- Idle without Main UI/framebuffer invariant violations.
- Launch one known real `.mgl` or `.mra`.
- Launch one structured `magik-plan:*` catalog row and verify Main seeds MGL
  state directly without a temporary `.mgl` descriptor.
- Exit to stock MiSTer menu.
- Kill Slint and verify the documented crash policy.
- After killing Slint, send `mister_magik_restart_launcher` and verify Main
  respawns the supervised launcher without showing stock Main UI/OSD.
- Run a supervised Arcade benchmark restart with and without catalog refresh
  noise, and verify no direct Slint launch or OSD overlay is involved.
- Reboot through `mister_magik_reboot` and verify HDMI holds the MagiK shutdown
  frame without stock OSD/menu flash before reset.
- Run the 15-sample supervised reboot Ethernet soak and fail the release if any
  sample does not recover agent, SSH, and `LauncherActive` within timeout.

2026-07-19, MiSTer at `192.168.1.117`, after deploying external `load_core`
compatibility to `MiSTer_MagiKDev`:

- `scripts/test-magik-state.sh` and the Apple-container ARM build passed.
- An API-emulated NFC token launched Missile Command from `LauncherActive`.
  Main recorded `external_load_core_handoff` for the real `.mra` path and
  re-executed the expected Arcade core.
- A second emulated token while Missile Command was active launched Black
  Widow through stock Main command handling, proving game-to-game switching
  remains intact while the MagiK launcher is inactive.
- After `load_core menu.rbf` restored the supervised launcher, an emulated
  Game Boy token produced `/media/fat/.LASTLAUNCH.mgl`; Main recorded the
  external handoff and entered the Game Boy core.
- A native `mister_magik_launch` smoke reached the expected Arcade core,
  confirming the existing MagiK handoff remained intact. Final cleanup
  restored `LauncherActive`, stopped the temporary token service, left TCP
  port `7497` closed, and found no live fault-arming files.
