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
- Keep the MagiK-specific Menu RBF's native video background solid black until
  the launcher reaches its internal ready boundary. Rust reports ready only
  after two completed, advancing latch posts on alternating slots that startup
  intended for display. HDMI/CRT timing, DE/sync, and downstream OSD
  composition remain available while native RGB is black; the stock Menu RBF
  is unchanged. This internal boundary does not prove physical sink visibility.
- Establish bootstrap black immediately after `video_init()` and before every
  supervised launcher spawn. Main must disable OSD, OSD keys, launcher input,
  and LFB routing before latch preflight, then transfer FPGA ownership only
  after preflight succeeds.
- Keep Main as the sole writer of the complete `UIO_BUT_SW` framework word,
  including the launcher framebuffer mux, composite sync, SoG, Direct Video,
  scaler, audio, and HDMI flags. After the canonical bootstrap LFB disable,
  reassert the launcher framebuffer mux for a resolved Direct Video route
  before transferring FPGA ownership to Rust.
- Start MiSTer MagiK Slint on `tty2` after Main video initialization. Main
  creates the child session, acquires `tty2` as its controlling terminal, and
  executes the generated Bash launcher directly; no `agetty` login wrapper is
  involved.
- Enter `LauncherStarting` after spawning Slint and reserve
  `LauncherActive` for the accepted token- and PID-bound ready report. Keep
  stock Main scheduler, OSD, input, and framebuffer work suppressed throughout
  startup, active, suspend, reboot, and crash ownership states.
- Keep Main in dormant launcher mode while Slint owns the launcher UI.
  Dormant launcher mode blocks on the command FIFO and supervised-child exit,
  with a bounded maintenance timeout, so Main does not burn CPU1 or wake at a
  fixed high rate while waiting for Slint or launcher commands.
- Transfer exclusive FPGA SPI/GPO ownership to the supervised launcher
  immediately before spawn and restore a safe Main shadow only after that child
  is reaped. Main's low-level FPGA entrypoints reject and count any attempted
  cross-owner write instead of corrupting a launcher transaction.
- Accept explicit handoff commands:
  - `mister_magik_launch <absolute path>` for real `.mra`, `.mgl`, and `.rbf` paths
  - `mister_magik_launch_plan_v1 <encoded-plan>` for MagiK structured catalog rows
  - `mister_magik_exit_to_menu`
- Add `Back to MagiK menu` as the first visible row of Main's generic
  console/Arcade F12 menu when the matching MagiK launcher is installed. Keep
  the first core-provided option initially focused, retain the stock Exit row,
  and return through Main's existing `menu.rbf` load path.
- Preserve MiSTer's public `load_core <absolute path>` contract while the
  supervised launcher owns the command FIFO. External `.mgl`, `.mra`, and
  `.rbf` requests use the same real-path handoff as MagiK-owned launches;
  inactive Main continues to use the stock command reader.
- Accept explicit lifecycle commands:
  - `mister_magik_suspend`
  - `mister_magik_resume`
  - `mister_magik_restart_launcher`
  - `mister_magik_reload_main` (Dev executable only)
  - `mister_magik_reboot`
- Accept explicit attended display diagnostics for rare HDMI/scaler bad states:
  - `mister_magik_hdmi_power_cycle`
  - `mister_magik_video_adjust`
  - `mister_magik_video_reinit`

  `mister_magik_video_reinit` is accepted only from `LauncherActive` because it
  stops the supervised child; startup cannot be diverted around bounded ready
  recovery.
- Source `/media/fat/mister-magik/launcher.env` from the generated launcher
  script before starting Slint, so tooling can benchmark the real launcher
  Arcade screen without direct Rust launches. Main re-exports its fresh
  per-spawn ready token and FIFO path after sourcing the file, so local
  configuration cannot substitute a stale readiness identity.
- Use Main's original loaders after handoff for `.mra`/`.mgl`/`.rbf` launch.
- Carry MagiK structured launch plans through Main's re-exec path as a
  `magik-plan-v1:` argument and seed the existing MGL action state directly,
  avoiding temporary `.mgl` files.
- Honor MagiK's per-launch simple joystick policy marker. When active, Main
  ignores stock/user joystick map files under `/media/fat/config/inputs/` for
  that launched core, uses MagiK-owned controller baselines from
  `/media/fat/mister-magik/input/`, and then falls back to Main's existing
  gamecontrollerdb/default mapping path if MagiK has no managed baseline.
- Keep Main's stock menu controller mapping authoritative while the launcher is
  active. Main polls evdev and hot-plug state with a bounded blocking wait,
  applies custom Menu OK/Back and normal gamecontrollerdb/user-map precedence,
  and emits resolved actions through its virtual input device for Rust.
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
- Allow an authenticated Dev deployment to replace Main without rebooting
  Linux. The command is accepted only from `LauncherActive` while the running
  executable is `/media/fat/MiSTer_MagiKDev`; Main acknowledges, stops and
  reaps Slint, enters bootstrap black, syncs, and re-execs that explicit path
  against the already-loaded latch RBF. The old Main remains alive until a
  close-on-exec status pipe proves the replacement exec; fork/exec failure is
  reported without calling the reset manager so the host transaction retains
  rollback and the sole bounded recovery-reboot decision. Status advertises the
  capability so the host uses a Linux reboot only for the initial installation
  or bounded recovery.
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
- `user_io.cpp` / `user_io.h` only for the post-`video_init()` Menu-core boot
  hook and the testable framework-word builder used by Main's existing
  `user_io_send_buttons()` path
- `input.cpp` only for MagiK simple joystick policy gating, MagiK-owned
  baseline-map loading, and the launcher menu-input proxy including preserving
  Main's generic command poll result outside launcher mode
- `joymapping.cpp` only for applying MagiK-provided simple-mode button override
  tokens by MRA button index
- `scheduler.cpp` dormant-mode polling seam
- `main.cpp` only for the non-scheduler dormant launcher wait hook
- `fpga_io.cpp` / `fpga_io.h` only for the central launcher ownership fence,
  safe GPO shadow transfer, and ownership diagnostics
- narrow command handoff wiring needed to launch through Main
- `menu.cpp` only for the production latch RBF's logical root-level core-browser
  identity, the generic core menu's MagiK return row, and minimal diagnostic
  guards
- `video.cpp` / `video.h` only for exposing existing video diagnostic
  entrypoints to MagiK command handlers and the canonical
  `UIO_SET_FBUF` bootstrap-disable primitive
- minimal diagnostic guards in OSD/video entrypoints

Build/docs/test changes may touch:

- `.devcontainer/Dockerfile.apple-container`
- `build-container.sh`
- `FORK.md`
- `MAGIK_PATCHSET.md`
- `scripts/`
- `tests/`

`build-container.sh clean all` is the canonical local experiment build. The
wrapper maps that conventional clean-build spelling to `make clean && make`
because upstream Main's Makefile has no explicit `all` target.

## Implemented Features And Tests

- Exclusive FPGA ownership update: the launcher lifecycle transfers SPI/GPO
  ownership only after Main has established bootstrap black, completed the
  runtime latch preflight, and prepared the VT/input state. Child exit, kill,
  handoff, suspend, fork failure, and reboot
  restore Main ownership before any Main hardware work. `main-status.json`
  publishes owner, epoch, blocked SPI/GPO counts, and the last blocked site.
  Host tests cover transfer ordering, stale transfer refusal, blocked access
  accounting, and restoration.

- Qualified black bootstrap: the MagiK-specific Menu RBF supplies a native
  black video source while preserving DE, sync, and downstream HDMI/analog OSD
  composition. `video_magik_enter_bootstrap_black()` issues only the canonical
  `UIO_SET_FBUF` disable word; it never changes framebuffer mode, clears
  `/dev/fb0`, or enables a legacy framebuffer. Main enters this idempotent
  transition after `video_init()` and before every initial, resume, active
  restart, and crash-respawn child start. The enforced order is bootstrap black,
  latch/runtime preflight, FPGA ownership transfer, then fork. Unsupported
  framebuffer commands, preflight failure, and fork failure spawn no child and
  restore stock OSD/input over the native black Menu background. Status remains
  `mister-magik-main-status-v2` with additive bootstrap phase/source/time/count
  fields; the event stream records each successful boundary.
  A resolved Direct Video route reasserts `CONF_VGA_FB` through Main's complete
  framework-word writer after the canonical LFB disable succeeds and before
  ownership transfer, so the analog output selects the downstream framebuffer
  mixer instead of the native-black source. The host policy test enforces that
  ordering while keeping `video_magik_enter_bootstrap_black()` limited to the
  canonical disable command.
  `MagikBootstrapSequence` is compiled into the production launcher and is also
  exercised as a host unit with injected ownership-guard, unsupported-command,
  preflight, ownership-transfer, fork, and ordering failures. Every injected
  failure forbids spawn and permits stock OSD recovery only after Main ownership
  is restored.

- Bounded launcher readiness: `ChildSpawned` enters `LauncherStarting`, which
  owns and suppresses the Main session but is not active and cannot hand off a
  core. Main creates one mode-0600 FIFO at
  `/tmp/mister-magik/launcher-ready-v2`, generates a fresh 32-hex token for
  every spawn, and exports the exact Main PID, Main generation, and FPGA owner
  epoch to that child only after ownership transfer. The preferred `ready-v3`
  record is a strict, canonical, fixed-order line no larger than 1024 bytes. It
  binds the token and supervised child PID to that Main PID/generation and
  owner epoch, latch protocol 5/capabilities `0x03ff`, RGB565 base and geometry,
  two advancing completed post sequences and route epochs on alternating slots,
  both receipt CRCs, and a nonblank visible RGB565 source assertion. Main also
  accepts the strict legacy `ready-v2` SHA-256/count form for rollback. Unknown,
  missing, duplicate, reordered, noncanonical, or
  trailing fields are rejected. Sequence and route deltas must advance within
  the legal modulo-16-bit half-range; slots must alternate; geometry and digest
  encodings are bounded; and the current FPGA owner name/epoch is rechecked at
  acceptance so a stale report cannot activate the launcher.
  Rust emits the one-way report only after two already-confirmed latch
  completions intended for display. The source digest/nonzero statistic proves
  the intended cached source is nonblank and the post receipts prove internal
  transport completion; neither is described as physical HDMI/CRT visibility.
  There is no reply FIFO, route lease, or duplicate Rust status mirror.
  Main waits eight seconds per attempt. The first timeout or pre-ready child
  exit stops and reaps the child and retries the complete supervised start
  once. The second failure stops the child, rolls a provisional display change
  back to its prior timing when applicable, restores stable stock Menu, and
  completes any pending command reply with an error. Display-apply and launcher
  restart/resume replies remain pending until ready succeeds or final recovery
  fails. Polling, timeout, and child reaping continue while Main owns the
  session even if the launcher executable disappears after spawn.
  Before the first retry, Main atomically writes the bounded current incident
  record `diagnostics/return-incident-current.json` with schema
  `mister-magik-return-incident-v1`, the failure phase/reason, attempt, token,
  launcher/Main identities, owner epoch/name, timestamps, recovery state, and
  `sink_visibility="unobserved"`. Symlinked destinations and unsafe directory
  types are rejected; incident persistence failure cannot block ownership or
  stock-UI recovery. The same record is atomically enriched after ownership
  recovery with `fresh-child-retry` or `stock-fallback`.
  Terminal stock recovery rearms readiness only after the incident update,
  stock restoration, and pending reply completion. It clears the old token,
  spawn identity, deadline, and attempt so the next independent launcher entry
  starts at attempt 1 and again receives exactly one fresh-child retry. The
  immediate retry path returns while attempt 2 remains armed and is therefore
  unchanged. Recovery never loads an RBF, resets the FPGA/core, reboots, or
  power-cycles HDMI in response to inferred black output.
  `main-status.json` exposes only the ready phase, attempt, remaining deadline,
  and last failure. Host tests cover canonical parsing, malformed/bounded field
  rejection, stale token/child/Main/generation/owner identity, current-owner
  recheck, deadline boundaries, the single retry, terminal rearm, later fresh
  retry, no retry loop, ordered child-stop/rollback/Menu/reply recovery, command
  polling during Starting, and rejection of video reinit outside Active.
  Physical HDMI/CRT visibility remains an attended output-capture qualification
  concern rather than a claim of this internal handshake.

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
  mode writes, framebuffer enable calls, and menu-background work. Immediately
  after `video_init()`, Main disables LFB routing so the MagiK Menu RBF's native
  black source remains visible during preflight.
- Launcher reveal update: Main reasserts bootstrap black before every
  supervised launcher spawn, including resume/restart/crash paths. Rust keeps
  its cleared-black startup frame and uses the two-post internal ready boundary
  before Main marks the launcher active; physical reveal is qualified
  separately at the USB-video sink.
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
  restarts only the launcher. Main starts a twenty-second deadline when the new
  launcher queries the pending state, retains a bounded no-launcher fallback,
  and restores the in-memory working mode on cancel or timeout. Confirmation
  delegates comment-preserving atomic `MiSTer.ini` persistence to a supervised,
  asynchronously polled matching `mister-magik-fb` child. Failure keeps the
  mode provisional for retry or cancellation; rollback terminates any active
  persistence child and publishes a one-shot Settings return intent. A crash or
  power loss before successful confirmation keeps the previously persisted output.
  Main exports the configured display-mode ID to every replacement launcher so
  startup never derives its framebuffer policy from the previous FPGA route.
  Operator tooling uses the separate acknowledged
  `mister_magik_display_apply_headless_v1 mode=<id>` command. It has the same
  provisional confirm/cancel safety contract, but Main exports
  `MISTER_MAGIK_DISPLAY_CONFIRM_UI=0` so the replacement launcher never opens
  the Settings confirmation dialog; UI-originated `display_apply_v1` exports
  `1` and retains the countdown. Main re-exports these transaction-owned
  values after optional `launcher.env` loading so local overrides cannot cross
  the UI/headless boundary. Timeout, cancellation, and rollback failure also
  preserve this route: errors remain in the typed display reply, while only
  UI-originated transactions request a Settings/error return. Older launchers ignore the new environment value;
  older Main builds reject the new typed command rather than silently applying
  it through the UI route.
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
- Launcher input proxy update: before spawning Rust, Main initializes its stock
  evdev/uinput subsystem and advertises `MISTER_MAGIK_INPUT_PROXY=1` plus
  `MISTER_MAGIK_INPUT_PROXY_PROTOCOL=2`. While
  `LauncherActive`, the normal mapping path remains responsible for user menu
  maps, gamecontrollerdb fallback, controller quirks, analogue thresholds,
  custom Menu OK/Back, combinations, and hot-plug discovery. Resolved menu
  actions, including resolved physical-keyboard menu keys, are aggregated by
  logical action and translated to a stable virtual-key protocol. Only aggregate
  press/release transitions are written through `MiSTer virtual input`; no OSD
  or core action is performed and Main generates no launcher repeat. The launcher poll
  waits on evdev, hot-plug, and the launcher-owned command descriptor, but never
  consumes the command bytes. Rust therefore receives original Main semantics
  while the existing launcher command parser retains FIFO ownership.
- Dormant idle CPU update: `LauncherActive` blocks in the stock input poll with
  a one-second maintenance timeout, while `LauncherSuspended` and
  `LauncherCrashed` use the command/SIGCHLD wait. The non-scheduler loop and
  scheduler coroutine share that policy, preserving normal stock Main polling
  after handoff while preventing the supervised launcher from pinning CPU1.
  The previous signal disposition is restored after each command/SIGCHLD wait.
  Host tests cover proxy key translation, timeout, FIFO wake, and child-exit
  wake behavior.
- Generic command FIFO dispatch fix: MagiK's launcher-input multiplexing now
  restores its saved command `pollfd` only when launcher mode temporarily
  replaced that entry. The unconditional restore previously reinstated the
  pre-poll `revents` value, initially `-1`, after handoff to a game. Main then
  repeatedly entered its inherited nonblocking command reader; after one real
  `screenshot` read, negative reads re-dispatched the static buffer's previous
  contents. The fix leaves Main's generic reader and command behavior unchanged
  and limits MagiK's descriptor substitution to launcher ownership. The host
  suite binds this invariant to the command-poll seam.
- Launcher path-isolation fix: MagiK's support layer resolves and checks its
  executable with its own absolute path plus `access()`. It no longer calls
  Main's shared `getFullPath()` or `FileExists()` helpers, so launcher
  maintenance cannot overwrite the buffer used by Main's asynchronous
  screenshot worker. Main's scaler and screenshot implementation stay stock.
- Attended display diagnostics update: Main accepts three explicit
  MagiK FIFO commands for rare HDMI/scaler bad-state experiments without a
  Linux reboot. `mister_magik_hdmi_power_cycle` toggles ADV7513 HDMI power.
  `mister_magik_video_adjust` forces Main's runtime video-mode adjustment pass.
  `mister_magik_video_reinit` stops/suspends the Slint child, temporarily
  releases MagiK video suppression, disables the HPS framebuffer, reruns Main's
  existing `video_reinit()` path, and leaves the launcher suspended for visual
  inspection. Reinit is rejected outside `LauncherActive`, including during
  `LauncherStarting`, so it cannot reap the child behind the readiness state
  machine. These are attended diagnostic levers, not automatic recovery or
  normal launcher lifecycle commands; host parser and state tests cover their
  command names, type labels, and reinit gate.
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
  It also compiles the production bootstrap sequencer and injects every
  fail-closed boundary, while source invariants prove the production launcher
  orders black, preflight, ownership transfer, and fork and uses only the
  canonical framebuffer-disable word. The same suite compiles the launcher
  readiness parser and recovery planner, rejects malformed/stale token and PID
  reports, checks the eight-second boundary, and proves first-failure retry then
  ordered child stop, display rollback, stock Menu convergence, and delayed
  command failure on the final attempt.
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
- Generic screenshot-command behavior must be verified through the typed agent
  after deploying the exact clean Main commit. Each of three independent MagiK
  handoffs to Pac-Man Plus writes `screenshot\n` exactly once as soon as
  `pacplus` is reported, allows up to five seconds for the asynchronous save,
  and requires exactly one new nonempty screenshot before returning to
  `LauncherActive`.
- Production latch boot must prove all three live signals after reboot or game
  return: Main command line contains
  `/media/fat/mister-magik/fpga/menu-magik-vblank-latch.rbf`,
  `mister_magik_scanout_slots` is present in `/proc/modules`, and
  `/dev/mister-magik-scanout-slots` exists before accepting Rust latch-mode
  results.

## Device Smoke Results

### Issue #46 pre-fix reproduction

On 2026-08-09, the development Main launched Pac-Man Plus through MiSTer MagiK.
One `screenshot\n` write to `/dev/MiSTer_cmd` produced two new screenshots in
nine seconds. The bounded agent probe then returned Main to the launcher and
confirmed `LauncherActive`. This is pre-fix evidence only; the required
post-fix acceptance is the three-trial check above.

The matched stock-Main baseline on the same device loaded Pac-Man Plus through
Main's public `load_core` command, then produced exactly one new nonempty
screenshot from one `screenshot\n` write during a 30-second observation and
returned to `MENU`. Earlier read-guard candidates produced zero screenshots and
were discarded; the final patch restores the inherited reader unchanged.

Further bounded tracing showed that the MagiK handoff consumed one command,
accepted one screenshot request, captured the scaler, and queued the worker.
The worker resolved
`/media/fat/screenshots/pacplus/20260809_125548-screen.png`, but the shared
`getFullPath()` buffer changed to
`/media/fat/mister-magik-dev/mister-magik-fb` before Imlib completed and the
save failed. The final fix therefore combines the command-poll ownership guard
with MagiK-local launcher path resolution, leaving Main's scaler untouched.

The exact runtime tree in this patch passed the post-fix check on 2026-08-09.
Three independent MagiK handoffs reported
`pacplus` after 400 ms; each single `screenshot\n` write produced exactly one
new nonempty screenshot after 100 ms. Counts advanced from 6 to 7, 7 to 8, and
8 to 9. Every trial returned to `LauncherActive`, and fault arming was clear
before each launch.

### Acknowledged command-channel status

State-changing MagiK commands have a paired reply FIFO at
`/dev/MiSTer_cmd_reply`. Main writes one short line for every parsed command:
`ok <state>`, `rejected <state>`, or `error <reason>`. Successful handoff replies
mean that Main accepted a readable launch payload and began the handoff; they
are written before Main terminates the supervised Slint child, not after the
core loader returns. `ok LauncherActive` means Main accepted the supervised
child's token- and PID-bound internal ready report after the required two latch
completions. It does not claim that HDMI, CRT, or any other physical sink is
visible. Callers must serialize commands and drain any abandoned reply
immediately before writing a new command. A caller must retain serialization
ownership until it reads the reply or Main closes the channel; heartbeat
failure starts supervisor
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

The early-black implementation evidence in this dated entry is historical and
was superseded by the qualified native-black bootstrap contract on 2026-07-30.
It must not be used as qualification evidence for the current platform bundle.

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
- Generic core-menu source-policy checks for first-row placement, launcher
  availability and root-page gating, core-selection index preservation, and
  return exclusively through the existing logical `menu.rbf` path.
- Patch-surface check.
- Main container build.
- Rebuild parity after env/restart changes.
- Qualified-bootstrap sequence tests for the success path plus injected
  ownership guard, unsupported framebuffer command, latch preflight,
  ownership-transfer, fork, and ordering failures. Failed paths must never
  reach the parent `ChildSpawned` transition and must recover stock OSD/input
  over native black.
- Ready-v2 parser and lifecycle tests covering exact canonical serialization,
  the 1024-byte boundary, protocol/capability, RGB565 geometry, digest/nonzero
  bounds, advancing sequence and route epochs, alternating slots, receipt CRCs,
  token/child/Main/generation/owner identity, current-owner recheck, one retry,
  terminal stock fallback, post-recovery rearm, and a later independent fresh
  retry. The terminal handler's production-order check must reject any RBF
  load, FPGA/core reset, reboot, or HDMI mutation in readiness recovery.
- Pinned MagiK Menu RTL/build checks proving native active RGB is zero while
  DE/sync and both HDMI and analog OSD composition paths remain connected.

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
- Record 30 fps USB Video for supervised cold reboot, launcher restart without
  RBF reload, game-to-launcher return, and injected preflight failure. After
  the first post-transition black frame, every frame must remain black until
  the first sustained MagiK frame; reject Menu static, OSD residue, terminal
  frames, and partial UI. Retain the frame-reviewed movies with matching Main
  boot events and latch status.
- Re-run the complete latch/platform qualification for the exact Main, MagiK
  runtime, scanout module, and RBF tuple. Require zero drops/rejections and the
  existing latch stress thresholds. The attended release qualification gate
  still requires explicit operator authorization.

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

2026-07-30 qualified-black bootstrap candidate:

- Main now uses the production `MagikBootstrapSequence` for the common
  post-`video_init()` and supervised-spawn path. The canonical framebuffer
  disable word is emitted even when the command acknowledgement is unavailable;
  failure still prevents spawn and restores Main-owned stock OSD/input.
- The paired MiSTer MagiK repository supplies the MagiK-only native-black Menu
  RTL, pinned integration checks, 30 fps USB Video evidence tooling, and the
  platform workflow. The stock Menu RBF remains unchanged.
- Host tests, patch-surface validation, and the Apple-container ARM build passed
  for this source candidate. RBF synthesis belongs only to the
  `Build MiSTer MagiK Platform` GitHub Actions workflow and remains pending
  until that workflow completes.
- This entry describes a candidate, not a qualified release. GitHub platform
  build completion, four-path 30 fps USB Video review, and the complete
  latch/platform qualification remain required before promotion.

2026-08-01 local Main return profiling:

- Main event records and status now include `CLOCK_BOOTTIME` timestamps in
  microseconds while retaining the millisecond fields used by existing tools.
- The return path records process entry, return-command receipt, latch
  resolution and verification, launcher verification and preflight, child
  spawn, and the launcher exec boundary. These markers make the black interval
  attributable without including host polling latency.
- Menu return RBF requests resolve to the readable MagiK latch before the stock
  Menu bitstream is opened or loaded. The inherited one-shot return marker is
  preserved for the final launcher, while an unavailable latch falls back once
  to the unchanged stock Menu path with an explicit diagnostic.
- Initial/default latch selection now uses only the configured launcher and
  readable latch path checks. It no longer serializes a whole-platform hash
  pass before loading the latch; full transaction identity remains enforced at
  deployment and the final launcher preflight is unchanged in this commit.
- Final launcher preflight no longer repeats deployment's manifest and SHA-256
  validation on every process start. Runtime checks retain launcher
  readability/executability, scanout module/device readiness, and the existing
  protocol/capability readiness report before ownership transfer and reveal.

2026-08-14 black-screen return recovery hardening:

- Commit `7c050d1` replaces ready-v1 with the strict ready-v2 contract described
  above. Main captures its PID/generation and the transferred FPGA owner epoch,
  exports them to the supervised launcher, requires latch protocol 5 and
  capabilities `0x03ff`, parses the complete record canonically, and rechecks
  current ownership before entering `LauncherActive`.
- Main now carries the actual receipt CRCs for both confirmed posts and accepts
  only advancing sequence/route epochs on alternating slots. The paired Rust
  launcher supplies the visible-row RGB565 SHA-256 and nonzero-pixel statistic.
  This is source/transport attribution, not a claim of sink visibility.
- A readiness failure creates or atomically updates the bounded current return
  incident before retry/fallback. Persistence is fail-open for recovery: an I/O
  error cannot prevent child reaping, ownership restoration, stock UI/input, or
  pending reply completion.
- Commit `4566825` corrects the terminal lifecycle. The immediate first-failure
  retry remains attempt 2. Only after the second-failure incident update, stock
  recovery, and reply completion does Main reset phase/attempt/deadline and
  clear stale spawn identity. A later independent return therefore starts at
  attempt 1 and again receives exactly one retry instead of inheriting a
  permanently failed attempt-2 transaction.
- `scripts/test-magik-state.sh` pins the production ordering and explicitly
  forbids RBF reload, raw/core FPGA reset, reboot, or HDMI recovery mutation in
  the readiness failure handler. `tests/launcher_ready_test.cpp` covers exact
  parsing, identity mismatch, first retry, terminal fallback, rearm, later fresh
  retry, and absence of a terminal retry loop.
- These Main changes do not repair the scaler completion CDC. The paired FPGA
  RBF owns that functional repair. Main makes activation and reveal fail-closed
  and ensures an incomplete repair cannot leave the product indefinitely black
  or corrupted. The exact Main/RBF/runtime tuple still requires attended
  physical-output smoke and the declared commercial return campaign before
  release promotion.

2026-08-17 generic core-menu MagiK return:

- Main's generic console and Arcade F12 menu now prepends a Main-owned `Back to
  MagiK menu` row when `mister_magik_launcher_configured()` confirms the
  matching launcher is installed. The row is limited to the root core page;
  core-defined subpages and the bespoke Minimig, Atari ST, Archimedes, SharpMZ,
  and Menu-core interfaces are unchanged.
- Fresh and reopened generic menus still focus the first core-provided option.
  The new row is immediately visible above it and is reached with one Up input.
  Core option, disabled-entry, MGL submenu, scrolling, and final stock Exit
  indices carry the same one-entry offset in rendering and dispatch.
- Selecting the row calls only `fpga_load_rbf("menu.rbf")`. The existing central
  MagiK resolver selects the readable manifest-bound latch and records the
  volatile launcher-return marker; if availability changes after the menu is
  drawn, the established missing-latch fallback remains the unchanged stock
  Menu path with its existing diagnostic.
- This is a Main C++ menu feature. It adds no command or protocol, changes no
  FPGA/RBF source, performs no direct reset or reboot, and requires no changes
  to the Rust launcher or individual cores. Host policy checks pin the narrow
  integration, and the standard fork host suite, patch-surface check, component
  contract test, and Apple-container clean build provide commit assurance.
